#ifndef GLOBALS_H
#define GLOBALS_H

// OLED Setup
#include <Adafruit_SSD1306.h>

//Global display object
extern Adafruit_SSD1306 display;

#define DEBUG_MODE 1 // toggle 1 for on, and 0 for off
#define NMEA_OUTPUT 0 // 1 for on, prints NMEA sentences to serial console.  0 for off.

#if DEBUG_MODE
  #define DBG_PRINT(x)     Serial0.print(x)
  #define DBG_PRINTLN(x)   Serial0.println(x)
  #define DBG_WRITE(x)     Serial0.write(x)

#else
  #define DBG_PRINT(x)     ((void)0)
  #define DBG_PRINTLN(x)   ((void)0)
  #define DBG_WRITE(x)     ((void)0)
#endif

#if NMEA_OUTPUT
  #define GPS_PRINT(x)     Serial0.print(x)
  #define GPS_PRINTLN(x)   Serial0.println(x)
  #define GPS_WRITE(x)     Serial0.write(x)

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
extern const int MOTOR_PWM;
extern const int MOTOR_DIR;
extern const int WORK_SW;
extern const int ENC_B;
extern const int ENC_A;
extern const int OLED_SCL;
extern const int OLED_SDA;
extern const uint8_t RGB_LED;
extern const int BOOT_BTN;
extern double counter;
extern bool motorActive;
extern double shaftRPM;
extern int errorCode;

void initPins();

extern bool calibrationMode;

// Returns 1 if the switch is active (pressed), 0 if not
int readWorkSwitch();

// Optional: a global that always has the most recent debounced state
extern int workSwitchState;

void setupLED();

void initDisplay();
void updateOLEDgps();
void updateOLEDcal();

#endif // GLOBALS_H