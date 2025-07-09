#ifndef GPS_H
#define GPS_H

#include <Arduino.h>  // Only if needed here; might already be in your cpp



// GPS data structure
struct GPSData {
  int fixType = 0;
  int satellites = 0;
  float speedKnots = 0.0;
  float speedMPH = 0.0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  bool timeValid = false;
  bool dataValid = false;
};

// Function prototypes
void initGPS();
void readGPSData();
void parseNMEA(String sentence);
void parseGGA(String sentence);
void parseRMC(String sentence);
void parseGSV(String sentence);

int convertToMDT(int utcHour);
float knotsToMPH(float knots);

extern GPSData GPS;
extern String nmeaBuffer;

#endif