#include <stdio.h>
#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"

#include "config.h"

#include "ota-tftp.h"
#include "rboot-api.h"

#include "midea-ir.h"
#include "httpd.h"

#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>

#include "esp/gpio.h"

static MideaIR ir;
static MQTTClient mqtt_client = DefaultClient;


#define STATUS_BUFF_SIZE 32
static inline void publish_status()
{
    char buff[STATUS_BUFF_SIZE];

    memset(buff, 0, STATUS_BUFF_SIZE);

    if (ir.enabled) {
        strcat(buff, "on ");
    } else {
        strcat(buff, "off ");
    }

    switch (ir.mode) {
        case MODE_AUTO:
            strcat(buff, "auto ");
            break;
        case MODE_COOL:
            strcat(buff, "cool ");
            break;
        case MODE_HEAT:
            strcat(buff, "heat ");
            break;
        case MODE_FAN:
            strcat(buff, "fan ");
            break;
    }
    
    sprintf(buff + strlen(buff), "%d %d", ir.temperature, ir.fan_level);
   
    printf("status: %s\n", buff); 

    MQTTMessage message;
    message.payload = buff;
    message.payloadlen = strlen(buff) + 1;
    message.dup = 0;
    message.qos = QOS1;
    message.retained = 0;
    if (MQTTPublish(&mqtt_client, config_get_status_topic(), &message) 
            == SUCCESS ){
        printf("status published\n");
    } else {
        printf("error while publishing message\n");
    }
}

static inline void process_command(const char *cmd)
{
    if (!strcmp(cmd, "on")) {
        ir.enabled = true; 
    } else if (!strcmp(cmd, "off")) {
        ir.enabled = false;
    } else if (!strcmp(cmd, "auto")) {
        ir.mode = MODE_AUTO;
    } else if (!strcmp(cmd, "cool")) {
        ir.mode = MODE_COOL;
    } else if (!strcmp(cmd, "heat")) {
        ir.mode = MODE_HEAT;
    } else if (!strncmp(cmd, "temp", 4)) {
        ir.temperature = atoi(&cmd[5]);
    } else if (!strcmp(cmd, "fan")) {
        ir.mode = MODE_FAN;
    } else if (!strncmp(cmd, "fan_level", 9)) {
        ir.fan_level = atoi(&cmd[10]);
    } else if (!strcmp(cmd, "move")) {
        midea_ir_move_deflector(&ir);
        return;
    } else {
        printf("Unknown  command: %s\n", cmd);
        return;
    }
    printf("Sending ir code\n");
    midea_ir_send(&ir); 
}

#define CMD_BUFF_SIZE 32
static void  topic_received(MessageData *md)
{
    char cmd[CMD_BUFF_SIZE];
    int i;
    MQTTMessage *message = md->message;

    printf("Received topic: ");
    for( i = 0; i < md->topic->lenstring.len; ++i)
        printf("%c", md->topic->lenstring.data[ i ]);

    printf("\ndata: ");
    for( i = 0; i < (int)message->payloadlen; ++i)
        printf("%c", ((char *)(message->payload))[i]);

    printf("\r\n");

    memset(cmd, 0, CMD_BUFF_SIZE);
    memcpy(cmd, message->payload, message->payloadlen);

    process_command(cmd);
    publish_status();
}


static inline void mqtt_task()
{
    struct Network network;
    char client_id[20];
    uint8_t mqtt_buf[100];
    uint8_t mqtt_readbuf[100];
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    NewNetwork( &network );
    strcpy(client_id, "esp-gizmo-ir");

    if (ConnectNetwork(&network, config_get_mqtt_host(),
                config_get_mqtt_port()) != 0) {
        printf("Connect to MQTT server failed\n");
        return;
    }

    NewMQTTClient(&mqtt_client, &network, 5000, mqtt_buf, 100, mqtt_readbuf, 100);

    data.willFlag       = 0;
    data.MQTTVersion    = 3;
    data.clientID.cstring   = client_id;
    data.username.cstring   = NULL;
    data.password.cstring   = NULL;
    data.keepAliveInterval  = 10;
    data.cleansession   = 0;

    printf("Send MQTT connect ... \n");
    if (MQTTConnect(&mqtt_client, &data) != 0) {
        printf("MQTT connect failed\n");
        return;
    }

    const char *topic = config_get_cmd_topic();
    printf("Sibscribe to topic: %s\n", topic);
    if (MQTTSubscribe(&mqtt_client, topic, QOS1, topic_received) != 0) {
        printf("Subscription failed\n");
        return;
    }

    while (MQTTYield(&mqtt_client, 1000) != DISCONNECTED) {
        printf("free heap: %d bytes\n", xPortGetFreeHeapSize());
        uint16_t free_stack = uxTaskGetStackHighWaterMark(xTaskGetCurrentTaskHandle());
        printf("minimum free stack: %d bytes\n", free_stack);
    };

    printf("Connection dropped, request restart\n");
}

static void main_task(void *pvParams)
{
    struct sdk_station_config config;
    strcpy((char*)config.ssid, config_get_ssid());
    strcpy((char*)config.password, config_get_ssid_pass());

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    while (true) {
        uint8_t status = sdk_wifi_station_get_connect_status();
        if (status == STATION_GOT_IP) {
            mqtt_task();
        } else {
            printf("Not connected\n");
            vTaskDelay(1000/portTICK_RATE_MS);
        }
    }
}

static char index_html[] = "\
<html><body><h1>Hello world</h1>\
<form action=\"upload\" method=\"post\" enctype=\"multipart/form-data\">\
Param1 <input type=\"text\" name=\"param_1\"><br>\
Param2 <input type=\"text\" name=\"param_2\"><br>\
Param3 <input type=\"text\" name=\"param_3\"><br>\
<input type=\"file\" name=\"firmware\" id=\"firmware\"><br>\
<input type=\"submit\" value=\"Upload\">\
</form></body></html>\r\n\r\n";

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
            if (!strcmp(url, "/upload")) {
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

static void test_task(void *pvParams)
{
    while (true) {
        vTaskDelay(5000/portTICK_RATE_MS);
        printf("Test config start\n");
        config_test();
        printf("Test config finished\n");
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);

    midea_ir_init(&ir, 14);
    config_init();

    rboot_config conf = rboot_get_config();
    printf("Currently running on flash slot %d / %d\n",
           conf.current_rom, conf.count);

    printf("Image addresses in flash:\n");
    for(int i = 0; i <conf.count; i++) {
        printf("%c%d: offset 0x%08x\n", i == conf.current_rom ? '*':' ', i,
                conf.roms[i]);
    }

    xTaskCreate(main_task, (signed char *)"main", 512, NULL, 4, NULL);
    xTaskCreate(httpd_task, (signed char *)"httpd", 512, NULL, 2, NULL);
    xTaskCreate(test_task, (signed char *)"test", 512, NULL, 2, NULL);

    ota_tftp_init_server(TFTP_PORT);
}
