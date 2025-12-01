#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "esp_wifi.h"
#include <HTTPClient.h>
#include <WiFiClient.h>

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

  HTTPClient http;
  bool connectionSuccess = false;
  
  // Use WiFiClientSecure for HTTPS connection
  #if HAS_WIFI_CLIENT_SECURE
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation for testing
    client.setTimeout(20000);
    
    // Parse HTTPS URL
    String url = String(WEB_SERVER_URL);
    if (url.startsWith("https://")) {
      url = url.substring(8); // Remove "https://"
      int pathIndex = url.indexOf('/');
      String host = (pathIndex > 0) ? url.substring(0, pathIndex) : url;
      String path = (pathIndex > 0) ? url.substring(pathIndex) : "/";
      
      Serial.printf("Using WiFiClientSecure: %s:443%s\n", host.c_str(), path.c_str());
      // Use NetworkClient version: begin(NetworkClient &client, String host, uint16_t port, String uri, bool https)
      connectionSuccess = http.begin(client, host, 443, path, true);
    }
  #else
    Serial.println("ERROR: WiFiClientSecure not available");
    Serial.println("HTTPS requires WiFiClientSecure which is not in this framework version");
    return;
  #endif
  
  if (!connectionSuccess) {
    Serial.println("ERROR: Failed to establish HTTPS connection");
    return;
  }
  
  Serial.println("HTTPS connection established successfully");
  
  // Configure HTTP client
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(20000); // 20 second timeout for HTTPS
  http.setReuse(false); // Don't reuse for HTTPS
  
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
  
  int httpCode = http.POST(payload);
  
  if (httpCode > 0) {
    Serial.printf("Server response code: %d\n", httpCode);
    
    if (httpCode >= 200 && httpCode < 300) {
      Serial.println("SUCCESS: Data sent successfully!");
      String response = http.getString();
      if (response.length() > 0) {
        Serial.print("Server response: ");
        Serial.println(response);
      }
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
    Serial.printf("Connection failed, error: %s (code: %d)\n", http.errorToString(httpCode).c_str(), httpCode);
    Serial.println("This may indicate HTTPS/TLS issues");
  }
  
  http.end();
}
#endif

// Callback when data is received
// Note: Older ESP32 framework versions use (mac_addr, data, len) signature
// Newer versions use (recvInfo, data, len) signature
void OnDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
  Serial.printf("\n=== ESP-NOW Packet Received ===\n");
  Serial.printf("From MAC: ");
  for (int i = 0; i < 6; ++i) {
    Serial.printf("%02X", mac_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.printf("\nData length: %d bytes (expected: %d bytes)\n", len, sizeof(sensor_msg));
  
  Station* st = getOrCreateStation(mac_addr);
  if (st && len == sizeof(sensor_msg)) {
    Serial.println("Valid sensor message - processing...");
    st->handleMessage(data, len);
#ifdef ROLE_GATEWAY
    Serial.println("Forwarding to web server...");
    sendToServer(st);
#endif
    Serial.println("=== Packet Processing Complete ===\n");
  } else {
    if (!st) {
      Serial.printf("ERROR: Cannot create station (max stations: %d, current: %d)\n", NUM_STATIONS, stationCount);
    } else {
      Serial.printf("ERROR: Invalid message length (got %d, expected %d)\n", len, sizeof(sensor_msg));
    }
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