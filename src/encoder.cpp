#include <Arduino.h>
#include "globals.h"
#include "encoder.h"

#include "driver/pcnt.h"

namespace {
  constexpr int PULSES_PER_REV = 1024;
  constexpr pcnt_unit_t PCNT_UNIT = PCNT_UNIT_0;
  constexpr unsigned long RPM_SAMPLE_INTERVAL_MS = 200;
  constexpr unsigned long MOVING_TIMEOUT_MS = 500;

  // Overflow-safe revolution count
  volatile long totalPulses = 0;
  int16_t lastPCNTCount = 0;

  unsigned long lastRPMUpdate = 0;
  float currentRPM = 0.0;

  unsigned long lastPulseTime = 0;

  void setupPCNT(int pinA) {
    pcnt_config_t config;

    config.pulse_gpio_num = pinA;
    config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    config.channel        = PCNT_CHANNEL_0;
    config.unit           = PCNT_UNIT;
    config.pos_mode       = PCNT_COUNT_INC;
    config.neg_mode       = PCNT_COUNT_DIS;
    config.lctrl_mode     = PCNT_MODE_KEEP;
    config.hctrl_mode     = PCNT_MODE_KEEP;
    config.counter_h_lim  = 32767;
    config.counter_l_lim  = -32768;

    pcnt_unit_config(&config);
    pcnt_counter_clear(PCNT_UNIT);
    pcnt_counter_resume(PCNT_UNIT);
  }

  long readAndTrackTotalPulses() {
    int16_t currentCount = 0;
    pcnt_get_counter_value(PCNT_UNIT, &currentCount);

    int16_t delta = currentCount - lastPCNTCount;

    // Handle PCNT overflow (wraparound)
    if (delta > 30000) {         // wrapped negatively
      delta -= 65536;
    } else if (delta < -30000) { // wrapped positively
      delta += 65536;
    }

    if (delta != 0) {
      lastPulseTime = millis();
    }

    totalPulses += delta;
    lastPCNTCount = currentCount;
   
    return totalPulses;
  }
}

namespace Encoder {
  float rpm = 0.0;
  float revs = 0.0;
  bool isMoving = false;

  void begin(int pinA) {
    setupPCNT(pinA);
    lastPCNTCount = 0;
    totalPulses = 0;
    lastPulseTime = millis();
    lastRPMUpdate = millis();
  }

  void resetRevolutions() {
    pcnt_counter_clear(PCNT_UNIT);
    lastPCNTCount = 0;
    totalPulses = 0;
    revs = 0.0;
  }

  void update() {
    unsigned long now = millis();
    if ((now - lastRPMUpdate) < RPM_SAMPLE_INTERVAL_MS) return;

    static long lastPulseSnapshot = 0;
    long currentTotalPulses = readAndTrackTotalPulses();
    long deltaPulses = currentTotalPulses - lastPulseSnapshot;

    revs = float(currentTotalPulses) / float(PULSES_PER_REV);
    float deltaRevs = float(deltaPulses) / float(PULSES_PER_REV);
    float elapsedMinutes = float(now - lastRPMUpdate) / 60000.0;

    rpm = (elapsedMinutes > 0) ? (deltaRevs / elapsedMinutes) : 0.0;
    isMoving = (millis() - lastPulseTime) < MOVING_TIMEOUT_MS;

    lastPulseSnapshot = currentTotalPulses;
    lastRPMUpdate = now;
  }
}