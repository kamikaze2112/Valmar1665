#include <Arduino.h>
#include "motor.h"
#include "globals.h"
#include "gps.h"
#include "encoder.h"

bool motorActive = false;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 10;  // 100 ms
float actualRate = 0.0;
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
    return;
  }

  // Next priority: Calibration button
  if (isCalButtonPressed()) {
    setMotorPWM(255);  // Run at 100%
    // Keep updating encoder during hold
    //Encoder::update();
    return;
  }

  // Work switch active
  if (readWorkSwitch()) {
    // Placeholder: run motor at 100% for now
    setMotorPWM(255);

    // Later you’ll replace this with PID output:
    // float targetRPM = calculateTargetRPM(...);
    // float actualRPM = getEncoderRPM();
    // float output = pid.compute(targetRPM, actualRPM);
    // setMotorPWM(output);
    
    return;
  }

  // None active — stop the motor
  setMotorPWM(0);
}

void setMotorPWM(int pwm){

  digitalWrite(MOTOR_DIR, LOW);
  analogWrite(MOTOR_PWM, pwm);

}
