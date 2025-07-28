#include <Arduino.h>
#include "globals.h"
#include "comms.h"
#include "encoder.h"
#include "gps.h"
#include "errorHandler.h"
#include "workFunctions.h"

// PID stuff

float Kp = 1.2f;
float Ki = 0.3f;
float Kd = 0.05f;

float pidIntegral = 0.0f;
float pidPrevError = 0.0f;
float pidOutput = 0.0f;

const float maxPWM = 255.0f;
const float minPWM = 30.0f; // Minimum to overcome motor deadband

int readWorkSwitch() {
  static int lastStableState = HIGH;
  static int lastReadState = HIGH;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;

  int currentState = digitalRead(WORK_SW);

  if (currentState != lastReadState) {
    lastDebounceTime = millis();
    lastReadState = currentState;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentState != lastStableState) {
      lastStableState = currentState;
    }
  }

  workSwitchState = (lastStableState == LOW) ? 1 : 0;
  return workSwitchState;
}

float calculateSeedPerRev(float totalRevs, float calibrationWeight, int runs)
{
    if (totalRevs == 0) return 0.0f; // Avoid divide-by-zero

    DBG_PRINTLN(totalRevs);
    DBG_PRINTLN(calibrationWeight);

    return (calibrationWeight * numberOfRuns) / totalRevs;  // lb/rev
}

float calculateTargetShaftRPM(float speedMph, float targetRateLbPerAcre, float seedPerRev, float implementWidthFt)
{
    if (seedPerRev == 0) return 0.0f; // Avoid divide-by-zero

    // Acres per minute = (speed in mph) × (width in ft) ÷ 495
    float acresPerMinute = (speedMph * implementWidthFt) / 495.0f;

    // Pounds per minute needed = desired rate * acres per minute
    float lbsPerMinute = targetRateLbPerAcre * acresPerMinute;

    // Shaft RPM needed = lb/min ÷ lb/rev × (1 rev/min)
    float shaftRPM = lbsPerMinute / seedPerRev;

    return shaftRPM;
}

uint8_t computePWM(float targetRPM, float actualRPM)
{
    float error = targetRPM - actualRPM;

    // Integrate error
    pidIntegral += error;

    // Optional: clamp integral to avoid wind-up
    if (pidIntegral > 1000.0f) pidIntegral = 1000.0f;
    if (pidIntegral < -1000.0f) pidIntegral = -1000.0f;

    // Derivative
    float derivative = error - pidPrevError;

    // PID output
    pidOutput = (Kp * error) + (Ki * pidIntegral) + (Kd * derivative);
    pidPrevError = error;

    // Clamp and apply deadband, raise errors as necessary.
    if (pidOutput < 0.0f) pidOutput = 0.0f;
    if (pidOutput > maxPWM && errorCode != 3) {
        pidOutput = maxPWM;
        if (!errorRaised) {
            raiseError(2); // Max pwm error
        }
    }
    if (pidOutput > 0.0f && pidOutput < minPWM && errorCode != 3) {
        pidOutput = minPWM;
        if (!errorRaised) {
            raiseError(1); // Min pwm error
        }
    }
    
    if (pidOutput > minPWM && pidOutput < maxPWM && errorCode != 3) {
        clearError();
    }

    return (uint8_t)pidOutput;
}

float calculateApplicationRate() {
    if (GPS.speedMPH <= 0.1) return 0.0;  // Avoid division by zero when stationary
    
    float lbsPerAcre = (Encoder::rpm * seedPerRev * 43560.0) / 
                       (GPS.speedMPH * workingWidth* 5280.0 / 60.0);
    
    return lbsPerAcre;
}