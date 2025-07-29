// otaUpdate.h
#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <DNSServer.h>

class OTAUpdater {
private:
    WebServer server;
    DNSServer dnsServer;
    bool otaActive;
    unsigned long otaStartTime;
    static const unsigned long OTA_TIMEOUT = 900000; // 15 minutes
    static const char* AP_SSID;
    static const char* AP_PASSWORD;
    
    void setupWebServer();
    void handleRoot();
    void handleUpdate();
    void handleUpload();
    void handleCaptive();
    static String getUpdateHTML();


    
public:
    OTAUpdater();
    bool startOTAMode();
    void handleOTA();
    void stopOTAMode();
    bool isOTAActive() { return otaActive; }
};

extern OTAUpdater otaUpdater;

#endif