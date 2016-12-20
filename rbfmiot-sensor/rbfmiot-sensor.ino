#include<ESP8266WiFi.h>
#include <RBFMIOT_BME280.h>
#include "PubSubClient.h"

extern "C" {
  #include "user_interface.h"
}

const char* ssid = "IoT";
//const char* password = "********";

const char *mqttServerName="";
const uint8_t mqttServerPort = 1883;
char macAddr[12];

RBFMIOT_BME280 rbfmiotBme280;

void readMACaddr(char *macAddr);
void initWiFi();

void setup() {
//  int8_t id;
  
  Serial.begin(115200);
  Serial.println();
  Serial.println();
//  Serial.println("Configure start...");
  rbfmiotBme280.configure(SDA_PIN, SCL_PIN);
  initWiFi();
//  Serial.println("Configure complete...");
//  rbfmiotBme280.readId(&id);
//  Serial.print("id 0x");
//  Serial.println(id, HEX);
//  Serial.println("Reading device id complete...");
  readMACaddr(macAddr);
//  Serial.println(macAddr);
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

void initWiFi() {
  delay(10);
  Serial.println();
  Serial.println("WiFi connect start...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected. IP address ");
  Serial.println(WiFi.localIP());
  Serial.println("WiFi connect complete...");
}

//TODO: implement this
void suspendSensor() {
}


