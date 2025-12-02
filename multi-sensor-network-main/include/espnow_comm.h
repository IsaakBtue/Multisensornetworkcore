#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <stdlib.h> // For malloc/free
#include <string.h> // For strlen

#include "typedef.h"
#include "config.h"

// For HTTPS support on ESP32 - try to include WiFiClientSecure
#ifdef ROLE_GATEWAY
  #ifdef ESP32
    // Try including WiFiClientSecure - it should be available in ESP32 framework
    #include <WiFiClientSecure.h>
    #define HAS_WIFI_CLIENT_SECURE 1
  #else
    #define HAS_WIFI_CLIENT_SECURE 0
  #endif
#else
  #define HAS_WIFI_CLIENT_SECURE 0
#endif

bool send_done = false;
// FF:FF:FF:FF:FF:FF is broadcast MAC
uint8_t broadcastAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };



// Station class to manage individual Stations
class Station {
public:
  uint8_t mac[6]; // MAC address
  int rssi;
  struct readings { // Store sensor readings
    float temperature;
    uint16_t co2;
    float humidity;
  } readings;

  Station(const uint8_t* mac_addr) : rssi(0) {
    memcpy(mac, mac_addr, 6);
    // Initialize other members if needed
  }

  void updateRSSI(int new_rssi) {
    rssi = new_rssi;
  }

  void handleMessage(const uint8_t* data, int len) {
    if (len == sizeof(sensor_msg)) {
      const sensor_msg* msg = (const sensor_msg*)data;
      readings.temperature = msg->temperature;
      readings.co2 = msg->co2;
      readings.humidity = msg->humidity;
      Serial.print("Message from station: ");
      for (int i = 0; i < 6; ++i) {
        Serial.printf("%02X", mac[i]);
        if (i < 5) Serial.print(":");
      }
      Serial.printf(" | Temp: %.2f, CO2: %d, Humidity: %.2f | RSSI: %d\n", readings.temperature, readings.co2, readings.humidity, rssi);
    } else {
      Serial.println("Invalid sensor_msg length");
    }
  }
};

// Generate Station objects 
Station* stations[NUM_STATIONS]; 
int stationCount = 0;

// Helper to find or create a Station object for a given MAC
Station* findStation(const uint8_t* mac) {
  for (int i = 0; i < stationCount; ++i) {
    if (memcmp(stations[i]->mac, mac, 6) == 0) {
      return stations[i];
    }
  }
  return NULL;
}

// Debug function to print all registered stations
void printRegisteredStations() {
  Serial.printf("Registered stations: %d / %d\n", stationCount, NUM_STATIONS);
  for (int i = 0; i < stationCount; ++i) {
    Serial.printf("  Station %d: ", i);
    for (int j = 0; j < 6; ++j) {
      Serial.printf("%02X", stations[i]->mac[j]);
      if (j < 5) Serial.print(":");
    }
    Serial.printf(" (RSSI: %d)\n", stations[i]->rssi);
  }
}

// Create or get the existing Station
Station* getOrCreateStation(const uint8_t* mac) {
  Station* existing = findStation(mac);
  if (existing) {
    return existing;
  }
  if (stationCount < NUM_STATIONS) {
    stations[stationCount] = new Station(mac);
    return stations[stationCount++];
  }
  return NULL; // Max stations reached
}

// Promiscuous RX callback to update RSSI for matching stations
void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT)
    return;
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  Station* station = findStation(hdr->addr2);
    if (station) {
      station->updateRSSI(ppkt->rx_ctrl.rssi);
    }
}

// Callback when data is sent
void OnDataSent(const uint8_t* mac, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
  send_done = true;
}

