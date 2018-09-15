#include <avr/sleep.h>
#include <log.h>
#include <_stdio.h>
#include <common.h>
#include <watchdog.h>
#include <adc.h>
#include <sys/buf.h>
#include <timer.h>
#include <scheduler.h>
#include <interrupts.h>
#include "alarm-module1.h"
#include "../module.h"

extern uint8_t inactivity;
static tim_t timer_1sec;

ISR(WDT_vect) {
	delay_ms(2000);
	DEBUG_LOG("WD interrupt\n");
}

static void watchdog_cb(void *arg)
{
	DEBUG_LOG("sleeping...\n");
	WDTCSR |= _BV(WDIE) | _BV(WDE);
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_mode();
	wdt_reset();
}

static void tim_1sec_cb(void *arg)
{
	tim_t *timer = arg;

	timer_reschedule(&timer_1sec, 1000000);
	inactivity++;
	PORTD ^= 1 << PD4;

	if (inactivity < 15)
		return;
	schedule_task(watchdog_cb, NULL);
}

INIT_ADC_DECL(c, DDRC, PORTC, 0);

#define UART_RING_SIZE 16
static ring_t *uart_ring;
extern iface_t rf_iface;
extern ring_t *pkt_pool;

static void parse_uart_commands(buf_t *buf)
{
	sbuf_t s;
	iface_t *ifce = NULL;

	if (buf_get_sbuf_upto_and_skip(buf, &s, "rf buf") >= 0)
		ifce = &rf_iface;
	else if (buf_get_sbuf_upto_and_skip(buf, &s, "eth buf") >= 0) {
		printf_P(PSTR(" unsupported\n"));
		return;
	}
	if (buf_get_sbuf_upto_and_skip(buf, &s, "get status") >= 0) {
		module_print_status();
		return;
	}
	if (buf_get_sbuf_upto_and_skip(buf, &s, "disarm") >= 0) {
		module_arm(0);
		return;
	}
	if (buf_get_sbuf_upto_and_skip(buf, &s, "arm") >= 0) {
		module_arm(1);
		return;
	}

	if (ifce) {
		printf_P(PSTR("\nifce pool: %d rx: %d tx:%d\npkt pool: %d\n"),
			      ring_len(ifce->pkt_pool), ring_len(ifce->rx),
			      ring_len(ifce->tx), ring_len(pkt_pool));
		return;
	}
}

static void uart_task(void *arg)
{
	buf_t buf;
	int rlen = ring_len(uart_ring);

	if (rlen > UART_RING_SIZE) {
		ring_reset(uart_ring);
		return;
	}
	buf= BUF(rlen + 1);
	__ring_get(uart_ring, &buf, rlen);
	__buf_addc(&buf, '\0');
	parse_uart_commands(&buf);
}

ISR(USART_RX_vect)
{
	uint8_t c = UDR0;

	if (c == '\r')
		return;
	if (c == '\n') {
		schedule_task(uart_task, NULL);
	} else
		ring_addc(uart_ring, c);
}

int main(void)
{
	init_adc_c();
#ifdef DEBUG
	init_stream0(&stdout, &stdin, 1);
	uart_ring = ring_create(UART_RING_SIZE);
	DEBUG_LOG("KW alarm module 1\n");
#endif
	timer_subsystem_init();
	scheduler_init();

#ifdef CONFIG_TIMER_CHECKS
	watchdog_shutdown();
	irq_enable();
	timer_checks();
#endif
	timer_add(&timer_1sec, 1000000, tim_1sec_cb, NULL);
	watchdog_enable();

	/* port D used by the LED and RF sender */
	DDRD = (1 << PD2);

	DDRB = (1 << PB0);

	/* port D used by PIR, set as input */
	DDRD &= ~(1 << PD1);

	/* enable pull-up resistor */
	/* PORTD |= (1 << PD1); */

	/* PCINT17 enabled (PIR) */
	PCMSK2 = 1 << PCINT17;
	PCICR = 1 << PCIE2;

#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_SENDER)
	alarm_module1_rf_init();
	module_init();
#endif
	irq_enable();

	/* interruptible functions */
	while (1) {
		if (scheduler_run_tasks())
			inactivity = 0;
		watchdog_reset();
	}
	return 0;
}
