#include "midea-ir.h"
#include <stdio.h>

/**
 * Midea Air conditioner protocol consists of 3 data bytes.
 * After each data byte follows its inverted byte (0 an 1 are switched). It
 * provides errors checking on the receiver side.
 *
 * Each 6 total bytes follows with additional repeat of the same 6 bytes to
 * provide even more errors checking.
 */

/**
 * Bits encoding.
 *
 * Bit 0 is encoded as 1T of high level and 1T of low level.
 * Bit 1 is encoded as 1T of high level and 3T of low level.
 *
 * Start condition: 8T of hight level and 8T of low level.
 * Middle condition (beetween two 6 bytes): 
 * TODO: describe
 */

/**
 * Data packet (3 bytes):
 * [1010 0010] [ffff ssss] [ttttcccc]
 *
 * 1010 0010 - (0xB2) is a constant
 *
 * ffff - Fan control
 *      1001 - low speed
 *      0101 - medium speed
 *      0011 - high speed
 *      1011 - automatic
 *      1110 - off
 *
 * ssss - State control (unconfirmed!)
 *      1111 - on
 *      1011 - off
 *
 * tttt - Temperature control (unconfirmed!)
 *      0000 - 17 Celsius
 *      ...
 *      1011 - 30 Celsius
 *      1110 - off
 *
 * cccc - Command
 *      0000 - cool
 *      1100 - heat
 *      1000 - automatic
 *      1101 - dehumidify
 */

#define TEMP_LOW  17
#define TEMP_HIGH 30


typedef struct 
{
    uint8_t magic;      // 0xB2 always
    uint8_t fan : 4; 
    uint8_t state : 4;
    uint8_t temp : 4;
    uint8_t command : 4;
} DataPacket;


void pack_data(MideaIR *ir, DataPacket *data)
{
    data->magic = 0xB2;
    data->fan = ir->fan_level;
    if (ir->enabled) {
        data->state = 0b1011;  // on
        data->command = ir->mode;
    } else {
        data->state = 0b1111; // off
        data->command = MODE_AUTO;
    }
    if (ir->temperature >= TEMP_LOW && ir->temperature <= TEMP_HIGH) {
        // TODO: check
        data->temp = ir->temperature - TEMP_LOW;
    } else {
        data->temp = 24 - TEMP_LOW;
    }
}

void midea_ir_init(MideaIR *ir)
{
    ir->temperature = 24;
    ir->enabled = false;
    ir->mode = MODE_AUTO;
    ir->fan_level = FAN_AUTO;
}

void print_bit(bool bit)
{
    if (bit) {
        printf("1");
    } else {
        printf("0");
    }
}

void midea_ir_send(MideaIR *ir)
{
    DataPacket packet; 
    pack_data(ir, &packet);
    uint32_t *p = (uint32_t*)&packet;

    printf("Data: ");

    for (uint8_t i = 0; i < 24; i++)
    {
        print_bit(*p & 1);
        *p >>= 1;
    }

    printf("\n");
}

void midea_ir_move_deflector(MideaIR *ir)
{

}
