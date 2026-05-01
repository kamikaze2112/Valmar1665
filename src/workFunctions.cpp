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

  if (!incomingData.workSwitchOverride){
    workSwitchState = (lastStableState == LOW) ? 1 : 0;
  } else {
    workSwitchState = true;
  } 
  
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

uint8_t computePWM(float targetRPM, float actualRPM, bool silent)
{
    float error = targetRPM - actualRPM;

    pidIntegral += error;

    if (pidIntegral > 1000.0f) pidIntegral = 1000.0f;
    if (pidIntegral < -1000.0f) pidIntegral = -1000.0f;

    float derivative = error - pidPrevError;

    pidOutput = (Kp * error) + (Ki * pidIntegral) + (Kd * derivative);
    pidPrevError = error;

    if (pidOutput < 0.0f) pidOutput = 0.0f;
    if (pidOutput > maxPWM) {
        pidOutput = maxPWM;
        if (!silent && !errorRaised && errorCode != 3) raiseError(2);
    }
    if (pidOutput > 0.0f && pidOutput < minPWM) {
        pidOutput = minPWM;
        if (!silent && !errorRaised && errorCode != 3) raiseError(1);
    }
    if (!silent && pidOutput > minPWM && pidOutput < maxPWM && errorCode != 3) {
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