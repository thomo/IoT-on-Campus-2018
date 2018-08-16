/*
 * Used libraries
 * - BME280 v2.3.0 by Tyler Glenn (https://github.com/finitespace/BME280)
 * - FastLED v3.1.6 by Daniel Garcia (https://github.com/FastLED/FastLED)
 * 
 * to install: Sketch -> Include Library -> Manage Libraries -> Search 
 */

#include "FastLED.h"
#include <BME280I2C.h>
#include <Wire.h>

#define NUM_LEDS 12
#define DATA_PIN D5
#define SW1_PIN D6
#define SW2_PIN D7

BME280I2C bme;

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  Serial.begin(9600);

  while (!Serial) {}

  Wire.begin();

  while (!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  switch (bme.chipModel())
  {
    case BME280::ChipModel_BME280:
      Serial.println("Found BME280 sensor! Success.");
      break;
    case BME280::ChipModel_BMP280:
      Serial.println("Found BMP280 sensor! No Humidity available.");
      break;
    default:
      Serial.println("Found UNKNOWN sensor! Error!");
  }
}

uint8_t l = 0;
uint8_t c = 0;

void loop() {

  uint8_t r = 255 * (c % 3 == 0);
  uint8_t g = 255 * (c % 3 == 1);
  uint8_t b = 255 * (c % 3 == 2);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(r * (l % NUM_LEDS == i), g * (l % NUM_LEDS == i), b * (l % NUM_LEDS == i));
  }

  if (l % NUM_LEDS == NUM_LEDS - 1) {
    c++;
  }
  l++;

  FastLED.show();
  delay(250);
  printSwitchStatus(&Serial);
  printBME280Data(&Serial);
}

void printBME280Data( Stream* client ) {
  float temp(NAN), hum(NAN), pres(NAN);

  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  bme.read(pres, temp, hum, tempUnit, presUnit);

  client->print("Temp: ");
  client->print(temp);
  client->print("°" + String(tempUnit == BME280::TempUnit_Celsius ? 'C' : 'F'));
  client->print("\t\tHumidity: ");
  client->print(hum);
  client->print("% RH");
  client->print("\t\tPressure: ");
  client->print(pres);
  client->println(" Pa");
}

void printSwitchStatus( Stream* client ) {
  client->print("S1: ");
  client->print(digitalRead(SW1_PIN));
  client->print(" S2: ");
  client->print(digitalRead(SW2_PIN));
  client->print(" ");
}
