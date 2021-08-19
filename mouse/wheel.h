#pragma once

#include <stdint.h>
#include <nrf.h>
#include "delay.h"
#include "pins.h"

#define WHL_MAX_TIMEOUT 16000 // = 2 seconds

static inline int wheel_read_A_B(void)
{

	// read "B"
	NRF_P0->OUTSET = 1 << WHL_DETECTOR;
	NRF_P0->DIRSET = 1 << WHL_DETECTOR;
	delay_us(1); // not sure how long to delay. 1us seems sufficient
	NRF_P0->OUTCLR = 1 << WHL_DETECTOR;
	delay_us(1);
	NRF_P0->DIRCLR = 1 << WHL_DETECTOR;
	delay_us(1);
	int8_t b = (NRF_P0->IN & (1 << WHL_DETECTOR)) != 0;

	delay_us(1);

	// read "A"
	NRF_P0->OUTSET = 1 << WHL_DETECTOR;
	NRF_P0->DIRSET = 1 << WHL_DETECTOR;
	delay_us(1);
	NRF_P0->OUTCLR = 1 << WHL_DETECTOR;
	delay_us(1);
	NRF_P0->DIRCLR = 1 << WHL_DETECTOR;
	delay_us(1);
	int8_t a = (NRF_P0->IN & (1 << WHL_DETECTOR)) != 0;

	return (a - b);
	// toggle wheel led for ~25us after calling this function
}


static inline void whl_led_on(void)
{
	NRF_P0->OUTCLR = 1 << WHL_LED;
}

static inline void whl_led_off(void)
{
	NRF_P0->OUTSET = 1 << WHL_LED;
}

static inline int wheel_read(void)
{
	int8_t whl_notches = 0;
	static int8_t whl_ticks = 0;
	// if no wheel motion after this many cycles, reset whl_ticks.
	static uint16_t whl_timeout = 0;
	
	if (whl_timeout != 0) whl_timeout++;
	if (whl_timeout >= WHL_MAX_TIMEOUT) {
		whl_timeout = 0;
		whl_ticks = 0; // assume wheel is in center of a notch
	}
	
	const int8_t _whl_ticks = wheel_read_A_B(); 
	whl_led_on();
	// delay_us(25); // toggle wheel for next cycle's read
	// whl_led_off();
			
	whl_ticks += _whl_ticks;
	if (_whl_ticks != 0) {
		whl_notches = whl_ticks / 5;
		whl_ticks -= whl_notches * 8;
		whl_timeout = 1; //reset timeout
	}
	
	return whl_notches;
}
		
static void wheel_init(void)
{
	NRF_P0->PIN_CNF[WHL_LED] =
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
	// whl_led_on();
	// delay_us(25);
	// whl_led_off();

	const int in =
		(GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos);
	NRF_P0->PIN_CNF[WHL_DETECTOR] = in;

	delay_us(1000); // pullup takes a bit of time
	(void)wheel_read(); // initialize static variables in wheel_read()
}