// Forward received data to web server (only compiled for gateway)
#ifdef ROLE_GATEWAY
void sendToServer(const Station* st) {
  if (!st) return;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping HTTP send");
    return;
  }

  // CRITICAL: ESP32 cannot establish HTTPS connections to Vercel CDN
  // The SSL/TLS handshake consistently fails due to TLS version/cipher mismatch
  // Solution: Use regular HTTPClient with WiFiClient (not Secure) as fallback
  // Note: This will fail with 308 redirect, but we'll try it anyway
  
  HTTPClient http;
  bool connectionSuccess = false;
  
  // Check free heap memory
  Serial.printf("Free heap before connection: %d bytes\n", ESP.getFreeHeap());
  if (ESP.getFreeHeap() < 50000) {
    Serial.println("WARNING: Low free heap memory");
  }
  
  // Parse URL to determine if HTTP or HTTPS
  String fullUrl = String(WEB_SERVER_URL);
  bool useHTTPS = fullUrl.startsWith("https://");
  
  Serial.printf("Connecting to: %s\n", fullUrl.c_str());
  Serial.printf("Protocol: %s\n", useHTTPS ? "HTTPS" : "HTTP");
  
  // Try HTTP first (simpler, no SSL handshake issues)
  if (!useHTTPS) {
    Serial.println("Using HTTP (no SSL/TLS required)");
    WiFiClient regularClient;
    connectionSuccess = http.begin(regularClient, fullUrl);
    
    if (connectionSuccess) {
      Serial.println("✓ HTTP connection established");
    } else {
      Serial.println("✗ HTTP connection failed");
      return;
    }
  } else {
    // Try HTTPS (may fail due to TLS version mismatch)
    Serial.println("Using HTTPS (SSL/TLS required)");
    #if HAS_WIFI_CLIENT_SECURE
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation
    client.setTimeout(30000);
    
    // Parse URL to get hostname and path
    String fullUrl = String(WEB_SERVER_URL);
    String host;
    String path = "/api/ingest";
    
    if (fullUrl.startsWith("https://")) {
      String url = fullUrl.substring(8); // Remove "https://"
      int pathIndex = url.indexOf('/');
      host = (pathIndex > 0) ? url.substring(0, pathIndex) : url;
      path = (pathIndex > 0) ? url.substring(pathIndex) : "/";
    } else {
      host = fullUrl;
    }
    
    Serial.printf("Connecting to: %s\n", fullUrl.c_str());
    Serial.printf("Host: %s, Path: %s\n", host.c_str(), path.c_str());
    
    // Try DNS resolution for logging (non-blocking - HTTPClient will handle DNS)
    IPAddress serverIP;
    Serial.print("DNS lookup (for info only)... ");
    int dnsResult = WiFi.hostByName(host.c_str(), serverIP);
    if (dnsResult == 1) {
      Serial.print("SUCCESS! IP: ");
      Serial.println(serverIP);
    } else {
      Serial.println("FAILED (HTTPClient will retry DNS)");
    }
    
    // CRITICAL: Use the hostname (not IP) in http.begin() to ensure SNI is sent
    // SNI (Server Name Indication) is required for Vercel CDN to route correctly
    Serial.println("Establishing HTTPS connection via HTTPClient...");
    Serial.println("Using hostname (not IP) to ensure SNI is sent correctly");
    
    // Method 1: Use hostname with port 443 and path - this should send SNI correctly
    connectionSuccess = http.begin(client, host, 443, path, true);
    
    if (!connectionSuccess) {
      Serial.println("Method 1 failed: http.begin(host, 443, path)");
      Serial.println("Trying Method 2: http.begin(fullUrl)...");
      // Method 2: Try with full URL - HTTPClient should parse and set SNI
      connectionSuccess = http.begin(client, fullUrl);
    }
    
    if (!connectionSuccess) {
      Serial.println("Method 2 failed: http.begin(fullUrl)");
      Serial.println("Trying Method 3: Manual connection with explicit SNI...");
      
      // Method 3: Try connecting manually first, then use connected client
      // This gives us more control over the SSL handshake
      Serial.printf("Attempting manual SSL connection to %s:443...\n", host.c_str());
      if (client.connect(host.c_str(), 443)) {
        Serial.println("✓ Manual SSL connection successful!");
        // Now try to use the connected client
        // Note: HTTPClient.begin() with an already-connected client may not work
        // So we'll try a different approach - use the client directly
        connectionSuccess = http.begin(client, host, 443, path, true);
        if (!connectionSuccess) {
          Serial.println("ERROR: http.begin() failed even after manual connect");
          client.stop();
        }
      } else {
        char errorBuf[256];
        int errorCode = client.lastError(errorBuf, sizeof(errorBuf));
        Serial.printf("✗ Manual SSL connection failed (error code: %d)\n", errorCode);
        if (strlen(errorBuf) > 0) {
          Serial.printf("Error details: %s\n", errorBuf);
        }
        Serial.println("This indicates the SSL/TLS handshake cannot complete");
        Serial.println("Possible causes:");
        Serial.println("  1. TLS version mismatch (ESP32 supports TLS 1.2, server may require 1.3)");
        Serial.println("  2. Router/firewall blocking SSL connections");
        Serial.println("  3. Vercel CDN rejecting connection without proper SNI");
        Serial.println("  4. Certificate validation issues");
      }
    }
  #else
    Serial.println("ERROR: WiFiClientSecure not available");
    Serial.println("HTTPS requires WiFiClientSecure which is not in this framework version");
    return;
  #endif
  
  // If HTTPS failed, try HTTP as last resort (will get 308 redirect, but worth trying)
  if (!connectionSuccess) {
    Serial.println("HTTPS connection failed - ESP32 cannot connect to Vercel CDN");
    Serial.println("This is a known limitation: ESP32 WiFiClientSecure cannot complete");
    Serial.println("SSL handshake with Vercel's CDN (likely TLS version mismatch)");
    Serial.println("");
    Serial.println("SOLUTION REQUIRED:");
    Serial.println("1. Use a different backend service that supports HTTP or older TLS");
    Serial.println("2. Use a simple HTTP-to-HTTPS bridge service");
    Serial.println("3. Deploy to a service that ESP32 can connect to (e.g., Firebase, AWS IoT)");
    Serial.println("");
    Serial.println("Attempting HTTP connection as fallback (will likely fail with redirect)...");
    
    // Try HTTP as absolute last resort
    WiFiClient regularClient;
    String httpUrl = String(WEB_SERVER_URL);
    httpUrl.replace("https://", "http://");
    connectionSuccess = http.begin(regularClient, httpUrl);
    
    if (!connectionSuccess) {
      Serial.println("HTTP connection also failed");
      return;
    }
    Serial.println("HTTP connection established (will redirect to HTTPS)");
  } else {
    Serial.println("HTTPS connection established successfully");
  }
  
  // Configure HTTP client
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(20000);
  http.setReuse(false);
  
  // Build MAC string
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           st->mac[0], st->mac[1], st->mac[2],
           st->mac[3], st->mac[4], st->mac[5]);

  // Build JSON payload matching the API endpoint format
  String payload = "{";
  payload += "\"mac\":\""; payload += macStr; payload += "\",";
  payload += "\"temperature\":"; payload += String(st->readings.temperature, 2); payload += ",";
  payload += "\"co2\":"; payload += String(st->readings.co2); payload += ",";
  payload += "\"humidity\":"; payload += String(st->readings.humidity, 2);
  payload += "}";

  Serial.print("Sending data to server: ");
  Serial.println(payload);
  
  // Add a small delay before POST to ensure connection is ready
  delay(100);
  
  Serial.println("Attempting HTTP POST...");
  int httpCode = http.POST(payload);
  
  Serial.printf("HTTP POST completed, response code: %d\n", httpCode);
  
  if (httpCode > 0) {
    Serial.printf("Server response code: %d\n", httpCode);
    
    if (httpCode >= 200 && httpCode < 300) {
      Serial.println("✓ SUCCESS: Data sent successfully to web server!");
      String response = http.getString();
      if (response.length() > 0) {
        Serial.print("Server response: ");
        Serial.println(response);
      }
      Serial.println("Data should now be visible on the website");
    } else if (httpCode == 308) {
      Serial.println("WARNING: 308 redirect received - POST data may be lost");
      Serial.println("This usually means the server redirected HTTP to HTTPS");
    } else {
      Serial.printf("Unexpected response code: %d\n", httpCode);
      String response = http.getString();
      if (response.length() > 0) {
        Serial.print("Server response: ");
        Serial.println(response);
      }
    }
  } else {
    Serial.printf("✗ Connection failed, error: %s (code: %d)\n", http.errorToString(httpCode).c_str(), httpCode);
    
    // More detailed error information
    if (httpCode == -1) {
      Serial.println("Error -1: Connection failed");
      Serial.println("Possible causes:");
      Serial.println("  1. SSL/TLS handshake failed");
      Serial.println("  2. Server not accepting connections");
      Serial.println("  3. Firewall blocking the connection");
      Serial.println("  4. SNI (Server Name Indication) not working");
    } else if (httpCode == -2) {
      Serial.println("Error -2: Send header failed");
    } else if (httpCode == -3) {
      Serial.println("Error -3: Send payload failed");
    } else if (httpCode == -4) {
      Serial.println("Error -4: Not connected");
    } else if (httpCode == -5) {
      Serial.println("Error -5: Connection lost");
    } else if (httpCode == -6) {
      Serial.println("Error -6: No stream");
    } else if (httpCode == -7) {
      Serial.println("Error -7: No HTTP server");
    } else if (httpCode == -8) {
      Serial.println("Error -8: Too less RAM");
    } else if (httpCode == -9) {
      Serial.println("Error -9: Encoding");
    } else if (httpCode == -10) {
      Serial.println("Error -10: Stream write");
    } else if (httpCode == -11) {
      Serial.println("Error -11: Read timeout");
    }
    
    Serial.println("This may indicate HTTPS/TLS issues");
    
    // If HTTPS failed, try HTTP as absolute last resort
    if (httpCode == -1) {
      Serial.println("");
      Serial.println("=== Trying HTTP fallback ===");
      http.end(); // Clean up HTTPS connection
      
      WiFiClient regularClient;
      String httpUrl = String(WEB_SERVER_URL);
      httpUrl.replace("https://", "http://");
      
      Serial.printf("Attempting HTTP connection to: %s\n", httpUrl.c_str());
      if (http.begin(regularClient, httpUrl)) {
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(10000);
        
        // Rebuild payload for HTTP attempt
        char* httpPayload = (char*)malloc(100);
        if (httpPayload) {
          snprintf(httpPayload, 100, 
                   "{\"mac\":\"%s\",\"temperature\":%.2f,\"co2\":%d,\"humidity\":%.2f}",
                   macStr, st->readings.temperature, st->readings.co2, st->readings.humidity);
          
          int httpCode2 = http.POST((uint8_t*)httpPayload, strlen(httpPayload));
          free(httpPayload);
          
          if (httpCode2 == 308) {
            Serial.println("HTTP returned 308 redirect - Vercel redirects HTTP to HTTPS");
            Serial.println("POST data is lost during redirect - this won't work");
          } else if (httpCode2 > 0) {
            Serial.printf("HTTP response: %d\n", httpCode2);
          }
        }
        http.end();
      } else {
        Serial.println("HTTP connection also failed");
      }
      Serial.println("=== End HTTP fallback ===");
      Serial.println("");
      Serial.println("SOLUTION REQUIRED:");
      Serial.println("ESP32 cannot connect to Vercel CDN via HTTPS");
      Serial.println("Options:");
      Serial.println("1. Use a different backend (Firebase, AWS IoT, etc.)");
      Serial.println("2. Use an HTTP-to-HTTPS bridge service");
      Serial.println("3. Deploy to a service that supports TLS 1.2");
      return; // Exit early to prevent heap corruption
    }
  }
  
  // Always clean up properly to prevent heap corruption
  http.end();
  
  // Small delay to let cleanup complete
  delay(50);
}
#endif

