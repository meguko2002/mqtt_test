/*****
  http://randomnerdtutorials.com/
*****/
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <EEPROM.h> 

Ticker flipper;

struct st_wifi { 
  char ssid[64];
  char pass[64];
};
//const char* ssid = "****";  
//const char* password = "****";

const int led_pin = 23;

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "192.168.2.106";

WiFiClient espClient;
PubSubClient client(espClient);

volatile float t = 15.4;
volatile float h = 0.57;
void incr() {
  t += 0.1;
  h -= 0.1;
}

long now = millis();
long lastMeasure = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");

  EEPROM.begin(128);
  st_wifi buf;
  EEPROM.get<st_wifi>(0, buf);
  WiFi.begin ( buf.ssid, buf.pass );
//  Serial.println(ssid);
//  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  if (topic == "room1/lamp") {
    Serial.print("Changing Room lamp to ");
    if (messageTemp == "on") {
      digitalWrite(led_pin, HIGH);
      Serial.print("On");
    }
    else if (messageTemp == "off") {
      digitalWrite(led_pin, LOW);
      Serial.print("Off");
    }
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("room1/lamp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(led_pin, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  flipper.attach(10, incr);
}

void loop() {
  if (!client.connected()) reconnect();
  if (!client.loop()) client.connect("ESP32Client");

  now = millis();
  if (now - lastMeasure > 10000) {
    lastMeasure = now;
    static char temperatureTemp[7];
    dtostrf(t, 6, 2, temperatureTemp);
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);
    client.publish("room1/temperature", temperatureTemp);
    client.publish("room1/humidity", humidityTemp);

    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(", ");
    Serial.print(humidityTemp);
    Serial.print(" %\t Temperature: ");
    Serial.print(t);
    Serial.print(", ");
    Serial.print(temperatureTemp);
    Serial.println(" *C ");
  }
}
