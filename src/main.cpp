/*

This is the main program for the Valmar 1665 controller built by Trevor Kidd
for S5 Farms LTD. No warranty is implied and use is at your own risk. You may
freely modify, copy, and or distribute this source code as long as this header
remains intatct. All reference materials and design files can be found on
github:  https://github.com/kamikaze2112/Valmar1665

06/2025

*/

#include <Arduino.h>
#include "globals.h"
#include "gps.h"
#include "motor.h"
#include "encoder.h"
#include "comms.h"
#include <FastLED.h>
#include "nonBlockingTimer.h"

NonBlockingTimer timer;


void debugPrint() {

    DBG_PRINTLN(incomingData.calibrationMode);
    DBG_PRINTLN(incomingData.calibrationWeight);
    DBG_PRINTLN(incomingData.seedingRate);
}

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 100; // Update every 100 ms

const unsigned long DEBOUNCE_DELAY = 50;  // ms

int buttonState = 0;  // toggled state (0 or 1)
bool lastButtonReading = HIGH;
bool lastDebouncedState = HIGH;
unsigned long lastDebounceTime = 0;

bool calibrationMode = 0; //sets calibration mode to false on startup by default

void handleBootButton() {
  bool reading = digitalRead(BOOT_BTN);

  if (reading != lastDebouncedState) {
    lastDebounceTime = millis();  // reset debounce timer
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != lastButtonReading) {
      lastButtonReading = reading;

      // Button press (falling edge, assuming active-low)
      if (reading == LOW) {
        buttonState = !buttonState;
        if (buttonState)
        counter = 0;
        Encoder::resetRevolutions;
      }
    }
  }

  lastDebouncedState = reading;
}

void setup() 
{
  Serial0.begin(115200);

  DBG_PRINTLN("**********************");
  DBG_PRINTLN("");
  DBG_PRINTLN("Valmar 1665 DEBUG MODE");
  DBG_PRINTLN("");
  DBG_PRINTLN("**********************");
  DBG_PRINTLN("");

  timer.set(debugPrint, 1000);
  
  initPins();
  
  initDisplay();
  
  setupGPS();

  setupComms();

  Encoder::begin(ENC_A);

  digitalWrite(PWR_LED, HIGH);

}

void loop()
{

  timer.update();
  Encoder::update();
  updateGPS();

if (readWorkSwitch()) {
    neopixelWrite(RGB_LED, 0, 2, 0);
  }
  else {
    neopixelWrite(RGB_LED, 2, 0, 0);
  }

  if (calibrationMode) {
    updateOLEDcal();
    digitalWrite(CAL_LED, HIGH);
  }
  else {
    updateOLEDgps();
    digitalWrite(CAL_LED, LOW);
  }

  handleBootButton();

  if (buttonState) {
    calibrationMode = 1;
  } else {
    calibrationMode = 0;
  }

  handleCalButton();

  sendCommsUpdate();

  if (incomingData.calibrationMode) {
    calibrationMode = true;
  } else {
    calibrationMode = false;
  }
}