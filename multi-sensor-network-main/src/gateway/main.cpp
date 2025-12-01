// Gateway: receives ESP-NOW data and forwards it to a web server over Wi-Fi.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "config.h"
#include "espnow_comm.h" // ESP-NOW communication

void connectToWiFi() {
    Serial.printf("Connecting to WiFi SSID '%s'...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi connected, IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Gateway will forward data to: ");
        Serial.println(WEB_SERVER_URL);
    } else {
        Serial.println("WiFi connection failed, continuing with ESP-NOW only");
        Serial.println("Note: Data will not be forwarded to web server without WiFi");
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n========================================");
    Serial.println("=== ESP32 Gateway Starting ===");
    Serial.println("========================================\n");
    
    Serial.println("Step 1: Connecting to WiFi...");
    connectToWiFi();
    
    Serial.println("\nStep 2: Initializing ESP-NOW...");
    ESPNOWSetup(); 
    
    Serial.println("\n========================================");
    Serial.println("=== Gateway Ready ===");
    Serial.println("========================================");
    Serial.println("Status: Listening for ESP-NOW packets");
    Serial.println("Action: Received data will be forwarded to web server");
    Serial.printf("Web Server: %s\n", WEB_SERVER_URL);
    Serial.println("Waiting for sensor stations to send data...\n");
}

void loop() {
    // Monitor WiFi connection status and attempt reconnection if needed
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 30000) { // Check every 30 seconds
        lastWiFiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected, attempting to reconnect...");
            connectToWiFi();
        }
    }
    
    // All data forwarding is handled in ESP-NOW callbacks (espnow_comm.h)
    // Real sensor data is automatically forwarded when received via ESP-NOW
}
