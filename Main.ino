#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

//// --------- Cấu hình ---------
#define FS      50.0   // Tần số lấy mẫu (Hz)
#define THRESHOLD 30.0 // Ngưỡng tránh nhiễu
#define WARMUP_SAMPLES 150 // Bỏ qua ~3s đầu (50Hz * 3s)

//// --------- Biến filter Butterworth bậc 2 (band-pass) ---------
// band-pass 0.5–8 Hz @ 50Hz, butterworth bậc 2
float b[] = {0.0675, 0.0, -0.0675};
float a[] = {1.0, -1.7991, 0.8643};

float x_buff[3] = {0,0,0};
float y_buff[3] = {0,0,0};

unsigned long now;
int warmup = 0;

//// --------- Bộ nhớ cho chỉ số ---------
float lastP1 = 0, lastP2 = 0;
unsigned long tP1 = 0, tP2 = 0, tLastP1 = 0, tFoot = 0;
float RR = 1000;  // ms
float HR = 0, RI = 0, SI = 0, AI = 0, deltaT12 = 0, deltaT_up = 0;
float DC = 0;     // giá trị trung bình DC giả định
float Glucose = 0; // giả lập (có thể thêm từ dữ liệu sau)

//// --------- Biến phát hiện chân sóng ---------
float prevVal = 0, prevDiff = 0;

//// --------- Bộ lọc band-pass ---------
float bandpass_filter(float x) {
  x_buff[2] = x_buff[1];
  x_buff[1] = x_buff[0];
  x_buff[0] = x;

  y_buff[2] = y_buff[1];
  y_buff[1] = y_buff[0];

  y_buff[0] = b[0]*x_buff[0] + b[1]*x_buff[1] + b[2]*x_buff[2]
              - a[1]*y_buff[1] - a[2]*y_buff[2];

  return y_buff[0];
}

//// --------- Tìm P1 và P2 ---------
bool find_peaks(float value, unsigned long t, float &P1, float &P2,
                bool &foundP1, bool &foundP2) {
  static float cur_peak = 0.0f;
  static unsigned long lastP1Time = 0;
  static float RR_local = 1000; // ms
  static bool lookingForP1 = true;
  static bool lookingForP2 = false;

  // --- Tìm P1 ---
  if (lookingForP1) {
    if (value > cur_peak) {
      cur_peak = value;
    } else if (cur_peak > THRESHOLD && value < cur_peak * 0.7f) {
      P1 = cur_peak;
      foundP1 = true;

      if (lastP1Time > 0) {
        RR_local = (t - lastP1Time) * 0.9f + RR_local * 0.1f;
      }
      lastP1Time = t;

      cur_peak = 0.0f;
      lookingForP1 = false;
      lookingForP2 = true;
      return true;
    }
  }

  // --- Tìm P2 ---
  else if (lookingForP2) {
    unsigned long dt = t - lastP1Time;

    if (dt >= (unsigned long)(0.1f * RR_local) && dt <= (unsigned long)(0.4f * RR_local)) {
      if (value > cur_peak) cur_peak = value;
    } else if (dt > (unsigned long)(0.4f * RR_local)) {
      if (cur_peak > THRESHOLD) {
        P2 = cur_peak;
        foundP2 = true;
      }
      cur_peak = 0.0f;
      lookingForP2 = false;
      lookingForP1 = true;
      return true;
    }
  }

  return false;
}

//// --------- Setup ---------
void setup() {
  Serial.begin(115200);
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Không tìm thấy MAX30105");
    while (1);
  }
  particleSensor.setup();
  Serial.println("AC_scaled,DC,P1,P2,deltaT12,SI,RI,AI,HR,deltaT_up,Glucose");
}

//// --------- Loop ---------
void loop() {
  now = millis();
  long raw = particleSensor.getRed();
  float AC = bandpass_filter((float)raw);
  float AC_scaled = AC * 10.0f;

  if (warmup < WARMUP_SAMPLES) {
    warmup++;
    return;
  }

  // --- Lọc DC (bình quân động) ---
  static float alpha = 0.01;
  DC = DC * (1 - alpha) + raw * alpha;

  // --- Phát hiện chân sóng (foot) ---
  float diff = AC_scaled - prevVal;
  if (prevDiff < 0 && diff > 0 && fabs(prevDiff) > 2.0) {
    tFoot = now;
  }
  prevDiff = diff;
  prevVal = AC_scaled;

  // --- Tìm P1, P2 ---
  float P1 = 0, P2 = 0;
  bool foundP1 = false, foundP2 = false;

  if (find_peaks(AC_scaled, now, P1, P2, foundP1, foundP2)) {

    // Nếu tìm được P1
    if (foundP1) {
      lastP1 = P1;
      tP1 = now;

      if (tLastP1 > 0) {
        RR = (tP1 - tLastP1);
        HR = 60000.0 / RR; // bpm
      }
      tLastP1 = tP1;
    }

    // Nếu tìm được P2
    if (foundP2) {
      lastP2 = P2;
      tP2 = now;

      // --- Tính các chỉ số ---
      deltaT12 = (float)(tP2 - tP1);
      deltaT_up = (tP1 > tFoot) ? (float)(tP1 - tFoot) : 0;

      SI = lastP1 / deltaT12;       // Stiffness Index
      RI = lastP2 / lastP1;         // Reflection Index
      AI = RI * 100.0;              // Augmentation Index %
      Glucose = 0;                  // chưa có dữ liệu thực tế

      // --- In ra CSV (để train TinyML) ---
      Serial.print(AC_scaled); Serial.print(",");
      Serial.print(DC); Serial.print(",");
      Serial.print(lastP1); Serial.print(",");
      Serial.print(lastP2); Serial.print(",");
      Serial.print(deltaT12); Serial.print(",");
      Serial.print(SI); Serial.print(",");
      Serial.print(RI); Serial.print(",");
      Serial.print(AI); Serial.print(",");
      Serial.print(HR); Serial.print(",");
      Serial.print(deltaT_up); Serial.print(",");
      Serial.println(Glucose);
    }
  }

  delay(20); // ~50Hz
}
