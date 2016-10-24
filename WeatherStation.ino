
#include <LTask.h>
#include <LWiFi.h>
//#include <LWiFiServer.h>
#include <LWiFiClient.h>
#include "DHT.h"

#define DHTPIN  2
#define PMPIN   8
#define DHTTYPE DHT22   // DHT 22  (AM2302)

#define WIFI_AP "A305-1"
#define WIFI_PASSWORD "yvtsyvts"
#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP according to your WiFi AP configuration

DHT dht(DHTPIN, DHTTYPE);

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "KXYUGH3UWEEQSF4Y";

long lastConnectionTime = 0;
boolean lastConnected = false;
int failedCounter = 0;

unsigned long duration;
unsigned long starttime;
unsigned long lastTime;
unsigned long sampletime_ms = 20000;//sampe 20s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

int nCount = 0;

LWiFiClient client;

void setup()
{
  pinMode(PMPIN, INPUT);

  dht.begin();
  LWiFi.begin();

  Serial.begin(9600);

  // keep retrying until connected to AP
  Serial.println("Connecting to AP");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
  {
    delay(1000);
  }

  startWifi();

  starttime = millis();//get the current time;
  lastTime = millis();
}

void loop()
{
  // Print Update Response to Serial Monitor
  if (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }

  // Disconnect from ThingSpeak
  if (!client.connected() && lastConnected)
  {
    Serial.println("...disconnected");
    Serial.println();

    client.stop();
  }

  // Update ThingSpeak

  // Read value from dust sensro
  if ((millis() - lastTime) > 2000) {
    lastTime = millis();

    duration = pulseIn(PMPIN, LOW);
    lowpulseoccupancy = lowpulseoccupancy + duration;

    if ((millis() - starttime) > sampletime_ms) //if the sampel time == 20s
    {
      ratio = lowpulseoccupancy / (sampletime_ms * 10.0); // Integer percentage 0=>100
      concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
      //        Serial.print(lowpulseoccupancy);
      //        Serial.print(",");
      //        Serial.print(ratio);
      //        Serial.print(",");
      //        Serial.println(concentration);
      String data = "";
      data += "field1=" + String(lowpulseoccupancy, DEC);
      data += "&field2=" + String(ratio, DEC);
      data += "&field3=" + String(concentration, DEC);


      float t = 0.0;
      float h = 0.0;

      do {
        dht.readHT(&t, &h);
      } while (t == 0 && h == 0);
      
      data += "&field4=" + String(t, DEC);
      data += "&field5=" + String(h, DEC);


      lowpulseoccupancy = 0;
      starttime = millis();
      updateThingSpeak(data);
    }
  }

  // Check if Arduino Ethernet needs to be restarted
  if (failedCounter > 3 ) {
    startWifi();
  }

  lastConnected = client.connected();
}

void updateThingSpeak(String tsData)
{
  if (client.connect(thingSpeakAddress, 80))
  {
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");

    client.print(tsData);

    lastConnectionTime = millis();

    if (client.connected())
    {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();

      failedCounter = 0;
    }
    else
    {
      failedCounter++;

      Serial.println("Connection to ThingSpeak failed (" + String(failedCounter, DEC) + ")");
      Serial.println();
    }

  }
  else
  {
    failedCounter++;

    Serial.println("Connection to ThingSpeak Failed (" + String(failedCounter, DEC) + ")");
    Serial.println();

    lastConnectionTime = millis();
  }
}

void startWifi()
{

  client.stop();

  Serial.println("Connecting Arduino to network...");
  Serial.println();

  delay(1000);
}
