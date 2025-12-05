
// Required standard libarries
#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cScd4x.h>      // https://github.com/Sensirion/arduino-i2c-scd4x/blob/master/src/SensirionI2cScd4x.h -- Thank you Sensirion!
#include "typedef.h"

// SCD41 standards
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0
// I2C pins for TTGO T-Energy (ESP32 WROVER-B)
// Sensor wiring:
// VIN/VCC -> 3.3V
// GND     -> GND
// SDA     -> GPIO 26
// SCL     -> GPIO 25
#define I2C_SDA_PIN 26
#define I2C_SCL_PIN 25

// Sensor class
SensirionI2cScd4x sensor;

static char errorMessage[64];
static int16_t error;
    // Message struct to hold sensor data
sensor_msg msg;

uint16_t co2Concentration = 0;
float temperature = 0.0;
float relativeHumidity = 0.0;

void initSensor(){
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, INPUT_PULLUP);
    Wire.begin(I2C_SDA_PIN,I2C_SCL_PIN);
    sensor.begin(Wire, SCD41_I2C_ADDR_62);
    delay(500);

    // Wake up the sensor (if in low power mode)
    error = sensor.wakeUp();
    if (error != NO_ERROR) {
        Serial.print("Error trying to execute wakeUp(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }

}

sensor_msg readSDA41() {
    // Read the measured data from the sensor
    error = sensor.readMeasurement(co2Concentration, temperature,
                                            relativeHumidity);
    if (error != NO_ERROR) {
        Serial.print("Error trying to execute measureMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }

    error = sensor.stopPeriodicMeasurement();
    if (error != NO_ERROR) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }

    Serial.print("CO2 concentration [ppm]: ");
    Serial.print(co2Concentration);
    Serial.println();
    Serial.print("Temperature [°C]: ");
    Serial.print(temperature);
    Serial.println();
    Serial.print("Relative Humidity [RH]: ");
    Serial.print(relativeHumidity);

    // Fill the message struct and return this
    msg.co2 = co2Concentration; 
    msg.temperature = temperature;
    msg.humidity = relativeHumidity;

    return msg; // return the message with sensor data
}
