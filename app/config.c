#include "config.h"
#include "ssid_config.h"
#include <string.h>
#include <stdlib.h>

static const char *name = "name_0";
static const char *location = "room_0";
static const char *ssid = WIFI_SSID;
static const char *ssid_pass = WIFI_PASS;
static const char *mqtt_host = "162.243.215.71";
static const int mqtt_port = 1883;

static char *cmd_topic;
static char *status_topic;

static inline void fill_topic_base(char *str_buff)
{
    strcpy(str_buff, "/");
    strcat(str_buff, config_get_location());
    strcat(str_buff, "/");
    strcat(str_buff, CONFIG_DEVICE_TYPE);
    strcat(str_buff, "/");
    strcat(str_buff, config_get_name());
    strcat(str_buff, "/");
}

void config_init()
{
    uint8_t base_size = strlen(config_get_location()) + sizeof(CONFIG_DEVICE_TYPE) +
        strlen(config_get_name()) + 4;

    cmd_topic = (char*)malloc(base_size + sizeof("cmd"));
    status_topic = (char*)malloc(base_size + sizeof("status"));

    fill_topic_base(cmd_topic);
    strcat(cmd_topic, "cmd");

    fill_topic_base(status_topic);
    strcat(status_topic, "status");
}

const char* config_get_name()
{
    return name;
}

const char* config_get_location()
{
    return location;
}

const char* config_get_ssid()
{
    return ssid;
}

const char* config_get_ssid_pass()
{
    return ssid_pass;
}

const char* config_get_mqtt_host()
{
    return mqtt_host;
}

const int config_get_mqtt_port()
{
    return mqtt_port;
}

const char* config_get_status_topic()
{
    return status_topic;
}

const char* config_get_cmd_topic()
{
    return cmd_topic;
}
