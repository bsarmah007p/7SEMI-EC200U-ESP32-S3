#include "DHT.h"
#define SerialMon Serial
HardwareSerial SerialAT(1);
#define RXD1 12
#define TXD1 13
#define MODEM_BAUDRATE 115200
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const char apn[] = "airtelgprs.com";  // Replace with your APN
const char user[] = "";               // Replace with APN username (if any)
const char pass[] = "";               // Replace with APN password (if any)
const char mqttServer[] = "io.adafruit.com";
const int mqttPort = 1883;
const char mqttUser[] = "USER-NAME"; 
const char mqttPassword[] = "AIO-KEY";
const char mqttPublishTopic[] = "USER-NAME/feeds/TOPIC-NAME";

unsigned long lastPublishTime = 0;            // Tracks the last publish time
const unsigned long publishInterval = 15000;  // Publish interval (5 seconds)

void setup() {
  SerialMon.begin(115200);
  while (!SerialMon);
  SerialAT.begin(115200, SERIAL_8N1, RXD1, TXD1);
  SerialMon.println("Initializing DHT22 sensor...");
  dht.begin();
  delay(2000);
  SerialMon.println("Initializing modem...");
  modem_init();  // Send initialization commands
  // Ensure no previous MQTT connections exist
  SerialMon.println("Disconnecting previous MQTT sessions...");
  sendATCommand("AT+QMTDISC=0");                                                                // Disconnect MQTT session (if any)
  sendATCommand("AT+QMTOPEN=0,\"" + String(mqttServer) + "\"," + String(mqttPort));             // Open MQTT connection
  checkResponse("AT+QMTCONN=0,\"123\",\"" + String(mqttUser) + "\",\"" + mqttPassword + "\"");  // Connect to MQTT broker
}

void modem_init() {
  sendATCommand("AT");         // Basic AT command to check communication
  sendATCommand("ATE0");       // Disable echo for cleaner responses
  sendATCommand("AT+CPIN?");   // Check SIM status
  sendATCommand("AT+CSQ");     // Check signal quality
  sendATCommand("AT+CREG?");   // Check network registration status
  sendATCommand("AT+CGATT?");  // Check if GPRS is attached
}

void sendATCommand(const String& command) {
  SerialMon.print("Sending: ");
  SerialMon.println(command);
  SerialAT.println(command);
  delay(2000);  // Minimal delay to allow command processing
  while (SerialAT.available()) {
    String response = SerialAT.readString();
    SerialMon.println("Response: " + response);
  }
}

void checkResponse(const String& command) {
  SerialMon.print("Sending: ");
  SerialMon.println(command);
  SerialAT.println(command);
  delay(2000);  // Minimal delay to allow command processing
  while (SerialAT.available()) {
    String response = SerialAT.readString();
    SerialMon.print("Response: " + response);
    if (response.indexOf("ERROR") != -1) {
      SerialMon.println("Error in response!");
      while (true)
        ;  // Halt on error
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();
  // Check if it's time to publish data
  if (currentMillis - lastPublishTime >= publishInterval) {
    publishData();
    lastPublishTime = currentMillis;
  }
}

void publishData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    SerialMon.println("Failed to read from DHT sensor");
    return;
  }
  String payload = "{";
  payload += "\"temperature\":";
  payload += String(temperature, 2);
  payload += ",";
  payload += "\"humidity\":";
  payload += String(humidity, 2);
  payload += "}";
  SerialMon.print(millis());
  SerialMon.print(" - Publishing data: ");
  SerialMon.println(payload);

  sendATCommand(String("AT+QMTPUB=0,0,0,0,\"") + mqttPublishTopic + "\"");
  SerialAT.print(payload);
  SerialAT.write(0x1A);  // End of input with Ctrl+Z
}