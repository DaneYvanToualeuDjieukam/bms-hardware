/*
 * For Getting Date & Time From NTP Server With ESP32 - https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
 */

#include "config.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
 
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String dateStamp;
String timeStamp;
String hourStamp;

//battery status related
float temperature;  // battery temperature
int bat_id = 2;
String esp32_id;
int counter = 0;
float chrg_current = 0.0;
float cell_1 = 0.0;
float cell_2 = 0.0;
float cell_3 = 0.0;
float cell_4 = 0.0;
int fault = 0;
 
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
  doc["temperature"] = String(temperature);
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


/*
 * get the current time stamp
 */
void getTimeStamp(){
  String formattedDate;
  
  //wait till the server update the time and get it (every micro sec or so)
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();

  // Extract date: yyyy-MM-DD
  int splitT = formattedDate.indexOf("T");
  dateStamp = formattedDate.substring(0, splitT);
  
  // Extract time:  HH-MM-SS
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
}


void setup()
{
  Serial.begin(115200);
  connectAWS();
  timeClient.begin();
  timeClient.setTimeOffset(-14400);
}
 
void loop()
{
  getTimeStamp();
  esp32_id = String(1) + "/" + dateStamp + "/" + timeStamp;
  chrg_current = float(random(60, 70))/10.0;
  cell_1 = float(random(30, 32))/10.0;
  cell_2 = float(random(40, 42))/10.0;
  cell_3 = float(random(40, 42))/10.0;
  cell_4 = float(random(40, 42))/10.0;
  temperature = random(20, 23);
  fault = 1;

  Serial.print(esp32_id + "," + String(1) + "," + String(chrg_current) + "," + String(temperature) + "," + String(fault) + "," + String(cell_1) + "," + String(cell_2) + "," + String(cell_3) + "," + String(cell_4));
 
  publishMessage();
  
  client.loop();
  delay(2000);
}
