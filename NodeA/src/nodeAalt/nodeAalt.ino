/*
 * Used libraries
 * - BME280 v2.3.0 by Tyler Glenn (https://github.com/finitespace/BME280)
 * - FastLED v3.1.6 by Daniel Garcia (https://github.com/FastLED/FastLED)
 * - WifiManager v0.14.0 by tzapu (https://github.com/tzapu/WiFiManager)
 * - ArduinoJson v5.13.2 (! use this version)
 * - PubSubClient by Nick O'Leary 2.6.0 (https://pubsubclient.knolleary.net/)
 * 
 * to install: Sketch -> Include Library -> Manage Libraries -> Search 
 */

#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include "FastLED.h"
#include <BME280I2C.h>
#include <Wire.h>

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <PubSubClient.h>

#define MY_DEBUG  1

#define NUM_LEDS  2
#define LEFT_LED  1
#define RIGHT_LED 0
#define DATA_PIN  D5
#define SW0_PIN   D6
#define SW1_PIN   D7

#define DEFAULT_MQTT_SERVER "mqttbroker"
#define DEFAULT_NODE_ID "white"
#define DEFAULT_COLOR "1_1_1"

#define SUB_TOPIC "campus/feedback"
#define PUB_TOPIC "campus/nodeA"
#define PUB_BTN0STATE "campus/Button0"
#define PUB_BTN1STATE "campus/Button1"
#define PUB_RATING "campus/rating"
#define SUB_MODE "campus/modus"

const char* mqtt_user = "tq";
const char* mqtt_password = "campus2018";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

BME280I2C bme;

CRGB leds[NUM_LEDS];

const CRGB LED_OFF  = CRGB(0,0,0);
const CRGB LED_RED  = CRGB(181,0,0);
const CRGB LED_GREEN = CRGB(0,181,0);
const CRGB LED_BLUE = CRGB(0,0,181);

char cfg_node_id[20];
char cfg_mqtt_server[40];
char cfg_color[12];

//flag for saving data
bool shouldSaveConfig = false;

// node mode: '0': normal mode, '1': rating mode
char node_mode = '0';
bool modeChanged = false;

long lastMsgMillis = 0;

//          1         2         3         4         5         6         7         8         9         0         1         2
// 123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
// sensors node=12345678901234567890 temp=-xx.xx,hum=100.00,press=100000.00,s0=x,s1=x,color=xxx_xxx_xxx
#define MQTT_MSG_LEN 120
#define MQTT_BTN_LEN 22
char mqttMsg[MQTT_MSG_LEN];
char mqttButtonMsg[MQTT_BTN_LEN];

// ------------------------------------------------------

//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}

void saveConfig () {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["mqtt_server"] = cfg_mqtt_server;
  json["node_id"] = cfg_node_id;
  json["color"] = cfg_color;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  } else {
    json.printTo(configFile);
    configFile.close();
  }
  //end save
}

void loadConfig() {
  //read configuration from FS json
  if (SPIFFS.exists("/config.json")) {
    //file exists, reading and loading
    Serial.println("reading config file");
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (json.success()) {
        strncpy(cfg_mqtt_server, json["mqtt_server"], 40);
        strncpy(cfg_node_id, json["node_id"], 20);
        strncpy(cfg_color, json["color"], 12);
      } else {
        Serial.println("Failed to load json config. Invalid?");
        json.printTo(Serial);
        Serial.println("\n");
      }
      configFile.close();
    }
  } else {
    Serial.println("config file not found!");
  }
}

void runConfig() {
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    wifiManager.setDebugOutput(false);

    WiFiManagerParameter labelMQTT("<label for=\"srv\">MQTT broker (hostname or ip)</label>");
    wifiManager.addParameter(&labelMQTT);
    WiFiManagerParameter custom_mqtt_server("srv", "mqtt broker", cfg_mqtt_server, 40);
    wifiManager.addParameter(&custom_mqtt_server);

    WiFiManagerParameter labelID("<label for=\"nid\">Node Id</label>");
    wifiManager.addParameter(&labelID);
    WiFiManagerParameter custom_node_id("nid", "node id", cfg_node_id, 20);
    wifiManager.addParameter(&custom_node_id);
    
    WiFiManagerParameter labelColor("<label for=\"color\">Color (xxx_xxx_xxx)</label>");
    wifiManager.addParameter(&labelColor);
    WiFiManagerParameter custom_color("color", "color", cfg_color, 12);
    wifiManager.addParameter(&custom_color);
    
    // set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);
  
    if (!wifiManager.startConfigPortal("NodeConfigWiFi")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }

    // if you get here you have connected to the WiFi
    Serial.println("connected ... :)");

    if (shouldSaveConfig) {
      // read updated parameters
      strncpy(cfg_mqtt_server, custom_mqtt_server.getValue(), 40);
      strncpy(cfg_node_id, custom_node_id.getValue(), 20);
      strncpy(cfg_color, custom_color.getValue(), 12);
  
      saveConfig(); 
    }
}

