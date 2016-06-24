#include "app_config.h"
#include "esp_config.h"
#include "ssid_config.h"
#include "FreeRTOS.h"
#include "task.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "httpd.h"

static const char *name = "name_0";
static const char *location = "room_0";
static const char *ssid = WIFI_SSID;
static const char *ssid_pass = WIFI_PASS;
static const char *mqtt_host = "162.243.215.71";
static const int mqtt_port = 1883;

static char *cmd_topic;
static char *status_topic;


typedef enum {
    CONFIG_NAME = 0,
    CONFIG_LOCATION,
    CONFIG_SSID,
    CONFIG_SSID_PASS,
    CONFIG_MQTT_HOST,
    CONFIG_MQTT_PORT,

    CONFIG_ID_SIZE
} ConfigItemId;


ConfigItem config[CONFIG_ID_SIZE] = {
    /* ID | pointer to data | lenght */
    {CONFIG_NAME, 0, 0},
    {CONFIG_LOCATION, 0, 0},
    {CONFIG_SSID, 0, 0},
    {CONFIG_SSID_PASS, 0, 0},
    {CONFIG_MQTT_HOST, 0, 0},
    {CONFIG_MQTT_PORT, 0, 0},
};

void config_test()
{
    for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) {
        char *buff = (char*)malloc(32);
        sprintf(buff, "test %d", i);
        config[i].data = buff;
        config[i].length = strlen(buff) + 1;
    }

    config_write(config, CONFIG_ID_SIZE);

    printf("test reading\n");
    config_free(config, CONFIG_ID_SIZE);

    uint8_t size = config_read(config, CONFIG_ID_SIZE);
    printf("Read %d items\n", size);

    for (uint8_t i = 0; i < size; i++) {
        printf("config id: %d, data: %s, len: %d\n", 
                config[i].id, 
                config[i].data,
                config[i].length);

        free(config[i].data);
        config[i].data = 0;
    }
}

static const char index_html[] = 
#include "index.const"

static void http_req_handler(Httpd *httpd, const char *url, MethodType method)
{
    switch (method) {
        case HTTP_GET:
            printf("Http get request for url %s received\n", url);
            if (!strcmp(url, "/")) {
                httpd_send_header(httpd, true);
                httpd_send_data(httpd, index_html, strlen(index_html));
            } else {
                httpd_send_header(httpd, false);
            }
            break;
        case HTTP_POST:
            printf("Http post request for url %s received\n", url);
            if (!strcmp(url, "/config")) {
                httpd_send_header(httpd, true);
            } else {
                httpd_send_header(httpd, false);
            }
            httpd->user_data = 0;
            break;
        default:
            printf("Unknown method\n");
    }
}

static void http_data_handler(Httpd *httpd, const char *name, 
        const void *data, uint16_t len)
{
    printf("data name=%s, len=%d\n", name, len);
}

static void http_data_complete_handler(Httpd *httpd, bool result)
{
    printf("data transfer complete\n"); 
    if (result) {
        char page[] = "<html><body>\
<h2>Successfuly uploaded.</h2></body></html>";
        httpd_send_data(httpd, page, strlen(page));
    } else {
        char page[] = "<html><body>\
<h2>Fail to upload.</h2></body></html>";
        httpd_send_data(httpd, page, strlen(page));
    }
}
static void httpd_task(void *pvParams)
{
    Httpd httpd;

    httpd.req_handler = http_req_handler;
    httpd.data_handler = http_data_handler;
    httpd.data_complete_handler = http_data_complete_handler;

    httpd_init(&httpd);

    httpd_serve(&httpd, 80);
}

void start_config_server()
{
    xTaskCreate(httpd_task, (signed char *)"httpd", 512, NULL, 2, NULL);
}

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
