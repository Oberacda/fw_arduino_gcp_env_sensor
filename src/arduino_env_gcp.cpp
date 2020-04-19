/*
  GCP (Google Cloud Platform) IoT Core WiFi

  This sketch securely connects to GCP IoT Core using MQTT over WiFi.
  It uses a private key stored in the ATECC508A and a JSON Web Token (JWT) with
  a JSON Web Signature (JWS).

  It publishes a message every 5 seconds to "/devices/{deviceId}/state" topic
  and subscribes to messages on the "/devices/{deviceId}/config" and
  "/devices/{deviceId}/commands/#" topics.

  The circuit:
  - Arduino MKR WiFi 1010 or MKR1000

  This example code is in the public domain.
*/

#include <Arduino.h>
#include <ArduinoECCX08.h>
#include <utility/ECCX08JWS.h>
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include <Arduino_MKRENV.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000
#include <WiFiUdp.h>
#include <NTPClient.h>

#include <google_cloud.hpp>
#include <env_sensor.hpp>

#include "arduino_secrets.h"

/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;


WiFiSSLClient wifiSslClient;

WiFiUDP       ntpUDP;
NTPClient     timeClient(ntpUDP, "de.pool.ntp.org");

JSONVar       json_telemetry;
JSONVar       json_status;
JSONVar       configuration;

GoogleCloudMqtt* googleMqttClientPtr;

long lastMillisEnv = 0;
long lastMillisState = 0;
long lastMillisTelemetry = 0;

bool configured = false;

void setup();
void loop();

void connectWiFi();
void onMessageRecieved(int size);


void setup() {
  configuration["env"] = 60000;
  configuration["status"] = 600000;
  configuration["telemetry"] = 600000;

  Serial.begin(9600);
  while (!Serial);

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  if (!ENV.begin()) {
    Serial.println("Failed to initialize MKR ENV shield!");
    while(1);
  }

  googleMqttClientPtr = new GoogleCloudMqtt(&wifiSslClient);
  googleMqttClientPtr->getMqttClient()->onMessage(onMessageRecieved);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    timeClient.begin();
  }
  
  if (!googleMqttClientPtr->connected()) {
    // MQTT client is disconnected, connect
    googleMqttClientPtr->connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  googleMqttClientPtr->poll();
  timeClient.update();

  // Update environemental data in the storage every 5 seconds.
  if((long) (millis() - lastMillisEnv) > (long) configuration["env"]) {
    lastMillisEnv = millis();
     updateEnvValues(&json_telemetry, &timeClient);
  }


  // Publish a status update every 5 min.
  if((long) (millis() - lastMillisState) > (long) configuration["status"]) {
    lastMillisState = millis();
    json_status["timestamp"] = timeClient.getEpochTime();
    json_status["next_telemetry_update"] = (long) configuration["telemetry"] - (long) (millis() - lastMillisTelemetry) ;

    googleMqttClientPtr->publishStatus(&json_status);
  }

  // Publish a telemetry update every 30 min.
  if ((long) (millis() - lastMillisTelemetry) >  (long) configuration["telemetry"]) {
    lastMillisTelemetry = millis();
    googleMqttClientPtr->publishTelemetry(&json_telemetry);
  }
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

  Serial.println("You're connected to the network");
  Serial.println();
}

void onMessageRecieved(int size) {
  googleMqttClientPtr->onMessageCallback(&configuration, size);
}