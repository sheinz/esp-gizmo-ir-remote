/**
 * This file implements configuration data storage in the SPI flash.
 *
 * Config data structure in the flash.
 *
 *  2 bytes   2 bytes             length bytes
 * +--------+--------+-------------------------------------------------+
 * | 0xAA55 | length |  1 byte  1 byte   2 bytes    length bytes       |
 * |        |        | +-------+--------+----------+------------+      |
 * |        |        | | id    | length | checksum |    data    | ...  |
 * |        |        | +-------+--------+----------+------------+      |
 * +--------+--------+-------------------------------------------------+
 *
 * The main header consist of 2 bytes signature (0xAA55), 2 bytes length of
 * all the data and the data structures itself.
 * The length in the main header indicate the total size of the following data
 * structures in bytes, 4 bytes alligned.
 * Each data structure has its own header. This header consist of id, length of
 * the data, checksum and the data itself.
 * The checksum is calculated only for data.
 * The length might be not 4 bytes alligned but the real data length will be
 * 4 bytes alligned. So, if length=5, the data length will be 8 but only 5 bytes
 * of meaning data.
 * If during reading checksum doesn't much the further processing is stopped and
 * all the data is discarded.
 *
 * For example:
 * +-------+-------+----+----+-------+----------+
 * | AA 55 | 0C 00 | 01 | 05 | XX XX | "test\0" |
 * +-------+-------+----+----+-------+----------+
 * where XX XX - checksum of "test\0" (5 bytes)
 * 0C - 12 bytes of data items (4 byte header + 8 bytes data 
 *                                        (8 is 5 with 4 bytes allignment))
 *
 *  4 bytes allignment is necessary because read/write operations with flash
 *  is 4-bytes alligned.
 */
#include "esp_config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <espressif/spi_flash.h>

#define SIGNATURE_CONST 0xAA55

typedef struct {
    uint16_t signature; 
    uint16_t length;
} MainHeader;

typedef struct {
    uint8_t id;
    uint8_t length;
    uint16_t checksum;
    char data[];    
} DataHeader;


