#include<ESP8266WiFi.h>
#include <RBFMIOT_BME280.h>
#include "PubSubClient.h"

extern "C" {
  #include "user_interface.h"
}

const char* ssid = "IoT";
const char* password = "********";
const char *mqttServerName="iot.eclipse.org";
const uint8_t mqttServerPort = 1883;
const char* mqttUsername = "rbfmiotUser";
const char* mqttPassword = "rbfmiotPasswd";
const char* statusOn = "on";
const char* statusOff = "off";
const int willQoS = 2;
const boolean willRetain = true;
char macAddr[12];
char statusTopic[23];
char controlTopic[24];
char telemTopicTemperature[28];
char telemTopicPressure[25];
char telemTopicHumidity[25];

RBFMIOT_BME280 rbfmiotBme280;
WiFiClient wifiClient;
PubSubClient pubSubClient(wifiClient);

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
//  rbfmiotBme280.readId(&id);
//  Serial.print("id 0x");
//  Serial.println(id, HEX);
//  Serial.println("Reading device id complete...");
  readMACaddr(macAddr);
  sprintf(statusTopic, "env/%s/status", macAddr);
  sprintf(statusTopic, "env/%s/control", macAddr);
  sprintf(telemTopicPressure, "env/%s/pressure", macAddr);
  sprintf(telemTopicHumidity, "env/%s/humidity", macAddr);
  sprintf(telemTopicTemperature, "env/%s/temperature", macAddr);
  delay(50);
  Serial.println("Got topic names...");
  Serial.println(statusTopic);
  Serial.println(controlTopic);
  Serial.println(telemTopicTemperature);
  Serial.println(telemTopicPressure);
  Serial.println(telemTopicHumidity);
  pubSubClient.setServer(mqttServerName, mqttServerPort);
//  Serial.println("Configure complete...");
}

void loop() {
  double temp, pres, hum;
  rbfmiotBme280.readAll(&temp, &pres, &hum);
  Serial.print("Publish temp=");
  Serial.print(temp);
  pubSubClient.publish(telemTopicTemperature, );
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

void reconnect() {
  while (!pubSubClient.connected()) {
    Serial.println("Trying to connect to MQTT broker...");
    if (pubSubClient.connect(macAddr, mqttUsername, mqttPassword, statusTopic, willQoS, willRetain, statusOff)) {
      Serial.println("Connected to MQTT broker...");
      pubSubClient.publish(statusTopic, statusOn, true);
      pubSubClient.subscribe(controlTopic, 1);
    } else {
      Serial.print("Unable to connect to MQTT broker! rc=");
      Serial.println(pubSubClient.state());
      Serial.println("Will try again in 5 seconds");
      delay(5000);
    }
  }
}


//TODO: implement this
void suspendSensor() {
}


