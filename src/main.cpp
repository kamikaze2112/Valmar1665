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
    */ 


DBG_PRINT("errorRaised: ");
DBG_PRINT(outgoingData.errorRaised);
DBG_PRINT(" Code: ");
DBG_PRINTLN(outgoingData.errorCode);

  }


const unsigned long DEBOUNCE_DELAY = 50;  // ms

int buttonState = 0;  // toggled state (0 or 1)
bool lastButtonReading = HIGH;
bool lastDebouncedState = HIGH;
unsigned long lastDebounceTime = 0;
bool sprUpdate = false;

void gpsTask(void* param) {
  while (true) {
    readGPSData();        // call your existing function
    vTaskDelay(pdMS_TO_TICKS(5));  // 5ms delay between reads
  }
}

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
                outgoingData.errorCode = 3; // No shaft RPM
                outgoingData.errorRaised = true;
                errorCode = 3;
                errorRaised = true;

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

  handlePairing();

  timer.update();

  Encoder::update();

/*   if (calibrationMode){
    calRevs = Encoder::revs;
  } */
  
if (readWorkSwitch() && !pairingMode) {
    neopixelWrite(RGB_LED, 0, 100, 0);

    float targetRPM = calculateTargetShaftRPM(GPS.speedMPH, targetSeedingRate, seedPerRev, workingWidth);
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

} else if (!readWorkSwitch() && !pairingMode) {
    neopixelWrite(RGB_LED, 100, 0, 0);
    actualRate = 0.0f;
} else if (!readWorkSwitch() && pairingMode) {
    neopixelWrite(RGB_LED, 0, 0, 100);
}

if (calibrationMode) {
    updateOLEDcal();
    digitalWrite(CAL_LED, HIGH);
} else {
    updateOLEDgps();
    digitalWrite(CAL_LED, LOW);
}

if (speedTestSwitch) {
  GPS.speedMPH = speedTestSpeed;
} else if (!speedTestSwitch && GPS.fixType == 0) {
  GPS.speedMPH = 0;
}

updateMotorControl();

if (screenPaired) {
    sendCommsUpdate();
}
  

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
        errorCode = clean.toInt();
        outgoingData.errorCode = errorCode;
        
        if (errorCode > 0) {
          errorRaised = true;
          
        } else if (errorCode == 0) {
          errorRaised = false;
        }

        outgoingData.errorRaised = errorRaised;
    }

/*           static String inputBuffer;

    while (Serial.available()) {
        char incomingChar = Serial.read();

        if (incomingChar == '\n' || incomingChar == '\r') {
            // Trim and process the input when Enter is pressed
            inputBuffer.trim();
            if (inputBuffer.length() == 1 && isDigit(inputBuffer[0])) {
                int code = inputBuffer.toInt();
                if (code > 0 && code <= 3) {
                    outgoingData.errorCode = code;
                    outgoingData.errorRaised = true;
                    Serial.printf("Injected error code: %d\n", code);
                } else if (code == 0) {
                    outgoingData.errorCode = 0;
                    outgoingData.errorRaised = false;
                } else {
                    Serial.println("Error: Code must be 0–3");
                }
            } else {
                Serial.println("Invalid input. Enter a single digit (0–3).");
            }

            inputBuffer = "";  // Clear for next entry
        } else {
            inputBuffer += incomingChar;  // Accumulate input
        }
    } */

/*     // Error Actual rate +/- 25% of target rate
    if (targetSeedingRate > 0.0f && (actualRate <= targetSeedingRate * 0.75f || actualRate >= targetSeedingRate * 1.25f))  {
      outgoingData.rateOutOfBounds = true;
    } else {
      outgoingData.rateOutOfBounds = false;
    }
 */

}