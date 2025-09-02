#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

#define WINDOW_SIZE 100   // cửa sổ trung bình DC
uint32_t redBuffer[WINDOW_SIZE];
int bufferIndex = 0;
bool bufferFilled = false;

// Thông số sóng để tính RI, AI, SI
float P1 = 0;     // systolic peak
float P2 = 0;     // dicrotic peak
unsigned long tP1 = 0, tP2 = 0;  // thời gian xuất hiện P1, P2
bool detectP1 = false;

float computeDC(uint32_t newSample) {
  redBuffer[bufferIndex] = newSample;
  bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;
  if (bufferIndex == 0) bufferFilled = true;

  long sum = 0;
  int count = bufferFilled ? WINDOW_SIZE : bufferIndex;
  for (int i = 0; i < count; i++) {
    sum += redBuffer[i];
  }
  return (float)sum / count;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Khởi động MAX30102...");

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Không tìm thấy MAX30102!");
    while (1);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); // bật LED RED
  particleSensor.setPulseAmplitudeIR(0);     // tắt IR
}

void loop() {
  uint32_t redValue = particleSensor.getRed();

  // DC và AC
  float DC = computeDC(redValue);
  float AC = redValue - DC;
  float Pi = (DC > 0) ? ((AC / DC)*10000) : 0;

  // Tính HR
  static unsigned long lastBeat = 0;
  float HR = 0;
  if (checkForBeat(redValue)) {
    unsigned long now = millis();
    unsigned long delta = now - lastBeat;
    lastBeat = now;
    if (delta > 0) {
      HR = 60.0 / (delta / 1000.0);
    }
  }

  // Gửi 6 giá trị: time_ms, RED, DC, AC, PI, HR
  Serial.print(millis()); Serial.print(",");
  Serial.print(redValue); Serial.print(",");
  Serial.print(DC); Serial.print(",");
  Serial.print(AC); Serial.print(",");
  Serial.print(Pi); Serial.print(",");
  Serial.print(HR); Serial.println();

  // --- Demo phát hiện P1 và P2 ---
  // Giả sử: P1 là đỉnh cao nhất, P2 là đỉnh nhỏ sau P1
  static uint32_t prev = 0;
  static bool rising = false;

  if (AC > prev && !rising) { // bắt đầu dốc lên
    rising = true;
  }
  if (AC < prev && rising) {  // kết thúc dốc lên -> P1
    P1 = prev;
    tP1 = millis();
    rising = false;
    detectP1 = true;
  }

  // P2 giả lập: tìm 1 đỉnh nhỏ sau P1
  if (detectP1 && AC < (0.6 * P1) && AC > (0.3 * P1)) {
    P2 = AC;
    tP2 = millis();

    // Tính RI, AI, SI
    float RI = (P1 > 0) ? (P2 / P1) : 0;
    float AI = RI; // gần bằng P2/P1
    float deltaT12 = (tP2 - tP1) / 1000.0; // giây
    float height = 1.7; // chiều cao cơ thể (m), chỉnh theo thực tế
    float SI = (deltaT12 > 0) ? (height / deltaT12) : 0;

    Serial.print("\tRI: "); Serial.print(RI, 3);
    Serial.print("\tAI: "); Serial.print(AI, 3);
    Serial.print("\tSI: "); Serial.print(SI, 3);

    detectP1 = false; // reset
  }

  prev = AC;
  Serial.println();
  delay(10);
}
