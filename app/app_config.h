#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include <stdint.h>

#define CONFIG_DEVICE_TYPE "esp-gizmo-ir"

void config_init();

void config_test();

void start_config_server();

const char* config_get_name();
const char* config_get_location();

const char* config_get_ssid();
const char* config_get_ssid_pass();

const char* config_get_mqtt_host();
const int config_get_mqtt_port();

const char* config_get_status_topic();
const char* config_get_cmd_topic();

#endif // __APP_CONFIG_H__
