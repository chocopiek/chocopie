#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

//// --------- Cấu hình ---------
#define FS      50.0   // Tần số lấy mẫu (Hz)
#define FC_LOW  8.0    // Low-pass cutoff
#define FC_HIGH 0.5    // High-pass cutoff

//// --------- Biến filter Butterworth bậc 2 (band-pass) ---------
// Hệ số filter được tính trước bằng Python/Matlab
// band-pass 0.5–8 Hz @ 50Hz, butterworth bậc 2
// Dùng dạng IIR: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
float b[] = {0.0675, 0.0, -0.0675}; 
float a[] = {1.0, -1.7991, 0.8643};

float x_buff[3] = {0,0,0}; 
float y_buff[3] = {0,0,0};

unsigned long now;

//// --------- Hàm band-pass filter ---------
float bandpass_filter(float x) {
  // Dịch buffer
  x_buff[2] = x_buff[1];
  x_buff[1] = x_buff[0];
  x_buff[0] = x;

  y_buff[2] = y_buff[1];
  y_buff[1] = y_buff[0];

  // IIR filter
  y_buff[0] = b[0]*x_buff[0] + b[1]*x_buff[1] + b[2]*x_buff[2]
              - a[1]*y_buff[1] - a[2]*y_buff[2];

  return y_buff[0];
}

void setup() {
  Serial.begin(115200);
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Không tìm thấy MAX30105");
    while (1);
  }
  particleSensor.setup(); // cấu hình mặc định (LED power, sample rate…)
}

void loop() {
  now = millis();

  // Lấy giá trị raw RED từ cảm biến
  long raw = particleSensor.getRed();

  // Đưa qua band-pass filter
  float AC = bandpass_filter((float)raw);

  // Scale lại để dễ nhìn
  float AC_scaled = AC * 10.0f;

  // Xuất ra serial (time, raw, filtered)
  Serial.print(now); Serial.print(",");
  //Serial.print(raw); Serial.print(",");
  Serial.println(AC_scaled);

  delay(20); // ~50 Hz
}
