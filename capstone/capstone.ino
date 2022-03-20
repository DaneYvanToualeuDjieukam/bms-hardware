#include "config.h"
#include "isl_esp_control.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

float t;  // battery temperature
int bat_id = 1;
String esp32_id = "default";
int counter = 0;
float chrg_current = 0.0;
float cell_1 = 0.0;
float cell_2 = 0.0;
float cell_3 = 0.0;
float cell_4 = 0.0;
String fault;
byte status1;
byte status2;
byte status3;
byte status4;
 
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
  //doc["temperature"] = String(t);
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
  //connectAWS();

  Wire.begin();
  Serial.println("ISL94202 initialize");
  Wire.beginTransmission(0x01);
  Wire.write(00);
  Wire.endTransmission(true);
  //False comms to unlock I2c state machine on the ic if we mess up^
  disableEEPROMAccess();
  writeReg(0x85, 0b00010000);//Set current shunt gain

  setISLDefaultSettings();
}
 
void loop()
{

  chrg_current = readCurrent();
  
  cell_1 = readCellV(0);
  cell_1 = readCellV(2);
  cell_1 = readCellV(3);
  cell_1 = readCellV(4);

  fault = String(readStatusReg(1)) + String(readStatusReg(2))+ String(readStatusReg(3)) + String(readStatusReg(4));
  
  Serial.print(esp32_id + "," + String(bat_id) + "," + String(chrg_current) + "," + String(readStatusReg(1)) + String(readStatusReg(2)) + String(readStatusReg(3)) + String(readStatusReg(4)) + "," + String(cell_1) + "," + String(cell_2) + "," + String(cell_3) + "," + String(cell_4));
 
  //publishMessage();
  client.loop();
  delay(2000);
}
