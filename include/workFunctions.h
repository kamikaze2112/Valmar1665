#ifndef WORKFUNCTIONS_H
#define WORKFUNCTIONS_H

#include <Arduino.h>

int readWorkSwitch();
float calculateSeedPerRev(float totalRevs, float calibrationWeight, int runs);
float calculateTargetShaftRPM(float speedMph, float targetRateLbPerAcre, float seedPerRev, float implementWidthFt);
uint8_t computePWM(float targetRPM, float actualRPM);
float calculateApplicationRate();

#endif