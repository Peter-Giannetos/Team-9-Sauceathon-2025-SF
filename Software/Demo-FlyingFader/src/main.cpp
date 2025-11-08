// SPDX-FileCopyrightText: 2022 John Park for Adafruit Industries
// SPDX-License-Identifier: MIT
// Motorized fader demo - Oscillating version
// capsense implementation by @todbot / Tod Kurt
// Adapted for ESP32 WROOM

#include <Arduino.h>

// ESP32 GPIO pins for motor control
const int pwmA = 32;  // motor pin A (PWM)
const int pwmB = 33;  // motor pin B (PWM)

// PWM settings for ESP32 LEDC
const int pwmFreq = 100;      // 100 Hz PWM frequency
const int pwmChannelA = 0;    // LEDC channel 0 for motor A
const int pwmChannelB = 1;    // LEDC channel 1 for motor B
const int pwmResolution = 8;  // 8-bit resolution (0-255)

const int fader = 26;  // ESP32 ADC pin (GPIO26) for fader position
int fader_pos = 0;
float filter_amt = 0.75;
float speed = 1.0;

// Oscillation settings
const int position_high = 250;
const int position_low = 10;
int target_position = position_high;
unsigned long last_switch_time = 0;
const unsigned long switch_interval = 2000;  // 2 seconds in milliseconds

// Forward declarations
int calculateEasedSpeed(int distance);
void go_to_position(int new_position);

void setup() {
  Serial.begin(115200);  // ESP32 typically uses 115200 baud
  delay(1000);
  
  Serial.println("Motorized Fader - Oscillating Mode");
  Serial.println("Oscillating between position 250 and 10 every 2 seconds");
  
  // Configure LEDC PWM channels for ESP32
  ledcSetup(pwmChannelA, pwmFreq, pwmResolution);
  ledcSetup(pwmChannelB, pwmFreq, pwmResolution);
  ledcAttachPin(pwmA, pwmChannelA);
  ledcAttachPin(pwmB, pwmChannelB);
  ledcWrite(pwmChannelA, 0);
  ledcWrite(pwmChannelB, 0);
  
  // Configure ADC for fader reading
  analogReadResolution(12);  // ESP32 has 12-bit ADC
  analogSetAttenuation(ADC_11db);  // Full range: 0-3.3V
  
  // Initialize timer
  last_switch_time = millis();
}

void loop() {
  unsigned long current_time = millis();
  
  // Check if it's time to switch positions
  if (current_time - last_switch_time >= switch_interval) {
    // Toggle target position
    if (target_position == position_high) {
      target_position = position_low;
      Serial.println("\n=== Switching to position 10 ===");
    } else {
      target_position = position_high;
      Serial.println("\n=== Switching to position 250 ===");
    }
    last_switch_time = current_time;
  }
  
  // Move to target position (position updates are printed inside this function)
  go_to_position(target_position);
  
  // Small delay to avoid flooding serial output when at target
  delay(100);
}

// Easing function for smooth deceleration
// Returns PWM value (0-255) based on distance to target
int calculateEasedSpeed(int distance) {
  // Minimum and maximum PWM values
  const int MIN_SPEED = 80;   // Minimum speed to prevent stalling
  const int MAX_SPEED = 255;  // Maximum speed
  
  // Distance thresholds for easing
  const int FAST_ZONE = 80;   // Full speed when distance > 80
  const int SLOW_ZONE = 15;   // Start easing at distance < 15
  
  if (distance > FAST_ZONE) {
    // Far away - full speed
    return MAX_SPEED;
  } else if (distance > SLOW_ZONE) {
    // Medium distance - linear interpolation
    float ratio = (float)(distance - SLOW_ZONE) / (FAST_ZONE - SLOW_ZONE);
    return MIN_SPEED + (int)(ratio * (MAX_SPEED - MIN_SPEED));
  } else {
    // Close to target - ease out using quadratic curve
    float ratio = (float)distance / SLOW_ZONE;
    ratio = ratio * ratio;  // Quadratic easing
    return MIN_SPEED + (int)(ratio * (MAX_SPEED - MIN_SPEED));
  }
}

void go_to_position(int new_position) {
  fader_pos = int(analogRead(fader) / 16);  // ESP32 12-bit ADC / 16 = 0-255 range
  
  static unsigned long last_print = 0;
  
  while (abs(fader_pos - new_position) > 4) {
    int distance = abs(fader_pos - new_position);
    int pwm_speed = calculateEasedSpeed(distance);
    
    if (fader_pos > new_position) {
      // Move toward lower position
      ledcWrite(pwmChannelA, pwm_speed);
      ledcWrite(pwmChannelB, 0);
    } else if (fader_pos < new_position) {
      // Move toward higher position
      ledcWrite(pwmChannelA, 0);
      ledcWrite(pwmChannelB, pwm_speed);
    }
      
    fader_pos = int(analogRead(fader) / 16);  // ESP32 12-bit ADC / 16 = 0-255 range
    
    // Print position updates while moving
    if (millis() - last_print > 100) {
      Serial.print("Current position: ");
      Serial.print(fader_pos);
      Serial.print(" (PWM: ");
      Serial.print(pwm_speed);
      Serial.println(")");
      last_print = millis();
    }
  }
  
  // Stop motors
  ledcWrite(pwmChannelA, 0);
  ledcWrite(pwmChannelB, 0);
  
  // Print final position when stopped
  Serial.print("Current position: ");
  Serial.print(fader_pos);
  Serial.println(" (STOPPED)");
}
