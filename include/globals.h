#ifndef GLOBALS_H
#define GLOBALS_H

extern const char* APP_VERSION;

#define DEBUG_MODE 1 // toggle 1 for on, and 0 for off
#define NMEA_OUTPUT 0 // 1 for on, prints NMEA sentences to serial console.  0 for off.

#if DEBUG_MODE
  #define DBG_PRINT(x)          Serial.print(x)
  #define DBG_PRINTLN(x)        Serial.println(x)
  #define DBG_WRITE(x)          Serial.write(x)
  #define DBG_PRINTF(fmt, ...)  Serial.printf(fmt, __VA_ARGS__)

#else
  #define DBG_PRINT(x)          ((void)0)
  #define DBG_PRINTLN(x)        ((void)0)
  #define DBG_WRITE(x)          ((void)0)
  #define DBG_PRINTF(fmt, ...)  ((void)0)
#endif

#if NMEA_OUTPUT
  #define GPS_PRINT(x)     Serial.print(x)
  #define GPS_PRINTLN(x)   Serial.println(x)
  #define GPS_WRITE(x)     Serial.write(x)

#else
  #define GPS_PRINT(x)     ((void)0)
  #define GPS_PRINTLN(x)   ((void)0)
  #define GPS_WRITE(x)     ((void)0)
#endif

// pin assignments

extern const int PWR_LED;
extern const int CAL_LED;
extern const int CAL_BTN;
extern const int GPS_RXD;
extern const int GPS_TXD;
extern const int GPS_BAUD;
extern const int MOTOR_PWM;
extern const int MOTOR_DIR;
extern const int WORK_SW;
extern const int ENC_B;
extern const int ENC_A;
extern const int OLED_SCL;
extern const int OLED_SDA;
extern const uint8_t RGB_LED;
extern const int BOOT_BTN;
extern float calRevs;

// global variables
extern double counter;
extern bool motorActive;
extern double shaftRPM;
extern int errorCode;
extern bool screenPaired;
extern int workSwitchState;
extern float seedPerRev;
extern int numberOfRuns;
extern bool errorRaised;

extern uint8_t screenAddress[6];
extern uint8_t broadcastAddress[6];

extern bool calibrationMode;
extern float calibrationWeight;
extern float seedPerRev;
extern float targetRPM;
extern bool motorTestSwitch;
extern int motorTestPWM;
extern bool speedTestSwitch;
extern float speedTestSpeed;
extern float targetSeedingRate;
extern float workingWidth;
extern float actualRate;
extern bool pairingMode;

void initPins();
void setupLED();
void handlePairing();

#endif // GLOBALS_H