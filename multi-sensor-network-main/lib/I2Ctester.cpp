#include <Arduino.h>
#include <Wire.h>

// Define the custom I2C pins for your ESP32-C6
#define I2C_SDA_PIN 22
#define I2C_SCL_PIN 21

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial Monitor to connect

  Serial.println("\nI2C Scanner Initializing...");
  
  // Set the custom pins as INPUT_PULLUP (Internal pull-ups are enabled here)
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, INPUT_PULLUP);

  // Start the I2C bus on the custom pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); 

  Serial.print("I2C Bus Initialized on SDA (GPIO ");
  Serial.print(I2C_SDA_PIN);
  Serial.print(") and SCL (GPIO ");
  Serial.print(I2C_SCL_PIN);
  Serial.println(").");
  Serial.println("Scanning...");
}

void loop() {
  byte error, address;
  int nDevices;

  Serial.println("---------------------------");
  Serial.println("Starting Scan...");
  nDevices = 0;
  
  // Check addresses from 1 to 127 (0 and 127 are reserved)
  for (address = 1; address < 127; address++) {
    // Start transmission to the address
    Wire.beginTransmission(address);
    // End transmission and check for errors
    error = Wire.endTransmission(); 

    if (error == 0) {
      // Error code 0 means success (device ACKed the address)
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      // Error code 4 means "Unknown error" which sometimes indicates a problem
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  
  Serial.println("---------------------------");

  if (nDevices == 0) {
    Serial.println("No I2C devices found. Check wiring, power, and pull-ups!");
  } else {
    Serial.print("Scan Complete. Found ");
    Serial.print(nDevices);
    Serial.println(" device(s).");
  }

  // Run the scan every 5 seconds
  delay(5000); 
}