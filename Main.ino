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

//// --------- Low-pass Butterworth 0.5 Hz @ 50 Hz (lấy DC) ---------
float b_dc[] = {0.00362168, 0.00724336, 0.00362168};
float a_dc[] = {1.0, -1.8226949, 0.83718165};
float x_dc[3] = {0, 0, 0};
float y_dc[3] = {0, 0, 0};


unsigned long now;
int warmup = 0; // đếm số mẫu warmup

//// --------- Hàm band-pass filter(lấy AC) ---------
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

//// --------- Hàm low-pass filter (lấy DC) ---------
float lowpass_dc(float x) {
  x_dc[2] = x_dc[1];
  x_dc[1] = x_dc[0];
  x_dc[0] = x;
  y_dc[2] = y_dc[1];
  y_dc[1] = y_dc[0];
  y_dc[0] = b_dc[0]*x_dc[0] + b_dc[1]*x_dc[1] + b_dc[2]*x_dc[2]
            - a_dc[1]*y_dc[1] - a_dc[2]*y_dc[2];
  return y_dc[0];
}


//// --------- Hàm tìm P1 và P2 ---------
bool find_peaks(float value, unsigned long t, float &P1, float &P2,
                bool &foundP1, bool &foundP2) {
  static float cur_peak = 0.0f;
  static unsigned long lastP1Time = 0;
  static float RR = 1000; // ms, giả định ban đầu 60 bpm
  static bool lookingForP1 = true;
  static bool lookingForP2 = false;

  // --- Tìm P1 ---
  if (lookingForP1) {
    if (value > cur_peak) {
      cur_peak = value;
    } else if (cur_peak > THRESHOLD && value < cur_peak * 0.7f) {
      // Xác nhận P1
      P1 = cur_peak;
      foundP1 = true;

      // cập nhật RR interval
      if (lastP1Time > 0) {
        RR = (t - lastP1Time) * 0.9f + RR * 0.1f; // moving average
      }
      lastP1Time = t;

      // reset để tìm P2
      cur_peak = 0.0f;
      lookingForP1 = false;
      lookingForP2 = true;
      return true;
    }
  }

  // --- Tìm P2 ---
  else if (lookingForP2) {
    unsigned long dt = t - lastP1Time;

    // P2 trong [0.1*RR, 0.4*RR]
    if (dt >= (unsigned long)(0.1f * RR) && dt <= (unsigned long)(0.4f * RR)) {
      if (value > cur_peak) {
        cur_peak = value; // giữ cực đại lớn nhất trong cửa sổ
      }
    }

    // Khi qua cửa sổ 0.4*RR thì xác nhận P2 nếu có
    else if (dt > (unsigned long)(0.4f * RR)) {
      if (cur_peak > THRESHOLD) {  
        P2 = cur_peak;
        foundP2 = true;
      }
      // reset state
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
}

//// --------- Loop ---------
void loop() {
  now = millis();
  long raw = particleSensor.getRed();
  float AC = bandpass_filter((float)raw);
  float AC_scaled = AC * 10.0f;
  float DC = lowpass_dc((float)raw); 
  // Bỏ qua 3s đầu
  if (warmup < WARMUP_SAMPLES) {
    warmup++;
    return;
  }

  // In tín hiệu thô để debug
  Serial.print(now);
  Serial.print(",");
  Serial.print(AC_scaled);
  Serial.print(",");

  // Tìm P1, P2
  float P1 = 0, P2 = 0;
  bool foundP1 = false, foundP2 = false;

  if (find_peaks(AC_scaled, now, P1, P2, foundP1, foundP2)) { 
    float Pi = ((P1 - P2) / DC) * 100.0f;
    Serial.println(Pi);
  }

  delay(20); // ~50Hz
}