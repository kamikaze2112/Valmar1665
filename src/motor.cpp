#include <Arduino.h>
#include "motor.h"
#include "globals.h"
#include "gps.h"
#include "encoder.h"

bool motorActive = false;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 10;  // 100 ms
float actualRate = 0.0;

void updateCounter() {
  unsigned long now = millis();
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    counter += 0.01;
  }
}

void handleCalButton() {

  if (calibrationMode){
    static bool wasHeld = false;
    static bool startedCal = false;

    if (digitalRead(CAL_BTN) == LOW) {
      // Button is held
      if (!wasHeld) {
        wasHeld = true;
        startedCal = true;
        
        // Start motor at 100%
        setMotorPWM(255);  // Assuming 255 is full speed
        motorActive = true;
        DBG_PRINTLN("Calibration started");
      }

      // Keep updating encoder during hold
      updateCounter();
      Encoder::update();

    } else {
      // Button is not held
      if (wasHeld) {
        wasHeld = false;

        if (startedCal) {
          startedCal = false;

          // Stop motor
          setMotorPWM(0);
          motorActive = false;

          // Read total revolutions
          Encoder::update();  // Final update just in case
          float revs = Encoder::revs;

          DBG_PRINT("Calibration done, revs = ");
          DBG_PRINTLN(counter);  // Print with 2 decimal places

          // You can now store/use `revs` as needed
        }
      }
    }
  } else {
      if (digitalRead(CAL_BTN) == LOW) {
        setMotorPWM(100);
        motorActive = true;
      } else {
        setMotorPWM(0);
        motorActive = false;
      }
  }

}

void setMotorPWM(int pwm){

  digitalWrite(MOTOR_DIR, LOW);
  analogWrite(MOTOR_PWM, pwm);

}
