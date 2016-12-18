#include <RBFMIOT_BME280.h>

RBFMIOT_BME280 rbfmiotBme280;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Configure start...");
  rbfmiotBme280.configure(SDA_PIN, SCL_PIN);
  Serial.println("Configure complete...");
  Serial.println("Reading device id start...");
  int8_t id;
  rbfmiotBme280.readId(&id);
  Serial.print("id 0x");
  Serial.println(id, HEX);
  Serial.println("Reading device id complete...");
}

void loop() {
  double temp, pres, hum;
  rbfmiotBme280.readAll(&temp, &pres, &hum);
  Serial.print("temp=");
  Serial.print(temp);
  Serial.print("\tpres=");
  Serial.print(pres);
  Serial.print("\thum=");
  Serial.print(hum);
  Serial.println();
  delay(2000);
}
