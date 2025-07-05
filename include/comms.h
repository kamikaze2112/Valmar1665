#pragma once
#include <Arduino.h>

// Incoming data from remote sender
struct IncomingData {
  bool calibrationMode;
  float seedingRate;
  float calibrationWeight;
  bool motorTestSwitch;
  int motorTestPWM;
  bool speedTestSwitch;
  float speedTestSpeed;
} __attribute__((packed));

// Outgoing data to send to remote
struct OutgoingData {
  int fixStatus;
  int numSats;
  float gpsSpeed;
  int gpsHour;
  int gpsMinute;
  int gpsSecond;
  double calibrationRevs;
  bool workSwitch;
  bool motorActive;
  double shaftRPM;
  int errorCode;
  float actualRate;
} __attribute__((packed));

// Public access to received data
extern IncomingData incomingData;

// Call during setup
void setupComms();

// Call this every ~300–350ms
void sendCommsUpdate();