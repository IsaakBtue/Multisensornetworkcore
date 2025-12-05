# ESP-IDF Migration Summary - Complete Project Overview

## Current Project State

### Hardware
- **Gateway**: ESP32-S3 DevKitC 1 N16R8 (4MB Flash, 2MB PSRAM)
- **Stations**: ESP32 (regular)
- **Sensor**: SCD41 (CO2, Temperature, Humidity) via I2C

### Current Framework: Arduino (PlatformIO)
- **Platform**: `espressif32@3.3.4`
- **Board**: `esp32-s3-devkitc1-n4r2`
- **Status**: All functionality works EXCEPT HTTP connection to cloud servers

### Problem
- Railway cloud server blocks embedded device HTTP connections
- Arduino framework 3.3.4 doesn't support `WiFiClientSecure` for ESP32-S3
- Cannot use HTTPS (required for reliable cloud connections)
- HTTPClient fails with "connection refused" after 5 seconds

## Project Architecture

### Communication Flow
```
Sensor Station (ESP32)
    ↓ ESP-NOW (broadcast)
Gateway (ESP32-S3)
    ↓ WiFi HTTP/HTTPS
Cloud Server (Railway/Vercel)
    ↓
Web Dashboard
```

### Key Components

1. **ESP-NOW Communication** (`include/espnow_comm.h`)
   - Stations send sensor data to gateway
   - Gateway receives and forwards to web server
   - Uses broadcast MAC: `FF:FF:FF:FF:FF:FF`
   - RSSI tracking enabled

2. **WiFi Connection** (`src/gateway/main.cpp`)
   - Connects to WiFi network
   - Configures Google DNS (8.8.8.8, 8.8.4.4)
   - DNS resolution testing

3. **HTTP Client** (`include/espnow_comm.h` - `sendToServer()`)
   - Sends JSON POST requests
   - Format: `{mac, device_id, temperature, humidity, co2}`
   - Endpoint: `/api/ingest-http-bridge`
   - Currently uses `HTTPClient` library (fails)

4. **Sensor Reading** (`src/station/main.cpp`)
   - SCD41 sensor via I2C
   - Sensirion Arduino library
   - Calibration support

## File Structure to Migrate

### Source Files
```
src/
├── gateway/main.cpp          # Gateway main code (ESP32-S3)
│   - WiFi connection
│   - ESP-NOW initialization
│   - MAC address reading
│   - DNS configuration
│
├── station/main.cpp          # Sensor station code (ESP32)
│   - Sensor reading (SCD41)
│   - ESP-NOW sending
│   - Calibration logic
│
├── button/main.cpp           # Button mode device
└── calidevice/main.cpp       # Calibration device

include/
├── config.h                  # Configuration
│   - WIFI_SSID, WIFI_PASSWORD
│   - SUPABASE_EDGE_FUNCTION_URL
│   - Measurement intervals
│
├── espnow_comm.h             # ESP-NOW + HTTP
│   - ESP-NOW setup
│   - OnDataRecv callback
│   - OnDataSent callback
│   - sendToServer() function
│   - Station management
│
├── typedef.h                 # Data structures
│   - Station struct
│   - Readings struct
│
└── SDA41_sensor.h            # Sensor wrapper
```

## Critical Code Sections

### 1. ESP-NOW Setup (Arduino)
```cpp
// include/espnow_comm.h
void ESPNOWSetup() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);
  
  // ESP32-S3 specific callback signature:
  void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t* data, int len) {
    // RSSI: recv_info->rx_ctrl->rssi (pointer!)
  }
}
```

### 2. HTTP POST (Arduino - Current, Fails)
```cpp
// include/espnow_comm.h - sendToServer()
HTTPClient http;
WiFiClient client;
http.begin(client, "http://multisensor.up.railway.app/api/ingest-http-bridge");
http.addHeader("Content-Type", "application/json");
int httpCode = http.POST(payload);  // Returns -1 (connection refused)
```

### 3. WiFi Connection (Arduino)
```cpp
// src/gateway/main.cpp
WiFi.mode(WIFI_STA);
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
WiFi.config(localIP, gateway, subnet, DNS1, DNS2);
```

### 4. MAC Address (Arduino)
```cpp
// src/gateway/main.cpp
#if defined(ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S3)
#include <esp_mac.h>
esp_read_mac(gatewayMac, ESP_MAC_WIFI_STA);
#endif
```

## ESP-IDF Migration Requirements

### Framework Change
**From**: `framework = arduino`  
**To**: `framework = espidf`

### Main Entry Point
**From**: `void setup()` / `void loop()`  
**To**: `void app_main(void)` with FreeRTOS tasks

