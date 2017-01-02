#include<ESP8266WiFi.h>
#include <RBFMIOT_BME280.h>
#include "PubSubClient.h"

extern "C" {
  #include "user_interface.h"
}

const char* ssid = "IoT";
const char* password = "********";

const char *mqttServerName="192.168.1.80";
const uint16_t mqttServerPort = 1883;
const char* mqttUsername = "rbfmiotUser";
const char* mqttPassword = "rbfmiotPasswd";

const char* statusOn = "on";
const char* statusSuspend = "suspend";
const char* statusOff = "off";

const String getidCmd = "getid";
const String activateCmd = "activate";
const String suspendCmd = "suspend";

const int willQoS = 2;
const boolean willRetain = true;
char macAddr[12];
char statusTopic[24];
char requestTopic[25];
char replyTopic[23];
char telemTopicTemperature[29];
char telemTopicPressure[26];
char telemTopicHumidity[26];
double temp, pres, hum;
//byte measData[8];

RBFMIOT_BME280 rbfmiotBme280;
WiFiClient wifiClient;
PubSubClient pubSubClient(mqttServerName, mqttServerPort, wifiClient);

String readMACaddr();
void double2byteArr(double srcVal, byte *destVal);
void initWiFi();
void reconnect();
void callback(char *topic, byte *payload, unsigned int length);
void handleCommand(char *command);

void setup() {
  String macAddrStr = readMACaddr();
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Configure start...");
  rbfmiotBme280.configure(SDA_PIN, SCL_PIN);
  initWiFi();
  macAddrStr.toCharArray(macAddr, 12);
  (String("env/" + macAddrStr + "/pressure")).toCharArray(telemTopicPressure, 26);
  (String("env/" + macAddrStr + "/humidity")).toCharArray(telemTopicHumidity, 26);
  (String("env/" + macAddrStr + "/temperature")).toCharArray(telemTopicTemperature, 29);
  (String("env/" + macAddrStr + "/request")).toCharArray(requestTopic, 25);
  (String("env/" + macAddrStr + "/reply")).toCharArray(replyTopic, 23);
  (String("env/" + macAddrStr + "/status")).toCharArray(statusTopic, 24);
  Serial.println("Got topic names...");
  Serial.println(statusTopic);
  Serial.println(requestTopic);
  Serial.println(replyTopic);
  Serial.println(telemTopicTemperature);
  Serial.println(telemTopicPressure);
  Serial.println(telemTopicHumidity);
  pubSubClient.setCallback(callback);
  Serial.println("Configure complete...");
}

void loop() {
  char *measVal;
  if (!pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();
  rbfmiotBme280.readAll(&temp, &pres, &hum);
  Serial.print("Publish temp=");
  Serial.print(temp);
  measVal = getValAsString(temp);
  pubSubClient.publish(telemTopicTemperature, measVal);
  free(measVal);
  Serial.print("\tpres=");
  Serial.print(pres);
  measVal = getValAsString(pres);
  pubSubClient.publish(telemTopicPressure, measVal);
  free(measVal);
  Serial.print("\thum=");
  Serial.print(hum);
  measVal = getValAsString(hum);
  pubSubClient.publish(telemTopicHumidity, measVal);
  free(measVal);
  Serial.println();
  delay(2000);
}

char *getValAsString(double aVal) {
  String strVal = String(aVal, 2);
  int strValLength = strVal.length() + 1;
  char *strValArr = (char*)malloc(strValLength);
  strVal.toCharArray(strValArr, strValLength);
  return strValArr;
}

String readMACaddr() {
  int i;
  uint8_t macBin[6];
  String macStr = "";
  WiFi.macAddress(macBin);
  for (i = 0; i < 6; i++) {
    macStr += String(macBin[i], HEX);
  }
  return macStr;
}

void initWiFi() {
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
      pubSubClient.subscribe(requestTopic, 1);
    } else {
      Serial.print("Unable to connect to MQTT broker! rc=");
      Serial.println(pubSubClient.state());
      Serial.println("Will try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  int i;
  String command = "";
  for (i = 0; i < length; i++) {
    command += String((char)payload[i]);
  }
  handleCommand(command);
}

void handleCommand(String command) {
  Serial.print("command=");
  Serial.println(command);
  if (getidCmd.equals(command)) {
    int8_t id;
    char idArr[3];
    rbfmiotBme280.readId(&id);
    String idStr = String(id, HEX);
    idStr.toCharArray(idArr, 3);
    pubSubClient.publish(replyTopic, idArr);
  }
}

//TODO: implement this
void suspendSensor() {
}

