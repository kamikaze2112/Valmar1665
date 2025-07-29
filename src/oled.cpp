#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "globals.h"
#include "gps.h"
#include "encoder.h"
#include "oled.h"
#include "errorHandler.h"
#include "workFunctions.h"
#include "comms.h"
#include "otaUpdate.h"
#include "bitmap.h"

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
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);          // Adjust to center as needed

  // Display the text

  display.drawBitmap(0,0, valmar_oled, 128, 64, WHITE);
  display.display();
  
  DBG_PRINTLN("Init Display complete.");
  DBG_PRINTLN("");
  delay(2000);

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

void updateOLEDfw() {

    const char* msg = "FIRMWARE UPDATE";
    const char* msg2 = "VALMAR_OTA";

    IPAddress ip = WiFi.softAPIP();

    char ipBuffer[16];
    snprintf(ipBuffer, sizeof(ipBuffer), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    const char* ipCStr = ipBuffer;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);

    int16_t x = (SCREEN_WIDTH - w) / 2;
    int16_t y = 0;

    display.setCursor(x, y);
    display.print(msg);
    
    // === Horizontal line ===
    display.drawLine(0, 9, SCREEN_WIDTH - 1, 9, SSD1306_WHITE);

    display.getTextBounds(msg2, 0, 0, &x1, &y1, &w, &h);
    int16_t ipLabelx = (SCREEN_WIDTH - w) / 2;
    y = 30;
    display.setCursor(ipLabelx, y);
    display.print(msg2);

    display.setTextSize(1);
    display.getTextBounds(ipCStr, 0, 0, &x1, &y1, &w, &h);
    int16_t ipAddrx = (SCREEN_WIDTH - w) / 2;
    y = 45;
    display.setCursor(ipAddrx, y);
    display.print(ipCStr);
    display.display();

}


