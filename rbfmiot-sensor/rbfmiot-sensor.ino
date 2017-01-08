#include<ESP8266WiFi.h>
#include <EEPROM.h>
#include <RBFMIOT_BME280.h>
#include "PubSubClient.h"

const int SDA_PIN = 2;
const int SCL_PIN = 14;
const char* SSID = "IoT";
const char* PASSWORD = "VeryL0ngPas$wd!2015";

const char *MQTT_SERVER_NAME="192.168.1.80";
const uint16_t MQTT_SERVER_PORT = 1883;
const char* MQTT_USER_NAME = "rbfmiotUser";
const char* MQTT_PASSWORD = "rbfmiotPasswd";

const char* STATUS_ON = "on";
const char* STATUS_SUSP = "suspended";
const char* STATUS_OFF = "off";

const String CMD_GET_ID = "getid";
const String CMD_ACTIVATE = "activate";
const String CMD_SUSP = "suspend";

const int willQoS = 2;
const boolean willRetain = true;

const int SUSP_STAT_ADDR = 0;
const int SLEEP_TIME_VAL_ADDR = SUSP_STAT_ADDR + 1;
const int SLEEP_TIME_VAL_LENGTH = 4;
const int WAIT_FOR_COMMAND_TIME = 5;

char macAddr[12];
char statusTopic[24];
char requestTopic[25];
char replyTopic[23];
char telemTopicTemperature[29];
char telemTopicPressure[26];
char telemTopicHumidity[26];
double temp, pres, hum;

enum SuspStatVal {
  ON = 1,
  OFF = 0
};

RBFMIOT_BME280 rbfmiotBme280(I2C_ADDR_76);
WiFiClient wifiClient;
PubSubClient pubSubClient(MQTT_SERVER_NAME, MQTT_SERVER_PORT, wifiClient);

String readMACaddr();
void initWiFi();
void preprocessStatus();
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
  EEPROM.begin(5);
  initWiFi();
  pubSubClient.setCallback(callback);
  macAddrStr.toCharArray(macAddr, 12);
  (String("env/" + macAddrStr + "/status")).toCharArray(statusTopic, 24);
  (String("env/" + macAddrStr + "/request")).toCharArray(requestTopic, 25);
  (String("env/" + macAddrStr + "/reply")).toCharArray(replyTopic, 23);
  
  preprocessStatus();
  
  (String("env/" + macAddrStr + "/pressure")).toCharArray(telemTopicPressure, 26);
  (String("env/" + macAddrStr + "/humidity")).toCharArray(telemTopicHumidity, 26);
  (String("env/" + macAddrStr + "/temperature")).toCharArray(telemTopicTemperature, 29);
  Serial.println("Got topic names...");
  Serial.println(statusTopic);
  Serial.println(requestTopic);
  Serial.println(replyTopic);
  Serial.println(telemTopicTemperature);
  Serial.println(telemTopicPressure);
  Serial.println(telemTopicHumidity);
  Serial.println("Configure complete...");
}

