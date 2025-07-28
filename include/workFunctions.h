#ifndef WORKFUNCTIONS_H
#define WORKFUNCTIONS_H

#include <Arduino.h>

// PWM stuff

extern float Kp;
extern float Ki;
extern float Kd;

extern float pidIntegral;
extern float pidPrevError;
extern float pidOutput;

extern const float maxPWM;
extern const float minPWM; // Minimum to overcome motor deadband

int readWorkSwitch();
float calculateSeedPerRev(float totalRevs, float calibrationWeight, int runs);
float calculateTargetShaftRPM(float speedMph, float targetRateLbPerAcre, float seedPerRev, float implementWidthFt);
uint8_t computePWM(float targetRPM, float actualRPM);
float calculateApplicationRate();

#endif