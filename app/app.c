#include <stdio.h>
#include <string.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
#include "ssid_config.h"

#include "ota-tftp.h"
#include "rboot-api.h"

#include "midea-ir.h"

#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>

#include "esp/gpio.h"

#define MQTT_HOST ("162.243.215.71")
#define MQTT_PORT 1883
#define CLIENT_ID "test_client"

MideaIR ir;


static void  topic_received(MessageData *md)
{
    char cmd[20];

    int i;
    MQTTMessage *message = md->message;
    printf("Received: ");
    for( i = 0; i < md->topic->lenstring.len; ++i)
        printf("%c", md->topic->lenstring.data[ i ]);

    printf(" = ");
    for( i = 0; i < (int)message->payloadlen; ++i)
        printf("%c", ((char *)(message->payload))[i]);

    printf("\r\n");

    memset(cmd, 0, 20);
    memcpy(cmd, message->payload, message->payloadlen);

    if (!strcmp(cmd, "on")) {
        ir.enabled = true;
        midea_ir_send(&ir);
    } else if (!strcmp(cmd, "off")) {
        ir.enabled = false; 
        midea_ir_send(&ir);
    } else if (!strcmp(cmd, "move")) {
        midea_ir_move_deflector(&ir);
    } else {
        printf("unknown command\n");
    }
}

static inline void mqtt_task()
{
    struct Network network;
    MQTTClient client   = DefaultClient;
    char client_id[20];
    uint8_t mqtt_buf[100];
    uint8_t mqtt_readbuf[100];
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    NewNetwork( &network );
    strcpy(client_id, "mqtt_example");

    printf("Creating network");
    printf("Network created\n");

    ConnectNetwork(&network, MQTT_HOST, MQTT_PORT);

    NewMQTTClient(&client, &network, 5000, mqtt_buf, 100, mqtt_readbuf, 100);

    printf("created mqtt client\n");

    data.willFlag       = 0;
    data.MQTTVersion    = 3;
    data.clientID.cstring   = client_id;
    data.username.cstring   = NULL;
    data.password.cstring   = NULL;
    data.keepAliveInterval  = 10;
    data.cleansession   = 0;

    printf("Send MQTT connect ... \n");
    MQTTConnect(&client, &data);

    printf("Sibscribe to topic\n");
    MQTTSubscribe(&client, "/esptopic", QOS1, topic_received);

    while(1){
        int ret = MQTTYield(&client, 1000);
        if (ret == DISCONNECTED)
            break;
    }
    printf("Connection dropped, request restart\n\r");
}

void test_task(void *pvParams)
{
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

void user_init(void)
{
    uart_set_baud(0, 115200);

    midea_ir_init(&ir, 14);


    rboot_config conf = rboot_get_config();
    printf("\r\n\r\nOTA Basic demo.\r\nCurrently running on flash slot %d / %d.\r\n\r\n",
           conf.current_rom, conf.count);

    printf("Image addresses in flash:\r\n");
    for(int i = 0; i <conf.count; i++) {
        printf("%c%d: offset 0x%08x\r\n", i == conf.current_rom ? '*':' ', i, conf.roms[i]);
    }

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    xTaskCreate(test_task, (signed char *)"test_task", 1024, NULL, 4, NULL);

    ota_tftp_init_server(TFTP_PORT);
}
