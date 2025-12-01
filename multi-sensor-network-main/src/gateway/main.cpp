// Gateway: receives ESP-NOW data and forwards it to a web server over Wi-Fi.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "config.h"
#include "espnow_comm.h" // ESP-NOW communication

// Fake data generator - creates a fake station for testing
Station* fakeStation = nullptr;

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

    Serial.println("=== Gateway Starting ===");
    Serial.println("Initializing connection to web server...");
    
    // Seed random number generator for realistic fake data variation
    randomSeed(analogRead(0));
    
    connectToWiFi();
    ESPNOWSetup(); 
    
    // Create fake station for testing with fake MAC address
    uint8_t fakeMac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    fakeStation = new Station(fakeMac);
    // Initialize with realistic starting values
    fakeStation->readings.temperature = 22.0;  // Room temperature in Celsius
    fakeStation->readings.co2 = 450;           // Normal indoor CO2 level (ppm)
    fakeStation->readings.humidity = 50.0;     // Normal indoor humidity (%)
    
    Serial.println("Gateway ready: listening for ESP-NOW packets and forwarding to web server.");
    Serial.println("FAKE DATA MODE: Sending realistic test data to web server");
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
    
    // Send fake data periodically (every 5 seconds to match measurement interval)
    static unsigned long lastFakeDataSend = 0;
    const unsigned long fakeDataInterval = MEASUREMENT_INTERVAL * 1000; // Convert seconds to milliseconds
    
    if (millis() - lastFakeDataSend >= fakeDataInterval) {
        lastFakeDataSend = millis();
        
        if (fakeStation && WiFi.status() == WL_CONNECTED) {
            // Generate realistic sensor data with small random variations
            // Temperature: 20-25°C with small fluctuations
            static float baseTemp = 22.5;
            baseTemp += (random(-10, 11) / 10.0); // ±1°C variation
            baseTemp = constrain(baseTemp, 20.0, 25.0);
            fakeStation->readings.temperature = baseTemp;
            
            // CO2: 400-800 ppm (normal to slightly elevated)
            static int baseCO2 = 500;
            baseCO2 += random(-20, 25); // ±20 ppm variation
            baseCO2 = constrain(baseCO2, 400, 800);
            fakeStation->readings.co2 = baseCO2;
            
            // Humidity: 40-60% (comfortable indoor range)
            static float baseHumidity = 50.0;
            baseHumidity += (random(-15, 16) / 10.0); // ±1.5% variation
            baseHumidity = constrain(baseHumidity, 40.0, 60.0);
            fakeStation->readings.humidity = baseHumidity;
            
            Serial.printf("Sending fake data to %s - Temp: %.1f°C, CO2: %d ppm, Humidity: %.1f%%\n", 
                         WEB_SERVER_URL,
                         fakeStation->readings.temperature, 
                         fakeStation->readings.co2, 
                         fakeStation->readings.humidity);
            
            sendToServer(fakeStation);
        } else if (!fakeStation) {
            Serial.println("Error: Fake station not initialized");
        } else {
            Serial.println("WiFi not connected, cannot send fake data");
        }
    }
    
    // All data forwarding is handled in ESP-NOW callbacks (espnow_comm.h)
}
