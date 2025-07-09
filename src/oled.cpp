#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "globals.h"
#include "gps.h"
#include "encoder.h"
#include "oled.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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
    snprintf(fixStr, sizeof(fixStr), "FIX:%d", GPS.fixType);
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
    display.print(GPS.satellites);

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