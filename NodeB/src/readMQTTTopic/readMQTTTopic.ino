/*
 * Used libraries
 * - PubSubClient by Nick O'Leary 2.6.0 (https://pubsubclient.knolleary.net/)
 * 
 * to install: Sketch -> Include Library -> Manage Libraries -> Search 
 */
 
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const char* mqtt_user = "tq";
const char* mqtt_password = "campus2018";
const char* clientId = "NodeB02";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  delay(2000);
  Serial.begin(115200);
  
  setup_mqtt();
}

void setup_mqtt()
{
  Serial.println("Starte MQTT Setup");
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("----- ----- -----");
}

void setup_wifi() {
  Serial.println("Starte WiFi Setup");
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int c = 0;
  while (WiFi.status() != WL_CONNECTED && 10 > c++)
  {
    delay(500);
    Serial.print(".");
  }  

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("----- ----- -----");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic,"ledStatus")==0) {
    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1') {
      digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is acive low on the ESP-01)
    } else {
      digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    }
  }
 
  if (strcmp(topic,"home/basement/work/temperatur")==0) {
    Serial.println("to something based on the temperature");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // ... and subscribe to one or more topics
      client.subscribe("ledStatus");
      client.subscribe("home/basement/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
} 

