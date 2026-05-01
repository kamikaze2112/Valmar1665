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
#include "errorHandler.h"
#include "workFunctions.h"
#include "otaUpdate.h"

NonBlockingTimer timer;

static bool otaStarted = false;
volatile uint16_t stallThresholdMs = 200;
volatile bool stallProtection = true;
volatile bool stallEventPending = false;

void gpsTask(void* param);
void stallMonitorTask(void* parameter);
void debugPrint();

void setup() {
  Serial.begin(115200);

  DBG_PRINTLN("**********************");
  DBG_PRINTLN("");
  DBG_PRINTLN("---   Valmar 1665  ---");
  DBG_PRINTLN("");
  DBG_PRINTLN("**********************");
  DBG_PRINTLN("");

  DBG_PRINT("Controller Ver: ");
  DBG_PRINTLN(APP_VERSION);

  timer.set(debugPrint, 1000);
    
  initPins();
  
  initDisplay();
  
  initGPS();

  xTaskCreatePinnedToCore(
    gpsTask, 
    "gpsTask", 
    4096, 
    NULL, 
    1, 
    NULL, 
    1);

  xTaskCreatePinnedToCore(
    stallMonitorTask,
    "StallMonitor", //Function
    2048,       // Stack size
    NULL,       // Parameter
    2,          // Priority
    NULL,       // Task handle (optional)
    APP_CPU_NUM // Core (typically 1)
);

  Encoder::begin(ENC_A);

  loadPrefs();

  loadComms();

  setupComms();

  outgoingData.fwUpdateComplete = true;
  outgoingData.heartbeat = 0;
  outgoingData.controllerBooted = true;
  incomingData.reset = false;
  digitalWrite(PWR_LED, HIGH);


  DBG_PRINTLN("Setup Complete.");
}



void loop() {

  //Check for reset flag

  if (incomingData.reset) {
    DBG_PRINTLN("Init reset...");

    clearPrefs();
    delay(100);
    clearComms();
    delay(100);

    DBG_PRINTLN("Controller reset to defaults.  Rebooting...");

    delay(100);

    ESP.restart();

  }

  if (incomingData.fwUpdateMode && !otaStarted) {
        if (otaUpdater.startOTAMode()) {
        otaStarted = true;
    }
  }

  otaUpdater.handleOTA(); // Only processes if OTA is active

  handlePairing();  // comms.cpp

  updateMotorControl();  // motor.cpp only used for motor testing from screen.

  timer.update();

  Encoder::update();  // encoder.cpp

  // start handling work conditions

if (readWorkSwitch() && !pairingMode && !otaStarted && !motorTestSwitch) {
    neopixelWrite(RGB_LED, 0, 100, 0);

    float targetRPM = calculateTargetShaftRPM(GPS.speedMPH, targetSeedingRate, seedPerRev, workingWidth);
    targetRPM *= (1.0f + incomingData.rateAdjust / 100.0f);
    float actualRPM = Encoder::rpm;

    uint8_t pwmValue = computePWM(targetRPM, actualRPM);

    setMotorPWM(pwmValue);
    actualRate = calculateApplicationRate();

} else if (!readWorkSwitch() && !pairingMode && !otaStarted && !motorTestSwitch) {
    neopixelWrite(RGB_LED, 100, 0, 0);
    actualRate = 0.0f;
    if (seedPerRev > 0.0f) {
        float shadowTargetRPM = calculateTargetShaftRPM(GPS.speedMPH, targetSeedingRate, seedPerRev, workingWidth);
        shadowTargetRPM *= (1.0f + incomingData.rateAdjust / 100.0f);
        computePWM(shadowTargetRPM, shadowTargetRPM, true);
    }
} else if (!readWorkSwitch() && pairingMode && !otaStarted) {
    neopixelWrite(RGB_LED, 0, 0, 100);
}

  //set the oled and calbutton to calibration mode and vice versa

if (calibrationMode) {
    updateOLEDcal();
    digitalWrite(CAL_LED, HIGH);
} else if (otaStarted) {
    updateOLEDfw();
} else {
    updateOLEDgps();
    digitalWrite(CAL_LED, LOW);
}

  //handle test/manual speed input from the screen
if (speedTestSwitch) {
  GPS.speedMPH = speedTestSpeed;
} else if (!speedTestSwitch && GPS.fixType == 0) {
  GPS.speedMPH = 0;
}

if (screenPaired && !otaStarted) {
    sendCommsUpdate();
}

stallThresholdMs = (uint16_t)incomingData.stallDelay;
stallProtection = incomingData.stallProtection;

if (pendingSavePrefs) {
    pendingSavePrefs = false;
    savePrefs();
}

if (stallEventPending) {
    stallEventPending = false;
    raiseError(3);
    for (int i = 0; i < 3; i++) {
        outgoingData.type = PACKET_TYPE_DATA;
        esp_now_send(screenAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));
        delay(15);
    }
}

}

void gpsTask(void* param) {
  while (true) {
    readGPSData();        // call your existing function
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms delay between reads
  }
}

// Motor Stall detection and error flagging
const TickType_t checkInterval = pdMS_TO_TICKS(10);  // Check every 10ms
const float stallRPMThreshold = 0.1f;

void stallMonitorTask(void* parameter) {
    uint32_t stallCounter = 0;
    float prevRpm = 0.0f;

    while (true) {
        if (stallProtection) {
            bool workActive = (workSwitchState == 1);
            float rpm = Encoder::rpm;
            float rpmDelta = rpm - prevRpm;
            prevRpm = rpm;

            uint32_t stallThresholdTicks = stallThresholdMs / 10;

            // rpmDelta > 0 means motor is accelerating — spinning up, not stalled
            if (workActive && rpm < stallRPMThreshold && rpmDelta <= 0.0f && errorCode != 3) {
                stallCounter++;

                if (stallCounter >= stallThresholdTicks) {
                    setMotorPWM(0);
                    stallEventPending = true;
                }
            } else {
                stallCounter = 0;
            }
        }

        vTaskDelay(checkInterval);
    }
}

void debugPrint() {

/*     DBG_PRINT("Incoming calibrationMode: ");
    DBG_PRINTLN(incomingData.calibrationMode);

    DBG_PRINT("Incoming seedingRate: ");
    DBG_PRINTLN(incomingData.seedingRate);

 /   DBG_PRINT("GPS.speedMPH: ");
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

    DBG_PRINT("calibrationMode: ");
    DBG_PRINTLN(calibrationMode ? "true" : "false");

    DBG_PRINT("Calibration revs: ");
    DBG_PRINTLN(Encoder::revs);
    DBG_PRINT("seedPerRev: ");
    DBG_PRINTLN(seedPerRev);
    DBG_PRINTLN(Encoder::revs);
    DBG_PRINTLN("-----------------------------------");
    DBG_PRINTLN(screenPaired);

    DBG_PRINT("errorRaised: ");
    DBG_PRINT(outgoingData.errorRaised);
    DBG_PRINT(" Code: ");
    DBG_PRINT(outgoingData.errorCode);
    DBG_PRINT(" errorAck: ");
    DBG_PRINTLN(incomingData.errorAck);
    
    DBG_PRINTF("incomingData.fwUpdateMode: %d  otaStarted: %d\n", incomingData.fwUpdateMode, otaStarted);
   

     DBG_PRINTF("stallProtection: %d  stallDelay: %d\n", stallProtection, stallThresholdMs); */
  
  }