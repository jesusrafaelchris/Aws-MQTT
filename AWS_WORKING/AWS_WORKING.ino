// Fill in  your WiFi networks SSID and password
#define SECRET_SSID "undercover FBI"
#define SECRET_PASS "Georgina2019"

// Fill in the hostname of your AWS IoT broker
#define SECRET_BROKER "a9dny8v5gounv-ats.iot.us-east-2.amazonaws.com"

// Fill in the boards public certificate
const char SECRET_CERTIFICATE[] = R"(
-----BEGIN CERTIFICATE-----
MIIChjCCAW6gAwIBAgIUBfwvpD2eInaLtUjK4vdTI2T4jwgwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIwMDUxMDAzMjIx
OVoXDTQ5MTIzMTIzNTk1OVowFjEUMBIGA1UEAxMLTUtSV0lGSTEwMTAwWTATBgcq
hkjOPQIBBggqhkjOPQMBBwNCAATZVRW8P+cpOr94gkrr2Vzc3V0TV8O3se+hOGxB
K95rgEO9vpBMQCYmofdYWot8+t65V6WnGX2ij54Q+QRwqsPFo2AwXjAfBgNVHSME
GDAWgBRnowzchseagmniaCpadQeZxxK2wjAdBgNVHQ4EFgQU99XaLvTeIePlXjwu
uRGA2fJ32yAwDAYDVR0TAQH/BAIwADAOBgNVHQ8BAf8EBAMCB4AwDQYJKoZIhvcN
AQELBQADggEBAGLtC9LMzlDjR2nLJtGaIZYMxblhNGQj9V2Dbb2gKyUWtDYkTjpj
ZyeWC57t5avi0tlGVfenNDRUtnGaEVScomwDzlyLxt+MEw4yiouev3hI3QBtVlPt
7Zc/VCqPc9LQvLjBifYR09qGywlTIBUEGXgPQkGVy3nWtuqz52ZO6MmXecWLWH89
jBXw8P/6tcmk8D8/Biw990wJOfDEZjUcDskUKQrJJhPNLHTnyP6gViX0qd+fCvhd
9wL44HlryloBAWQhVGXRVB1GbSzZPJgzunmbdao/eHMSaZgolRJ4TYx8dXxy6Blr
W68EvrID2V0Hha1V2YT5MDGpxz6oAfb/29M=
-----END CERTIFICATE-----
)";

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16

static const unsigned char PROGMEM logo16_glcd_bmp[] = { B00000000, B11000000, B00000001, B11000000, B00000001, B11000000, B00000011, B11100000, B11110011, B11100000, B11111110, B11111000, B01111110, B11111111, B00110011, B10011111, B00011111, B11111100, B00001101, B01110000, B00011011, B10100000, B00111111, B11100000, B00111111, B11110000, B01111100, B11110000, B01110000, B01110000, B00000000, B00110000 };


#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000


/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = SECRET_CERTIFICATE;

WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

unsigned long lastMillis = 0;

void setup() {
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3D (for the 128x64) // init done // Show image buffer on the display hardware.
  display.display(); //delay(2000);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.clearDisplay();
  display.println("Starting Up...");
  
  // invert the display
  
  display.invertDisplay(true);
  display.invertDisplay(false);
  display.display();
  display.clearDisplay();

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  // Set a callback to get the current time
  // used to validate the servers certificate
  ArduinoBearSSL.onGetTime(getTime);

  // Set the ECCX08 slot to use for the private key
  // and the accompanying public certificate for it
  sslClient.setEccSlot(0, certificate);

  // Optional, set the client id used for MQTT,
  // each device that is connected to the broker
  // must have a unique client id. The MQTTClient will generate
  // a client id for you based on the millis() value if not set
  //
  // mqttClient.setId("clientId");

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  mqttClient.onMessage(onMessageReceived);
  
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  mqttClient.poll();

  // publish a message roughly every 5 seconds.
  if (millis() - lastMillis > 10000) {
    lastMillis = millis();

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(32, 0);
    display.clearDisplay();
    publishMessage();
    display.display();
    display.clearDisplay();

  }

}

unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}

void connectWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();
  display.setCursor(10, 0);
  display.println("You're connected to the network");
  display.display();
  delay(2000);
  
}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();
  display.clearDisplay();
  display.setCursor(10, 0);
  display.println("You're connected to the MQTT broker");
  display.display();
  delay(2000);
  

  // subscribe to a topic
  mqttClient.subscribe("arduino/incoming");
}

void publishMessage() {
  Serial.println("Publishing message");

  // send message, the Print interface can be used to set the message contents
  mqttClient.beginMessage("arduino/outgoing");
  mqttClient.print("hello ");
  mqttClient.print(millis());
  mqttClient.endMessage();
}

void onMessageReceived(int messageSize) {
  // we received a message, print out the topic and contents 
  Serial.print(messageSize);
  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    display.print((char)mqttClient.read());
    display.display();
  }

}
