Boards, sources and configuration to run the Mini-Hackathon at SQS Campus 2018.
The infrastructure consists of two kind of ESP8266 nodes - called NodeA and NodeB - and a master. The master is running on a Raspberry Pi 3. On the master a bundle of applications is installed:
- Mosquitto - a MQTT server
- Influxdb - a time based database
- Grafana 
- NodeRed

The NodeA/NodeB are small IoT devices with a temperature/humidity/pressure sensor, two switches and some RGB leds. While NodeA is prebuild and will be distributed on the campus location to collect sensor data the NodeB will be put together by the participants in a workshop.

## Setup Arduino for ESP8266

(Source: https://github.com/esp8266/Arduino)

- Install the current upstream Arduino IDE at the 1.8 level or later. The current version is at the Arduino website.
- Start Arduino and open Preferences window.
- Enter http://arduino.esp8266.com/stable/package_esp8266com_index.json into Additional Board Manager URLs field. You can add multiple URLs, separating them with commas.
- Open Boards Manager from Tools > Board menu and install esp8266 platform (and don't forget to select your ESP8266 board from Tools > Board menu after installation).