/*
 * Used libraries
 * - BME280 v2.3.0 by Tyler Glenn (https://github.com/finitespace/BME280)
 * - FastLED v3.1.6 by Daniel Garcia (https://github.com/FastLED/FastLED)
 * - WifiManager v0.14.0 by tzapu (https://github.com/tzapu/WiFiManager)
 * - ArduinoJson v5.13.2 (!)
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

#define NUM_LEDS 2
#define DATA_PIN D5
#define SW1_PIN D6
#define SW2_PIN D7

#define BOOTSTRAP_SSID "IOT_NODE"

BME280I2C bme;

CRGB leds[NUM_LEDS];

const CRGB LED_OFF  = CRGB(0,0,0);
const CRGB LED_BLUE = CRGB(0,0,255);

char cfg_node_id[20];
char cfg_mqtt_server[40];

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("mark config as to be saved");
  shouldSaveConfig = true;
}

void saveConfig () {
  Serial.println("saving config");
  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["mqtt_server"] = cfg_mqtt_server;
  json["node_id"] = cfg_node_id;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  } else {
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    Serial.println("Saved!!!");
  }
  //end save
}

void loadConfig() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strncpy(cfg_mqtt_server, json["mqtt_server"],40);
          strncpy(cfg_node_id, json["node_id"],20);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    } else {
      Serial.println("config file not found!");
    }
  } else {
    Serial.println("failed to mount FS");
  }  
}

void runConfig() {
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // id/name, placeholder/prompt, default, length
    WiFiManagerParameter custom_node_id("nid", "node id", cfg_node_id, 20);
    WiFiManagerParameter custom_mqtt_server("srv", "mqtt server", cfg_mqtt_server, 40);

    wifiManager.addParameter(&custom_node_id);
    wifiManager.addParameter(&custom_mqtt_server);
    
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
    Serial.println("connected...yeey :)");

    if (shouldSaveConfig) {
      // read updated parameters
      strncpy(cfg_mqtt_server, custom_mqtt_server.getValue(),40);
      strncpy(cfg_node_id, custom_node_id.getValue(),20);
  
      saveConfig(); 
    }
}


void setup() {
  Serial.begin(115200);
  Serial.println();

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  loadConfig();
  
  if (digitalRead(SW1_PIN) == LOW) {
    leds[0] = LED_BLUE;
    leds[1] = LED_BLUE;
    FastLED.show();

    runConfig();

    // will not reach this line - restart of Raspi is forced in runConfig() !!!
  }
  
  leds[0] = LED_OFF;
  leds[1] = LED_OFF;
  FastLED.show();
}

void loop(void) {
}
