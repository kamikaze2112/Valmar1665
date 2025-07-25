#include <Arduino.h>
#include "globals.h"
#include "gps.h"

GPSData GPS;
String nmeaBuffer = "";

void initGPS() {
  Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_RXD, GPS_TXD);
  Serial.println("GPS UART initialized");
}

void readGPSData() {
  while (Serial1.available()) {
    char c = Serial1.read();
    
    if (c == '\n') {
      if (nmeaBuffer.length() > 0) {
        parseNMEA(nmeaBuffer);
        nmeaBuffer = "";
      }
    } else if (c != '\r') {
      nmeaBuffer += c;
    }
  }
}

void parseNMEA(String sentence) {

  GPS_PRINTLN(sentence);
    
  if (sentence.startsWith("$GPGGA") || sentence.startsWith("$GNGGA")) {
    parseGGA(sentence);
  } else if (sentence.startsWith("$GPRMC") || sentence.startsWith("$GNRMC")) {
    parseRMC(sentence);
  } else if (sentence.startsWith("$GPGSV") || sentence.startsWith("$GNGSV")) {
    parseGSV(sentence);
  }
}

void parseGGA(String sentence) {
  int commaIndex[14];
  int commaCount = 0;
  
  // Find comma positions
  for (int i = 0; i < sentence.length() && commaCount < 14; i++) {
    if (sentence.charAt(i) == ',') {
      commaIndex[commaCount] = i;
      commaCount++;
    }
  }
  
  if (commaCount >= 6) {
    // Extract fix quality (field 6)
    String fixQuality = sentence.substring(commaIndex[5] + 1, commaIndex[6]);
    GPS.fixType = fixQuality.toInt();
    
    // Extract number of satellites (field 7)
    String satCount = sentence.substring(commaIndex[6] + 1, commaIndex[7]);
    GPS.satellites = satCount.toInt();
    
    GPS.dataValid = true;
  }
}

void parseRMC(String sentence) {
  int commaIndex[12];
  int commaCount = 0;
  
  // Find comma positions
  for (int i = 0; i < sentence.length() && commaCount < 12; i++) {
    if (sentence.charAt(i) == ',') {
      commaIndex[commaCount] = i;
      commaCount++;
    }
  }
  
  if (commaCount >= 7) {
    // Extract time (field 1)
    String timeStr = sentence.substring(commaIndex[0] + 1, commaIndex[1]);
    if (timeStr.length() >= 6) {
      GPS.hour = convertToMDT(timeStr.substring(0, 2).toInt());
      GPS.minute = timeStr.substring(2, 4).toInt();
      GPS.second = timeStr.substring(4, 6).toInt();
      GPS.timeValid = true;
    }
    
    // Extract speed in knots (field 7)

    // Use Test speed if set

    if (!speedTestSwitch) {

      String speedStr = sentence.substring(commaIndex[6] + 1, commaIndex[7]);
      if (speedStr.length() > 0) {
        GPS.speedKnots = speedStr.toFloat();
        GPS.speedMPH = knotsToMPH(GPS.speedKnots);
      }
    }
  }
}

void parseGSV(String sentence) {
  // GSV parsing for satellite count (alternative method)
  // This is a simplified version - full implementation would track all GSV messages
}

int convertToMDT(int utcHour) {
  // MDT is UTC-6
  int mdtHour = utcHour - 6;
  if (mdtHour < 0) {
    mdtHour += 24;
  }
  return mdtHour;
}

float knotsToMPH(float knots) {

  if (knots <= 0.5){     // filter off speeds of less than ~0.6 mph
      return 0;
  } else {
      return knots * 1.15078;
  }
}