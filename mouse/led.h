#pragma once

#include <stdint.h>
#include <nrf.h>
#include "pins.h"

// maybe should use inline functions... whatever
#define LED_ON(L) NRF_P0->OUTSET = (1 << (L))
#define LED_OFF(L) NRF_P0->OUTCLR = (1 << (L))
// #define LED_TOGGLE(L) NRF_P0->OUT ^= (1 << (L))

static void led_init(void)
{
	const uint32_t out =
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
	LED_OFF(LED_R);
	NRF_P0->PIN_CNF[LED_R] = out;
}

void blink(int times)
{
	int j;
	for ( j = 0; j < times; j++ ) {
		uint32_t volatile tmo;

		tmo = 2000000;
		while (tmo--);
		NRF_P0->OUTSET = (1 << LED_R);

		tmo = 2000000;
		while (tmo--);
		NRF_P0->OUTCLR = (1 << LED_R);
	}
}