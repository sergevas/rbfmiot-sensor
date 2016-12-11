# rbfmiot-sensor

Field IoT sensor related code for the Rule-Based Fault Management for Environmental Monitoring IoT system.

i2c_device_scanner/i2c_device_scanner.ino - ESP8266 Arduino IDE scatch with a simple i2c bus scanner which can be used to detect connected i2c dvices;

BME280 - ESP8266 Arduino lib for BME280 Temperature Humidity Barometric Pressure Digital Sensor;
  
  Functions:
    void config(int _SDApin, int _SCLpin, uint8_t i2cAddress);
	readTemperature();
	readHumidity();
	readPressure();
	
	