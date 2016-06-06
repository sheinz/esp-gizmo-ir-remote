#include "config.h"
#include "ssid_config.h"

static const char *name = "name_0";
static const char *location = "room_0";
static const char *ssid = WIFI_SSID;
static const char *ssid_pass = WIFI_PASS;
static const char *mqtt_host = "162.243.215.71";
static const int mqtt_port = 1883;

void config_init()
{
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
