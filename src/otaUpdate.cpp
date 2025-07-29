// otaUpdate.cpp
#include "otaUpdate.h"
#include "globals.h"
#include "comms.h"
#include <esp_now.h>

const char* OTAUpdater::AP_SSID = "Valmar_OTA";
const char* OTAUpdater::AP_PASSWORD = "";

OTAUpdater otaUpdater;

OTAUpdater::OTAUpdater() : server(80), otaActive(false), otaStartTime(0) {}

bool OTAUpdater::startOTAMode() {
    Serial.println("Starting OTA mode...");
    Serial.println("Sending ESPNow burst.");

    outgoingData.fwUpdateComplete = false;

    for (int i = 0; i < 3; i++) {
        outgoingData.type = PACKET_TYPE_DATA;
        esp_now_send(screenAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));                
        delay(5);
        Serial.printf("Packet: %d\n", i + 1);
        neopixelWrite(RGB_LED, 100, 100, 100);
    }

    // Stop ESP-NOW
    esp_now_deinit();
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // Start AP mode
    WiFi.mode(WIFI_AP);
    bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    if (!apStarted) {
        Serial.println("Failed to start AP mode");
        return false;
    }
    
    // Start captive portal DNS server
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    Serial.print("AP started. Connect to: ");
    Serial.println(AP_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    
    setupWebServer();
    server.begin();
    
    otaActive = true;
    otaStartTime = millis();    
    return true;
}

void OTAUpdater::setupWebServer() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/upload", HTTP_POST, [this]() { handleUpdate(); }, [this]() { handleUpload(); });
    server.on("/cancel", HTTP_POST, [this]() { 
        server.send(200, "text/html", "<h1>Rebooting...</h1><script>setTimeout(function(){window.close();}, 2000);</script>");
        delay(1000);
        ESP.restart();
    });
    
    // Captive portal - redirect everything else to root
    server.onNotFound([this]() { handleCaptive(); });
}

void OTAUpdater::handleCaptive() {
    // Redirect any request to the main page
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
}

void OTAUpdater::handleRoot() {
    server.send(200, "text/html", getUpdateHTML());
}

void OTAUpdater::handleUpdate() {
    server.sendHeader("Connection", "close");
    if (Update.hasError()) {
        server.send(500, "text/plain", "Update failed!");
    } else {
        server.send(200, "text/plain", "Update successful! Rebooting...");
        delay(1000);
        ESP.restart();
    }
}

