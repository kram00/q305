#pragma once

#include <stdint.h>
#include "hero.h"

#define DPI_INDEX_BOOT 1

const uint32_t dpi_list[] = {400, 800, 1600, 3200, 6400};
const int dpi_list_len = sizeof(dpi_list)/sizeof(dpi_list[0]);

const uint32_t fr_list[] = {50, 6}; // 1000fps, 8300fps
const int fr_list_len = sizeof(fr_list)/sizeof(fr_list[0]);

const uint32_t led_list[] = {1, 2};
const int led_list_len = sizeof(led_list)/sizeof(led_list[0]);

void dpi_process(uint8_t btn_now) {
    static uint8_t btn_prev = 0;
    static int dpi_index = 1;
    static int fr_index = 0;
    static int led_index = 0;

    if ((btn_now & ~btn_prev) & (1 << 5)) {
        dpi_index++;
        if (dpi_index == dpi_list_len) dpi_index = 0;
        hero_set_dpi(dpi_list[dpi_index]);
        delay_us(1);
        hero_conf_motion();
    }

    if (!(NRF_P0->IN & (1 << BTN_M)) && !(NRF_P0->IN & (1 << BTN_F))) {
        fr_index++;
        if (fr_index  == fr_list_len) fr_index  = 0;
        spi_cs_low();
        delay_us(1);
        hero_reg_write(0x20, fr_list[fr_index]);
        spi_cs_high();
        hero_conf_motion();

        delay_us(10);

        led_index++;
        if (led_index == led_list_len) led_index = 0;
        blink(led_list[led_index]);
    }
    btn_prev = btn_now;
}