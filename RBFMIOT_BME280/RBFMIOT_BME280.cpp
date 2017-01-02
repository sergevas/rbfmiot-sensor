#include "RBFMIOT_BME280.h"

RBFMIOT_BME280::RBFMIOT_BME280() {
  i2cAddr = I2C_ADDR_76;
}

RBFMIOT_BME280::RBFMIOT_BME280(int8_t anI2cAddr) {
  i2cAddr = anI2cAddr;
}

void RBFMIOT_BME280::configure(int _SDApin, int _SCLpin) {
  Wire.begin(_SDApin, _SCLpin);
}

void RBFMIOT_BME280::write(uint8_t regAddr, uint8_t data) {
  Wire.beginTransmission(i2cAddr);
  Wire.write(regAddr);
  Wire.write(data);
  Wire.endTransmission();
}

void RBFMIOT_BME280::initForcedMode() {
  uint8_t confg;
  uint8_t ctrl_meas;
  uint8_t ctrl_hum;
  confg = IIR_FILTER_OFF << 2 | SPI_OFF;
  ctrl_meas = OSRS_T_1 << 5 | OSRS_P_1 << 2 | MODE_FORCED;
  ctrl_hum = OSRS_H_1;
  write(CONFG_ADDR, confg);
  write(CTRL_HUM_ADDR, ctrl_hum);
  write(CTRL_MEAS_ADDR, ctrl_meas);
}

void RBFMIOT_BME280::readId(int8_t *id) {
  *id = -1; // id is unavailable
  Wire.beginTransmission(i2cAddr);
  Wire.write(ID_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddr, 1);
  if (Wire.available()) {
    *id = Wire.read();
  }
  Wire.endTransmission();
}

TrimmParams RBFMIOT_BME280::readTrimmParams() {
  TrimmParams trimmParams;
  uint8_t trimmParamsArray[32];
  int trimmParamsArrayCount = 0;
  Wire.beginTransmission(i2cAddr);
  Wire.write(DIG_T1_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddr, 24);
  while (Wire.available()) {
   trimmParamsArray[trimmParamsArrayCount] = Wire.read();
   trimmParamsArrayCount++;
  }
  Wire.endTransmission();
  Wire.beginTransmission(i2cAddr);
  Wire.write(DIG_H1_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddr, 1);
  trimmParamsArray[trimmParamsArrayCount] = Wire.read();
  trimmParamsArrayCount++;
  Wire.endTransmission();
  Wire.beginTransmission(i2cAddr);
  Wire.write(DIG_H2_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddr, 7);
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

RawData RBFMIOT_BME280::burstRead() { 
  RawData rawData;
  uint8_t rawDataArray[8];
  int rawDataArrayCount = 0;
  Wire.beginTransmission(i2cAddr);
  Wire.write(PRES_MSB_ADDR);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddr, 8);
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

int32_t RBFMIOT_BME280::getFineTemp(RawData rd, TrimmParams tp) {
  int32_t var1, var2, fineTemp;
  var1 = ((((rd.temp >> 3) - ((int32_t)tp.digT1 << 1))) * ((int32_t)tp.digT2)) >> 11;
  var2 = (((((rd.temp >> 4) - ((int32_t)tp.digT1)) * ((rd.temp >> 4) - ((int32_t)tp.digT1))) >> 12) * ((int32_t)tp.digT3)) >> 14;
  fineTemp = var1 + var2;
  return fineTemp;
}
// Returns temperature in DegC, resolution is 0.01 DegC. Output value of "5123" equals 51.23 DegC.
// t_fine carries fine temperature as global value
int32_t RBFMIOT_BME280::compensateTemp(int32_t fineTemp) {
  return (fineTemp * 5 + 128) >> 8;
}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of "24674867" represents 24674867/256 = 96386.2 Pa = 963.862 hPa
uint32_t RBFMIOT_BME280::compensatePressure(RawData rd, TrimmParams tp, int32_t fineTemp) {
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
uint32_t RBFMIOT_BME280::compensateHumidity(RawData rd, TrimmParams tp, int32_t fineTemp) {
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

double RBFMIOT_BME280::getTemperature(int32_t inTemp) {
  return (double)inTemp / 100.0;
}

double RBFMIOT_BME280::getPressure(uint32_t inPres) {
  return (double)inPres / 256.0;
}

double RBFMIOT_BME280::getHumidity(uint32_t inHum) {
  return (double)inHum / 1024.0;
}

void RBFMIOT_BME280::readAll(double *temperature, double *pressure, double *humidity) {
  RawData rd;
  TrimmParams tp;
  int32_t fineTemp, compensatedTemp;
  uint32_t compensatedPres, compensatedHum;
  initForcedMode();
  rd = burstRead();
  tp = readTrimmParams();
  fineTemp = getFineTemp(rd, tp);
  *temperature = getTemperature(compensateTemp(fineTemp));
  *pressure = getPressure(compensatePressure(rd, tp, fineTemp));
  *humidity = getHumidity(compensateHumidity(rd, tp, fineTemp));
}