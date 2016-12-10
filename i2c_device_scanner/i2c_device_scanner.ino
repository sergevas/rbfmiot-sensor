// Scans the device address connected to I2C bus

#include <ESP8266WiFi.h>
#include <Wire.h>

#define SDA_PIN 14
#define SCL_PIN 12

uint8_t startI2cAddr = 0;
uint8_t endI2cAddr = 127;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Init I2C scanner start...");
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Init I2C scanner complete...");
  Serial.println("I2C devices scanning start...");
  uint8_t i;
  int status;
  for (i = startI2cAddr; i < endI2cAddr; i++) {
    Serial.print("Trying addr 0x");
    Serial.print(i, HEX);
    
    Wire.beginTransmission(i);
    status = Wire.endTransmission();
    if (status == 0) {
      Serial.print("\t <-- Found a device!");
    }
    Serial.println();
  }
  Serial.println("I2C devices scanning complete...");
}

void loop() {
}
