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
float Amp = 0;
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
  Serial.println("time_ms,RED,DC,AC,PI,HR,P1,P2,Amp,DT_up,DT12,W50,RI,AI,SI");

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

  // DC[n] = 1/N * sum(PPG[n-i])
  float DC = computeDC(redValue);
  // AC[n] = PPG[n] - DC[n]
  float AC = redValue - DC;
  // PI = AC / DC
  float Pi = (DC != 0) ? (AC / DC) : 0;

  // Heart Rate: HR = 60 / ΔT_beat (giây)
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

  // Đặc trưng sóng
  static float lastP1 = 0;
  static uint32_t prev = 0;
  static bool rising = false;
  static unsigned long tStart = 0; // cho ΔT_up
  static unsigned long tLeft50 = 0, tRight50 = 0;

  if (AC > prev && !rising) {
    rising = true;
    tStart = millis(); // bắt đầu rise (t_foot)
  }
  if (AC < prev && rising) {  // P1 = max(AC)
    P1 = prev;
    tP1 = millis();
    Amp = P1 ;
    rising = false;
    detectP1 = true;
    lastP1 = P1;
    tLeft50 = tP1; // t_left@50%
  }

  float RI = 0, AI = 0, SI = 0, DT_up = 0, DT12 = 0, W50 = 0;

  // P2 = local peak sau P1
  if (detectP1 && AC < (0.6 * lastP1) && AC > (0.3 * lastP1)) {
    P2 = AC;
    tP2 = millis();

    // RI = P2 / P1
    RI = (lastP1 != 0) ? (P2 / lastP1) : 0;
    // AI = P2 / P1 hoặc AI = (P2-DC)/(P1-DC)
    AI = (lastP1 != DC) ? ((P2-DC)/(lastP1-DC)) : 0;
    // DT12 = tP2 - tP1
    DT12 = (tP2 - tP1) / 1000.0;
    // DT_up = tP1 - t_foot
    DT_up = (tP1 - tStart) / 1000.0;
    // SI = chiều cao / DT12
    float height = 1.7; // mét
    SI = (DT12 > 0) ? (height / DT12) : 0;

    // W50: chiều rộng tại 50% biên độ
    if (AC <= 0.5 * Amp) {
      tRight50 = millis(); // t_right@50%
      W50 = (tRight50 - tLeft50) / 1000.0;
    }

    detectP1 = false;
  }

  prev = AC;

  // Xuất CSV: time_ms,RED,DC,AC,PI,HR,P1,P2,Amp,DT_up,DT12,W50,RI,AI,SI
  Serial.print(millis()); Serial.print(",");
  Serial.print(redValue); Serial.print(",");
  Serial.print(DC); Serial.print(",");
  Serial.print(AC); Serial.print(",");
  Serial.print(Pi); Serial.print(",");
  Serial.print(HR); Serial.print(",");
  Serial.print(P1); Serial.print(",");
  Serial.print(P2); Serial.print(",");
  Serial.print(Amp); Serial.print(",");
  Serial.print(DT_up); Serial.print(",");
  Serial.print(DT12); Serial.print(",");
  Serial.print(W50); Serial.print(",");
  Serial.print(RI); Serial.print(",");
  Serial.print(AI); Serial.print(",");
  Serial.print(SI); Serial.println();

  delay(10);
}