### WiFi API
**From**: `WiFi.begin()`, `WiFi.status()`  
**To**: `esp_wifi_init()`, `esp_wifi_start()`, `esp_wifi_connect()`, event handlers

### ESP-NOW API
**From**: `esp_now_init()`, `esp_now_register_recv_cb()`  
**To**: Same functions but different callback signature for ESP32-S3

### HTTP Client
**From**: `HTTPClient` library (no HTTPS on ESP32-S3)  
**To**: `esp_http_client` with HTTPS support + root CA certificate

### I2C Sensor
**From**: `Wire` library + Sensirion Arduino library  
**To**: `driver/i2c_master.h` + port Sensirion code or use ESP-IDF component

## ESP-IDF Project Structure Needed

```
project/
├── main/
│   ├── CMakeLists.txt
│   ├── gateway_main.c          # Main application
│   ├── config.h                # Configuration
│   └── typedef.h              # Data structures
│
├── components/
│   ├── espnow_comm/
│   │   ├── CMakeLists.txt
│   │   ├── espnow_comm.h
│   │   └── espnow_comm.c       # ESP-NOW + HTTP client
│   │
│   └── scd41_sensor/           # Sensor driver (if needed)
│       ├── CMakeLists.txt
│       └── scd41.c
│
├── CMakeLists.txt
├── sdkconfig
└── platformio.ini              # Updated for ESP-IDF
```

## Key ESP-IDF APIs to Use

### WiFi
```c
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

esp_netif_init();
esp_event_loop_create_default();
esp_netif_create_default_wifi_sta();
esp_wifi_init(&cfg);
esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
esp_wifi_start();
esp_wifi_connect();
```

### ESP-NOW
```c
#include "esp_now.h"
#include "esp_wifi.h"

esp_now_init();
esp_now_register_recv_cb(OnDataRecv);
esp_now_send(peer_addr, data, len, NULL);

// ESP32-S3 callback:
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t* data, int len) {
    int rssi = recv_info->rx_ctrl->rssi;  // Note: pointer!
}
```

### HTTPS Client
```c
#include "esp_http_client.h"

esp_http_client_config_t config = {
    .url = "https://multisensor.up.railway.app/api/ingest-http-bridge",
    .cert_pem = root_ca_pem_start,  // Let's Encrypt root CA
    .timeout_ms = 20000,
};

esp_http_client_handle_t client = esp_http_client_init(&config);
esp_http_client_set_method(client, HTTP_METHOD_POST);
esp_http_client_set_header(client, "Content-Type", "application/json");
esp_http_client_set_post_field(client, payload, strlen(payload));
esp_err_t err = esp_http_client_perform(client);
int status_code = esp_http_client_get_status_code(client);
esp_http_client_cleanup(client);
```

### I2C
```c
#include "driver/i2c_master.h"

i2c_master_bus_handle_t i2c_bus_handle;
i2c_master_bus_config_t i2c_bus_cfg = {
    .i2c_port = I2C_NUM_0,
    .sda_io_num = GPIO_NUM_21,
    .scl_io_num = GPIO_NUM_22,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags = {
        .enable_internal_pullup = true,
    },
};
i2c_master_bus_handle_create(&i2c_bus_cfg, &i2c_bus_handle);
```

## Configuration Values

### WiFi (from `include/config.h`)
```cpp
#define WIFI_SSID      "Odido-20E8A1"
#define WIFI_PASSWORD  "N3GMVKDJQA9EYQRF"
```

### Server URL (needs HTTPS after migration)
```cpp
// Current (HTTP, doesn't work):
#define SUPABASE_EDGE_FUNCTION_URL "http://multisensor.up.railway.app/api/ingest-http-bridge"

// After migration (HTTPS, will work):
#define SUPABASE_EDGE_FUNCTION_URL "https://multisensor.up.railway.app/api/ingest-http-bridge"
```

### Other Config
```cpp
#define MEASUREMENT_INTERVAL 10  // seconds
#define NUM_STATIONS 10
#define CALI_DURATION 2  // hours
```

## Data Structures (from `include/typedef.h`)

```cpp
struct Readings {
    float temperature;
    float humidity;
    uint16_t co2;
};

struct Station {
    uint8_t mac[6];
    int rssi;
    Readings readings;
    unsigned long lastSeen;
};
```

## Migration Steps (Priority Order)

### Phase 1: Project Setup
1. Create ESP-IDF project structure
2. Update `platformio.ini` to use `framework = espidf`
3. Create `CMakeLists.txt` files
4. Set up `sdkconfig` for ESP32-S3

