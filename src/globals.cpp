#include <Arduino.h>

#include "gps.h"
#include "globals.h"
#include "encoder.h"

// APP VERSION
const char* APP_VERSION = "1.6.0";

// encoder pinout
// pin 1 power RED -> yellow
// pin 2 bottom spur b GREEN
// pin 3 ground WHITE -> black
// pin 4 top spur a BLACK

// Global Variables

bool calibrationMode = false;
float calibrationWeight = 0.00f;
float seedPerRev = 0.00f;
float targetRPM = 0.0f;
bool motorTestSwitch = false;
int motorTestPWM = 10;
bool speedTestSwitch = false;
float speedTestSpeed = 0.0f;
float targetSeedingRate = 0.0f;
float workingWidth = 60.0f;
float actualRate = 0.0f;
int numberOfRuns = 8;
float calRevs;
bool errorRaised = false;

// PID stuff

float Kp = 1.2f;
float Ki = 0.3f;
float Kd = 0.05f;

float pidIntegral = 0.0f;
float pidPrevError = 0.0f;
float pidOutput = 0.0f;

const float maxPWM = 255.0f;
const float minPWM = 30.0f; // Minimum to overcome motor deadband

// Now define the constants
const int PWR_LED     = 4;
const int CAL_LED     = 5;
const int CAL_BTN     = 6;
const int GPS_RXD     = 7;          //reversed for new gps module
const int GPS_TXD     = 15;         //reversed for new gps module
const int GPS_BAUD    = 460800;
const int MOTOR_PWM   = 16;
const int MOTOR_DIR   = 17;
const int WORK_SW     = 18;
const int ENC_B       = 8;
const int ENC_A       = 3;
const int OLED_SCL    = 42;
const int OLED_SDA    = 41;
const uint8_t RGB_LED = 48;
const int BOOT_BTN    = 0;
double counter = 0.00;
double shaftRPM = 0;
int errorCode = 0;  //0 no error, 1 rate warning, 2 motor stopped

// Local variables

unsigned long buttonPressStart = 0;
bool buttonHeld = false;
bool pairingTriggered = false;
unsigned long lastPairingTime = 0;

// Functions

void handlePairing() {
      bool buttonState = digitalRead(BOOT_BTN);

  if (buttonState == LOW) {
      if (!buttonHeld) {
          buttonPressStart = millis();
          buttonHeld = true;
          pairingTriggered = false;  // reset on fresh press
      } else if ((millis() - buttonPressStart >= 3000) && !pairingTriggered) {
          pairingMode = true;
          lastPairingTime = 0;
          pairingTriggered = true;  // ✅ prevent repeat triggers
          Serial.println("🔁 3-second hold detected, entering pairing mode");
      }
  } else {
      // Reset everything when button released
      buttonHeld = false;
      pairingTriggered = false;
      buttonPressStart = 0;
  }
}

void initPins() {
    DBG_PRINTLN("Init Pins...");

    pinMode(PWR_LED, OUTPUT);
    pinMode(CAL_LED, OUTPUT);
    pinMode(CAL_BTN, INPUT_PULLUP);
    pinMode(MOTOR_DIR, OUTPUT);
    pinMode(WORK_SW, INPUT_PULLUP);
    pinMode(ENC_B, INPUT);
    pinMode(ENC_A, INPUT);
    pinMode(BOOT_BTN, INPUT_PULLUP);
    
    DBG_PRINTLN("Init pins complete.");
    DBG_PRINTLN("");

}

int workSwitchState = 0;

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

    // Clamp and apply deadband
    if (pidOutput < 0.0f) pidOutput = 0.0f;
    if (pidOutput > maxPWM) pidOutput = maxPWM;
    if (pidOutput > 0.0f && pidOutput < minPWM) pidOutput = minPWM;

    return (uint8_t)pidOutput;
}

float calculateApplicationRate() {
    if (GPS.speedMPH <= 0.1) return 0.0;  // Avoid division by zero when stationary
    
    float lbsPerAcre = (Encoder::rpm * seedPerRev * 43560.0) / 
                       (GPS.speedMPH * workingWidth* 5280.0 / 60.0);
    
    return lbsPerAcre;
}

