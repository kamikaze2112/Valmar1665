#pragma once
#include <Arduino.h>

// Existing structs
struct IncomingData {
  bool calibrationMode;
  float seedingRate;
  float calibrationWeight;
  bool motorTestSwitch;
  int motorTestPWM;
  bool speedTestSwitch;
  float speedTestSpeed;
} __attribute__((packed));

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

// New enum for packet types
enum PacketType : uint8_t {
  PACKET_TYPE_DATA = 0x01,
  PACKET_TYPE_PAIR_REQUEST = 0xF0,
  PACKET_TYPE_PAIR_ACK = 0xF1
};

// New packet wrapper struct
struct Packet {
  uint8_t type;
  union {
    IncomingData incomingData;
    // future payloads can go here
  } payload;
} __attribute__((packed));

// Public access to received data
extern IncomingData incomingData;

// Call during setup
void setupComms();

// Call this every ~300–350ms
void sendCommsUpdate();

// New pairing mode control functions
void beginPairingMode();
void pairingLoop();