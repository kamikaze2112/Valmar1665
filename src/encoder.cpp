#include <Arduino.h>
#include "globals.h"
#include "encoder.h"
#include "driver/pcnt.h"

namespace {
constexpr int PULSES_PER_REV = 1024;
constexpr pcnt_unit_t PCNT_UNIT = PCNT_UNIT_0;
constexpr unsigned long RPM_SAMPLE_INTERVAL_MS = 100;  // Reduced for cleaner sampling
constexpr unsigned long MOVING_TIMEOUT_MS = 500;

// Simple counters - no overflow handling needed
volatile long completedRevolutions = 0;  // Full revolutions completed
volatile int currentPulses = 0;          // Pulses in current incomplete revolution

// RPM calculation and smoothing
unsigned long lastRPMUpdate = 0;
float currentRPM = 0.0;
unsigned long lastPulseTime = 0;

// RPM smoothing - rolling average
constexpr int RPM_HISTORY_SIZE = 5;
float rpmHistory[RPM_HISTORY_SIZE] = {0};
int rpmHistoryIndex = 0;
bool rpmHistoryFull = false;

// Debug variables
bool debugMode = true;

void setupPCNT(int pinA) {
    pcnt_config_t config;
    config.pulse_gpio_num = pinA;
    config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    config.channel = PCNT_CHANNEL_0;
    config.unit = PCNT_UNIT;
    config.pos_mode = PCNT_COUNT_INC;
    config.neg_mode = PCNT_COUNT_DIS;
    config.lctrl_mode = PCNT_MODE_KEEP;
    config.hctrl_mode = PCNT_MODE_KEEP;
    config.counter_h_lim = 32767;
    config.counter_l_lim = -32768;
    
    pcnt_unit_config(&config);
    pcnt_counter_clear(PCNT_UNIT);
    pcnt_counter_resume(PCNT_UNIT);
}

void updateCounters() {
    int16_t rawPulses;
    pcnt_get_counter_value(PCNT_UNIT, &rawPulses);
    pcnt_counter_clear(PCNT_UNIT);
    
    if (rawPulses != 0) {
        lastPulseTime = millis();
    }
    
    currentPulses += rawPulses;
    
    // Check if we completed any full revolutions
    while (currentPulses >= PULSES_PER_REV) {
        completedRevolutions++;
        currentPulses -= PULSES_PER_REV;
        if (debugMode) {
            Serial.printf("Revolution completed: %ld total revs\n", completedRevolutions);
        }
    }
    
    // Handle reverse direction if needed
    while (currentPulses < 0) {
        completedRevolutions--;
        currentPulses += PULSES_PER_REV;
        if (debugMode) {
            Serial.printf("Reverse revolution: %ld total revs\n", completedRevolutions);
        }
    }
    
    if (debugMode && abs(rawPulses) > 100) {
        Serial.printf("Large pulse count: %d, currentPulses=%d, completedRevs=%ld\n", 
                     rawPulses, currentPulses, completedRevolutions);
    }
}

float getSmoothedRPM() {
    if (!rpmHistoryFull && rpmHistoryIndex == 0) {
        return currentRPM; // No history yet
    }
    
    float sum = 0;
    int count = rpmHistoryFull ? RPM_HISTORY_SIZE : rpmHistoryIndex;
    
    for (int i = 0; i < count; i++) {
        sum += rpmHistory[i];
    }
    
    return sum / count;
}

void addRPMToHistory(float newRPM) {
    rpmHistory[rpmHistoryIndex] = newRPM;
    rpmHistoryIndex = (rpmHistoryIndex + 1) % RPM_HISTORY_SIZE;
    
    if (rpmHistoryIndex == 0) {
        rpmHistoryFull = true;
    }
}

float getTotalRevolutions() {
    return completedRevolutions + (float(currentPulses) / PULSES_PER_REV);
}

void clearRPMHistory() {
    for (int i = 0; i < RPM_HISTORY_SIZE; i++) {
        rpmHistory[i] = 0;
    }
    rpmHistoryIndex = 0;
    rpmHistoryFull = false;
}
}

namespace Encoder {
float rpm = 0.0;
float revs = 0.0;
bool isMoving = false;

void begin(int pinA) {
    setupPCNT(pinA);
    completedRevolutions = 0;
    currentPulses = 0;
    lastPulseTime = millis();
    lastRPMUpdate = millis();
    clearRPMHistory();
    
    if (debugMode) {
        Serial.println("Encoder initialized.");
    }
}

void resetRevolutions() {
    pcnt_counter_clear(PCNT_UNIT);
    completedRevolutions = 0;
    currentPulses = 0;
    revs = 0.0;
    
    if (debugMode) {
        Serial.println("Revolution counter reset");
    }
}

void update() {
    updateCounters(); // Always update counters
    
    unsigned long now = millis();
    if ((now - lastRPMUpdate) < RPM_SAMPLE_INTERVAL_MS) return;
    
    // Calculate time interval with rollover protection
    unsigned long elapsedMs = now - lastRPMUpdate;
    if (elapsedMs > 1000) { // Cap at 1 second to prevent rollover issues
        elapsedMs = RPM_SAMPLE_INTERVAL_MS;
    }
    
    // Get current revolution count
    float currentRevs = getTotalRevolutions();
    float deltaRevs = currentRevs - revs;
        
    // Calculate RPM
    float elapsedMinutes = float(elapsedMs) / 60000.0;
    float newRPM = (elapsedMinutes > 0) ? (deltaRevs / elapsedMinutes) : 0.0;
    
    // Add to smoothing history
    addRPMToHistory(newRPM);
    
    // Debug RPM spikes
    if (debugMode && abs(newRPM - rpm) > 500) {
        Serial.printf("RPM change: %0.1f -> %0.1f, deltaRevs=%0.3f, elapsedMs=%lu\n", 
                     rpm, newRPM, deltaRevs, elapsedMs);
        Serial.printf("  currentRevs=%0.3f, completedRevs=%ld, currentPulses=%d\n", 
                     currentRevs, completedRevolutions, currentPulses);
    }
    
    // Update values with smoothed RPM
    rpm = getSmoothedRPM();
    revs = currentRevs;
    isMoving = (millis() - lastPulseTime) < MOVING_TIMEOUT_MS;
    
    lastRPMUpdate = now;
}

void setDebugMode(bool enabled) {
    debugMode = enabled;
}
}