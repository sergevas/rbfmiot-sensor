#include <ESP8266WiFi.h>
#include <Wire.h>

#define SDA_PIN 14
#define SCL_PIN 12

#define I2C_ADDR 0x76
#define ID_ADDR 0xD0

void configure(int _SDApin, int _SCLpin, uint8_t i2cAddress);
int8_t readId();

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  configure(SDA_PIN, SCL_PIN, I2C_ADDR);
  Serial.println("Reading device id...");
  Serial.print("Device id 0x");
  Serial.println(readId(), HEX);
}

void loop() {
}

void configure(int _SDApin, int _SCLpin, uint8_t i2cAddress) {
  Serial.println("BME280 config start...");
  Serial.print("SDA pin\t");
  Serial.println(_SDApin);
  Serial.print("SCL pin\t");
  Serial.println(_SCLpin);
  Serial.print("I2C address\t0x");
  Serial.println(i2cAddress, HEX);
  
  Wire.begin(_SDApin, _SCLpin);
  
  Serial.println("BME280 config complete...");
}

int8_t readId() {
  int8_t id = -1; // id is unavailable
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(ID_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(I2C_ADDR, 1);
  if (Wire.available()) {
    id = Wire.read();
  }
  Wire.endTransmission();
  return id;
}

