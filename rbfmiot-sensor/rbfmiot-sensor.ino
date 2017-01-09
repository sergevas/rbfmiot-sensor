#include<ESP8266WiFi.h>
#include <EEPROM.h>
#include <RBFMIOT_BME280.h>
#include "PubSubClient.h"

const int SDA_PIN = 2;
const int SCL_PIN = 14;
const char* SSID = "IoT";
const char* PASSWORD = "********";

const char* MQTT_SERVER_NAME="192.168.1.80";
const uint16_t MQTT_SERVER_PORT = 1883;
const char* MQTT_USER_NAME = "rbfmiotUser";
const char* MQTT_PASSWORD = "rbfmiotPasswd";

const char* STATUS_ON = "on";
const char* STATUS_SLEEPING = "sleeping";
const char* STATUS_OFF = "off";

const char* REPLY_STATUS_OK = "200";

const String CMD_GET_SENSOR_ID = "getsensid";
const String CMD_BATT = "getbattery";
const String CMD_SET_MODE = "setmode";

const int willQoS = 2;
const boolean willRetain = true;

const int MODE_ADDR = 0;
const int SLEEP_TIME_VAL_ADDR = MODE_ADDR + 1;
const int SLEEP_TIME_VAL_LENGTH = 4;
const uint32_t WAIT_FOR_COMMAND_MSG_DELAY = 5000;
const uint32_t ACTIVE_MODE_DELAY = 2000;
const char COMMA = ',';

enum Mode {
  SUSPENDED = '0',
  ACTIVE = '1',
  PWR_SAVE = '2'
};

typedef struct {
  String msgId;
  String command;
  String params;
} cmdRequestMsg_t;

typedef struct {
  String correlId;
  String status;
  String payload;
} cmdReplyMsg_t;

char macAddr[12];
char statusTopic[24];
char requestTopic[25];
char replyTopic[23];
char telemTopicTemperature[29];
char telemTopicPressure[26];
char telemTopicHumidity[26];
double temp, pres, hum;
cmdRequestMsg_t cmdRequestMsg;
cmdReplyMsg_t cmdReplyMsg;

RBFMIOT_BME280 rbfmiotBme280(I2C_ADDR_76);
WiFiClient wifiClient;
PubSubClient pubSubClient(MQTT_SERVER_NAME, MQTT_SERVER_PORT, wifiClient);
ADC_MODE(ADC_VCC);

String readMACaddr();
void initWiFi();
void preprocessMode();
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
  preprocessMode();
}

void loop() {
  if (!pubSubClient.connected()) {
    reconnect();
  }
  execMeasur();
  pubSubClient.loop();
  delay(ACTIVE_MODE_DELAY);
}

void execMeasur() {
  char *measVal;
  rbfmiotBme280.readAll(&temp, &pres, &hum);
  Serial.print("Publish temp="); Serial.print(temp);
  measVal = getValAsString(temp);
  pubSubClient.publish(telemTopicTemperature, measVal);
  free(measVal);
  Serial.print("\tpres="); Serial.print(pres);
  measVal = getValAsString(pres);
  pubSubClient.publish(telemTopicPressure, measVal);
  free(measVal); Serial.print("\thum="); Serial.println(hum);
  measVal = getValAsString(hum);
  pubSubClient.publish(telemTopicHumidity, measVal);
  free(measVal);
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
  String requestCmdMsg;
  for (i = 0; i < length; i++) {
    requestCmdMsg += String((char)payload[i]);
  }
  Serial.print("requestCmdMsg="); Serial.println(requestCmdMsg);
  parseRequestCmdMsg(requestCmdMsg);
  if (cmdRequestMsg.command.equals(CMD_GET_SENSOR_ID)) {
    handleGetSensId();
  } else if (cmdRequestMsg.command.equals(CMD_BATT)) {
    handleGetBattery();
  } else if (cmdRequestMsg.command.equals(CMD_SET_MODE)) {
    handleSetMode();
  }
}

void parseRequestCmdMsg(String aCommand) {
  int sepIdxBeg = 0, sepIdxEnd;
  sepIdxEnd = aCommand.indexOf(COMMA);
  cmdRequestMsg.msgId = aCommand.substring(sepIdxBeg, sepIdxEnd);
  sepIdxBeg = sepIdxEnd + 1;
  sepIdxEnd = aCommand.indexOf(COMMA, sepIdxBeg);
  if (sepIdxEnd == -1) {
    cmdRequestMsg.command = aCommand.substring(sepIdxBeg);
  } else {
    cmdRequestMsg.command = aCommand.substring(sepIdxBeg, sepIdxEnd);
    sepIdxBeg = sepIdxEnd + 1;
    cmdRequestMsg.params = aCommand.substring(sepIdxBeg);
  }
}

