#include <Arduino.h>
#include <Arduino_JSON.h>
#include <ArduinoECCX08.h>
#include <utility/ECCX08JWS.h>
#include <WiFiNINA.h>

#include "google_cloud.hpp"
#include "gcp_secret.h"

const char broker[]      = "mqtt.googleapis.com";

const char projectId[]   = SECRET_PROJECT_ID;
const char cloudRegion[] = SECRET_CLOUD_REGION;
const char registryId[]  = SECRET_REGISTRY_ID;
const String deviceId    = SECRET_DEVICE_ID;



unsigned long getTime() {
  // get the current time from the WiFi module
  return WiFi.getTime();
}

GoogleCloudMqtt::GoogleCloudMqtt(WiFiSSLClient* wifiSslClientPtr) {
  this->mqttClientPtr = new MqttClient(*wifiSslClientPtr);
  
  // Calculate and set the client id used for MQTT
  String clientId = this->calculateClientId();

  (*this->mqttClientPtr).setId(clientId);
}

String GoogleCloudMqtt::calculateClientId() {
  String clientId;

  // Format:
  //
  //   projects/{project-id}/locations/{cloud-region}/registries/{registry-id}/devices/{device-id}
  //

  clientId += "projects/";
  clientId += projectId;
  clientId += "/locations/";
  clientId += cloudRegion;
  clientId += "/registries/";
  clientId += registryId;
  clientId += "/devices/";
  clientId += deviceId;

  return clientId;
}

String GoogleCloudMqtt::calculateJWT() {
  unsigned long now = getTime();
  
  // calculate the JWT, based on:
  //   https://cloud.google.com/iot/docs/how-tos/credentials/jwts
  JSONVar jwtHeader;
  JSONVar jwtClaim;

  jwtHeader["alg"] = "ES256";
  jwtHeader["typ"] = "JWT";

  jwtClaim["aud"] = projectId;
  jwtClaim["iat"] = now;
  jwtClaim["exp"] = now + (24L * 60L * 60L); // expires in 24 hours 

  return ECCX08JWS.sign(0, JSON.stringify(jwtHeader), JSON.stringify(jwtClaim));
}

void GoogleCloudMqtt::connectMQTT() {
    Serial.print("Attempting to connect to MQTT broker: ");
    Serial.print(broker);
    Serial.println(" ");

  while (!this->mqttClientPtr->connected()) {
    // Calculate the JWT and assign it as the password
    String jwt = calculateJWT();

    this->mqttClientPtr->setUsernamePassword("", jwt);

    if (!this->mqttClientPtr->connect(broker, 8883)) {
      // failed, retry
      Serial.print(".");
      delay(5000);
    }
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to topics
  this->mqttClientPtr->subscribe("/devices/" + deviceId + "/config", 1);
  this->mqttClientPtr->subscribe("/devices/" + deviceId + "/commands/#");
}

void GoogleCloudMqtt::publishStatus(JSONVar* statusDataPtr) {
  String jsonString = JSON.stringify(*statusDataPtr);

  Serial.println("Publishing status message");
  this->mqttClientPtr->beginMessage("/devices/" + deviceId + "/state");
  this->mqttClientPtr->print(jsonString);
  this->mqttClientPtr->endMessage();
}


void GoogleCloudMqtt::publishTelemetry(JSONVar* telemetryDataPtr) {
  
  // send message, the Print interface can be used to set the message contents
  String jsonString = JSON.stringify(*telemetryDataPtr);
  
  this->mqttClientPtr->beginMessage("/devices/" + deviceId + "/events");
  this->mqttClientPtr->print(jsonString);
  this->mqttClientPtr->endMessage();

}

void GoogleCloudMqtt::poll() {
  this->mqttClientPtr->poll();
}

bool GoogleCloudMqtt::connected() {
  return (bool) this->mqttClientPtr->connected();
}

MqttClient* GoogleCloudMqtt::getMqttClient() {
  return this->mqttClientPtr;
}

void GoogleCloudMqtt::onMessageCallback(JSONVar* configuration, size_t messageSize) {
  // we received a message, print out the topic and contents

  String configTopic = String("/devices/" + deviceId + "/config");

  if (configTopic.equals(this->mqttClientPtr->messageTopic())) {
    char* message = new char[messageSize];

    this->mqttClientPtr->readBytes(message, messageSize);
  
    JSONVar new_configuration = JSON.parse(message);

    if (new_configuration.hasOwnProperty("telemetry")) {
      (*configuration)["telemetry"] = (int) new_configuration["telemetry"];
    }
    if (new_configuration.hasOwnProperty("status")) {
      (*configuration)["status"] = (int) new_configuration["status"];
    }
    if (new_configuration.hasOwnProperty("env")) {
      (*configuration)["env"] = (int) new_configuration["env"];
    }
  }
}