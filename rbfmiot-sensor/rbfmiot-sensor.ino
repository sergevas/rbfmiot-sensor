#include<ESP8266WiFi.h>
#include <RBFMIOT_BME280.h>

RBFMIOT_BME280 rbfmiotBme280;

char macAddr[12];

void setup() {
//  int8_t id;
  
  Serial.begin(115200);
  Serial.println();
  Serial.println();
//  Serial.println("Configure start...");
  rbfmiotBme280.configure(SDA_PIN, SCL_PIN);
//  Serial.println("Configure complete...");
//  rbfmiotBme280.readId(&id);
//  Serial.print("id 0x");
//  Serial.println(id, HEX);
//  Serial.println("Reading device id complete...");
  readMACaddr(macAddr);
  Serial.println(macAddr);
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

void readMACaddr(char *macAddr) {
  int i;
  uint8_t macBin[6];
  WiFi.macAddress(macBin);
  for (i = 0; i < sizeof(macBin); i++) {
    sprintf(macAddr,"%s%02x", macAddr, macBin[i]);
  }
}
