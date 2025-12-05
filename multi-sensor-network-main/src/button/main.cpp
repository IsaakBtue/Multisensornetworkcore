// Button Connection Mode: Uses WiFiManager library for easy WiFi configuration
// Press button (GPIO 32-34 connection) to enter configuration mode
// ESP saves credentials and connects to WiFi to send data to server

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "config.h"

#define BUTTON_PIN_1 GPIO_NUM_32  // Can be output
#define BUTTON_PIN_2 GPIO_NUM_34  // Input-only

WiFiManager wm;
Preferences preferences;

// Store WiFi credentials
String savedSSID = "";
String savedPassword = "";

bool shouldSaveConfig = false;

// Forward declarations
void connectToWiFi();
void sendConnectedMessage();

// Callback to save config after web server updates SSID and password
void saveConfigCallback() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

// Check if button is pressed (GPIO 32 and 34 are connected)
// GPIO 32 can be output, GPIO 34 is input-only
bool isButtonPressed() {
    // Set GPIO 32 as output HIGH, GPIO 34 as input with pull-down
    pinMode(BUTTON_PIN_1, OUTPUT);  // GPIO 32
    pinMode(BUTTON_PIN_2, INPUT_PULLDOWN); // GPIO 34
    
    digitalWrite(BUTTON_PIN_1, HIGH);
    delay(50); // Delay for signal to stabilize
    
    // If connected, GPIO 34 should read HIGH (pulled up by GPIO 32)
    bool connected = digitalRead(BUTTON_PIN_2) == HIGH;
    
    // Reset pins
    if (!connected) {
        pinMode(BUTTON_PIN_1, INPUT);
    }
    
    return connected;
}

// Load saved WiFi credentials from preferences
void loadWiFiCredentials() {
    if (preferences.begin("wifi", true)) { // Read-only mode
        savedSSID = preferences.getString("ssid", "");
        savedPassword = preferences.getString("password", "");
        preferences.end();
        
        if (savedSSID.length() > 0) {
            Serial.println("Loaded saved WiFi credentials:");
            Serial.printf("  SSID: %s\n", savedSSID.c_str());
        } else {
            Serial.println("No saved WiFi credentials found.");
        }
    } else {
        Serial.println("Preferences namespace not found (first run).");
        savedSSID = "";
        savedPassword = "";
    }
}

// Save WiFi credentials to preferences
void saveWiFiCredentials(const char* ssid, const char* password) {
    preferences.begin("wifi", false); // Read-write mode
    preferences.putString("ssid", String(ssid));
    preferences.putString("password", String(password));
    preferences.end();
    Serial.println("WiFi credentials saved to flash memory.");
}

// Reset/Clear WiFi credentials from preferences
void resetWiFiCredentials() {
    preferences.begin("wifi", false); // Read-write mode
    preferences.remove("ssid");
    preferences.remove("password");
    preferences.end();
    Serial.println("WiFi credentials cleared from flash memory.");
    savedSSID = "";
    savedPassword = "";
}

