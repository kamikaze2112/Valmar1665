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
#include "oled.h"
#include "prefs.h"
#include <FastLED.h>
#include "nonBlockingTimer.h"

NonBlockingTimer timer;

void debugPrint() {

/*     DBG_PRINT("Incoming calibrationMode: ");
    DBG_PRINTLN(incomingData.calibrationMode);

    DBG_PRINT("Incoming seedingRate: ");
    DBG_PRINTLN(incomingData.seedingRate);

    DBG_PRINT("GPS.speedMPH: ");
    DBG_PRINTLN(GPS.speedMPH);
    DBG_PRINT("shaftRPM");
    DBG_PRINTLN(shaftRPM);
    
    DBG_PRINT("calibrationMode: ");
    DBG_PRINTLN(calibrationMode ? "true" : "false");

    DBG_PRINT("Incoming Motor Switch: ");
    DBG_PRINTLN(incomingData.motorTestSwitch);

    DBG_PRINT("Incoming Motor PWM: ");
    DBG_PRINTLN(incomingData.motorTestPWM);
  
    DBG_PRINT("Incoming speedTestSwitch: ");
    DBG_PRINTLN(incomingData.speedTestSwitch);

    DBG_PRINT("Incoming speedTestSpeed: ");
    DBG_PRINTLN(incomingData.speedTestSpeed);
  
 
    DBG_PRINT("Incoming seedingRate: ");
    DBG_PRINTLN(incomingData.seedingRate);

    DBG_PRINT("Incoming calibrationWeight: ");
    DBG_PRINTLN(incomingData.calibrationWeight);

    DBG_PRINT("Calibration revs: ");
    DBG_PRINTLN(Encoder::revs);

    DBG_PRINT("seedPerRev: ");
    DBG_PRINTLN(seedPerRev);
*/ 

}

const unsigned long DEBOUNCE_DELAY = 50;  // ms

int buttonState = 0;  // toggled state (0 or 1)
bool lastButtonReading = HIGH;
bool lastDebouncedState = HIGH;
unsigned long lastDebounceTime = 0;

void gpsTask(void* param) {
  while (true) {
    readGPSData();        // call your existing function
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms delay between reads
  }
}

void setup() 
{
  Serial.begin(115200);

  DBG_PRINTLN("**********************");
  DBG_PRINTLN("");
  DBG_PRINTLN("Valmar 1665 DEBUG MODE");
  DBG_PRINTLN("");
  DBG_PRINTLN("**********************");
  DBG_PRINTLN("");

  timer.set(debugPrint, 1000);
    
  initPins();
  
  initDisplay();
  
  initGPS();

  xTaskCreatePinnedToCore(gpsTask, "gpsTask", 4096, NULL, 1, NULL, 1);

  setupComms();

  Encoder::begin(ENC_A);

  loadPrefs();
  
  digitalWrite(PWR_LED, HIGH);

  DBG_PRINTLN("Setup Complete.");
}

void loop()
{

  timer.update();

  Encoder::update();
  
if (readWorkSwitch()) {
    neopixelWrite(RGB_LED, 0, 2, 0);

    float targetRPM = calculateTargetShaftRPM(GPS.speedMPH, targetSeedingRate, seedPerRev, 60.0f);
    float actualRPM = Encoder::rpm;

    uint8_t pwmValue = computePWM(targetRPM, actualRPM);

    setMotorPWM(pwmValue);
    actualRate = calculateApplicationRate();

/*     DBG_PRINT("Target RPM: ");
    DBG_PRINT(targetRPM);
    DBG_PRINT(" | Actual RPM: ");
    DBG_PRINT(actualRPM);
    DBG_PRINT(" | PWM: ");
    DBG_PRINTLN(pwmValue); */

} else {
    neopixelWrite(RGB_LED, 2, 0, 0);
    actualRate = 0.0f;
  }

  if (calibrationMode) {
    updateOLEDcal();
    digitalWrite(CAL_LED, HIGH);
  }
  else {
    updateOLEDgps();
    digitalWrite(CAL_LED, LOW);
  }

  updateMotorControl();

  sendCommsUpdate();
  
}