void setupWiFi() {
  Serial.println("Start WiFi Setup");

  WiFi.persistent(false); // just store updated ssid/passwd values
  WiFi.begin(); // connect using the ssid/passwd configured with wifi manager

  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED && cnt < 10) {
    ++cnt;
    delay(500);
    Serial.print(".");
  }  

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nERROR: WiFi connection failed!");
    leds[0] = LED_RED;
  }
  Serial.println("----- ----- -----");
}

void setupMQTT(){
  Serial.println("Start MQTT Setup");
  mqttClient.setServer(cfg_mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);
}

void setupBME() {
  Wire.begin();

  int cnt = 0;
  while (!bme.begin() && cnt < 10) {
    ++cnt;
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  if (!bme.begin()) {
    Serial.println("Could not find BME280 sensor! - Giving up :-(");
  }
}

int splitColorString(char* color, char* buf) {
  int idx = 0;
  while ((color[idx] != '_') && (color[idx] != '\0')) {
    buf[idx] = color[idx];
    ++idx;
  }
  buf[idx] = '\0';
  return idx;
}

void parseRGB(char* color, CRGB* rgb) {
  int idx = 0;
  char tmp[4];
  
  idx = idx + splitColorString(color, tmp);
  rgb->r = atoi(tmp);

  ++idx;
  idx = idx + splitColorString(&color[idx], tmp);
  rgb->g = atoi(tmp);
  
  ++idx;
  idx = idx + splitColorString(&color[idx], tmp);
  rgb->b = atoi(tmp);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  // set default values
  strncpy(cfg_mqtt_server, DEFAULT_MQTT_SERVER, 40);
  strncpy(cfg_node_id, DEFAULT_NODE_ID, 20);
  strncpy(cfg_color, DEFAULT_COLOR, 12);

  if (!SPIFFS.begin()) {
    Serial.println("failed to mount FS");
  }

  if (digitalRead(SW1_PIN) == HIGH) {
    loadConfig();
  }
  
  if (digitalRead(SW0_PIN) == LOW) {
    leds[0] = LED_BLUE;
    leds[1] = LED_BLUE;
    FastLED.show();

    runConfig();

    // will not reach this line - restart of ESP8266 is forced in runConfig() !!!
  }
  
  parseRGB(cfg_color, &leds[0]);
  parseRGB(cfg_color, &leds[1]);

  setupWiFi();
  setupMQTT();
  setupBME();
  
  FastLED.show();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (MY_DEBUG) {
    Serial.print("< [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  }

  // INCOMING: /campus/modus
  if (strcmp(topic, SUB_MODE) == 0) {
    if ((char)payload[0] == '0' || (char)payload[0] == '1') {
      if (node_mode != (char)payload[0]){
        modeChanged = true;
        node_mode = (char)payload[0];
      }
    }
  }

// incoming payload: nodeName=rrr_ggg_bbb
  
  if (strcmp(topic,SUB_TOPIC)==0) {
    // content contains color of led 0 and led 1
    // adjust the color of leds accordingly

    // get node name length and color length
    char* pos = strstr((char*)payload, "=");    
    int nodeNameLength = 0;
    nodeNameLength = pos - (char*)payload;
    int colorLength = length - nodeNameLength - 1;

    pos++;
    char cbuf[12];
    strncpy(cbuf, pos, colorLength);
    cbuf[min(11U,length-nodeNameLength-1)] = '\0';
    Serial.print("cbuf=");
    Serial.println(cbuf);
    char node[20];
    int j = 0;

    for (int i = 0; i < length; i++) {
      if (payload[i] == '=') break;
      node[j++] = payload[i];
    }
    
    node[j]='\0';

    // wenn NODE=R_G_B -> LED 1
    // wenn 1=R_G_B -> LED 0

    if (strcmp(node, cfg_node_id) == 0) {
      parseRGB(cbuf, &leds[LEFT_LED]);
      FastLED.show();
  }
    else if (payload[0] == '1') {
      parseRGB(cbuf, &leds[RIGHT_LED]);
    FastLED.show();
    }
  }
}

void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    
    Serial.print("Attempting MQTT connection ... ");
    
    // Attempt to connect
    if (mqttClient.connect(WiFi.macAddress().c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      mqttClient.subscribe(SUB_TOPIC);
      mqttClient.subscribe(SUB_MODE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in some seconds");
      delay(2000);       // Wait before retrying
    }
  }
}

int btn0State = HIGH;
int btn1State = HIGH;
int lastClickMillis = 0;

void normalMode() {
  long now = millis();  
  if (now - lastClickMillis > 75) {
    lastClickMillis = now;
    
    int currentBtn0State = digitalRead(SW0_PIN);

    if (currentBtn0State == LOW && btn0State == HIGH) {
      btn0State = LOW;
      Serial.println("Button 0 pressed");
      snprintf (mqttButtonMsg, MQTT_BTN_LEN, "%s#%d#%s", cfg_node_id, btn0State, cfg_color);
      mqttClient.publish(PUB_BTN0STATE, mqttButtonMsg);
    }
    else if (currentBtn0State == HIGH && btn0State == LOW) {
      Serial.println("Button 0 released");
      btn0State = HIGH;
    }
    
    int currentBtn1State = digitalRead(SW1_PIN);
  
    if (currentBtn1State == LOW && btn1State == HIGH) {
      btn1State = LOW;
      Serial.println("Button 1 pressed");
      snprintf (mqttButtonMsg, MQTT_BTN_LEN, "%s#%d#%s", cfg_node_id, btn1State, cfg_color);
      mqttClient.publish(PUB_BTN1STATE, mqttButtonMsg);
    }
    else if (currentBtn1State == HIGH && btn1State == LOW) {
      Serial.println("Button 1 released");
      btn1State = HIGH;
    }
  }
}

int numClicks = 0;

void ratingMode() {
  long now = millis();
  bool ratingSend = false;
  
  if (now - lastClickMillis > 75) {
    lastClickMillis = now;
    
    int currentBtn0State = digitalRead(SW0_PIN);

    if (currentBtn0State == LOW && btn0State == HIGH && numClicks < 3) {
      numClicks++;
      btn0State = LOW;
      Serial.println("Button 0 pressed");
      snprintf (mqttButtonMsg, MQTT_BTN_LEN, "%s#%d#%s", cfg_node_id, btn0State, cfg_color);
      mqttClient.publish(PUB_RATING, "0");
      ratingSend = true;
    }
    else if (currentBtn0State == HIGH && btn0State == LOW) {
      Serial.println("Button 0 released");
      btn0State = HIGH;
    }
    
    int currentBtn1State = digitalRead(SW1_PIN);
  
    if (currentBtn1State == LOW && btn1State == HIGH && numClicks < 3) {
      numClicks++;
      btn1State = LOW;
      Serial.println("Button 1 pressed");
      snprintf (mqttButtonMsg, MQTT_BTN_LEN, "%s#%d#%s", cfg_node_id, btn1State, cfg_color);
      mqttClient.publish(PUB_RATING, "1");
      ratingSend = true;
    }
    else if (currentBtn1State == HIGH && btn1State == LOW) {
      Serial.println("Button 1 released");
      btn1State = HIGH;
    }

    if (ratingSend) {
      if (numClicks == 1) {
        leds[LEFT_LED] = LED_GREEN;
        leds[RIGHT_LED] = LED_OFF;
      }
      else if (numClicks == 2) {
        leds[LEFT_LED] = LED_GREEN;
        leds[RIGHT_LED] = LED_GREEN;
      }
      else if (numClicks == 3) {
        leds[LEFT_LED] = LED_RED;
        leds[RIGHT_LED] = LED_RED;
      }
      FastLED.show();
    }
  }
}

void publishSensorData() {
  long now = millis();
  if (now - lastMsgMillis > 2000) {
    lastMsgMillis = now;

    float temp(NAN), hum(NAN), pres(NAN);

    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

    bme.read(pres, temp, hum, tempUnit, presUnit);
    
    snprintf (mqttMsg, MQTT_MSG_LEN, 
              "sensors,node=%s temp=%.2f,hum=%.2f,press=%.2f,s0=%d,s1=%d,color=%s", 
              cfg_node_id, temp, hum, pres, digitalRead(SW0_PIN), digitalRead(SW1_PIN), cfg_color);
    if (MY_DEBUG) {
      Serial.print("> [");
      Serial.print(PUB_TOPIC);
      Serial.print("] ");
      Serial.println(mqttMsg);
    }
    mqttClient.publish(PUB_TOPIC, mqttMsg);
  }
}

void loop(void) {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  publishSensorData();

  if (modeChanged) {
    numClicks = 0;
    modeChanged = false;
    leds[0] = LED_RED;
    leds[1] = LED_OFF;
    FastLED.show();
    delay(250);
    leds[1] = LED_BLUE;
    FastLED.show();
    delay(250);
    leds[0] = LED_OFF;
    leds[1] = LED_OFF;
    FastLED.show();
  }
  
  if (node_mode == '0') {
    normalMode();
  }
  else if (node_mode == '1') {
    ratingMode();
  }
  else {
    Serial.println("Unknown mode!");
  }
}
