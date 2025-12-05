// Gateway: receives ESP-NOW data and forwards it to a web server over Wi-Fi.

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#if defined(ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S3)
#include <esp_mac.h>
#endif

#include "config.h"
#include "espnow_comm.h" // ESP-NOW communication

void connectToWiFi() {
    Serial.printf("Connecting to WiFi SSID '%s'...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi connected, IP: ");
        Serial.println(WiFi.localIP());
        
        // Get current network configuration
        IPAddress localIP = WiFi.localIP();
        IPAddress gateway = WiFi.gatewayIP();
        IPAddress subnet = WiFi.subnetMask();
        
        // Force Google DNS - reconfigure with DNS servers
        Serial.println("Configuring Google DNS (8.8.8.8, 8.8.4.4)...");
        WiFi.config(localIP, gateway, subnet, IPAddress(8, 8, 8, 8), IPAddress(8, 8, 4, 4));
        delay(2000); // Wait for DNS config to apply
        
        // Disconnect and reconnect to apply DNS settings
        WiFi.disconnect();
        delay(1000);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        
        // Wait for reconnection
        start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        
        // Verify DNS configuration
        IPAddress dns1 = WiFi.dnsIP(0);
        IPAddress dns2 = WiFi.dnsIP(1);
        Serial.print("DNS Server 1: ");
        Serial.println(dns1);
        Serial.print("DNS Server 2: ");
        Serial.println(dns2);
        
        // Test DNS resolution
        IPAddress testIP;
        Serial.print("Testing DNS with google.com... ");
        if (WiFi.hostByName("google.com", testIP) == 1) {
            Serial.print("SUCCESS! IP: ");
            Serial.println(testIP);
        } else {
            Serial.println("FAILED");
        }
        
        // Test Vercel domain specifically
        Serial.print("Testing DNS with multisensornetwork.vercel.app... ");
        IPAddress vercelIP;
        if (WiFi.hostByName("multisensornetwork.vercel.app", vercelIP) == 1) {
            Serial.print("SUCCESS! IP: ");
            Serial.println(vercelIP);
        } else {
            Serial.println("FAILED - This domain may not resolve");
        }
        
        // Test Supabase domain DNS resolution
        Serial.print("Testing DNS with hvmhlvaoovmyctmctqyb.supabase.co... ");
        IPAddress supabaseIP;
        if (WiFi.hostByName("hvmhlvaoovmyctmctqyb.supabase.co", supabaseIP) == 1) {
            Serial.print("SUCCESS! IP: ");
            Serial.println(supabaseIP);
        } else {
            Serial.println("FAILED - Supabase domain may not resolve");
            Serial.println("This will cause connection issues. Check DNS settings.");
        }
        
        // Test Cloudflare Worker DNS resolution
        String workerUrl = String(SUPABASE_EDGE_FUNCTION_URL);
        int protocolEnd = workerUrl.indexOf("://");
        if (protocolEnd > 0) {
            String domain = workerUrl.substring(protocolEnd + 3);
            int pathStart = domain.indexOf("/");
            if (pathStart > 0) {
                domain = domain.substring(0, pathStart);
            }
            Serial.print("Testing DNS with Cloudflare Worker: ");
            Serial.print(domain);
            Serial.print("... ");
            IPAddress workerIP;
            if (WiFi.hostByName(domain.c_str(), workerIP) == 1) {
                Serial.print("SUCCESS! IP: ");
                Serial.println(workerIP);
            } else {
                Serial.println("FAILED - Worker domain may not resolve");
                Serial.println("This will cause connection issues.");
            }
        }
        
        Serial.print("Gateway will forward data to: ");
        Serial.println(SUPABASE_EDGE_FUNCTION_URL);
    } else {
        Serial.println("WiFi connection failed, continuing with ESP-NOW only");
        Serial.println("Note: Data will not be forwarded to web server without WiFi");
    }
}

void setup() {
    // Initialize serial communication (use COM USB-C port for programming/serial monitor)
    Serial.begin(115200);
    // Wait for serial port to connect (important for ESP32-S3 with dual USB ports)
    // Use COM port (USB-to-UART) for serial communication
    while (!Serial && millis() < 3000) {
        delay(10);
    }
    delay(500);

    Serial.println("\n========================================");
    Serial.println("=== ESP32-S3 Gateway Starting ===");
    Serial.println("========================================\n");
    
    Serial.println("Step 1: Connecting to WiFi...");
    connectToWiFi();
    
    Serial.println("\nStep 2: Initializing ESP-NOW...");
    ESPNOWSetup(); 
    
    Serial.println("\n========================================");
    Serial.println("=== Gateway Ready ===");
    Serial.println("========================================");
    Serial.println("Status: Listening for ESP-NOW packets");
    Serial.println("Action: Received data will be forwarded to Supabase Edge Function");
    Serial.printf("Supabase Edge Function: %s\n", SUPABASE_EDGE_FUNCTION_URL);
    
    // Print gateway MAC address for debugging
    Serial.print("Gateway MAC Address: ");
    uint8_t gatewayMac[6];
#if defined(ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S3)
    // ESP32-S3 uses ESP_MAC_WIFI_SOFTAP or ESP_MAC_WIFI_STA
    esp_read_mac(gatewayMac, ESP_MAC_WIFI_STA);
#else
    esp_read_mac(gatewayMac, ESP_MAC_WIFI_STA);
#endif
    for (int i = 0; i < 6; ++i) {
        Serial.printf("%02X", gatewayMac[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
    
    Serial.println("Waiting for sensor stations to send data...");
    Serial.println("(Stations should send to broadcast address FF:FF:FF:FF:FF:FF)");
    Serial.println();
}

void loop() {
    // Monitor WiFi connection status and attempt reconnection if needed
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 30000) { // Check every 30 seconds
        lastWiFiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi disconnected, attempting to reconnect...");
            connectToWiFi();
        } else {
            // Periodic heartbeat to show gateway is alive
            Serial.println("[Heartbeat] Gateway is alive and listening for ESP-NOW packets...");
        }
    }
    
    // All data forwarding is handled in ESP-NOW callbacks (espnow_comm.h)
    // Real sensor data is automatically forwarded when received via ESP-NOW
}
