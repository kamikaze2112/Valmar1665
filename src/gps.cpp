#include <Arduino.h>
#include "globals.h"
#include "gps.h"

HardwareSerial gps(1);

GPSData GPS;

void setupGPS()
{
    DBG_PRINTLN("Init GPS...");
    DBG_PRINTLN("Starting GPS at 9600bps");
    
    gps.begin(9600, SERIAL_8N1, GPS_TXD, GPS_RXD);
    
    DBG_PRINT("GPS Started at ");
    DBG_PRINT(gps.baudRate());

    DBG_PRINTLN(" Changing GPS baudrate to 115200...");
    
    gps.println("$PMTK251,115200*1F");
    delay(100);
    gps.end();
    delay(100);

    gps.setRxBufferSize(1024);

    gps.begin(115200, SERIAL_8N1, GPS_TXD, GPS_RXD);
    delay(100);

    DBG_PRINT("GPS restarted at ");
    DBG_PRINTLN(gps.baudRate());

    DBG_PRINTLN("Setting 5hz mode...");

    gps.println("$PMTK220,200*2C");

    DBG_PRINTLN("GPS refresh rate set to 5hz.");
    DBG_PRINTLN("Setting GPS to return only GPRMC + GGA...");

    gps.println("$PMTK314,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"); // <-- RMC, GGA, and VTG "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28");  // Output only RMC and GGA
    delay(100);

    DBG_PRINTLN("Init GPS complete.");
    DBG_PRINTLN("");
}

static bool validateChecksum(const char *nmea) 
{
  const char *asterisk = strchr(nmea, '*');
  if (!asterisk || strlen(asterisk) < 3) return false;
  uint8_t calc = 0;
  for (const char *p = nmea + 1; p < asterisk; ++p) calc ^= *p;
  return (calc == strtol(asterisk + 1, nullptr, 16));
}

static const char *getField(const char *str, int fieldNum) 
{
  for (int i = 0; i < fieldNum; i++) {
    str = strchr(str, ',');
    if (!str) return "";
    ++str;
  }
  return str;
}

static void parseTime(const char *utc) 
{
  if (strlen(utc) < 6) return;
  int h = (utc[0]-'0')*10 + (utc[1]-'0');
  int m = (utc[2]-'0')*10 + (utc[3]-'0');
  int s = (utc[4]-'0')*10 + (utc[5]-'0');

  h -= 6; // MDT = UTC-6
  if (h < 0) h += 24;

  GPS.hour = h;
  GPS.minute = m;
  GPS.second = s;
}

static void parseGPRMC(const char *nmea) 
{
  if (speedTestSwitch) {          // use test speed regardless of fix status.
  
    GPS.speedMPH = speedTestSpeed;
  
  }

  if (!validateChecksum(nmea)) return;

  const char *status = getField(nmea, 2);
  GPS.fixValid = (status[0] == 'A');

  if (!GPS.fixValid && !speedTestSwitch) GPS.speedMPH = 0;
  if (!GPS.fixValid) return;

  parseTime(getField(nmea, 1));
 
  const char *speedStr = getField(nmea, 7);
  float speedKnots = atof(speedStr);
  
  if (!speedTestSwitch) {         // use gps based speed when fix available.
                         
    GPS.speedMPH = speedKnots * 1.15078f;
  
  }

}

static void parseGPGGA(const char *nmea) 
{
  if (!validateChecksum(nmea)) return;

  const char *fix = getField(nmea, 6);
  if (fix[0] == '0') {
    GPS.fixValid = false;
    return;
  }

  GPS.fixValid = true;  

  const char *satStr = getField(nmea, 7);
  GPS.numSats = atoi(satStr);
}

/* static void parseGPVTG(const char *nmea) {
  if (!validateChecksum(nmea)) return;

  const char *speedKnotsStr = getField(nmea, 5);
  float speedKnots = atof(speedKnotsStr);
  GPS.speedMPH = speedKnots * 1.15078f;  // Optional
} */

void updateGPS() 
{
  static char buffer[100];
  static size_t pos = 0;

  while (gps.available()) {
    char c = gps.read();
    GPS_WRITE(c);

    if (c == '$') {
      pos = 0;
      buffer[pos++] = c;
    } else if (c == '\n') {
      buffer[pos] = '\0';
      if (strncmp(buffer, "$GPRMC", 6) == 0)
        parseGPRMC(buffer);
      else if (strncmp(buffer, "$GPGGA", 6) == 0)
        parseGPGGA(buffer);
/*       else if (strncmp(buffer, "$GPVTG", 6) == 0)
        parseGPVTG(buffer); */

      pos = 0;
    } else if (pos < sizeof(buffer) - 1) {
      buffer[pos++] = c;
    }
  }
}

const GPSData& getGPSData() {
  return GPS;
}