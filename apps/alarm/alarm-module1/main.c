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

#define UART_RING_SIZE 16

#ifdef DEBUG
static ring_t *uart_ring;
#endif

static tim_t wd_timer;
static uint16_t timer_watchdog;
#define TIMER_WD_TIMEOUT 500000U

static void wd_timer_cb(void *arg)
{
	timer_watchdog = 0;
	timer_reschedule(&wd_timer, TIMER_WD_TIMEOUT);
}

#ifdef DEBUG
static void uart_task(void *arg)
{
	buf_t buf;
	int rlen = ring_len(uart_ring);

	if (rlen > UART_RING_SIZE) {
		ring_reset(uart_ring);
		return;
	}
	buf = BUF(rlen + 1);
	__ring_get(uart_ring, &buf, rlen);
	__buf_addc(&buf, '\0');
	module1_parse_uart_commands(&buf);
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
#endif

int main(void)
{
	analog_init();
#ifdef DEBUG
	init_stream0(&stdout, &stdin, 1);
	uart_ring = ring_create(UART_RING_SIZE);
	DEBUG_LOG("KW alarm module 1\n");
#endif
	timer_subsystem_init();
	irq_enable();

	scheduler_init();

#ifdef CONFIG_TIMER_CHECKS
	watchdog_shutdown();
	timer_checks();
#endif
	watchdog_enable(WATCHDOG_TIMEOUT_8S);

	/* port D used by the LED and RF sender */
	DDRD = (1 << PD2);

	DDRB = (1 << PB0);

	/* port D used by PIR, set as input */
	DDRD &= ~(1 << PD1);

	/* enable pull-up resistor */
	/* PORTD |= (1 << PD1); */

#ifndef CONFIG_AVR_SIMU
	/* PCINT17 enabled (PIR) */
	PCMSK2 = 1 << PCINT17;
	PCICR = 1 << PCIE2;
#endif

#if defined (CONFIG_RF_RECEIVER) && defined (CONFIG_RF_SENDER)
	pkt_mempool_init(CONFIG_PKT_NB_MAX, CONFIG_PKT_SIZE);
	module1_init();
#endif
	timer_init(&wd_timer);
	timer_add(&wd_timer, TIMER_WD_TIMEOUT, wd_timer_cb, NULL);

	/* interruptible functions */
	while (1) {
		scheduler_run_tasks();
		watchdog_reset();
		if (timer_watchdog++ == 0xFFFF) {
			DEBUG_LOG("timer watchdog expired\n");
			__abort();
		}
	}
	return 0;
}