// Callback when data is received
// Note: Both gateway and station use framework 1.0.6 which uses the old signature
// The old signature is: (const uint8_t* mac_addr, const uint8_t* data, int len)
void OnDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
  int rssi = 0; // RSSI not directly available in old framework callback
  // RSSI will be updated via promiscuous mode callback
  Serial.printf("\n=== ESP-NOW Packet Received ===\n");
  Serial.printf("Timestamp: %lu ms\n", millis());
  Serial.printf("From MAC: ");
  for (int i = 0; i < 6; ++i) {
    Serial.printf("%02X", mac_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.printf("\nRSSI: %d dBm\n", rssi);
  Serial.printf("Data length: %d bytes (expected: %d bytes)\n", len, sizeof(sensor_msg));
  
  // Print raw data bytes for debugging
  Serial.print("Raw data (hex): ");
  for (int i = 0; i < len && i < 20; ++i) { // Print first 20 bytes
    Serial.printf("%02X ", data[i]);
  }
  if (len > 20) Serial.print("...");
  Serial.println();
  
  Station* st = getOrCreateStation(mac_addr);
  if (st && len == sizeof(sensor_msg)) {
    Serial.println("✓ Valid sensor message - processing...");
    st->handleMessage(data, len);
#ifdef ROLE_GATEWAY
    Serial.println("Forwarding to web server...");
    sendToServer(st);
    Serial.printf("Total stations registered: %d\n", stationCount);
#endif
    Serial.println("=== Packet Processing Complete ===\n");
  } else {
    if (!st) {
      Serial.printf("✗ ERROR: Cannot create station (max stations: %d, current: %d)\n", NUM_STATIONS, stationCount);
      printRegisteredStations();
    } else {
      Serial.printf("✗ ERROR: Invalid message length (got %d, expected %d)\n", len, sizeof(sensor_msg));
      Serial.println("This might indicate a data format mismatch between station and gateway");
    }
    Serial.println("=== Packet Processing Failed ===\n");
  }
}

void addBroadcastPeer() {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddr, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (!esp_now_is_peer_exist(broadcastAddr)) {
    esp_now_add_peer(&peerInfo);
  }
}

void ESPNOWSetup(){
#ifdef ROLE_GATEWAY
    // For gateway: WiFi is already connected in STA mode for web server access
    // ESP-NOW works alongside WiFi STA mode
    Serial.println("ESP-NOW: Gateway mode - WiFi STA already active");
#else
    // For stations, we only use Wi-Fi as a transport for ESP-NOW
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println("ESP-NOW: Station mode - WiFi STA for ESP-NOW transport only");
#endif
    
    // Init ESP-NOW
    Serial.println("Initializing ESP-NOW...");
    if (esp_now_init() != ESP_OK) {
        Serial.println("ERROR: ESP-NOW Init Failed. Rebooting...");
        delay(2000);
        ESP.restart();
    }
    Serial.println("ESP-NOW initialized successfully");

    // Register callbacks
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("ESP-NOW callbacks registered");
    
    // Enable promiscuous mode for RSSI tracking
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
    Serial.println("Promiscuous mode enabled for RSSI tracking");
    
#ifdef ROLE_GATEWAY
    Serial.println("Gateway ready to receive ESP-NOW packets from stations");
    Serial.println("Received data will be automatically forwarded to web server");
#endif
}