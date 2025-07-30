#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// Define packet types
enum PacketType : uint8_t {
    PACKET_TYPE_DATA = 0,
    PACKET_TYPE_PAIR_SEND = 1,
    PACKET_TYPE_PAIR_ACK = 2
};
// Existing structs
struct IncomingData {
  PacketType type = PACKET_TYPE_DATA;
  bool calibrationMode;
  float seedingRate;
  float calibrationWeight;
  bool calcSeedPerRev;
  bool motorTestSwitch;
  int motorTestPWM;
  bool speedTestSwitch;
  float speedTestSpeed;
  float workingWidth;
  int numberOfRuns;
  float newSeedPerRev;
  bool manualSeedUpdate;
  bool errorAck;
  bool fwUpdateMode;
  bool stallProtection;
  int stallDelay;
} __attribute__((packed));

struct OutgoingData {
  PacketType type = PACKET_TYPE_DATA;
  int fixStatus;
  int numSats;
  float gpsSpeed;
  int gpsHour;
  int gpsMinute;
  int gpsSecond;
  double calibrationRevs;
  bool workSwitch;
  bool motorActive;
  float seedPerRev;
  double shaftRPM;
  int errorCode;
  bool errorRaised;
  float actualRate;
  char controllerVersion[12];
  bool rateOutOfBounds;
  bool fwUpdateComplete;
} __attribute__((packed));

// Public access to received data
extern IncomingData incomingData;
extern OutgoingData outgoingData;

// Call during setup
void setupComms();

// Call this every ~300–350ms
void sendCommsUpdate();

void sendPairingACK();
void printMac(const uint8_t *mac);