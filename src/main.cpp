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

NonBlockingTimer timer;

void debugPrint();

void gpsTask(void* param) {
  while (true) {
    readGPSData();        // call your existing function
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms delay between reads
  }
}

// Motor Stall detection and error flagging
// Optional: adjust sampling period
const TickType_t checkInterval = pdMS_TO_TICKS(10);  // Check every 10ms
const uint32_t stallThresholdMs = 200;
const uint32_t stallThresholdTicks = stallThresholdMs / 10;  // Based on check interval
const float stallRPMThreshold = 0.1f;

void stallMonitorTask(void* parameter) {
    uint32_t stallCounter = 0;

    while (true) {
        bool workActive = readWorkSwitch();
        float rpm = Encoder::rpm;

        if (workActive && rpm < stallRPMThreshold && !errorRaised) {
            stallCounter++;
            
            if (stallCounter >= stallThresholdTicks) {
                setMotorPWM(0);
                raiseError(3);

                // Burst ESP-NOW error message 3 times
                for (int i = 0; i < 3; i++) {
                    outgoingData.type = PACKET_TYPE_DATA;
                    esp_now_send(screenAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));                
                    vTaskDelay(pdMS_TO_TICKS(15));      // Short delay between sends
                } 
            }
        } else {
            stallCounter = 0;  // Reset if conditions don't persist
        }

        vTaskDelay(checkInterval);
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

  digitalWrite(PWR_LED, HIGH);

  DBG_PRINTLN("Setup Complete.");
}

void loop()
{

  handlePairing();  // comms.cpp

  updateMotorControl();  // motor.cpp only used for motor testing from screen.

  timer.update();

  Encoder::update();  // encoder.cpp

  // start handling work conditions

if (readWorkSwitch() && !pairingMode) {
    neopixelWrite(RGB_LED, 0, 100, 0);

    float targetRPM = calculateTargetShaftRPM(GPS.speedMPH, targetSeedingRate, seedPerRev, workingWidth);
    float actualRPM = Encoder::rpm;

    uint8_t pwmValue = computePWM(targetRPM, actualRPM);

    setMotorPWM(pwmValue);
    actualRate = calculateApplicationRate();

} else if (!readWorkSwitch() && !pairingMode) {
    neopixelWrite(RGB_LED, 100, 0, 0);
    actualRate = 0.0f;
} else if (!readWorkSwitch() && pairingMode) {
    neopixelWrite(RGB_LED, 0, 0, 100);
}

  //set the oled and calbutton to calibration mode and vice versa

if (calibrationMode) {
    updateOLEDcal();
    digitalWrite(CAL_LED, HIGH);
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

if (screenPaired) {
    sendCommsUpdate();
}
  

  // Used for testing error codes.  can be removed for final version.

    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');  // Wait until Enter is pressed

        // Clean and parse the input
        String clean = "";
        bool dotSeen = false;
        for (char c : input) {
            if (c >= '0' && c <= '9') {
                clean += c;
            } else if (c == '.' && !dotSeen) {
                clean += c;
                dotSeen = true;
            }
        }

        // Only update when Enter is pressed
        int code = clean.toInt();
        
        if (code > 0) {
          raiseError(code);
          
        } else if (code == 0) {
          clearError();
        }
    }

}

void debugPrint() {

/*  DBG_PRINT("Incoming calibrationMode: ");
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

    DBG_PRINT("calibrationMode: ");
    DBG_PRINTLN(calibrationMode ? "true" : "false");

    DBG_PRINT("Calibration revs: ");
    DBG_PRINTLN(calRevs);
    DBG_PRINT("seedPerRev: ");
    DBG_PRINTLN(seedPerRev);

    DBG_PRINTLN(screenPaired);

    DBG_PRINT("errorRaised: ");
    DBG_PRINT(outgoingData.errorRaised);
    DBG_PRINT(" Code: ");
    DBG_PRINT(outgoingData.errorCode);
    DBG_PRINT(" errorAck: ");
    DBG_PRINTLN(incomingData.errorAck);
    */ 
  }