void createAndPublishReplyMsg() {
  String repMsgStr = cmdReplyMsg.correlId
    + COMMA
    + cmdReplyMsg.status
    + (cmdReplyMsg.payload.length() > 0) ? (COMMA + cmdReplyMsg.payload) : "";
  int repLength = repMsgStr.length();
  char reply[repLength];
  repMsgStr.toCharArray(reply, repLength);
  pubSubClient.publish(replyTopic, reply);
}

void handleGetSensId() {
  int8_t id;
  rbfmiotBme280.readId(&id);
  cmdReplyMsg.correlId = cmdRequestMsg.msgId;
  cmdReplyMsg.status = REPLY_STATUS_OK;
  cmdReplyMsg.payload = String(id, HEX);
  createAndPublishReplyMsg();
}

void handleGetBattery() {
  uint32_t vcc;
  vcc = ESP.getVcc();
  Serial.print("vcc="); Serial.println(vcc);
  cmdReplyMsg.correlId = cmdRequestMsg.msgId;
  cmdReplyMsg.status = REPLY_STATUS_OK;
  cmdReplyMsg.payload = String(vcc);
  createAndPublishReplyMsg();
}

void handleSetMode() {
  Serial.println("handleSetMode() start...");
  char modeVal = cmdRequestMsg.params[0];
  Serial.print("modeVal="); Serial.println(modeVal);
  if (modeVal == ACTIVE) {
    updateModeVal(ACTIVE);
    cmdReplyMsg.correlId = cmdRequestMsg.msgId;
    cmdReplyMsg.status = REPLY_STATUS_OK;
    createAndPublishReplyMsg();
    ESP.restart();
  } else if (modeVal == SUSPENDED || modeVal == PWR_SAVE) {
    String sleepTimeStr = cmdRequestMsg.params.substring(2);
    long sleepTime = sleepTimeToMcSec(sleepTimeStr);
    if (modeVal == PWR_SAVE) {
      execMeasur();
    }
    pubSubClient.publish(statusTopic, STATUS_SLEEPING);
    updateModeVal((Mode)modeVal);
    updateSleepTime(sleepTimeStr);
    cmdReplyMsg.correlId = cmdRequestMsg.msgId;
    cmdReplyMsg.status = REPLY_STATUS_OK;
    createAndPublishReplyMsg();
    Serial.print("Sleep for ["); Serial.print(sleepTime); Serial.println("] mcs");
    ESP.deepSleep(sleepTime);
  }
}

void preprocessMode() {
   Serial.println("preprocessMode() start...");
   if (!pubSubClient.connected()) {
      reconnect();
    }
   Mode mode = readModeVal();
   if (mode == PWR_SAVE) {
     execMeasur();
   }
   if (mode == PWR_SAVE || mode == SUSPENDED) {
    Serial.println("Waiting for command msg start...");
    delay(WAIT_FOR_COMMAND_MSG_DELAY);
    Serial.println("Waiting for command msg complete...");
    pubSubClient.loop();
    pubSubClient.loop();
    long sleepTime = sleepTimeToMcSec(readSleepTime());
    pubSubClient.publish(statusTopic, STATUS_SLEEPING);
    Serial.print("Sleep for ["); Serial.print(sleepTime); Serial.println("] mcs");
    ESP.deepSleep(sleepTime);
  }
  Serial.println("preprocessMode() complete...");
}

Mode readModeVal() {
  Serial.println("readModeVal() start...");
  Mode mode;
  char modeVal = char(EEPROM.read(MODE_ADDR));
  Serial.print("modeVal="); Serial.println(modeVal);
  if (modeVal == SUSPENDED) {
    mode = SUSPENDED;
  } else if (modeVal == ACTIVE) {
    mode = ACTIVE;
  } else {
    mode = PWR_SAVE;
  }
  Serial.println("readModeVal() complete...");
  return mode;
}

void updateModeVal(Mode mode) {
  EEPROM.write(MODE_ADDR, mode);
  EEPROM.commit();
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

