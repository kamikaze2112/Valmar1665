#include <WiFi.h>
#include <esp_now.h>
#include "gps.h"
#include "encoder.h"
#include "motor.h"
#include "comms.h"
#include "globals.h"  // Assuming all outgoing variables are defined here


// Receiver MAC address: 98:3D:AE:E9:26:58
uint8_t peerAddress[] = { 0x98, 0x3D, 0xAE, 0xE9, 0x26, 0x58 };

// Storage for received values
IncomingData incomingData = {};

// Track last send time
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 200;  // 1 per second

// === Incoming receive callback ===
void onDataRecv(const uint8_t *mac, const uint8_t *incoming, int len) {
  if (len == sizeof(IncomingData)) {
    memcpy(&incomingData, incoming, sizeof(IncomingData));
    
    calibrationMode = incomingData.calibrationMode;
  
  }
  
}

// === Send status callback (optional debugging) ===
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  //DBG_PRINT("ESP-NOW send status: ");
  //DBG_PRINTLN(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// === Setup ESP-NOW ===
void setupComms() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    DBG_PRINTLN("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(peerAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      DBG_PRINTLN("Failed to add ESP-NOW peer");
    }
  }
}

// === Send OutgoingData struct ===
void sendCommsUpdate() {
  unsigned long now = millis();
  if (now - lastSendTime < sendInterval) return;
  lastSendTime = now;

  OutgoingData out;
  out.fixStatus = GPS.fixValid;
  out.numSats = GPS.numSats;
  out.gpsSpeed = GPS.speedMPH;
  out.gpsHour = GPS.hour;
  out.gpsMinute = GPS.minute;
  out.gpsSecond = GPS.second;
  out.calibrationRevs = Encoder::revs;
  out.workSwitch = readWorkSwitch();
  out.motorActive = motorActive;
  out.shaftRPM = shaftRPM;
  out.errorCode = errorCode;  // Assuming you have this defined
  out.actualRate = actualRate;
  esp_err_t result = esp_now_send(peerAddress, (uint8_t *)&out, sizeof(out));

  if (result != ESP_OK) {
    DBG_PRINT("ESP-NOW send error: ");
    DBG_PRINTLN(result);
  }
}