#pragma once

#include <stdint.h>
#include "hero.h"

#define DPI_INDEX_BOOT 1

void nrf_nvmc_page_erase(uint32_t address)
{
    // Enable erase.
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
    }

    // Erase the page
    NRF_NVMC->ERASEPAGE = address;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
    }

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
    }
}

void nrf_nvmc_write_word(uint32_t address, uint32_t value)
{
    // Enable write.
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){
    }

    *(uint32_t*)address = value;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){
    }

    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
    }
}

uint32_t const f_addr = 0x0002F000; // page 47
uint32_t * p_addr = (uint32_t *)f_addr;

uint32_t const f_addr2 = 0x0002E000; // page 46
uint32_t * p_addr2 = (uint32_t *)f_addr2;

uint32_t const f_addr3 = 0x0002D000; // page 45
uint32_t * p_addr3 = (uint32_t *)f_addr3;

const uint32_t dpi_list[] = {400, 800, 1600, 3200, 6400};
const int dpi_list_len = sizeof(dpi_list)/sizeof(dpi_list[0]);

const uint32_t fr_list[] = {50, 6}; // 1000fps ~1.3mA-5.2mA, 8300fps ~5.5mA-8.7mA
const int fr_list_len = sizeof(fr_list)/sizeof(fr_list[0]);

const uint32_t led_list[] = {1, 2};
const int led_list_len = sizeof(led_list)/sizeof(led_list[0]);

void dpi_process(uint8_t btn_now) {
    static uint8_t btn_prev = 0;
    static int dpi_index = 1;
    static int fr_index = 0;
    static int led_index = 0;

    if ((btn_now & ~btn_prev) & (1 << 5)) {
		dpi_index = *(p_addr);
        dpi_index++;
        if (dpi_index == dpi_list_len) dpi_index = 0;
        hero_set_dpi(dpi_list[dpi_index]);
        delay_us(1);
		nrf_nvmc_page_erase(f_addr);
		nrf_nvmc_write_word(f_addr, dpi_index);
        hero_conf_motion();

        delay_us(10);
    }

    if (!(NRF_P0->IN & (1 << BTN_M)) && !(NRF_P0->IN & (1 << BTN_F))) {
		fr_index = *(p_addr2);
        fr_index++;
        if (fr_index  == fr_list_len) fr_index  = 0;

        spi_cs_low();
        delay_us(1);
        hero_reg_write(0x20, fr_list[fr_index]);
		nrf_nvmc_page_erase(f_addr2);
		nrf_nvmc_write_word(f_addr2, fr_index);
        spi_cs_high();
        hero_conf_motion();

        delay_us(10);

		led_index = *(p_addr3);
        led_index++;
        if (led_index == led_list_len) led_index = 0;
		nrf_nvmc_page_erase(f_addr3);
		nrf_nvmc_write_word(f_addr3, led_index);
        blink(led_list[led_index]);
    }
    btn_prev = btn_now;
}