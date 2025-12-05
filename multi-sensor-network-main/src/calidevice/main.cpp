/*
    This is the code supposed to run on the verification sensor.
    The ESP for this testing purpose must therefore always run as a powered device, not from battery power
    It will consume significantly more battery.

    To do; Copy code that allows it to communicate every so often over ESPnow, continuous measurement
*/



// Includesclear
#include <Arduino.h>
#include "SDA41_sensor.h" 
#include "esp32-hal-gpio.h" // Needed for the specific low-level GPIO functions
#include <driver/rtc_io.h>  
#include <esp_sleep.h> 
#include "config.h"  // Where information is stored about constants, e.g. fan duration etc.
#include "espnow_comm.h" // Header files for esp now communication

// Run once (init)
void setup() {

    // Anything calibration required is preset by device itself, by default.
  Serial.begin(115200);
     // Initialize stuff
  ESPNOWSetup();
  initSensor();
  addBroadcastPeer();
}


// Run continuously
void loop(){

    // Start measurement loop

    Serial.println("Starting measurement");
    error = sensor.startPeriodicMeasurement(); // Start the measurement process (non blocking)
        
    if (error != NO_ERROR) {
        Serial.print("Error trying to execute periodic measurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }   

    // Measurement pause
    delay(5000); //ms afaik - now 5 sec

    // Start transmission step
    Serial.println("Reading value and sending");
    msg = readSDA41(); // This uses sensor.readMeasurement()
    esp_now_send(broadcastAddr, (uint8_t *)&msg, sizeof(msg));

    // Wait for send to complete
    while (!send_done) {
        delay(1);
    }

    error = sensor.stopPeriodicMeasurement();
    if (error != NO_ERROR) {
        Serial.print("Error trying to stop periodic measurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }
}


        


