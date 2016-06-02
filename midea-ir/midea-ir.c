#include "midea-ir.h"
#include <stdio.h>
#include "pwm.h"
#include "espressif/esp_misc.h"

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

// Time of one shortest impulse in microseconds
#define TIME_T  555

typedef struct 
{
    uint8_t magic;      // 0xB2 always
    uint8_t state : 4;
    uint8_t fan : 4; 
    uint8_t command : 4;
    uint8_t temp : 4;
} DataPacket;


void pack_data(MideaIR *ir, DataPacket *data)
{
    data->magic = 0xB2;
    data->fan = ir->fan_level;
    if (ir->enabled) {
        data->state = 0b1111;  // on
        data->command = ir->mode;
    } else {
        data->state = 0b1011; // off
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
    uint8_t pins[1];

    pins[0] = 14;
    pwm_init(1, pins);
    pwm_set_freq(46300);
    pwm_set_duty(UINT16_MAX/2);
    /* pwm_start(); */
    pwm_stop();

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

void send_start()
{
    pwm_start();
    sdk_os_delay_us(8 * TIME_T);
    pwm_stop();
    sdk_os_delay_us(8 * TIME_T);
}

void send_middle()
{
    sdk_os_delay_us(5200);
    pwm_start();
    sdk_os_delay_us(4400);
    pwm_stop();
    sdk_os_delay_us(4400);
}

void send_bit(bool bit)
{
    pwm_start();
    sdk_os_delay_us(TIME_T);
    pwm_stop();
    if (bit) {
        sdk_os_delay_us(3 * TIME_T);
    } else {
        sdk_os_delay_us(TIME_T);
    }
}

void add_complementary_bytes(const uint8_t *src, uint8_t *dst)
{
    for (int i = 0; i < 3; i++) {
        *dst = *src;
        dst++;
        *dst = ~(*src);
        dst++;
        src++;
    }
}


void midea_ir_send(MideaIR *ir)
{
    DataPacket packet; 
    pack_data(ir, &packet);
    uint8_t data[6];
    add_complementary_bytes((uint8_t*)&packet, data);

    /* printf("Data: "); */

    send_start();

    for (int b = 0; b < 6; b++) {
        uint8_t v = data[b];
        for (uint8_t i = 0; i < 8; i++) {
            /* print_bit(*p & (1<<7)); */
            send_bit(v & (1<<7));
            v <<= 1;
        } 
    }

    send_middle();

    for (int b = 0; b < 6; b++) {
        uint8_t v = data[b];
        for (uint8_t i = 0; i < 8; i++) {
            /* print_bit(*p & (1<<7)); */
            send_bit(v & (1<<7));
            v <<= 1;
        } 
    }

    /* printf("\n"); */
}

void midea_ir_move_deflector(MideaIR *ir)
{

}
