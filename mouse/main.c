#include <assert.h>
#include <stdint.h>
#include <nrf.h>

#define SET_OUTPUT(p) NRF_P0->PIN_CNF[p] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos)
#define HIGH(p) NRF_P0->OUTSET = (1 << (p))
#define LOW(p) NRF_P0->OUTCLR = (1 << (p))

#include "pins.h"
#include "delay.h"
#include "loop.h"
#include "led.h"
#include "buttons.h"
#include "wheel.h"
#include "spi.h"
#include "hero.h"
#include "dpi.h"
#include "radio.h"

static void init(void)
{
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	NRF_POWER->TASKS_LOWPWR = 1; // can't tell difference from CONSTLAT
	NRF_POWER->DCDCEN = 1;

	delay_init();

	led_init();
	buttons_init();
	wheel_init();

	spi_init();
	if (hero_init(dpi_list[DPI_INDEX_BOOT]) != 0) { // blob upload failed
		LED_ON(LED_B);
		while (1) __WFI();
	}
	hero_conf_motion();

	radio_init();
	radio_conf_tx();

	loop_init();
}

int main(void)
{
#ifdef VECT_TAB
	SCB->VTOR = VECT_TAB;
#endif
	__disable_irq();

	init();

	// tell receiver about initial coordinates and set loop phase right
	for (;;) {
		loop_wait();
		(void)hero_read_motion();
		(void)wheel_read();
		(void)dpi_process(buttons_read_debounced());
		radio_pkt_tx.mouse.x = 0;
		radio_pkt_tx.mouse.y = 0;
		radio_pkt_tx.mouse.whl = 0;
		radio_pkt_tx.mouse.btn = RADIO_MOUSE_FIRST | RADIO_MOUSE_SYNC;

		NRF_RADIO->TASKS_TXEN = 1;
		radio_wait_disabled();
		radio_pkt_tx_prev.mouse_compact = radio_pkt_tx.mouse_compact;

		radio_conf_rx();
		NRF_RADIO->TASKS_RXEN = 1;
		NRF_RADIO->EVENTS_ADDRESS = 0;
		delay_us(80);
		if (NRF_RADIO->EVENTS_ADDRESS == 0) { // no address match, try again
			NRF_RADIO->TASKS_STOP = 1;
			NRF_RADIO->TASKS_DISABLE = 1;
			radio_conf_tx();
			
			LED_ON(LED_B);
			delay_us(8000);
			LED_OFF(LED_B);
			
			hero_deepsleep();
			delay_us(1);
			NRF_POWER->SYSTEMOFF = 1;
			hero_wake_from_sleep();
		} else { // address match
			radio_wait_disabled();
			radio_conf_tx();
			if (NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk) {
				loop_tune_phase(radio_pkt_rx.time_diff);
				break; // else try again
			}
		}
	}

	uint32_t sync_timeout = 0; // synchronize after 1s inactivity
	uint32_t idle_timeout = 0; // TODO implement power saving
	for (;; sync_timeout++) {
		loop_wait();
		const union motion_data motion = hero_read_motion();
		radio_pkt_tx.mouse.x += motion.dx;
		radio_pkt_tx.mouse.y += motion.dy;

		radio_pkt_tx.mouse.whl += wheel_read();

		const uint8_t btn_now = buttons_read_debounced();
		dpi_process(btn_now);
		radio_pkt_tx.mouse.btn = btn_now & 0b00011111;

		// if (sync_timeout == 8000) { // sync
			// radio_pkt_tx.mouse.btn |= RADIO_MOUSE_SYNC;
		// }

		if (radio_pkt_tx.mouse_compact.x_y != radio_pkt_tx_prev.mouse_compact.x_y ||
			radio_pkt_tx.mouse_compact.btn_whl != radio_pkt_tx_prev.mouse_compact.btn_whl) {
			NRF_RADIO->TASKS_TXEN = 1;
			radio_wait_disabled();
			radio_pkt_tx_prev = radio_pkt_tx;
			sync_timeout = 0;
			idle_timeout = 0;
		} else if (sync_timeout == 8000) { // sync
			radio_pkt_tx.mouse.btn |= RADIO_MOUSE_SYNC;
		} else {
			// sync_timeout++;
			idle_timeout++;
		}

		if ((radio_pkt_tx.mouse.btn & RADIO_MOUSE_SYNC) != 0) {
			sync_timeout = 0;
			idle_timeout++;

			radio_conf_rx();
			NRF_RADIO->TASKS_RXEN = 1;
			NRF_RADIO->EVENTS_ADDRESS = 0;
			delay_us(80);
			if (NRF_RADIO->EVENTS_ADDRESS == 0) { // no address match, give up
				NRF_RADIO->TASKS_STOP = 1;
				NRF_RADIO->TASKS_DISABLE = 1;
				radio_wait_disabled();
			} else { // address match
				radio_wait_disabled();
				if (NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk) {
					int error = radio_pkt_rx.time_diff;
					if (error > 4 || error < -4) {
						loop_tune_phase(error);
						loop_tune_period(error);
					}
				}
			}
			radio_conf_tx();
		}
		// if (idle_timeout > 480000) { // 1min
			// -- system on --
			// sync_timeout = 0;
			// idle_timeout = 0;
			// whl_led_off();
			// delay_us(1000);
			// NRF_POWER->EVENTS_SLEEPENTER = 1;
		// }

		if (idle_timeout > 960000) { // 2min
			// -- system off --
			// sync_timeout = 0;
			// idle_timeout = 0;
			delay_us(1000);
			// hero_sleep();
			hero_deepsleep();
			delay_us(1000);
			NRF_POWER->SYSTEMOFF = 1;
			hero_wake_from_sleep();
		}
	}
}
