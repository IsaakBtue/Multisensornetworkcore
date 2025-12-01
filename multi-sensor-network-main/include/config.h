#include <Arduino.h>
#pragma once


#define MEASUREMENT_INTERVAL 1  // Interval between measurements in seconds (1 second for testing)

#define LED_PIN GPIO_NUM_2      // GPIO pin for onboard indicator LED

#define FAN_PIN 20              // Pin where fan is connected to (only reqyured if fan is used)

#define FAN_DURATION 3          // Duration to run the fan in seconds (only relevant if fan is used). 1-5 seconds should be sufficient to clear stale air

#define NUM_STATIONS 10         // Number of stations objects that can be "connected" to gateway

#define CALI_DURATION 2         // Hours that it will take for the device to recalibrate itself - default = 4

#define CALI_PERIOD (CALI_DURATION*(3600/MEASUREMENT_INTERVAL))        // Amount of cycles between calibrations - counts from 1

bool useFan = false;             // Set to true if fan is used, false otherwise

bool useOnboardLED = true;      // Set to true if onboard LED is used, false otherwise (save power if not used)

bool needCalibration = true;      // set to true by default, turned into false if not required.

// Wi-Fi & server configuration for the gateway
// NOTE: Only the gateway uses these; stations ignore them.
// Fill these in with your own network and server details.
#define WIFI_SSID      "Odido-20E8A1"
#define WIFI_PASSWORD  "N3GMVKDJQA9EYQRF"
// Vercel web server endpoint for sensor data ingestion (HTTPS required)
#define WEB_SERVER_URL "https://multisensornetwork.vercel.app/api/ingest"