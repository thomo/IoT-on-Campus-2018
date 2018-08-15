#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
const char* mqtt_user = "tq";
const char* mqtt_password = "campus2018";
const char* clientId = "NodeB03";

WiFiClient espClient;
PubSubClient client(espClient);

int value = 0;

void setup() {
  delay(2000);
  Serial.begin(115200);
  
  setup_mqtt();
}

void setup_mqtt()
{
  Serial.println("Starte MQTT Setup");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
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

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientId, mqtt_user, mqtt_password)) {
      Serial.println("connected");
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

  value = (value + 1) % 2;
  String msg = String(value);
  
  client.publish("ledStatus", msg.c_str());
}

