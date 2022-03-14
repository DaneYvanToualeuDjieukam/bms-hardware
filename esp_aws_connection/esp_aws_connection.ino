#include "config.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
 
float t;  // battery temperature
int bat_id = 1;
String esp32_id;
int counter = 0;
float chrg_current = 0.0;
float cell_1 = 0.0;
float cell_2 = 0.0;
float cell_3 = 0.0;
float cell_4 = 0.0;
int fault = 0;
 
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}
 
void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["esp32_id"] = String(esp32_id);
  doc["bat_id"] = String(bat_id);
  doc["chrg_current"] = String(chrg_current);
  doc["temperature"] = String(t);
  doc["fault"] = String(fault);
  doc["cell_1"] = String(cell_1);
  doc["cell_2"] = String(cell_2);
  doc["cell_3"] = String(cell_3);
  doc["cell_4"] = String(cell_4);
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.println(", sent");
}
 
void setup()
{
  Serial.begin(115200);
  connectAWS();
}
 
void loop()
{
  counter = counter + 1;
  esp32_id = String(counter) + "/" + String(millis());
  chrg_current = float(random(46, 50))/10.0;
  cell_1 = float(random(40, 42))/10.0;
  cell_2 = float(random(40, 42))/10.0;
  cell_3 = float(random(40, 42))/10.0;
  cell_4 = float(random(40, 42))/10.0;
  t = random(20, 23);
  fault = 0;
  
  Serial.print(esp32_id + "," + String(bat_id) + "," + String(chrg_current) + "," + String(t) + "," + String(fault) + "," + String(cell_1) + "," + String(cell_2) + "," + String(cell_3) + "," + String(cell_4));
 
  publishMessage();
  client.loop();
  delay(2000);
}
