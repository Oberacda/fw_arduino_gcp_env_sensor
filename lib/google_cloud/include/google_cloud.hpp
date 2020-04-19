#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <Arduino_JSON.h>
#include <NTPClient.h>

class GoogleCloudMqtt {
    private:
    MqttClient* mqttClientPtr;

    String calculateClientId();

    String calculateJWT();

    public:
    GoogleCloudMqtt(WiFiSSLClient* wifiSslClientPtr);
    
    void connectMQTT();

    void publishStatus(JSONVar* statusDataPtr);

    void publishTelemetry(JSONVar* telemetryDataPtr);

    void poll();

    bool connected();

    MqttClient* getMqttClient();

    void onMessageCallback(JSONVar* configuration, size_t messageSize);
}; 