### Phase 2: Core Infrastructure
1. Migrate WiFi connection code
2. Implement event handlers for WiFi
3. Add DNS configuration
4. Test WiFi connectivity

### Phase 3: ESP-NOW
1. Migrate ESP-NOW initialization
2. Update callback signatures for ESP32-S3
3. Fix RSSI reading (pointer vs struct)
4. Test ESP-NOW communication

### Phase 4: HTTPS Client
1. Implement `esp_http_client` with HTTPS
2. Add Let's Encrypt root CA certificate
3. Implement JSON POST request
4. Test connection to Railway

### Phase 5: Sensor Integration
1. Port I2C code to ESP-IDF driver
2. Port Sensirion library or find ESP-IDF version
3. Test sensor reading

### Phase 6: Integration & Testing
1. Combine all components
2. Test end-to-end: Station → Gateway → Server
3. Verify data on web dashboard
4. Performance optimization

## Reference Code

### HTTPS Example
Location: `FreeRTOS-ESP-IDF-HTTPS-Server-main/main.c`
- Shows ESP-IDF project structure
- Shows HTTPS server setup (adapt for client)
- Shows certificate embedding
- Shows FreeRTOS task usage

### ESP-IDF Documentation
- **WiFi**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_wifi.html
- **ESP-NOW**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/network/esp_now.html
- **HTTP Client**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/protocols/esp_http_client.html
- **I2C Master**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/i2c_master.html

## Critical Differences: Arduino vs ESP-IDF

| Feature | Arduino | ESP-IDF |
|---------|---------|---------|
| Entry Point | `setup()`, `loop()` | `app_main()` + FreeRTOS tasks |
| Language | C++ | C (can use C++ but C is standard) |
| WiFi | `WiFi.begin()` | `esp_wifi_init()`, event handlers |
| HTTP | `HTTPClient` (no HTTPS on S3) | `esp_http_client` (HTTPS supported) |
| ESP-NOW | Same API, different callback | Same API, different callback |
| I2C | `Wire` library | `driver/i2c_master.h` |
| Memory | Automatic | Manual management |
| Configuration | `#define` in headers | `sdkconfig` + menuconfig |

## Known Issues to Address

1. **ESP32-S3 ESP-NOW Callback**
   - RSSI is pointer: `recv_info->rx_ctrl->rssi` (not `recv_info->rx_ctrl.rssi`)
   - Already handled in current Arduino code

2. **Root CA Certificate**
   - Need Let's Encrypt ISRG Root X1 certificate
   - Embed as string in code or as binary resource

3. **FreeRTOS Tasks**
   - Need separate tasks for:
     - WiFi management
     - ESP-NOW receiving
     - HTTP sending (can be blocking)
     - Sensor reading (for stations)

4. **Memory Management**
   - ESP-IDF requires explicit heap management
   - Watch for memory leaks in HTTP client

## Success Criteria

After migration:
- ✅ ESP32-S3 connects to WiFi
- ✅ ESP-NOW communication works (stations → gateway)
- ✅ Gateway sends HTTPS POST to Railway successfully
- ✅ Data appears on web server dashboard
- ✅ No "connection refused" errors
- ✅ Stable, reliable operation

## Estimated Timeline

- **Setup & WiFi**: 4-6 hours
- **ESP-NOW**: 4-6 hours
- **HTTPS Client**: 6-8 hours
- **Sensor Porting**: 4-6 hours
- **Integration & Testing**: 4-6 hours
- **Total**: 22-32 hours (3-4 days for experienced developer)

## Next AI Model Instructions

1. **Start with project structure**: Create ESP-IDF project with proper CMakeLists.txt
2. **Migrate WiFi first**: Simplest component, good starting point
3. **Add ESP-NOW**: Build on WiFi foundation
4. **Implement HTTPS client**: Critical for solving the connection problem
5. **Port sensor code last**: Can use Arduino version temporarily if needed
6. **Test incrementally**: Don't migrate everything at once

## Important Files to Reference

- `include/espnow_comm.h` - Contains all ESP-NOW and HTTP logic
- `src/gateway/main.cpp` - Gateway main code
- `include/config.h` - All configuration values
- `include/typedef.h` - Data structures
- `FreeRTOS-ESP-IDF-HTTPS-Server-main/main.c` - ESP-IDF example
- `platformio.ini` - Current PlatformIO config

## Notes

- Keep Arduino version as backup until ESP-IDF is fully tested
- ESP-IDF uses event-driven architecture (not polling like Arduino loop)
- FreeRTOS tasks provide better concurrency
- HTTPS will solve the Railway connection problem
- More control but more complexity than Arduino
