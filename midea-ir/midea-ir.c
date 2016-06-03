#include "midea-ir.h"
#include <stdio.h>
#include "espressif/esp_misc.h"
#include "esp/interrupts.h"
#include "esp/gpio.h"
#include "esp/timer.h"

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
 *      0111 - off
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
    uint8_t state : 4;
    uint8_t fan : 4; 
    uint8_t command : 4;
    uint8_t temp : 4;
} DataPacket;


#define bit_buff_capacity 29                // 8T + 8T + (4T * 8 * 6) + 8T
uint8_t bit_buff[bit_buff_capacity];        // pulses to spit out
uint8_t bit_buff_size;
uint8_t repeat_count;                       // how many times to repeat
uint8_t current_pulse;                      // current processing pulse
const uint8_t sub_pulses_per_pulse = 42;    // (high + low) * 21
uint8_t current_sub_pulse;                  // 38000 kHz pulse


static const uint8_t pin_number = 14;

static void timer_interrupt_handler(void)
{
    // get bit number 'current_pulse'
    bool pulse_val = bit_buff[current_pulse/8] & (1<<(current_pulse%8));

    if (current_sub_pulse < sub_pulses_per_pulse) {
        if (!(current_sub_pulse % 2) && pulse_val) {
            gpio_write(pin_number, true);
        } else {
            gpio_write(pin_number, false);
        }
        current_sub_pulse++;
    } else { // pulse is finished
        current_sub_pulse = 0;
        current_pulse++;
        if (current_pulse >= bit_buff_size) {
            repeat_count--;
            if (repeat_count) {
                current_pulse = 0;
                current_sub_pulse = 0;
            } else {
                timer_set_run(FRC1, false);
            }
        }
    }
}

void pack_data(MideaIR *ir, DataPacket *data)
{
    data->magic = 0xB2;
    if (ir->enabled) {
        data->fan = ir->fan_level;
        data->state = 0b1111;  // on
        data->command = ir->mode;

        if (ir->temperature >= TEMP_LOW && ir->temperature <= TEMP_HIGH) {
        // TODO: check
            data->temp = ir->temperature - TEMP_LOW;
        } else {
            data->temp = 0b0100;
        }
    } else {
        data->fan = 0b0111;
        data->state = 0b1011; // off
        data->command = 0b0000;
        data->temp = 0b1110;
    }
}

void midea_ir_init(MideaIR *ir)
{
    ir->temperature = 24;
    ir->enabled = true;
    ir->mode = MODE_AUTO;
    ir->fan_level = FAN_AUTO;

    gpio_enable(pin_number, GPIO_OUTPUT);
    gpio_write(pin_number, false);

    _xt_isr_attach(INUM_TIMER_FRC1, timer_interrupt_handler);
    timer_set_frequency(FRC1, 38000 * 2);
    timer_set_interrupts(FRC1, true);
}

void print_bit(bool bit)
{
    if (bit) {
        printf("1");
    } else {
        printf("0");
    }
}

static inline void init_buff()
{
    current_pulse = 0;
    current_sub_pulse = 0;

    for (uint8_t i = 0; i < bit_buff_capacity; i++) {
        bit_buff[i] = 0;
    }
}

static inline void add_start()
{
    bit_buff[0] = 0b11111111; 
    bit_buff[1] = 0b00000000; 
    current_pulse = 8 * 2;
}

static inline void add_bit(bool bit)
{
    bit_buff[current_pulse/8] |= (1<<(current_pulse%8));
    current_pulse++;

    if (bit) {
        current_pulse += 3;
    } else {
        current_pulse++;        
    }
}

static inline void add_stop()
{
    add_bit(true);
    current_pulse += 8;
}

static inline void start()
{
    bit_buff_size = current_pulse; 
    current_pulse = 0;
    current_sub_pulse = 0;
    repeat_count = 2;
    timer_set_run(FRC1, true);
}

static inline void add_complementary_bytes(const uint8_t *src, uint8_t *dst)
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

    init_buff();
    add_start();

    for (int b = 0; b < 6; b++) {
        uint8_t v = data[b];
        for (uint8_t i = 0; i < 8; i++) {
            /* print_bit(*p & (1<<7)); */
            add_bit(v & (1<<7));
            v <<= 1;
        } 
    }
    add_stop();
    start();
    /* printf("\n"); */
}

void midea_ir_move_deflector(MideaIR *ir)
{

}