// Connect to WiFi and send message to server
void connectToWiFi() {
    Serial.println("\n========================================");
    Serial.println("=== Connecting to WiFi ===");
    Serial.println("========================================");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
    Serial.printf("Connecting to '%s'...\n", savedSSID.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("Signal strength (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        
        // Send "Connected!" message to webserver
        sendConnectedMessage();
        
        Serial.println("\n✓ WiFi setup complete! ESP32 is ready to send data.");
    } else {
        Serial.println("WiFi connection failed!");
        Serial.println("Please check your credentials and try again.");
        Serial.println("You can reconnect GPIO 32 and 34 to reconfigure.");
    }
}

// Send "Connected!" message to webserver
void sendConnectedMessage() {
    Serial.println("\n========================================");
    Serial.println("=== Sending Connection Message ===");
    Serial.println("========================================");
    
    HTTPClient http;
    String url = String(SUPABASE_EDGE_FUNCTION_URL);
    
    Serial.printf("Connecting to: %s\n", url.c_str());
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Follow redirects (308 -> HTTPS)
    
    // Create JSON payload with "Connected!" message
    String jsonPayload = "{\"message\":\"Connected!\",\"device_id\":\"button-mode\",\"temperature\":0,\"co2\":0,\"humidity\":0}";
    
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
        
        // 308 is a redirect (HTTP to HTTPS), 200 is success
        if (httpResponseCode == 308 || httpResponseCode == 200) {
            String response = http.getString();
            Serial.printf("Response: %s\n", response.c_str());
            if (httpResponseCode == 308) {
                Serial.println("✓ Redirect received (HTTP->HTTPS). Message likely sent successfully!");
            } else {
                Serial.println("✓ Successfully sent 'Connected!' message to webserver!");
            }
        } else {
            Serial.printf("Response code: %d (check if message was received)\n", httpResponseCode);
        }
    } else {
        Serial.printf("Error sending message: %s\n", http.errorToString(httpResponseCode).c_str());
    }
    
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("=== ESP32 Button Connection Mode ===");
    Serial.println("========================================\n");
    
    // Load saved WiFi credentials
    loadWiFiCredentials();
    
    // Set WiFiManager save config callback
    wm.setSaveConfigCallback(saveConfigCallback);
    
    // Configure WiFiManager for better captive portal support
    wm.setConfigPortalTimeout(300); // Portal stays open for 5 minutes
    wm.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0)); // Set AP IP to 192.168.4.1
    wm.setAPCallback([](WiFiManager *myWiFiManager) {
        Serial.println("Entered config mode");
        Serial.print("AP IP address: ");
        Serial.println(WiFi.softAPIP());
        Serial.println("Tap on 'ESP32_Config' network name to open configuration page!");
    });
    
    // Enable automatic captive portal (this makes tapping network name work)
    wm.setEnableConfigPortal(true);
    wm.setBreakAfterConfig(false); // Don't exit after config, wait for save
    
    // Check for button press (momentary press detection)
    Serial.println("Waiting for button press (GPIO 32-34 connection)...");
    Serial.println("Press button now (you have 3 seconds)...");
    
    bool buttonPressed = false;
    unsigned long startTime = millis();
    
    // Check for button press for 3 seconds
    while (millis() - startTime < 3000) {
        if (isButtonPressed()) {
            buttonPressed = true;
            Serial.println("✓ Button press detected!");
            break;
        }
        delay(50);
    }
    
    if (buttonPressed) {
        // Button was pressed - reset WiFi and enter configuration mode
        Serial.println("Button pressed! Resetting WiFi credentials...");
        resetWiFiCredentials();
        Serial.println("Starting WiFiManager configuration portal...\n");
        
        // Start WiFiManager config portal with captive portal enabled
        // This creates "ESP32_Config" network with automatic captive portal
        // When you tap the network name, it will open 192.168.4.1 automatically
        if (!wm.startConfigPortal("ESP32_Config")) {
            Serial.println("Failed to start config portal!");
        } else {
            Serial.println("\n========================================");
            Serial.println("=== Configuration Portal Active ===");
            Serial.println("========================================");
            Serial.println("WiFi Network Name (SSID): ESP32_Config");
            Serial.println("\nOn your phone:");
            Serial.println("1. Connect to WiFi: ESP32_Config");
            Serial.println("2. Configuration page will open automatically!");
            Serial.println("3. Select your WiFi network and enter password");
            Serial.println("4. ESP32 will save and connect automatically");
            Serial.println("========================================\n");
            
            // Save config if needed
            if (shouldSaveConfig) {
                String ssid = wm.getWiFiSSID();
                String pass = wm.getWiFiPass();
                
                Serial.println("Saving WiFi credentials...");
                Serial.printf("  SSID: %s\n", ssid.c_str());
                Serial.printf("  Password: %s\n", pass.c_str());
                
                saveWiFiCredentials(ssid.c_str(), pass.c_str());
                savedSSID = ssid;
                savedPassword = pass;
                
                Serial.println("Credentials saved. Restarting to apply settings...");
                delay(1000);
                ESP.restart();
            }
        }
    } else {
        // Button not pressed - try to connect with saved credentials
        Serial.println("No button press detected.");
        
        if (savedSSID.length() > 0) {
            Serial.println("Found saved WiFi credentials. Connecting...");
            connectToWiFi();
        } else {
            Serial.println("No saved WiFi credentials found.");
            Serial.println("Press button (connect GPIO 32-34) and reset to enter configuration mode.");
            Serial.println("ESP32 will wait...");
        }
    }
}

void loop() {
    // Check for button press to reset WiFi (works whether connected or not)
    static bool lastButtonState = false;
    static unsigned long lastButtonCheck = 0;
    
    if (millis() - lastButtonCheck > 100) { // Check every 100ms
        lastButtonCheck = millis();
        bool currentButtonState = isButtonPressed();
        
        // Detect button press (transition from not pressed to pressed)
        if (currentButtonState && !lastButtonState) {
            Serial.println("\n========================================");
            Serial.println("Button pressed! Resetting WiFi...");
            Serial.println("========================================");
            
            // Disconnect from WiFi if connected
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("Disconnecting from WiFi...");
                WiFi.disconnect();
                delay(500);
            }
            
            // Clear saved WiFi credentials
            resetWiFiCredentials();
            
            Serial.println("WiFi reset complete. Restarting in configuration mode...");
            delay(1000);
            ESP.restart();
        }
        
        lastButtonState = currentButtonState;
    }
    
    // If connected to WiFi, check connection status
    if (WiFi.status() == WL_CONNECTED) {
        // WiFi is connected, can send data here
        // This will be integrated with gateway mode later
        static unsigned long lastCheck = 0;
        if (millis() - lastCheck > 30000) { // Every 30 seconds
            lastCheck = millis();
            Serial.println("[Status] WiFi connected. Ready to send data.");
            Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        }
    }
    
    delay(10);
}
