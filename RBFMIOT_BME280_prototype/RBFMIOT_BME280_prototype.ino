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

typedef signed long BME280_S16_t;

struct RawData {
  int32_t temp;
  int32_t pres;
  int32_t hum;
};

struct TrimmParams {
  uint16_t digT1;
  int16_t digT2;
  int16_t digT3;
  uint16_t digP1;
  int16_t digP2;
  int16_t digP3;
  int16_t digP4;
  int16_t digP5;
  int16_t digP6;
  int16_t digP7;
  int16_t digP8;
  int16_t digP9;
  uint8_t digH1;
  int16_t digH2;
  uint8_t digH3;
  int16_t digH4;
  int16_t digH5;
  int8_t digH6;
}; 

void configure(int _SDApin, int _SCLpin, uint8_t i2cAddress);
void initForcedMode();
void write(uint8_t regAddr, uint8_t data);
int8_t readId();
RawData burstRead();
TrimmParams readTrimmParams();
int32_t getFineTemp(RawData rd, TrimmParams tp);
int32_t compensateTemp(int32_t fineTemp);
uint32_t compensatePressure(RawData rd, TrimmParams tp, int32_t fineTemp);
uint32_t compensateHumidity(RawData rd, TrimmParams tp, int32_t fineTemp);
double getTemperature(int32_t inTemp);
double getPressure(uint32_t inPres);
double getHumidity(uint32_t inHum);

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
  RawData rd;
  TrimmParams tp;
  int32_t fineTemp;
  double temp, pres, hum;
  initForcedMode();
  rd = burstRead();
  tp = readTrimmParams();
  fineTemp = getFineTemp(rd, tp);
  temp = getTemperature(compensateTemp(fineTemp));
  pres = getPressure(compensatePressure(rd, tp, fineTemp));
  hum = getHumidity(compensateHumidity(rd, tp, fineTemp));
  Serial.print("temp=");
  Serial.print(temp);
  Serial.print("\tpres=");
  Serial.print(pres);
  Serial.print("\thum=");
  Serial.print(hum);
  Serial.println();
  
//  Serial.print("raw temp 0x");
//  Serial.println(rawData.temp, HEX);
//  Serial.print("raw pres 0x");
//  Serial.println(rawData.pres, HEX);
//  Serial.print("raw hum 0x");
//  Serial.println(rawData.hum, HEX);
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
  uint8_t rawDataArray[8];
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

