#include <Arduino.h>
#include "motor.h"
#include "globals.h"
#include "gps.h"
#include "encoder.h"

bool motorActive = false;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 10;  // 100 ms
bool hasPrinted = false;

bool lastCalBtnState = false;
unsigned long lastCalDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Function to read the debounced state of the calibration button
bool isCalButtonPressed() {
  static bool debouncedState = false;
  static bool lastReading = HIGH;  // default not pressed

  bool reading = digitalRead(CAL_BTN);

  if (reading != lastReading) {
    lastCalDebounceTime = millis();
  }

  if ((millis() - lastCalDebounceTime) > debounceDelay) {
    if (reading != debouncedState) {
      debouncedState = reading;
    }
  }

  lastReading = reading;
  return (debouncedState == LOW);  // Active LOW = pressed
}

void updateMotorControl() {
  // Highest priority: Motor test mode from screen
  if (motorTestSwitch) {
    setMotorPWM(motorTestPWM);
    motorActive = true;
    return;
  }

  // Next priority: Calibration button
  if (isCalButtonPressed()) {
    setMotorPWM(255);
    motorActive = true;
    return;
  }

  // Work switch active
  if (readWorkSwitch()) {
    motorActive = true;    
    return;
  }

  // None active — stop the motor
  setMotorPWM(0);
  motorActive = false;
}

void setMotorPWM(int pwm){

  if (pwm < minPWM && readWorkSwitch()) {
      digitalWrite(MOTOR_DIR, HIGH);
      analogWrite(MOTOR_PWM, 30);
      errorCode = 1; // Motor min speed
  } else {
      digitalWrite(MOTOR_DIR, HIGH);
      analogWrite(MOTOR_PWM, pwm);
  }
}
