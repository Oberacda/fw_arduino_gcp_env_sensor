#include <Arduino_JSON.h>
#include <NTPClient.h>

void updateEnvValues(JSONVar* envValues, NTPClient* timeClient);