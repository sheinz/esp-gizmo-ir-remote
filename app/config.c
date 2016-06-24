/**
 *
 * TODO: implement some kind of data integrity checking
 * For example each config item might contain a lenght and a check sum of
 * the data. Lenght is 2 bytes and checksum is 2 bytes. So, the first 4 bytes 
 * of config item will contain service information.
 *
 */
#include "config.h"
#include "ssid_config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <espressif/spi_flash.h>

static const char *name = "name_0";
static const char *location = "room_0";
static const char *ssid = WIFI_SSID;
static const char *ssid_pass = WIFI_PASS;
static const char *mqtt_host = "162.243.215.71";
static const int mqtt_port = 1883;

static char *cmd_topic;
static char *status_topic;

// config start address in the SPI flash
#define CONFIG_BASE_ADDR 0xE0000   // 127k till the end of flash

typedef enum {
    CONFIG_NAME = 0,
    CONFIG_LOCATION,
    CONFIG_SSID,
    CONFIG_SSID_PASS,
    CONFIG_MQTT_HOST,
    CONFIG_MQTT_PORT,

    CONFIG_ID_SIZE
} ConfigItemId;

typedef struct {
    ConfigItemId id; 
    char *data;
    uint8_t max_length;
} ConfigItem;


ConfigItem config[CONFIG_ID_SIZE] = {
    /* ID | pointer to data | max_lenght */
    {CONFIG_NAME, 0, 32},
    {CONFIG_LOCATION, 0, 64},
    {CONFIG_SSID, 0, 16},
    {CONFIG_SSID_PASS, 0, 8},
    {CONFIG_MQTT_HOST, 0, 39},
    {CONFIG_MQTT_PORT, 0, 41},
};

static inline uint8_t allign_size(uint8_t size)
{
    if (size % 4) {
        return (size / 4 + 1) * 4;
    } else {
        return size;
    }
}

static inline uint8_t max_config_item_length()
{
    uint8_t max_length = 0;
    for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) {
        if (config[i].max_length > max_length) {
            max_length = config[i].max_length;
        }
    } 
    return max_length;
}

/**
 * Read and write operations with SPI flash requires buffer address and size
 * to be 4-bytes alligned. The following two functions 'alligned_malloc' and
 * 'alligned_free' are return 4-byte alligned buffers.
 * The testing discovered that malloc already return 4-bytes alligned buffer.
 * So, additional allignment are not necessary.
 */

/**
 * Allocate memory and return 4-bytes alligned address pointer
 * Return alignment offset in offset.
 */
static inline void *alligned_malloc(size_t size, uint8_t *offset)
{
    char *buff = (char*)malloc(size + 3);  // need extra 3 bytes to allign buffer
    
    *offset = (uint32_t)buff % 4;
    if (*offset) {
        printf("malloc misalligned\n");
    }
    *offset = (4 - *offset);

    return buff + *offset;
}

/**
 * Free 4-bytes alligned memory with given offset.
 */
static inline void alligned_free(void *p, uint8_t offset)
{
    free((char*)p - offset);
}

void config_read()
{
    uint32_t addr = CONFIG_BASE_ADDR;
    uint8_t offset;
    // get buffer that fits each of config items
    char *buff = (char*)alligned_malloc(allign_size(max_config_item_length()), 
            &offset);

    for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) {
        uint8_t config_length = allign_size(config[i].max_length);
        if (sdk_spi_flash_read(addr, buff, config_length) 
                != SPI_FLASH_RESULT_OK) {
            printf("SPI flash read error\n");
        }
        printf("read buff: %s\n", buff);

        if (config[i].data) {
           free(config[i].data); 
        }
        uint8_t data_length = strlen(buff);
        if (data_length > config[i].max_length) {
            data_length = config[i].max_length;
        }
        config[i].data = (char*)malloc(data_length + 1);
        strncpy(config[i].data, buff, strlen(buff) + 1);
        memcpy(config[i].data, buff, data_length);
        config[i].data[data_length] = '\0';
        printf("copied to data: %s\n", config[i].data);
        addr += config_length;
    }
    alligned_free(buff, offset);
}

void config_write()
{
    uint32_t addr = CONFIG_BASE_ADDR;
    sdk_spi_flash_erase_sector(addr / SPI_FLASH_SEC_SIZE);

    for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) {
        uint8_t buff_size = allign_size(config[i].max_length);
        uint8_t offset;
        char *buff = (char*)alligned_malloc(buff_size, &offset);
        memset(buff, 0, buff_size);
        if (config[i].data) {
            strncpy(buff, config[i].data, config[i].max_length);
        } 
        printf("writing flash, addr=%x, size=%d\n", addr, buff_size);
        printf("buff: %s\n", buff);
        if (sdk_spi_flash_write(addr, buff, buff_size) != SPI_FLASH_RESULT_OK) {
            printf("SPI write error\n");
        }
        alligned_free(buff, offset);
        addr += buff_size;
    }
}

void config_test()
{
    for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) {
        char *buff = (char*)malloc(32);
        sprintf(buff, "test %d", i);
        config[i].data = buff;
    }

    config_write();

    printf("test reading\n");
    for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) {
        free(config[i].data);
        config[i].data = 0;
    }

    config_read();

    for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) {
        printf("config %d, data: %s\n", i, config[i].data);
        free(config[i].data);
        config[i].data = 0;
    }
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