void loop() {
  char *measVal;
  if (!pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();
  rbfmiotBme280.readAll(&temp, &pres, &hum);
  Serial.print("Publish temp="); Serial.print(temp);
  measVal = getValAsString(temp);
  pubSubClient.publish(telemTopicTemperature, measVal);
  free(measVal);
  Serial.print("\tpres="); Serial.print(pres);
  measVal = getValAsString(pres);
  pubSubClient.publish(telemTopicPressure, measVal);
  free(measVal); Serial.print("\thum="); Serial.print(hum);
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
  String macStr;
  WiFi.macAddress(macBin);
  for (i = 0; i < 6; i++) {
    macStr += String(macBin[i], HEX);
  }
  return macStr;
}

void initWiFi() {
  Serial.println();
  Serial.println("WiFi connect start...");
  WiFi.begin(SSID, PASSWORD);
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
    if (pubSubClient.connect(macAddr, MQTT_USER_NAME, MQTT_PASSWORD, statusTopic, willQoS, willRetain, STATUS_OFF)) {
      Serial.println("Connected to MQTT broker...");
      pubSubClient.publish(statusTopic, STATUS_ON, true);
      pubSubClient.subscribe(requestTopic, 1);
    } else {
      Serial.print("Unable to connect to MQTT broker! rc="); Serial.println(pubSubClient.state()); Serial.println("Will try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  int i;
  String command;
  for (i = 0; i < length; i++) {
    command += String((char)payload[i]);
  }
  Serial.print("command="); Serial.println(command);
  if (command.startsWith(CMD_GET_ID)) {
    handleGetId();
  } else if (command.startsWith(CMD_SUSP)) {
    handleSuspend(command);
  } else if (command.startsWith(CMD_ACTIVATE)) {
    handleActivate();
  }
}

void handleGetId() {
  int8_t id;
  char idArr[3];
  rbfmiotBme280.readId(&id);
  String idStr = String(id, HEX);
  idStr.toCharArray(idArr, 3);
  pubSubClient.publish(replyTopic, idArr);
}

void handleSuspend(String command) {
  long sleepTime;
  int cmdLength = CMD_SUSP.length();
  String strVal = command.substring(cmdLength + 1);
  sleepTime = sleepTimeToMcSec(strVal);
  pubSubClient.publish(statusTopic, STATUS_SUSP);
  updateSuspendedFlag(ON);
  updateSleepTime(strVal);
  Serial.print("Sleep for ["); Serial.print(sleepTime); Serial.println("] mcs");
  ESP.deepSleep(sleepTime);
}

void handleActivate() {
  updateSuspendedFlag(OFF);
  ESP.restart();
}

void preprocessStatus() {
   Serial.println("preprocessStatus() start...");
  if (ON == readSuspendedFlag()) {
    Serial.println("Suspend start...");
    if (!pubSubClient.connected()) {
      reconnect();
    }
    Serial.println("ESP.deepSleep() start...");
    long currStMcS = sleepTimeToMcSec(readSleepTime());
    delay(WAIT_FOR_COMMAND_TIME);
    pubSubClient.loop();
    pubSubClient.loop();
    pubSubClient.publish(statusTopic, STATUS_SUSP);
    ESP.deepSleep(currStMcS);
    Serial.println("ESP.deepSleep() complete...");
    
  }
  Serial.println("preprocessStatus() complete...");
}

SuspStatVal readSuspendedFlag() {
  Serial.println("readSuspendedFlag() start...");
  uint8_t flagVal = EEPROM.read(SUSP_STAT_ADDR);
  Serial.print("flagVal="); Serial.println(flagVal);
  Serial.println("readSuspendedFlag() complete...");
  return flagVal == ON ? ON : OFF;
}

void updateSuspendedFlag(SuspStatVal flagVal) {
  Serial.println("updateSuspendedFlag() start...");
  Serial.print("flagVal="); Serial.println(flagVal);
  EEPROM.write(SUSP_STAT_ADDR, flagVal);
  EEPROM.commit();
  Serial.println("updateSuspendedFlag() complete...");
}

String readSleepTime() {
  Serial.println("readSleepTime() start...");
  String sleepTime;
  int i;
  for (i = 0; i < SLEEP_TIME_VAL_LENGTH; i++) {
    sleepTime += char(EEPROM.read(SLEEP_TIME_VAL_ADDR + i));
  }
  Serial.print("sleepTime="); Serial.println(sleepTime);
  Serial.println("readSleepTime() complete...");
  return sleepTime;
}

long sleepTimeToMcSec(String sleepTimeStr) {
  long st = sleepTimeStr.toInt() * 1000000;
  return st;
}

void updateSleepTime(String sleepTimeStr) {
  Serial.println("updateSleepTime() start...");
  Serial.print("sleepTimeStr="); Serial.println(sleepTimeStr);
  int i;
  int sleepTimeStrLth  = sleepTimeStr.length();
  int currSleepTimeValLth = SLEEP_TIME_VAL_LENGTH;
  String stBuild = sleepTimeStr;
  for (i = 0; i < (SLEEP_TIME_VAL_LENGTH - sleepTimeStrLth); i++) {
    stBuild = "0" + stBuild;
  }
  Serial.print("Modified stBuild:"); Serial.println(stBuild);
  for (i = 0; i < SLEEP_TIME_VAL_LENGTH; i++) {
    Serial.print("Write char:"); Serial.println(stBuild[i]);
    EEPROM.write(SLEEP_TIME_VAL_ADDR + i, stBuild[i]);
  }
  EEPROM.commit();
  Serial.println("updateSleepTime() complete...");
}

