#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>

#define CONFIG_DEVICE_TYPE "esp-gizmo-ir"

void config_init();

const char* config_get_name();
const char* config_get_location();

const char* config_get_ssid();
const char* config_get_ssid_pass();

const char* config_get_mqtt_host();
const int config_get_mqtt_port();

#endif // __CONFIG_H__
