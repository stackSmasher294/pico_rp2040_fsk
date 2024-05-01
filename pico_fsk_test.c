/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Fade an LED between low and high brightness. An interrupt handler updates
// the PWM slice's output level each time the counter wraps.

#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "pico_fsk/pico_fsk.h"
#define CLKOUT_PIN 21
#define BLOCK_LEN   1024

uint32_t g_divs[BLOCK_LEN * 8] = {0};
uint8_t  data[BLOCK_LEN] = {0};



int main() 
{
    stdio_init_all();
    sleep_ms(2000);

    int freq_sys = clock_get_hz(clk_sys);
    struct pico_bfsk_handle h_bfsk;
    struct pico_bfsk_cfg cfg = {
        .gpio = CLKOUT_PIN,
        .freq_symbol_low = 40e6,
        .freq_symbol_high = 41e6,
        .N = BLOCK_LEN,
        .freq_sys = freq_sys
    };

    printf("system clock: %d Hz\n", freq_sys);
    h_bfsk.p_clk_divs = g_divs;
    int ret = pico_bfsk_init(&h_bfsk, &cfg);

    gpio_set_slew_rate(CLKOUT_PIN, GPIO_SLEW_RATE_FAST);

    memset(&data, 0x55, sizeof(data));
    pico_bfsk_transmit(&h_bfsk, data, 1);

    while(1)
    {
        tight_loop_contents();
        // pico_bfsk_transmit(&h_bfsk, data, 1);
    }
}
