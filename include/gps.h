#ifndef GPS_H
#define GPS_H

#include <Arduino.h>  // Only if needed here; might already be in your cpp

struct GPSData {
  bool    fixValid = false;
  uint8_t numSats = 0;
  float   speedMPH = 0.0f;
  uint8_t hour = 0, minute = 0, second = 0;
};

// Declare global instance
extern GPSData GPS;

void setupGPS();
void updateGPS();

#endif