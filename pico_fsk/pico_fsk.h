#pragma once

#include <stdint.h>

#define LOG_ERROR     printf
#define LOG_WARN      printf
#define LOG_INFO      printf
#define LOG_DEBUG     printf

// Error codes
#define ERR_BFSK_OK                     (0)
#define ERR_INVALID_GPIO                (-1)
#define ERR_BFSK_ILLEGAL_TX_FREQ        (-2)


struct pico_bfsk_cfg
{
    int   gpio;
    float freq_symbol_low;
    float freq_symbol_high;
    int   baud_rate;
    int   N;
    int   freq_sys;
};

struct pico_bfsk_handle
{
    uint32_t* p_clk_divs; // user defined buffer of length = 8 * cfg.buffer_length
    int clk_div_buff_len;
    struct pico_bfsk_cfg* p_cfg;
    int dma_idx;
    int dma_timer_idx;
    int   clkout_idx;
    uint32_t clk_div_0;
    uint32_t clk_div_1;
};

int pico_bfsk_init(struct pico_bfsk_handle* h_bfsk, struct pico_bfsk_cfg* cfg);
int pico_bfsk_transmit(struct pico_bfsk_handle* h_bfsk, uint8_t* buffer, uint8_t blocking);

