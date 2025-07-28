#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
HardwareSerial SerialAT(1);
#define SerialMon Serial
#define MODEM_BAUDRATE 115200
#define RXD1 12
#define TXD1 13

const char apn[] = "airtelgprs.com";  // Replace with your APN
const char user[] = "";               // Replace with APN username (if any)
const char pass[] = "";
const char mqttServer[] = "io.adafruit.com";
const int mqttPort = 1883;
const char mqttUser[] = "USER-NAME";
const char mqttPassword[] = "AIO-KEY";
const char mqttPublishTopic[] = "USER-NAME/feeds/TOPIC-NAME";

unsigned long lastPublishTime = 0;            // Tracks the last publish time
const unsigned long publishInterval = 15000;

#define DS18B20A_PIN 7
OneWire oneWireA(DS18B20A_PIN);
DallasTemperature ds18b20a(&oneWireA);

void setup() {
  Serial.begin(115200); 
  while(!SerialMon);
  SerialAT.begin(115200, SERIAL_8N1, RXD1, TXD1);
  SerialMon.println("Initial the Sensor...");
  ds18b20a.begin();
  delay(1000);
  SerialMon.println("Initializing modem...");
  modem_init();
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
      while (true);  // Halt on error
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishTime >= publishInterval) {
    publishData();
    lastPublishTime = currentMillis;
  }
}

void publishData() {
  ds18b20a.requestTemperatures();  
  float dsTempA = ds18b20a.getTempCByIndex(0);
  String payload = "{";
  payload += "\"ds18b20a_temp\":";
  payload += String(dsTempA, 2);
  payload += "}";
  SerialMon.print(millis());
  SerialMon.print(" - Publishing data: ");
  Serial.println(payload);
  delay(2000);
  sendATCommand(String("AT+QMTPUB=0,0,0,0,\"") + mqttPublishTopic + "\"");
  SerialAT.print(payload);
  SerialAT.write(0x1A);
}