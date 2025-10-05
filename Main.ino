#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

//// --------- Cấu hình ---------
#define FS      50.0   // Tần số lấy mẫu (Hz)
#define THRESHOLD 30.0 // Ngưỡng tránh nhiễu
#define WARMUP_SAMPLES 150 // Bỏ qua ~3s đầu (50Hz * 3s)
#define BUFFER_SIZE 200     // ~4s dữ liệu để tìm chân sóng

//// --------- Biến filter Butterworth bậc 2 (band-pass) ---------
float b[] = {0.0675, 0.0, -0.0675};
float a[] = {1.0, -1.7991, 0.8643};

float x_buff[3] = {0,0,0};
float y_buff[3] = {0,0,0};

//// --------- Low-pass Butterworth 0.5 Hz @ 50 Hz (lấy DC) ---------
float b_dc[] = {0.00362168, 0.00724336, 0.00362168};
float a_dc[] = {1.0, -1.8226949, 0.83718165};
float x_dc[3] = {0, 0, 0};
float y_dc[3] = {0, 0, 0};

//// --------- Buffer lưu giá trị AC và thời gian ---------
float ac_buffer[BUFFER_SIZE];
unsigned long t_buffer[BUFFER_SIZE];
int buf_index = 0;

unsigned long now;
int warmup = 0;

//// --------- Bộ lọc ---------
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

unsigned long find_wavefoot(unsigned long t_P1) {
  int end = buf_index - 1;
  if (end < 0) end = BUFFER_SIZE - 1;

  int start = (end - 50 + BUFFER_SIZE) % BUFFER_SIZE; // khoảng ~1s trước P1

  float min_value = 1e9;
  int min_index = end;

  for (int i = start; i != end; i = (i + 1) % BUFFER_SIZE) {
    if (ac_buffer[i] < min_value) {
      min_value = ac_buffer[i];
      min_index = i;
    }
  }

  return t_buffer[min_index];
}


//// --------- Hàm tìm P1, P2 ---------
bool find_peaks(float value, unsigned long t, float &P1, float &P2,
                bool &foundP1, bool &foundP2, unsigned long &t_P1, unsigned long &t_P2,
                float &HR) {
  static float cur_peak = 0.0f;
  static unsigned long lastP1Time = 0;
  static float RR = 1000;
  static bool lookingForP1 = true;
  static bool lookingForP2 = false;

  if (lookingForP1) {
    if (value > cur_peak) {
      cur_peak = value;
    } else if (cur_peak > THRESHOLD && value < cur_peak * 0.7f) {
      P1 = cur_peak;
      foundP1 = true;
      t_P1 = t;

      if (lastP1Time > 0) {
        RR = (t - lastP1Time) * 0.9f + RR * 0.1f;
        HR = 60000.0f / RR;
      }
      lastP1Time = t;

      cur_peak = 0.0f;
      lookingForP1 = false;
      lookingForP2 = true;
      return true;
    }
  }
  else if (lookingForP2) {
    unsigned long dt = t - lastP1Time;
    if (dt >= (unsigned long)(0.1f * RR) && dt <= (unsigned long)(0.4f * RR)) {
      if (value > cur_peak) cur_peak = value;
    }
    else if (dt > (unsigned long)(0.4f * RR)) {
      if (cur_peak > THRESHOLD) {  
        P2 = cur_peak;
        foundP2 = true;
        t_P2 = t;
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

  // In tiêu đề cột
  Serial.println("time,raw,AC,DC,Pi,ΔT12,SI,RI,AI,HR,ΔT_up");
}

//// --------- Loop ---------
void loop() {
  now = millis();
  long raw = particleSensor.getRed();
  float AC = bandpass_filter((float)raw);
  float AC_scaled = AC * 10.0f;
  float DC = lowpass_dc((float)raw);

  // Lưu vào buffer
  ac_buffer[buf_index] = AC_scaled;
  t_buffer[buf_index] = now;
  buf_index = (buf_index + 1) % BUFFER_SIZE;

  if (warmup < WARMUP_SAMPLES) {
    warmup++;
    return;
  }

  float P1 = 0, P2 = 0;
  bool foundP1 = false, foundP2 = false;
  unsigned long t_P1 = 0, t_P2 = 0;
  float HR = 0.0f, deltaT12 = 0, deltaT_up = 0;
  float SI = 0, RI = 0, AI = 0, Pi = 0;

  if (find_peaks(AC_scaled, now, P1, P2, foundP1, foundP2, t_P1, t_P2, HR)) { 
    Pi = ((P1 - P2) / DC) * 100.0f;

    if (foundP1) {
      unsigned long t_foot = find_wavefoot(t_P1);
      deltaT_up = (t_P1 - t_foot) / 1000.0f;
    }

    if (foundP1 && foundP2) {
      deltaT12 = (t_P2 - t_P1) / 1000.0f;
      if (deltaT12 > 0) SI = 180.0f / deltaT12;
      RI = P2 / P1;
      AI = (P1 - P2) / P1;
    }

    Serial.print(now); Serial.print(",");
    Serial.print(raw); Serial.print(",");
    Serial.print(AC_scaled); Serial.print(",");
    Serial.print(DC); Serial.print(",");
    Serial.print(Pi); Serial.print(",");
    Serial.print(deltaT12); Serial.print(",");
    Serial.print(SI); Serial.print(",");
    Serial.print(RI); Serial.print(",");
    Serial.print(AI); Serial.print(",");
    Serial.print(HR); Serial.print(",");
    Serial.println(deltaT_up);
  }

  delay(20); // ~50Hz
}
