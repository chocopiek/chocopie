#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

#define WINDOW_SIZE 100
uint32_t redBuffer[WINDOW_SIZE];
int bufferIndex = 0;
bool bufferFilled = false;

// Biến phân tích sóng
float P1 = 0, P2 = 0, Amp = 0;
unsigned long tP1 = 0, tP2 = 0, tFoot = 0;

// Nhịp tim
unsigned long lastBeatTime = 0;
float HR = 0;

//--------------------- Tính DC ---------------------
float computeDC(uint32_t newSample) {
  redBuffer[bufferIndex] = newSample;
  bufferIndex = (bufferIndex + 1) % WINDOW_SIZE;
  if (bufferIndex == 0) bufferFilled = true;

  long sum = 0;
  int count = bufferFilled ? WINDOW_SIZE : bufferIndex;
  for (int i = 0; i < count; i++) sum += redBuffer[i];
  return (float)sum / count;
}

//--------------------- SETUP ---------------------
void setup() {
  Serial.begin(115200);
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Không tìm thấy MAX30102!");
    while (1);
  }
  particleSensor.setup();
  Serial.println("time_ms,RED,DC,AC,PI,HR,P1,P2,Amp,DT_up,DT12,W50,RI,AI,SI"); // header
}

//--------------------- LOOP ---------------------
void loop() {
  unsigned long now = millis();
  long redValue = particleSensor.getRed();

  // DC và AC
  float DC = computeDC(redValue);
  float AC = redValue - DC;
  float Pi = (DC != 0) ? fabs(AC) / DC : 0;

  // Phát hiện P1 (đỉnh chính)
  static float prevAC = 0;
  static bool rising = false;

  if (AC > prevAC && !rising) { // bắt đầu lên
    rising = true;
    tFoot = now;
  }

  if (AC < prevAC && rising) { // phát hiện P1
    rising = false;
    P1 = prevAC;
    tP1 = now;
    Amp = P1;

    if (lastBeatTime > 0) {
      float dt = (now - lastBeatTime) / 1000.0;
      HR = 60.0 / dt;
    }
    lastBeatTime = now;
  }

  // Tìm P2 trong 300ms sau P1
  static bool waitingP2 = false;
  float RI = 0, AI = 0, DT12 = 0, DT_up = 0, SI = 0, W50 = 0;

  if (!rising && (now - tP1 < 300)) {
    if (AC > P2) {
      P2 = AC;
      tP2 = now;
    }
    waitingP2 = true;
  } else if (waitingP2) {
    waitingP2 = false;

    RI = (P1 != 0) ? P2 / P1 : 0;
    AI = RI;
    DT12 = (tP2 - tP1) / 1000.0;
    DT_up = (tP1 - tFoot) / 1000.0;
    float height = 170; // cm
    SI = (DT12 > 0) ? height / DT12 : 0;

    if (Amp > 0) {
      W50 = (tP1 - tFoot) * 0.5 / 1000.0; // gần đúng
    }

    // Xuất ra 1 dòng dữ liệu CSV đúng chuẩn
    Serial.print(now); Serial.print(",");
    Serial.print(redValue); Serial.print(",");
    Serial.print(DC, 2); Serial.print(",");
    Serial.print(AC, 2); Serial.print(",");
    Serial.print(Pi, 4); Serial.print(",");
    Serial.print(HR, 2); Serial.print(",");
    Serial.print(P1, 2); Serial.print(",");
    Serial.print(P2, 2); Serial.print(",");
    Serial.print(Amp, 2); Serial.print(",");
    Serial.print(DT_up, 3); Serial.print(",");
    Serial.print(DT12, 3); Serial.print(",");
    Serial.print(W50, 3); Serial.print(",");
    Serial.print(RI, 4); Serial.print(",");
    Serial.print(AI, 4); Serial.print(",");
    Serial.println(SI, 2);

    // reset P2
    P2 = 0;
  }

  prevAC = AC;
}
