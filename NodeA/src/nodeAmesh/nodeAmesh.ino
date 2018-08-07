/*
 * Used libraries
 * - BME280 v2.3.0 by Tyler Glenn (https://github.com/finitespace/BME280)
 * - FastLED v3.1.6 by Daniel Garcia (https://github.com/FastLED/FastLED)
 * - WifiManager v0.14.0 by tzapu (https://github.com/tzapu/WiFiManager)
 * 
 * to install: Sketch -> Include Library -> Manage Libraries -> Search 
 */
 
#include "FastLED.h"
#include <BME280I2C.h>
#include <Wire.h>

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#define NUM_LEDS 2
#define DATA_PIN D5
#define SW1_PIN D6
#define SW2_PIN D7

#define BOOTSTRAP_SSID "IOT_NODE"

BME280I2C bme;

CRGB leds[NUM_LEDS];

const CRGB LED_OFF  = CRGB(0,0,0);
const CRGB LED_BLUE = CRGB(0,0,255);

void runConfig() {
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // id/name, placeholder/prompt, default, length
    // WiFiManagerParameter custom_node_id("id", "node id", node_id, 20);
    // wifiManager.addParameter(&custom_node_id);
    
    if (!wifiManager.startConfigPortal("NodeConfigWiFi")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
}


void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  
  if (digitalRead(SW1_PIN) == LOW) {
    leds[0] = LED_BLUE;
    leds[1] = LED_BLUE;
    FastLED.show();

    runConfig();

     // will restart in runConfig()
  } 
  
  leds[0] = LED_OFF;
  leds[1] = LED_OFF;
  FastLED.show();
}

void loop(void) {
}
