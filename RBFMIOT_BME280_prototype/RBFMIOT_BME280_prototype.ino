#include <ESP8266WiFi.h>
#include <Wire.h>

#define SDA_PIN 14
#define SCL_PIN 12

#define I2C_ADDR 0x76
#define ID_ADDR 0xD0
#define CTRL_MEAS_ADDR 0xF4
#define CTRL_HUM_ADDR 0xF2
#define CONFG_ADDR 0xF5
#define PRES_MSB_ADDR 0xF7
#define TEMP_MSB_ADDR 0xFA
#define HUM_MSB_ADDR 0xFD
#define DIG_T1_ADDR 0x88
#define DIG_T2_ADDR 0x8A
#define DIG_T3_ADDR 0x8C
#define DIG_P1_ADDR 0x8E
#define DIG_P2_ADDR 0x90
#define DIG_P3_ADDR 0x92
#define DIG_P4_ADDR 0x94
#define DIG_P5_ADDR 0x96
#define DIG_P6_ADDR 0x98
#define DIG_P7_ADDR 0x9A
#define DIG_P8_ADDR 0x9C
#define DIG_P9_ADDR 0x9E
#define DIG_H1_ADDR 0xA1
#define DIG_H2_ADDR 0xE1
#define DIG_H3_ADDR 0xE3
#define DIG_H4_ADDR 0xE4
#define DIG_H5_ADDR 0xE5
#define DIG_H6_ADDR 0xE7

#define MODE_FORCED 0x01
#define OSRS_H_1 0x01
#define OSRS_P_1 0x01
#define OSRS_T_1 0x01
#define IIR_FILTER_OFF 0x00
#define SPI_OFF 0x00

typedef signed long BME280_S32_t;

struct RawData {
  uint32_t temp;
  uint32_t pres;
  uint32_t hum;
};

void configure(int _SDApin, int _SCLpin, uint8_t i2cAddress);
void initForcedMode();
void write(uint8_t regAddr, uint8_t data);
int8_t readId();
RawData burstRead();

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
  RawData rawData;
  Serial.println("Reading raw data...");
  initForcedMode();
  rawData = burstRead();
  Serial.print("raw temp 0x");
  Serial.println(rawData.temp, HEX);
  Serial.print("raw pres 0x");
  Serial.println(rawData.pres, HEX);
  Serial.print("raw hum 0x");
  Serial.println(rawData.hum, HEX);
  delay(2000);
}

void configure(int _SDApin, int _SCLpin, uint8_t i2cAddress) {
 
  Serial.println("BME280 config start...");
  Serial.print("SDA pin ");
  Serial.println(_SDApin);
  Serial.print("SCL pin ");
  Serial.println(_SCLpin);
  Serial.print("I2C address 0x");
  Serial.println(i2cAddress, HEX);
  Wire.begin(_SDApin, _SCLpin);
  Serial.println("BME280 config complete...");
}

void initForcedMode() {
   uint8_t confg;
  uint8_t ctrl_meas;
  uint8_t ctrl_hum;
  confg = IIR_FILTER_OFF << 2 | SPI_OFF;
//  Serial.print("confg 0x");
//  Serial.println(confg, HEX);
  ctrl_meas = OSRS_T_1 << 5 | OSRS_P_1 << 2 | MODE_FORCED;
//  Serial.print("ctrl_meas 0x");
//  Serial.println(ctrl_meas, HEX);
  ctrl_hum = OSRS_H_1;
//  Serial.print("ctrl_hum 0x");
//  Serial.println(ctrl_hum, HEX);
  write(CONFG_ADDR, confg);
  write(CTRL_HUM_ADDR, ctrl_hum);
  write(CTRL_MEAS_ADDR, ctrl_meas);
}

void write(uint8_t regAddr, uint8_t data) {
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(regAddr);
  Wire.write(data);
  Wire.endTransmission();
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

RawData burstRead() {
  RawData rawData;
  uint32_t rawDataArray[8];
  int rawDataArrayCount = 0;
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(PRES_MSB_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(I2C_ADDR, 8);
  while (Wire.available()) {
   rawDataArray[rawDataArrayCount] = Wire.read();
   rawDataArrayCount++;
  }
  Wire.endTransmission();
  rawData.pres = rawDataArray[0] << 12 | rawDataArray[1] << 4 | rawDataArray[2] >> 4;
  rawData.temp = rawDataArray[3] << 12 | rawDataArray[4] << 4 | rawDataArray[5] >> 4;
  rawData.hum = rawDataArray[6] << 8 | rawDataArray[7];
  return rawData;
}

