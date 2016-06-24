/**
 * The file implements reading/writing configuration data from/to flash.
 *
 */
#ifndef __ESP_CONFIG_H__
#define __ESP_CONFIG_H__

#include <stdint.h>

#define CONFIG_FLASH_BASE_ADDR   0xE0000
#define CONFIG_FLASH_SIZE        0x1FFFF

typedef struct {
    uint8_t id; 
    char *data;
    uint8_t length;
} ConfigItem;

/**
 * Write array of config items to the flash.
 */
void config_write(ConfigItem *items, uint8_t size);

/**
 * Read config items from the flash. Read max 'size' items.
 * For each item the function will allocate memory for the data
 * and writes its address to items[n].data.
 * It will not free already allocated memory in items[n].data, so
 * make sure data pointer is set to null.
 *
 * Return the number of items read.
 */
uint8_t config_read(ConfigItem *items, uint8_t size);

/**
 * Free the memory that was allocated during reading.
 * This function also sets items[n].data to 0.
 */
void config_free(ConfigItem *items, uint8_t size);


#endif // __ESP_CONFIG_H__
