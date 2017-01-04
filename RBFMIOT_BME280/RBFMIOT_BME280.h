#ifndef RbfmiotBme280_h
#define RbfmiotBme280_h

#include <Wire.h>

// #define SDA_PIN 14
// #define SCL_PIN 12
#define I2C_ADDR_76 0x76
#define I2C_ADDR_77 0x77
#define ID_ADDR 0xD0
#define CTRL_MEAS_ADDR 0xF4
#define CTRL_HUM_ADDR 0xF2
#define CONFG_ADDR 0xF5
#define PRES_MSB_ADDR 0xF7
#define TEMP_MSB_ADDR 0xFA
#define HUM_MSB_ADDR 0xFD
#define DIG_T1_ADDR 0x88
#define DIG_H1_ADDR 0xA1
#define DIG_H2_ADDR 0xE1
#define MODE_SLEEP 0x00
#define MODE_FORCED 0x01
#define OSRS_H_1 0x01
#define OSRS_P_1 0x01
#define OSRS_T_1 0x01
#define IIR_FILTER_OFF 0x00
#define SPI_OFF 0x00

typedef struct {
  int32_t temp;
  int32_t pres;
  int32_t hum;
} RawData;
	
typedef struct {
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
} TrimmParams;

class RBFMIOT_BME280 {
  public:
	RBFMIOT_BME280();
	RBFMIOT_BME280(int8_t anI2cAddr);
	void configure(int _SDApin, int _SCLpin);
	void readId(int8_t *id);
	void readAll(double *temperature, double *pressure, double *humidity);
  private:
    TrimmParams tp;
    int8_t i2cAddr;
	uint8_t ctrl_meas;
	void write(uint8_t regAddr, uint8_t data);
	void initForcedMode();
	RawData burstRead();
    TrimmParams readTrimmParams();
    int32_t getFineTemp(RawData rd, TrimmParams tp);
    int32_t compensateTemp(int32_t fineTemp);
    uint32_t compensatePressure(RawData rd, TrimmParams tp, int32_t fineTemp);
    uint32_t compensateHumidity(RawData rd, TrimmParams tp, int32_t fineTemp);
	double getTemperature(int32_t compensatedTemp);
    double getPressure(uint32_t compensatedPres);
    double getHumidity(uint32_t compensatedHum);
};
#endif
