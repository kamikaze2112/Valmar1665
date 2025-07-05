#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "gps.h"
#include "globals.h"
#include "encoder.h"

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

// PID stuff

float Kp = 1.2f;
float Ki = 0.3f;
float Kd = 0.05f;

float pidIntegral = 0.0f;
float pidPrevError = 0.0f;
float pidOutput = 0.0f;

const float maxPWM = 255.0f;
const float minPWM = 30.0f; // Minimum to overcome motor deadband

// OLED Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Now define the constants
const int PWR_LED     = 4;
const int CAL_LED     = 5;
const int CAL_BTN     = 6;
const int GPS_RXD     = 7;
const int GPS_TXD     = 15;
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
int errorCode = 0;



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

void initDisplay() {

  DBG_PRINTLN("Init Display...");

  // Start I2C with custom pins
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is the default I2C address
    DBG_PRINTLN(F("SSD1306 allocation failed"));
    for (;;); // halt
  }

  // Clear display buffer
  display.clearDisplay();

  // Set text properties
  display.setTextSize(2);             // Size 2 for larger text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 26);          // Adjust to center as needed

  // Display the text
  display.print(F("VALMAR"));
  display.display();
  
  DBG_PRINTLN("Init Display complete.");
  DBG_PRINTLN("");
  delay(1000);

}

void updateOLEDgps() {

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // === Top row ===
    display.setCursor(0, 0);
    display.print("VALMAR");

    // Compose and right-align FIX
    char fixStr[10];
    snprintf(fixStr, sizeof(fixStr), "FIX:%d", GPS.fixValid ? 1 : 0);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(fixStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w, 0);
    display.print(fixStr);

    // === Horizontal line ===
    display.drawLine(0, 9, SCREEN_WIDTH - 1, 9, SSD1306_WHITE);

    // === MPH label ===
    display.setCursor(0, 12);
    display.print("MPH:");

    // === Centered speed (size 2) ===
    char speedStr[10];
    snprintf(speedStr, sizeof(speedStr), "%.1f", GPS.speedMPH);

    display.setTextSize(2);
    display.getTextBounds(speedStr, 0, 0, &x1, &y1, &w, &h);
    int16_t centerX = (SCREEN_WIDTH - w) / 2;
    int16_t centerY = (SCREEN_HEIGHT - h) / 2;
    display.setCursor(centerX, centerY);
    display.print(speedStr);

    // === Bottom row ===
    display.setTextSize(1);
    display.setCursor(0, SCREEN_HEIGHT - 8);
    display.print("SATS:");
    display.print(GPS.numSats);

    // Placeholder variable for motor state
    extern bool motorActive;  // Declare it somewhere in your globals

    if (motorActive) {
        display.setTextSize(1);
        const char *motorLabel = "MOTOR";
        display.getTextBounds(motorLabel, 0, 0, &x1, &y1, &w, &h);
        int16_t motorX = (SCREEN_WIDTH - w) / 2;
        display.setCursor(motorX, SCREEN_HEIGHT - 8);
        display.print(motorLabel);
    }

    if (readWorkSwitch()) {
        display.setCursor(102, SCREEN_HEIGHT - 8);
        display.print("WORK");
    }

    display.display();
}

void updateOLEDcal() {

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);


    // === Top row ===
    display.setCursor(0, 0);
    display.print("VALMAR");

    // Compose and right-align CALIBRAION
    char fixStr[12];
    snprintf(fixStr, sizeof(fixStr), "CALIBRATION");

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(fixStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w, 0);
    display.print(fixStr);

    // === Horizontal line ===
    display.drawLine(0, 9, SCREEN_WIDTH - 1, 9, SSD1306_WHITE);

    // === Revs label ===
    display.setCursor(0, 12);
    display.print("REVS:");

    // === Centered revolutions (size 2) ===
    char revsStr[10];
    
    snprintf(revsStr, sizeof(revsStr), "%.2f", Encoder::revs);

    display.setTextSize(2);
    display.getTextBounds(revsStr, 0, 0, &x1, &y1, &w, &h);
    int16_t centerX = (SCREEN_WIDTH - w) / 2;
    int16_t centerY = (SCREEN_HEIGHT - h) / 2;
    display.setCursor(centerX, centerY);
    display.print(revsStr);

    if (motorActive) {
        display.setTextSize(1);
        const char *motorLabel = "MOTOR CAL";
        display.getTextBounds(motorLabel, 0, 0, &x1, &y1, &w, &h);
        int16_t motorX = (SCREEN_WIDTH - w) / 2;
        display.setCursor(motorX, SCREEN_HEIGHT - 8);
        display.print(motorLabel);
    }
    display.display();
}

float calculateSeedPerRev(float totalRevs, float calibrationWeight)
{
    if (totalRevs == 0) return 0.0f; // Avoid divide-by-zero

    DBG_PRINTLN(totalRevs);
    DBG_PRINTLN(calibrationWeight);

    return calibrationWeight / totalRevs;  // lb/rev
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