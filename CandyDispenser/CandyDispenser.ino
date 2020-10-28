#include <ESP8266WiFi.h>
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <Stepper.h>

// change this to the number of steps on your motor
#define STEPS 200
#define CANDY 200
#define BROKER "YOUR-BROKER"
#define CLIENTID "CandyMan"
#define COMMAND_TOPIC "candy"
#define PASSWORD ""
// create an instance of the stepper class, specifying
// the number of steps of the motor and the pins it's
// attached to
Stepper stepper(STEPS, D4, D3, D2, D1);

const char *ssid = "YOURSSID";
const char *password = "YOURPASSWORD";
int status = WL_IDLE_STATUS; // the Wifi radio's status
WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
  Serial.begin(115200);
  Serial.println("Stepper test!");
  // set the speed of the motor to 30 RPMs
  stepper.setSpeed(60);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  client.setServer(BROKER, 8883);
  client.setCallback(callback);
}

int wait_for_wifi()
{
  int tries = 30;
  int thisTry = 0;
  Serial.println("waiting for Wifi");
  while (WiFi.status() != WL_CONNECTED && thisTry < tries)
  {
    delay(1000);
    thisTry += 1;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    return 1;
  }
  Serial.println("");
  Serial.println("WiFi connected");
  return 0;
}

void reconnect()
{
  boolean first = true;
  // Loop until we're reconnected to the broker
  while (!client.connected())
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      wait_for_wifi();
      first = true;
    }
    // Attempt to connect
    if (client.connect(CLIENTID, "use-token-auth", PASSWORD))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.println(client.state());
    }
  }
  // subscribe to the command topic
  client.subscribe(COMMAND_TOPIC);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String s = String((char *)payload);
  s.trim();
  // "+1" to skip over the '{'
  s = s.substring(s.indexOf("=") + 1, s.lastIndexOf("}"));
  s.trim();
  int c = s.toInt();
  Serial.print("Dispensing ");
  Serial.print(c);
  Serial.println(" pieces of candy.");
  int t = c * CANDY;
  Serial.println(t);
  for (int x = 0; x < c; x++)
  {
    stepper.step(-CANDY);
  }
  Serial.println("Done!");
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}