#include <ArduinoJson.h>
#include <HardwareSerial.h>
#define RXD1 12
#define TXD1 13
#define MODEM_BAUDRATE 115200
HardwareSerial SerialAT(1);
const int TURBIDITY_ANALOG_PIN = 7;
#define LED_PIN 2

const char apn[] = "airtelgprs.com";
const char user[] = "";
const char pass[] = "";

const char mqttServer[] = "io.adafruit.com";
const int mqttPort = 1883;
const char mqttUser[] = "USER-NAME";
const char mqttPassword[] = "AIO-KEY";
const char mqttPublishTopic[] = "USER-NAME/feeds/TOPIC-NAME";

unsigned long lastPublishTime = 0;
const unsigned long publishInterval = 5000;

String sendATCommand(const String &command, unsigned long timeout) {
  String response = "";
  while (SerialAT.available()) {
    SerialAT.read();
  }

  SerialAT.println(command);
  Serial.print("\n> Sending AT Command: ");
  Serial.println(command);

  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    while (SerialAT.available()) {
      char c = (char)SerialAT.read();
      response += c;
    }
  }
  response.trim();

  Serial.print("< Modem Response: ");
  Serial.println(response);
  if (response.indexOf("ERROR") != -1) {
    Serial.println("!!! ERROR detected in response for command: " + command);
  } else if (response.indexOf("OK") == -1 && command.startsWith("AT+")) {
    Serial.println("!!! 'OK' not found in response for command: " + command);
  }
  return response;
}

void modem_init() {
  Serial.println("\n--- Initializing EC200U Modem ---");
  sendATCommand("AT", 2000);
  sendATCommand("ATE0", 2000);
  sendATCommand("AT+CPIN?", 2000);
  sendATCommand("AT+CSQ", 2000);
  sendATCommand("AT+CREG?", 2000);

  Serial.println("\nSetting APN and activating PDP context...");
  sendATCommand(String("AT+QICSGP=1,1,\"") + apn + "\",\"" + user + "\",\"" + pass + "\"", 5000);
  sendATCommand("AT+QIACT=1", 15000);
  sendATCommand("AT+CGATT?", 2000);

  Serial.println("\n--- Initializing MQTT Connection ---");
  sendATCommand("AT+QMTDISC=0", 5000);
  sendATCommand("AT+QMTOPEN=0,\"" + String(mqttServer) + "\"," + String(mqttPort), 10000);
  sendATCommand("AT+QMTCONN=0,\"ESP32-Turbidity-Client\",\"" + String(mqttUser) + "\",\"" + mqttPassword + "\"", 10000);
  Serial.println("\nModem and MQTT setup completed.");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  SerialAT.begin(MODEM_BAUDRATE, SERIAL_8N1, RXD1, TXD1);
  Serial.println("\n--- Initializing Turbidity Sensor ---");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  modem_init();
  digitalWrite(LED_PIN, LOW);
  Serial.println("\n--- Turbidity Sensor & Communication Setup Completed ---");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishTime >= publishInterval) {
    publishData();
    lastPublishTime = currentMillis;
  }
  delay(10);
}
void publishData() {
  Serial.println("\n--- Reading Turbidity Sensor Data ---");
  int rawAnalogValue = analogRead(TURBIDITY_ANALOG_PIN);
  float turbidityNTU = map(rawAnalogValue, 0, 4095, 0, 1000);
  Serial.print("Turbidity Raw: "); Serial.print(rawAnalogValue);
  Serial.print(" | Turbidity (NTU): "); Serial.println(turbidityNTU);
  const size_t capacity = JSON_OBJECT_SIZE(2);
  DynamicJsonDocument doc(capacity);
  doc["turbidity_raw"] = rawAnalogValue;
  doc["turbidity_ntu"] = String(turbidityNTU, 2);

  String payload;
  serializeJson(doc, payload);
  Serial.print("\n--- Publishing Data to MQTT: ");
  Serial.println(payload);
  String pubCommand = String("AT+QMTPUB=0,0,0,0,\"" + String(mqttPublishTopic) + "\"," + String(payload.length()));
  sendATCommand(pubCommand, 2000);

  SerialAT.print(payload);
  SerialAT.write(0x1A);
  delay(1000);

  String responseAfterPayload = sendATCommand("", 2000);
  if (responseAfterPayload.indexOf("OK") != -1) {
    Serial.println("MQTT Publish successful!");
  } else {
    Serial.println("MQTT Publish failed or no OK response after payload.");
  }
}