TrimmParams readTrimmParams() {
  TrimmParams trimmParams;
  uint8_t trimmParamsArray[32];
  int trimmParamsArrayCount = 0;
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(DIG_T1_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(I2C_ADDR, 24);
  while (Wire.available()) {
   trimmParamsArray[trimmParamsArrayCount] = Wire.read();
   trimmParamsArrayCount++;
  }
  Wire.endTransmission();
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(DIG_H1_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(I2C_ADDR, 1);
  trimmParamsArray[trimmParamsArrayCount] = Wire.read();
  trimmParamsArrayCount++;
  Wire.endTransmission();
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(DIG_H2_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(I2C_ADDR, 7);
  while (Wire.available()) {
   trimmParamsArray[trimmParamsArrayCount] = Wire.read();
   trimmParamsArrayCount++;
  }
  Wire.endTransmission();
  trimmParams.digT1 = trimmParamsArray[1] << 8 | trimmParamsArray[0];
  trimmParams.digT2 = trimmParamsArray[3] << 8 | trimmParamsArray[2];
  trimmParams.digT3 = trimmParamsArray[5] << 8 | trimmParamsArray[4];
  trimmParams.digP1 = trimmParamsArray[7] << 8 | trimmParamsArray[6];
  trimmParams.digP2 = trimmParamsArray[9] << 8 | trimmParamsArray[8];
  trimmParams.digP3 = trimmParamsArray[11] << 8 | trimmParamsArray[10];
  trimmParams.digP4 = trimmParamsArray[13] << 8 | trimmParamsArray[12];
  trimmParams.digP5 = trimmParamsArray[15] << 8 | trimmParamsArray[14];
  trimmParams.digP6 = trimmParamsArray[17] << 8 | trimmParamsArray[16];
  trimmParams.digP7 = trimmParamsArray[19] << 8 | trimmParamsArray[18];
  trimmParams.digP8 = trimmParamsArray[21] << 8 | trimmParamsArray[20];
  trimmParams.digP9 = trimmParamsArray[23] << 8 | trimmParamsArray[22];
  trimmParams.digH1 = trimmParamsArray[24];
  trimmParams.digH2 = trimmParamsArray[26] << 8 | trimmParamsArray[25];
  trimmParams.digH3 = trimmParamsArray[27];
  trimmParams.digH4 = trimmParamsArray[28] << 4 | 0x0F & trimmParamsArray[29];
  trimmParams.digH5 = trimmParamsArray[30] << 4 | 0x0F & (trimmParamsArray[29] >> 4);
  trimmParams.digH6 = trimmParamsArray[31];
  return trimmParams;
}

int32_t getFineTemp(RawData rd, TrimmParams tp) {
  int32_t var1, var2, fineTemp;
  var1 = ((((rd.temp >> 3) - ((int32_t)tp.digT1 << 1))) * ((int32_t)tp.digT2)) >> 11;
  var2 = (((((rd.temp >> 4) - ((int32_t)tp.digT1)) * ((rd.temp >> 4) - ((int32_t)tp.digT1))) >> 12) * ((int32_t)tp.digT3)) >> 14;
  fineTemp = var1 + var2;
  return fineTemp;
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of "5123" equals 51.23 DegC.
// t_fine carries fine temperature as global value
int32_t compensateTemp(int32_t fineTemp) {
  return (fineTemp * 5 + 128) >> 8;
}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of "24674867" represents 24674867/256 = 96386.2 Pa = 963.862 hPa
uint32_t compensatePressure(RawData rd, TrimmParams tp, int32_t fineTemp) {
  int64_t var1, var2, pres;
  var1 = ((int64_t)fineTemp) - 128000;
  var2 = var1 * var1 * (int64_t)tp.digP6;
  var2 = var2 + ((var1 * (int64_t)tp.digP5) << 17);
  var2 = var2 + (((int64_t)tp.digP4) << 35);
  var1 = ((var1 * var1 * (int64_t)tp.digP3) >> 8) + ((var1 * (int64_t)tp.digP2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)tp.digP1) >> 33;
  if (var1 == 0) {
    return 0; // avoid exception caused by division by zero
  }
  pres = 1048576 - rd.pres;
  pres = (((pres << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)tp.digP9) * (pres >> 13) * (pres >> 13)) >> 25;
  var2 = (((int64_t)tp.digP8) * pres) >> 19;
  pres = ((pres + var1 + var2) >> 8) + (((int64_t)tp.digP7) << 4);
  return (uint32_t)pres;
}

// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
// Output value of "47445" represents 47445/1024 = 46.333 %RH
uint32_t compensateHumidity(RawData rd, TrimmParams tp, int32_t fineTemp) {
  int32_t v_x1_u32r;
  v_x1_u32r = (fineTemp - ((int32_t)76800));
  v_x1_u32r = (((((rd.hum << 14) - (((int32_t)tp.digH4) << 20) - (((int32_t)tp.digH5) * v_x1_u32r))
  + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)tp.digH6)) >> 10) * (((v_x1_u32r *((int32_t)tp.digH3)) >> 11)
  + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)tp.digH2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)tp.digH1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
  v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
  return (uint32_t) (v_x1_u32r >> 12);
}

double getTemperature(int32_t inTemp) {
  return (double)inTemp / 100.0;
}

double getPressure(uint32_t inPres) {
  return (double)inPres / 256.0;
}

double getHumidity(uint32_t inHum) {
  return (double)inHum / 1024.0;
}

