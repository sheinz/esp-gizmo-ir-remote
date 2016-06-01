#include <stdio.h>
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
#include "ssid_config.h"

#include "ota-tftp.h"
#include "rboot-api.h"

#include "midea-ir.h"


MideaIR ir;

void test_task(void *pvParams)
{
    while (true) {
        printf("Send IR data");

        midea_ir_send(&ir);

        vTaskDelay(3000);
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);

    midea_ir_init(&ir);

    xTaskCreate(test_task, (signed char *)"test_task", 256, NULL, 2, NULL);

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

    ota_tftp_init_server(TFTP_PORT);
}
