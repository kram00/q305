#pragma once

#include <nrf.h>

// #define TRIGGER_INTERVAL1 250     // ms
// #define TRIGGER_INTERVAL2 500     // ms

void battery_init(void)
{
	// Configure SAADC singled-ended channel, Internal reference (0.6V) and 1/6 gain.
	NRF_SAADC->CH[0].CONFIG = (SAADC_CH_CONFIG_GAIN_Gain1_6    << SAADC_CH_CONFIG_GAIN_Pos) |
							(SAADC_CH_CONFIG_MODE_SE         << SAADC_CH_CONFIG_MODE_Pos) |
							(SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
							(SAADC_CH_CONFIG_RESN_Bypass     << SAADC_CH_CONFIG_RESN_Pos) |
							(SAADC_CH_CONFIG_RESP_Bypass     << SAADC_CH_CONFIG_RESP_Pos) |
							(SAADC_CH_CONFIG_TACQ_3us        << SAADC_CH_CONFIG_TACQ_Pos);

	// Configure the SAADC channel with AIN0 as positive input, no negative input(single ended).
	NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_AnalogInput0 << SAADC_CH_PSELP_PSELP_Pos;
	NRF_SAADC->CH[0].PSELN = SAADC_CH_PSELN_PSELN_NC << SAADC_CH_PSELN_PSELN_Pos;

	// Configure the SAADC resolution.
	NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_12bit << SAADC_RESOLUTION_VAL_Pos;

	// No automatic sampling, will trigger with TASKS_SAMPLE.
	NRF_SAADC->SAMPLERATE = SAADC_SAMPLERATE_MODE_Task << SAADC_SAMPLERATE_MODE_Pos;
}

void check_battery_voltage(void)
{
	volatile int16_t result = 0;

	// Configure result to be put in RAM at the location of "result" variable.
	NRF_SAADC->RESULT.MAXCNT = 1;
	NRF_SAADC->RESULT.PTR = (uint32_t)&result;

	// Enable SAADC (would capture analog pins if they were used in CH[0].PSELP)
	NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos;

	// Calibrate the SAADC (only needs to be done once in a while)
	NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
	while (NRF_SAADC->EVENTS_CALIBRATEDONE == 0);
	NRF_SAADC->EVENTS_CALIBRATEDONE = 0;
	while (NRF_SAADC->STATUS == (SAADC_STATUS_STATUS_Busy <<SAADC_STATUS_STATUS_Pos));

	// Start the SAADC and wait for the started event.
	NRF_SAADC->TASKS_START = 1;
	while (NRF_SAADC->EVENTS_STARTED == 0);
	NRF_SAADC->EVENTS_STARTED = 0;

	// Do a SAADC sample, will put the result in the configured RAM buffer.
	NRF_SAADC->TASKS_SAMPLE = 1;
	while (NRF_SAADC->EVENTS_END == 0);
	NRF_SAADC->EVENTS_END = 0;

	// if (result < 2950) {
	if (result < 1550) { // 3V
		LED_ON(LED_B);
	}

	// Stop the SAADC, since it's not used anymore.
	NRF_SAADC->TASKS_STOP = 1;
	while (NRF_SAADC->EVENTS_STOPPED == 0);
	NRF_SAADC->EVENTS_STOPPED = 0;
}

void timer_init(void)
{
	// NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
	// NRF_TIMER0->TASKS_CLEAR = 1;

	// NRF_TIMER0->SHORTS = TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos;
	// NRF_TIMER0->PRESCALER = 4;
	// NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
	// NRF_TIMER0->CC[0] = TRIGGER_INTERVAL1 * 1000;
	// NRF_TIMER0->CC[1] = TRIGGER_INTERVAL2 * 1000;

	// NRF_TIMER0->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos) | (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);

	// NVIC_EnableIRQ(TIMER0_IRQn);

	// NRF_TIMER0->TASKS_START = 1;

	NRF_TIMER0->MODE = TIMER_MODE_MODE_LowPowerCounter;
	NRF_TIMER0->TASKS_CLEAR = 1;

	NRF_TIMER0->SHORTS = (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos);

	NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
	NRF_TIMER0->CC[0] = 2000;
	NRF_TIMER0->CC[1] = 4000;

	NRF_TIMER0->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos) | (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);
	// NVIC_EnableIRQ(TIMER0_IRQn);

	NRF_TIMER0->TASKS_START = 1;
}

void run_timer(void)
{
	NRF_TIMER0->TASKS_COUNT = 1;
	if ((NRF_TIMER0->EVENTS_COMPARE[0] != 0))
		{
			NRF_TIMER0->EVENTS_COMPARE[0] = 0;
			check_battery_voltage();
		}
	if ((NRF_TIMER0->EVENTS_COMPARE[1] != 0))
		{
			NRF_TIMER0->EVENTS_COMPARE[1] = 0;
			LED_OFF(LED_B);
		}
}