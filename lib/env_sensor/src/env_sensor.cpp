#include <Arduino_MKRENV.h>

#include "env_sensor.hpp"

void updateEnvValues(JSONVar* envValues, NTPClient* timeClient) {
  (*envValues)["timestamp"] = (*timeClient).getEpochTime();
  (*envValues)["temperature"] = float(ENV.readTemperature());
  (*envValues)["humidity"] = float(ENV.readHumidity());
  (*envValues)["pressure"] = float(ENV.readPressure());
  (*envValues)["illuminance"] = float(ENV.readIlluminance());
  (*envValues)["uva"] = float(ENV.readUVA());
  (*envValues)["uvb"] = float(ENV.readUVB());
  (*envValues)["uv_index"] = float(ENV.readUVIndex());
}