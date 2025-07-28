#include <WiFi.h>
#include <esp_now.h>
#include "gps.h"
#include "encoder.h"
#include "comms.h"
#include "prefs.h"
#include "globals.h"  // Assuming all outgoing variables are defined here
#include "errorHandler.h"
#include "workFunctions.h"

uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };\
uint8_t screenAddress[6];

bool resetRevs = false;
bool screenPaired = false;
bool pairingMode = false;
unsigned long buttonPressStart = 0;
bool buttonHeld = false;
bool pairingTriggered = false;
unsigned long lastPairingTime = 0;

static esp_now_peer_info_t peerInfo;

// Storage for received values
IncomingData incomingData = {};
OutgoingData outgoingData = {};

// Track last send time
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 200;  // 1 per second

void printMac(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print("0"); // Leading zero if needed
        Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

void addPeer(const uint8_t mac[6]) {

  memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.print("Failed to add peer: ");
    } else {
        Serial.print("Peer added: ");

        if (mac == broadcastAddress) {
            screenPaired = false;
        } else {
              screenPaired = true;
              saveComms();
        }
    }
    printMac(mac);
    DBG_PRINTLN(screenPaired);
}

void onDataRecv(const uint8_t *mac, const uint8_t *incoming, int len) {

  if (len < sizeof(PacketType)) return;

  PacketType type = static_cast<PacketType>(incoming[0]);

  if (type == PACKET_TYPE_PAIR_SEND) {
        
      if (pairingMode) {
          incomingData.type = PACKET_TYPE_PAIR_SEND;
          memcpy(screenAddress, mac, 6);
          Serial.print("Pairing request received from: ");
          printMac(screenAddress);
          addPeer(screenAddress);
          sendPairingACK();
          pairingMode = false;
      }

  } else if (type == PACKET_TYPE_DATA) {
        
    incomingData.type = PACKET_TYPE_DATA;
    memcpy(&incomingData, incoming, min(len, (int)sizeof(IncomingData)));

    calibrationMode = incomingData.calibrationMode;
    calibrationWeight = incomingData.calibrationWeight;
    targetSeedingRate = incomingData.seedingRate;

    motorTestSwitch = incomingData.motorTestSwitch;
    motorTestPWM = incomingData.motorTestPWM;
    speedTestSwitch = incomingData.speedTestSwitch;
    speedTestSpeed = incomingData.speedTestSpeed;

    if (incomingData.errorAck && errorCode == 3) {
      clearError();
    }

    if (incomingData.manualSeedUpdate) {
      seedPerRev = incomingData.newSeedPerRev;
      savePrefs();
      incomingData.manualSeedUpdate = false;
    }

    if (incomingData.calcSeedPerRev) {
        seedPerRev = calculateSeedPerRev(calRevs, calibrationWeight, numberOfRuns);

        DBG_PRINT("seedPerRev");
        DBG_PRINTLN(seedPerRev);
        savePrefs();
        resetRevs = true;
        incomingData.calcSeedPerRev = false;

    } else if (calibrationMode && resetRevs) {
        Encoder::resetRevolutions();
        calRevs = 0.0f;
        resetRevs = false;
    }
  }
}

// === Send status callback (optional debugging) ===
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
/*   DBG_PRINT("ESP-NOW send status: ");
  DBG_PRINTLN(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail"); */
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

  if (!screenPaired) {
      addPeer(broadcastAddress);
  } else {
      addPeer(screenAddress);
  }
}

// === Send OutgoingData struct ===
void sendCommsUpdate() {
  unsigned long now = millis();
  if (now - lastSendTime < sendInterval) return;
  lastSendTime = now;

  outgoingData.fixStatus = GPS.fixType;
  outgoingData.numSats = GPS.satellites;
  outgoingData.gpsSpeed = GPS.speedMPH;
  outgoingData.gpsHour = GPS.hour;
  outgoingData.gpsMinute = GPS.minute;
  outgoingData.gpsSecond = GPS.second;
  outgoingData.calibrationRevs = Encoder::revs;
  outgoingData.workSwitch = readWorkSwitch();
  outgoingData.motorActive = motorActive;
  outgoingData.shaftRPM = Encoder::rpm;
  outgoingData.actualRate = actualRate;
  outgoingData.seedPerRev = seedPerRev;
  
  strncpy(outgoingData.controllerVersion, APP_VERSION, sizeof(outgoingData.controllerVersion));
  outgoingData.controllerVersion[sizeof(outgoingData.controllerVersion) - 1] = '\0';  // null-terminate just in case
  
  outgoingData.type = PACKET_TYPE_DATA;
  esp_err_t result = esp_now_send(screenAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));

  if (result != ESP_OK) {
      DBG_PRINT("ESP-NOW send error: ");
      DBG_PRINTLN(result);
  }

}

void handlePairing() {
      bool buttonState = digitalRead(BOOT_BTN);

  if (buttonState == LOW) {
      if (!buttonHeld) {
          buttonPressStart = millis();
          buttonHeld = true;
          pairingTriggered = false;  // reset on fresh press
      } else if ((millis() - buttonPressStart >= 3000) && !pairingTriggered) {
          pairingMode = true;
          lastPairingTime = 0;
          pairingTriggered = true;  // ✅ prevent repeat triggers
          Serial.println("🔁 3-second hold detected, entering pairing mode");
      }
  } else {
      // Reset everything when button released
      buttonHeld = false;
      pairingTriggered = false;
      buttonPressStart = 0;
  }
}

void sendPairingACK() {
    struct {
        PacketType type = PACKET_TYPE_PAIR_ACK;
    } pairingACK;

    esp_now_send(screenAddress, (uint8_t*)&pairingACK, sizeof(pairingACK));
    Serial.print("Paring ACK sent to: ");
    printMac(screenAddress);
}