static inline uint8_t allign_size(uint8_t size)
{
    if (size % 4) {
        return (size / 4 + 1) * 4;
    } else {
        return size;
    }
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

static uint16_t calculate_checksum(const char *data, uint8_t size)
{
    // TODO: implmenet
    return 0;
}

static inline uint16_t read_main_header()
{
    uint16_t length = 0;
    uint8_t offset;
    MainHeader *header = (MainHeader*)alligned_malloc(
            sizeof(MainHeader), &offset);

    if (sdk_spi_flash_read(CONFIG_FLASH_BASE_ADDR, header, sizeof(MainHeader)) 
            != SPI_FLASH_RESULT_OK) {
        printf("SPI flash read error\n");
    }

    if (header->signature == SIGNATURE_CONST) {
        length = header->length; 
    } else {
        printf("Signature mismatch\n");
    }

    alligned_free(header, offset);
    return length;
}

/**
 * Read data header and write to item.
 * Return checksum.
 */
inline static uint16_t read_data_header(uint32_t addr, ConfigItem *item)
{
    uint16_t checksum;
    uint8_t offset;
    DataHeader *header = (DataHeader*)alligned_malloc(sizeof(DataHeader), &offset);

    if (sdk_spi_flash_read(addr, header, sizeof(DataHeader)) 
            != SPI_FLASH_RESULT_OK) {
        printf("SPI flash read error\n");
    }
    item->id = header->id;
    item->length = header->length;
    checksum = header->checksum;

    alligned_free(header, offset);

    return  checksum;
}

/**
 * Read config item from flash and store it in 'item'
 * Return number of read bytes from flash.
 */
uint8_t read_data_item(uint32_t addr, ConfigItem *item)
{
    uint16_t checksum = read_data_header(addr, item);

    uint8_t buff_size = allign_size(item->length);
    uint8_t offset;

    char *buff = (char*)alligned_malloc(buff_size, &offset);
    if (sdk_spi_flash_read(addr + sizeof(DataHeader), buff, buff_size) 
            != SPI_FLASH_RESULT_OK) {
        printf("SPI flash read error\n");
        item->length = 0;
    }

    if (checksum == calculate_checksum(buff, item->length)) {
        item->data = (char*)malloc(item->length); 
        memcpy(item->data, buff, item->length);
    } else {
        printf("Checksum mismatch\n");
        item->length = 0;
    }

    alligned_free(buff, offset);
    if (item->length) {
        return buff_size + sizeof(DataHeader); 
    } else {
        return 0;  // 0 indicate an error
    }
}

uint8_t config_read(ConfigItem *items, uint8_t size)
{
    uint32_t addr = CONFIG_FLASH_BASE_ADDR + sizeof(MainHeader);
    uint16_t data_length = read_main_header();
    uint8_t counter = 0;

    for (uint8_t i = 0; i < size; i++) {
        uint8_t read_bytes = read_data_item(addr, &items[i]);
        if (read_bytes) {
            if (data_length < read_bytes) {
                printf("Read more data than specified in header\n");
                break; 
            } else {
                data_length -= read_bytes;
            }

            addr += read_bytes;
            counter++;
        } else {
            break;  // something wrong
        }
    }
    if (data_length) {  // not all data is read
        printf("There's still data in the flash\n");
    }
    return counter;
}

static inline uint16_t total_data_length(ConfigItem *items, uint8_t size)
{
    uint16_t total = 0;
    for (uint8_t i = 0; i < size; i++) {
        total += sizeof(DataHeader) + allign_size(items[i].length);
    }
    return total;
}

static inline void write_main_header(uint16_t length)
{
    uint8_t offset;
    MainHeader *header = (MainHeader*)alligned_malloc(
            sizeof(MainHeader), &offset);

    header->signature = SIGNATURE_CONST;
    header->length = length;

    sdk_spi_flash_write(CONFIG_FLASH_BASE_ADDR, header, sizeof(header));

    alligned_free(header, offset);
}


static inline uint8_t write_data_item(uint32_t addr, ConfigItem *item)
{
    uint8_t buff_size = allign_size(item->length) + sizeof(DataHeader);
    uint8_t offset;
    DataHeader *header = (DataHeader*)alligned_malloc(buff_size, &offset);
    header->id = item->id;
    header->length = item->length;
    header->checksum = calculate_checksum(item->data, item->length);
    memcpy(header->data, item->data, item->length);

    if (sdk_spi_flash_write(addr, header, buff_size) != SPI_FLASH_RESULT_OK) {
        printf("SPI write error\n");
    }

    alligned_free(header, offset);
    return buff_size;
}

void config_write(ConfigItem *items, uint8_t size)
{
    uint32_t addr = CONFIG_FLASH_BASE_ADDR + sizeof(MainHeader);
    sdk_spi_flash_erase_sector(addr / SPI_FLASH_SEC_SIZE);

    uint16_t length = total_data_length(items, size);
    write_main_header(length);

    for (uint8_t i = 0; i < size; i++) {
        addr += write_data_item(addr, &items[i]);
    }
}

void config_free(ConfigItem *items, uint8_t size)
{
    for (uint8_t i = 0; i < size; i++) {
        free(items[i].data);
        items[i].data = 0;
    }
}
    
/* void config_test() */
/* { */
/*     for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) { */
/*         char *buff = (char*)malloc(32); */
/*         sprintf(buff, "test %d", i); */
/*         config[i].data = buff; */
/*     } */
/*  */
/*     config_write(); */
/*  */
/*     printf("test reading\n"); */
/*     for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) { */
/*         free(config[i].data); */
/*         config[i].data = 0; */
/*     } */
/*  */
/*     config_read(); */
/*  */
/*     for (uint8_t i = 0; i < CONFIG_ID_SIZE; i++) { */
/*         printf("config %d, data: %s\n", i, config[i].data); */
/*         free(config[i].data); */
/*         config[i].data = 0; */
/*     } */
/* } */