void OTAUpdater::handleUpload() {
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("Update Success: %u bytes\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    }
}


String OTAUpdater::getUpdateHTML() {
    return String(
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
            "<title>Rate Controller Firmware Update</title>"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
            "<style>"
                "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }"
                ".container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); max-width: 500px; margin: 0 auto; }"
                "h1 { color: #333; text-align: center; margin-bottom: 30px; }"
                
                ".browser-section { text-align: center; margin-bottom: 30px; padding-bottom: 20px; border-bottom: 2px solid #eee; }"
                ".open-btn { display: inline-block; background-color: #2196F3; color: white; padding: 15px 25px; text-decoration: none; border-radius: 8px; font-size: 16px; font-weight: bold; }"
                ".open-btn:hover { background-color: #1976D2; }"
                
                ".upload-form { margin: 20px 0; }"
                "input[type=\"file\"] { width: 100%; padding: 15px; margin: 10px 0; border: 2px solid #ddd; border-radius: 8px; font-size: 16px; box-sizing: border-box; }"
                "input[type=\"submit\"] { background-color: #4CAF50; color: white; padding: 18px 20px; border: none; border-radius: 8px; cursor: pointer; width: 100%; font-size: 18px; margin: 10px 0; box-sizing: border-box; }"
                "input[type=\"submit\"]:hover { background-color: #45a049; }"
                ".cancel-btn { background-color: #f44336; }"
                ".cancel-btn:hover { background-color: #da190b; }"
                ".warning { background-color: #fff3cd; border: 1px solid #ffeaa7; color: #856404; padding: 15px; border-radius: 8px; margin: 20px 0; }"
                ".info { background-color: #d1ecf1; border: 1px solid #bee5eb; color: #0c5460; padding: 15px; border-radius: 8px; margin: 20px 0; }"
                ".progress { width: 100%; background-color: #f0f0f0; border-radius: 8px; margin: 20px 0; display: none; overflow: hidden; }"
                ".progress-bar { width: 0%; height: 40px; background-color: #4CAF50; text-align: center; line-height: 40px; color: white; font-weight: bold; transition: width 0.3s ease; }"
                ".file-info { margin: 10px 0; padding: 10px; background-color: #f8f9fa; border-radius: 5px; font-size: 14px; display: none; }"
                "@media (max-width: 600px) {"
                    "body { margin: 10px; }"
                    ".container { padding: 20px; }"
                "}"
            "</style>"
            "<script>"
                "function updateFileName() {"
                    "var input = document.getElementById('firmware');"
                    "var fileInfo = document.getElementById('fileInfo');"
                    "if (input.files.length > 0) {"
                        "var file = input.files[0];"
                        "fileInfo.innerHTML = '<strong>Selected:</strong> ' + file.name + ' (' + Math.round(file.size/1024) + ' KB)';"
                        "fileInfo.style.display = 'block';"
                    "} else {"
                        "fileInfo.style.display = 'none';"
                    "}"
                "}"
                
                "function uploadFirmware() {"
                    "var fileInput = document.getElementById('firmware');"
                    "var file = fileInput.files[0];"
                    
                    "if (!file) {"
                        "alert('Please select a firmware file first');"
                        "return false;"
                    "}"
                    
                    "if (!file.name.toLowerCase().endsWith('.bin')) {"
                        "alert('Please select a .bin file');"
                        "return false;"
                    "}"
                    
                    "if (file.size > 4 * 1024 * 1024) {"  // 4MB limit
                        "alert('File is too large. Maximum size is 4MB.');"
                        "return false;"
                    "}"
                    
                    "if (!confirm('Are you sure you want to update the firmware? This will reboot the device.')) {"
                        "return false;"
                    "}"
                    
                    "document.getElementById('uploadForm').style.display = 'none';"
                    "document.getElementById('progress').style.display = 'block';"
                    
                    "var formData = new FormData();"
                    "formData.append('firmware', file);"
                    
                    "var xhr = new XMLHttpRequest();"
                    
                    "xhr.upload.addEventListener('progress', function(e) {"
                        "if (e.lengthComputable) {"
                            "var percentComplete = Math.round((e.loaded / e.total) * 100);"
                            "var progressBar = document.getElementById('progressBar');"
                            "progressBar.style.width = percentComplete + '%';"
                            "progressBar.textContent = percentComplete + '%';"
                        "}"
                    "});"
                    
                    "xhr.addEventListener('load', function() {"
                        "var progressBar = document.getElementById('progressBar');"
                        "if (xhr.status === 200) {"
                            "progressBar.style.backgroundColor = '#4CAF50';"
                            "progressBar.textContent = 'Update Complete! Rebooting...';"
                            "setTimeout(function() {"
                                "alert('Update completed successfully. The device is rebooting.');"
                            "}, 2000);"
                        "} else {"
                            "progressBar.style.backgroundColor = '#f44336';"
                            "progressBar.textContent = 'Update Failed!';"
                            "alert('Upload failed: ' + xhr.responseText);"
                            "setTimeout(function() {"
                                "location.reload();"
                            "}, 3000);"
                        "}"
                    "});"
                    
                    "xhr.addEventListener('error', function() {"
                        "var progressBar = document.getElementById('progressBar');"
                        "progressBar.style.backgroundColor = '#f44336';"
                        "progressBar.textContent = 'Connection Error!';"
                        "alert('Network error occurred during upload');"
                        "setTimeout(function() {"
                            "location.reload();"
                        "}, 3000);"
                    "});"
                    
                    "xhr.open('POST', '/upload');"
                    "xhr.send(formData);"
                    "return false;"
                "}"
                
                "function cancelUpdate() {"
                    "if (confirm('Are you sure you want to cancel and reboot the device?')) {"
                        "window.location.href = '/cancel';"
                    "}"
                "}"
            "</script>"
        "</head>"
        "<body>"
            "<div class=\"container\">"
                "<h1>Rate Controller Firmware Update</h1>"
                
                "<div class=\"browser-section\">"
                    "<p><strong>Prefer to use your regular browser?</strong></p>"
                    "<a href=\"intent://192.168.4.1/update#Intent;scheme=http;end\" class=\"open-btn\">"
                        "Open in External Browser"
                    "</a>"
                "</div>"
                
                "<div class=\"warning\">"
                    "<strong>Important:</strong> Do not power off or disconnect the device during the update process. "
                    "Ensure the device has stable power supply before proceeding."
                "</div>"
                
                "<div class=\"info\">"
                    "<strong>Instructions:</strong><br>"
                    "1. Select your firmware .bin file below<br>"
                    "2. Verify the file name and size<br>"
                    "3. Click 'Upload Firmware' and wait for completion<br>"
                    "4. Device will automatically reboot when finished"
                "</div>"
                
                "<div id=\"uploadForm\" class=\"upload-form\">"
                    "<input type=\"file\" name=\"firmware\" id=\"firmware\" accept=\".bin\" onchange=\"updateFileName()\" required>"
                    "<div id=\"fileInfo\" class=\"file-info\"></div>"
                    
                    "<input type=\"submit\" value=\"Upload Firmware\" onclick=\"return uploadFirmware()\">"
                    "<input type=\"button\" value=\"Cancel & Reboot\" class=\"cancel-btn\" onclick=\"cancelUpdate()\">"
                "</div>"
                
                "<div id=\"progress\" class=\"progress\">"
                    "<div id=\"progressBar\" class=\"progress-bar\">0%</div>"
                "</div>"
                
                "<p style=\"text-align: center; color: #666; font-size: 14px; margin-top: 30px;\">"
                    "<small>White LED indicates update mode is active</small>"
                "</p>"
            "</div>"
        "</body>"
        "</html>"
    );
}

void OTAUpdater::handleOTA() {
    if (!otaActive) return;
    
    dnsServer.processNextRequest(); // Handle captive portal DNS requests
    server.handleClient();
    
    // Check for timeout
    if (millis() - otaStartTime > OTA_TIMEOUT) {
        Serial.println("OTA timeout - exiting OTA mode");
        stopOTAMode();
    }
}

void OTAUpdater::stopOTAMode() {
    if (!otaActive) return;
    
    Serial.println("Stopping OTA mode...");
    dnsServer.stop();
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    
    otaActive = false;
    
    // Restart ESP-NOW (you'll need to call your ESP-NOW init function)
    delay(1000);
    ESP.restart(); // Simple approach - restart to restore normal operation
}
