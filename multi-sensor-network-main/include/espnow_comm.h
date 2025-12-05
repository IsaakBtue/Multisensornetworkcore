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

// HTTP support - ESP32-S3 Arduino framework doesn't include WiFiClientSecure in version 3.3.4
// Using HTTP to Cloudflare Worker which forwards to HTTPS Vercel
#ifdef ROLE_GATEWAY
  #define HAS_WIFI_CLIENT_SECURE 0
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

  // Using WiFiClientSecure for HTTPS connections (works on ESP32-S3!)
  
  // Check free heap memory
  Serial.printf("Free heap before connection: %d bytes\n", ESP.getFreeHeap());
  if (ESP.getFreeHeap() < 50000) {
    Serial.println("WARNING: Low free heap memory");
  }
  
  String fullUrl = String(SUPABASE_EDGE_FUNCTION_URL);
  bool useHTTPS = fullUrl.startsWith("https://");
  
  Serial.printf("Connecting to server: %s\n", fullUrl.c_str());
  Serial.printf("Protocol: %s\n", useHTTPS ? "HTTPS" : "HTTP");
  
  // Extract domain and path from URL
  String domain = fullUrl;
  int protocolEnd = domain.indexOf("://");
  if (protocolEnd > 0) {
    domain = domain.substring(protocolEnd + 3);
  }
  
  String path = "/";
  int pathStart = domain.indexOf("/");
  if (pathStart > 0) {
    path = domain.substring(pathStart);
    domain = domain.substring(0, pathStart);
  }
  
  // Extract port (default 443 for HTTPS, 80 for HTTP)
  int port = useHTTPS ? 443 : 80;
  int portStart = domain.indexOf(":");
  if (portStart > 0) {
    String portStr = domain.substring(portStart + 1);
    port = portStr.toInt();
    domain = domain.substring(0, portStart);
  }
  
  Serial.printf("Host: %s, Port: %d, Path: %s\n", domain.c_str(), port, path.c_str());
  
  // Build MAC string for device_id
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           st->mac[0], st->mac[1], st->mac[2],
           st->mac[3], st->mac[4], st->mac[5]);

  // Build JSON payload
  String payload = "{";
  payload += "\"mac\":\""; payload += macStr; payload += "\",";
  payload += "\"device_id\":\""; payload += macStr; payload += "\",";
  payload += "\"temperature\":"; payload += String(st->readings.temperature, 2); payload += ",";
  payload += "\"humidity\":"; payload += String(st->readings.humidity, 2); payload += ",";
  payload += "\"co2\":"; payload += String(st->readings.co2);
  payload += "}";

  Serial.print("Sending data to server: ");
  Serial.println(payload);
  
  int httpCode = -1;
  
  // Use HTTP to Cloudflare Worker (ESP32-S3 doesn't have WiFiClientSecure in Arduino framework 3.3.4)
  Serial.println("Using HTTP to Cloudflare Worker...");
  WiFiClient regularClient;
  
  // Set longer timeout for connection
  regularClient.setTimeout(15000);
  
  Serial.printf("Attempting HTTP connection to %s:%d...\n", domain.c_str(), port);
  Serial.printf("Domain length: %d chars\n", domain.length());
  
  // Try connecting with hostname (required for Cloudflare)
  unsigned long connectStart = millis();
  bool connected = regularClient.connect(domain.c_str(), port);
  unsigned long connectTime = millis() - connectStart;
  
  Serial.printf("Connection attempt took %lu ms\n", connectTime);
  Serial.printf("Connected: %s\n", connected ? "YES" : "NO");
  Serial.printf("Client status: %d\n", regularClient.connected());
  
  if (connected) {
    Serial.println("✓ HTTP connection established!");
    
    // Send HTTP POST request with proper headers
    Serial.println("Sending HTTP POST request...");
    regularClient.print("POST ");
    regularClient.print(path);
    regularClient.println(" HTTP/1.1");
    regularClient.print("Host: ");
    regularClient.println(domain);
    regularClient.println("Content-Type: application/json");
    regularClient.println("User-Agent: ESP32-S3-Gateway/1.0");
    regularClient.println("Accept: application/json");
    regularClient.println("Connection: close");
    regularClient.print("Content-Length: ");
    regularClient.println(payload.length());
    regularClient.println(); // Empty line before body
    regularClient.print(payload);
    regularClient.println(); // Final newline
    
    Serial.println("✓ HTTP POST request sent");
    Serial.printf("Payload length: %d bytes\n", payload.length());
    
    // Wait for response with longer timeout
    unsigned long timeout = millis() + 20000; // 20 second timeout
    Serial.println("Waiting for response...");
    int availableCount = 0;
    while (!regularClient.available() && millis() < timeout) {
      delay(100);
      if (millis() % 2000 < 100) { // Print every 2 seconds
        Serial.print(".");
      }
    }
    Serial.println();
    
    if (regularClient.available()) {
      Serial.println("✓ Response received!");
      Serial.printf("Bytes available: %d\n", regularClient.available());
      
      String response = "";
      unsigned long readStart = millis();
      while (regularClient.available() || (millis() - readStart < 5000)) {
        if (regularClient.available()) {
          char c = regularClient.read();
          response += c;
          if (response.length() > 2000) break; // Limit response size
        } else {
          delay(10);
        }
      }
      
      Serial.printf("Response length: %d chars\n", response.length());
      
      // Extract status code
      int statusCodePos = response.indexOf("HTTP/1.");
      if (statusCodePos >= 0) {
        int codeStart = response.indexOf(" ", statusCodePos) + 1;
        int codeEnd = response.indexOf(" ", codeStart);
        if (codeEnd > codeStart) {
          httpCode = response.substring(codeStart, codeEnd).toInt();
          Serial.printf("HTTP Status Code: %d\n", httpCode);
        }
      }
      
      Serial.print("Response (first 500 chars): ");
      Serial.println(response.substring(0, 500));
    } else {
      Serial.println("✗ No response received (timeout)");
      Serial.printf("Connection still active: %s\n", regularClient.connected() ? "YES" : "NO");
      httpCode = -1;
    }
    
    regularClient.stop();
    Serial.println("Connection closed");
  } else {
    Serial.printf("✗ HTTP connection failed to %s:%d\n", domain.c_str(), port);
    Serial.printf("Last error: %d\n", regularClient.getWriteError());
    Serial.println("Possible causes:");
    Serial.println("  1. Cloudflare Workers on *.workers.dev only accept HTTPS from embedded devices");
    Serial.println("  2. Need to use custom domain with HTTP support");
    Serial.println("  3. Firewall/router blocking outbound port 80");
    Serial.println("  4. Cloudflare blocking embedded device connections");
    httpCode = -1;
  }
  
  // Handle response
  if (httpCode > 0) {
    if (httpCode >= 200 && httpCode < 300) {
      Serial.println("✓ SUCCESS: Data sent successfully to web server!");
      Serial.println("Data should now be visible on the website");
    } else if (httpCode == 308) {
      Serial.println("WARNING: 308 redirect received - POST data may be lost");
      Serial.println("This usually means the server redirected HTTP to HTTPS");
    } else {
      Serial.printf("Unexpected response code: %d\n", httpCode);
    }
  } else {
    Serial.printf("✗ Connection failed, error code: %d\n", httpCode);
    
    // More detailed error information
    if (httpCode == -1) {
      Serial.println("Error -1: Connection refused");
      Serial.println("Vercel blocks HTTP connections - requires HTTPS");
      Serial.println("Solutions:");
      Serial.println("  1. Use a local server: http://YOUR_LOCAL_IP:3000/api/ingest-http-bridge");
      Serial.println("  2. Use a different cloud service that accepts HTTP");
      Serial.println("  3. Set up a reverse proxy that accepts HTTP");
      Serial.println("  4. ESP32-S3 cannot do HTTPS without WiFiClientSecure");
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
    
    Serial.println("This may indicate connection issues");
  }
  
  // Connection cleanup is handled by regularClient.stop() above
  // Small delay to let cleanup complete
  delay(50);
}
#endif

// Callback when data is received
// ESP32-S3 uses new callback signature with esp_now_recv_info_t
#if defined(ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S3)
  void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t* data, int len) {
    const uint8_t* mac_addr = recv_info->src_addr;
    int rssi = recv_info->rx_ctrl->rssi;
#else
  // Old ESP32 signature: (const uint8_t* mac_addr, const uint8_t* data, int len)
  void OnDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
    int rssi = 0; // RSSI not directly available in old framework callback
    // RSSI will be updated via promiscuous mode callback
#endif
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