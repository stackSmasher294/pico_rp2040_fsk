/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Fade an LED between low and high brightness. An interrupt handler updates
// the PWM slice's output level each time the counter wraps.

#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#define CLKOUT_PIN 21
#define SEQ_LEN     2048
#define FREQ_START  15e3
#define FREQ_END    20e3

volatile float freq_table[SEQ_LEN];
volatile uint32_t divisors[SEQ_LEN];

void generate_linear_sweep(float* freq_table, int N)
{
    float step = (FREQ_END - FREQ_START) / (N - 1.0f);
    for(int i = 0; i < N; i++)
    {
        freq_table[i] = FREQ_START + (i * step); 
    }
}

void calculate_clkdivs(const int freq_sys, const float* freqs, uint32_t* divisors, int N)
{
    int int_div;
    uint8_t frac_div;
    float fdiv;
    for(int i = 0; i < N; i++)
    {
        fdiv = (float)freq_sys / freqs[i];
        int_div = (int) fdiv;
        frac_div = (uint8_t) ((fdiv - (float)int_div) * 256.0); // int or round?
        divisors[i] = (int_div << 8) | frac_div; 
    }
}

int setup_dma(uint32_t* divisors, int N)
{
    uint16_t numerator, denominator;
    uint data_chan = dma_claim_unused_channel(true);
    uint dma_timer = dma_claim_unused_timer(true);

    numerator = 1;
    denominator = 62500; // 2000/s @ 125MHz
    dma_timer_set_fraction(dma_timer, numerator, denominator);


    dma_channel_config c = dma_channel_get_default_config(data_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, dma_get_timer_dreq(dma_timer));
    dma_channel_configure(data_chan, &c,
                        &clocks_hw->clk[0].div,
                        divisors,
                        N,
                        false);
    return data_chan;
}



int main() {
    int freq_sys = clock_get_hz(clk_sys);
    int dma_chan;
    stdio_init_all();
    // sleep_ms(2000);

    printf("system clock: %d Hz\n", freq_sys);
    gpio_set_slew_rate(CLKOUT_PIN, GPIO_SLEW_RATE_FAST);
    // clock_gpio_init_int_frac(CLKOUT_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, 5, 1);


    generate_linear_sweep(freq_table, SEQ_LEN);
    calculate_clkdivs(freq_sys, freq_table, divisors, SEQ_LEN);
    clock_gpio_init(CLKOUT_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, freq_sys / freq_table[0]);


    dma_chan = setup_dma(divisors, SEQ_LEN);
    dma_start_channel_mask(1 << dma_chan);
    printf("DMA start\n");
    dma_channel_wait_for_finish_blocking(dma_chan);
    printf("DMA end\n");

    // clock_gpio_init_int_frac(CLKOUT_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, div_int, div_frac);

    while(1)
    {
        tight_loop_contents();
        dma_hw->ch[dma_chan].al3_read_addr_trig = &divisors[0];
        // printf("DMA start\n");
        dma_channel_wait_for_finish_blocking(dma_chan);
        // printf("DMA end\n");
    }
}
