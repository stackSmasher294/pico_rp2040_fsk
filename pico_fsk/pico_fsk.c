#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include "pico_fsk/pico_fsk.h"


void setup_dma(struct pico_bfsk_handle* h_bfsk, uint32_t* divisors)
{
    struct pico_bfsk_cfg cfg;
    uint16_t numerator, denominator;
    h_bfsk->dma_idx        = dma_claim_unused_channel(true);
    h_bfsk->dma_timer_idx  = dma_claim_unused_timer(true);

    numerator = 3;
    denominator = 62500; // 2000/s @ 125MHz
    dma_timer_set_fraction(h_bfsk->dma_timer_idx, numerator, denominator);


    dma_channel_config c = dma_channel_get_default_config(h_bfsk->dma_idx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, dma_get_timer_dreq(h_bfsk->dma_timer_idx));
    dma_channel_configure(h_bfsk->dma_idx, &c,
                        &clocks_hw->clk[h_bfsk->clkout_idx].div,
                        h_bfsk->p_clk_divs,
                        h_bfsk->clk_div_buff_len,
                        false);
    // return data_chan;
}

int calc_clkout_divider(float freq, int freq_sys)
{
    uint32_t div;
    int int_div;
    uint8_t frac_div;
    float fdiv;
    fdiv = (float)freq_sys / freq;
    int_div = (int)fdiv;
    frac_div = (uint8_t)((fdiv - (float)int_div) * 256.0); // int or round?
    if (int_div <= 1)
    {
        LOG_ERROR("Invalid freq_symbol_low, integer divider <= 1, div_int = %d, frac_div = %d\n", int_div, frac_div);
        return ERR_BFSK_ILLEGAL_TX_FREQ;
    }
    LOG_DEBUG("freq: %f, int_div: %d, frac_div: %d\n", freq, int_div, frac_div);
    div = (int_div << 8) | frac_div;
    return div;
}

int pico_bfsk_init(struct pico_bfsk_handle *h_bfsk, struct pico_bfsk_cfg *cfg)
{
    int ret = ERR_BFSK_OK;
    int gpio;

    h_bfsk->p_cfg = cfg;

    switch (cfg->gpio)
    {
    case 21:
        h_bfsk->clkout_idx = 0;
        break;
    case 23:
        h_bfsk->clkout_idx = 1;
        break;
    case 24:
        h_bfsk->clkout_idx = 2;
        break;
    case 25:
        h_bfsk->clkout_idx = 3;
        break;
    default:
        LOG_ERROR("Invalid gpio: %d, valid values are: 21,23,24,25\n");
        h_bfsk->clkout_idx = -1;
        ret = ERR_INVALID_GPIO;
        break;
    }

    if (ret < 0)
    {
        return ret;
    }
    LOG_DEBUG("GPCLK: %d\n", h_bfsk->clkout_idx);

    // configure gpio pin as gpclk out and set the input clock source.
    clocks_hw->clk[h_bfsk->clkout_idx].ctrl = (CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS << CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_LSB);
    gpio_set_function(cfg->gpio, GPIO_FUNC_GPCK);


    // Calculate clock divider params for freq_low
    h_bfsk->clk_div_0 = calc_clkout_divider(cfg->freq_symbol_low, cfg->freq_sys);

    // Calculate clock divider params for freq_high
    h_bfsk->clk_div_1 = calc_clkout_divider(cfg->freq_symbol_high, cfg->freq_sys);

    h_bfsk->clk_div_buff_len = cfg->N * 8;

    setup_dma(h_bfsk, h_bfsk->p_clk_divs);

    return ret;
}

int pico_bfsk_transmit(struct pico_bfsk_handle *h_bfsk, uint8_t *buffer, uint8_t blocking)
{
    struct pico_bfsk_cfg *cfg = h_bfsk->p_cfg;
    uint8_t data;
    int iclk_div = 0;
    uint32_t div0 = h_bfsk->clk_div_0;
    uint32_t div1 = h_bfsk->clk_div_1;
    for (int ibyte = 0; ibyte < cfg->N; ++ibyte)
    {
        data = buffer[ibyte];
        for (int ibit = 0; ibit < 8; ++ibit)
        {
            h_bfsk->p_clk_divs[iclk_div++] = ((data & (1 << ibit)) == 0) ? div0 : div1;
        }
    }

    // start clocking
    clocks_hw->clk[h_bfsk->clkout_idx].ctrl |= CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS;

    // Trigger the DMA to start transfer
    dma_hw->ch[h_bfsk->dma_idx].al3_read_addr_trig = &h_bfsk->p_clk_divs[0];
    if (blocking == 1)
    {
        dma_channel_wait_for_finish_blocking(h_bfsk->dma_idx);
        clocks_hw->clk[h_bfsk->clkout_idx].ctrl &= ~CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS;
    }
}