#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

// Sampling settings
const float dt = 0.02f; // 20 ms sampling => 50 Hz

// HPF params (first-order high-pass, causal)
// fc_hp = 0.5 Hz (cắt drift / hô hấp)
const float fc_hp = 0.5f;
float alpha_hp;

// LPF params (first-order low-pass)
// fc_lp = 8.0 Hz (lọc nhiễu cao tần)
const float fc_lp = 8.0f;
float alpha_lp;

// HPF state
float x_prev_hp = 0.0f;
float y_prev_hp = 0.0f;

// LPF state
float y_prev_lp = 0.0f;

void setup() {
  Serial.begin(115200);
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Không tìm thấy MAX30102!");
    while (1);
  }
  particleSensor.setup();

  // Tính hệ số alpha cho IIR RC (first-order)
  // LPF: y[n] = y[n-1] + alpha_lp * (x[n] - y[n-1])  với alpha_lp = dt/(RC + dt), RC = 1/(2*pi*fc)
  float RC_lp = 1.0f / (2.0f * 3.14159265f * fc_lp);
  alpha_lp = dt / (RC_lp + dt);

  // HPF (implement bằng differencing + LPF form):
  // High-pass via y = alpha_hp*(y_prev + x - x_prev)
  // alpha_hp = RC / (RC + dt)  với RC = 1/(2*pi*fc_hp)
  float RC_hp = 1.0f / (2.0f * 3.14159265f * fc_hp);
  alpha_hp = RC_hp / (RC_hp + dt);


}

void loop() {
  unsigned long now = millis();
  uint32_t raw = particleSensor.getRed();

  // --- High-pass (causal) ---
  float x = (float)raw;
  float y_hp = alpha_hp * (y_prev_hp + x - x_prev_hp);
  x_prev_hp = x;
  y_prev_hp = y_hp;

  // --- Low-pass (causal) ---
  float y_lp = y_prev_lp + alpha_lp * (y_hp - y_prev_lp);
  y_prev_lp = y_lp;

  // --- Shift to positive if needed (so AC > 0)
  static float min_seen = 1e9f;
  if (y_lp < min_seen) min_seen = y_lp;
  float AC_pos = y_lp - min_seen + 1.0f; // +1 to avoid zero

  // Output CSV
  if(AC_pos != 1){
  Serial.print(now); Serial.print(",");
  Serial.println(AC_pos);}

  delay(20); // maintain sampling
}
