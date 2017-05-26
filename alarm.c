#include <avr/io.h>
#include <stdio.h>
#include <avr/power.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <string.h>

#define DEBUG
#ifdef DEBUG
#include "usart0.h"
#define SERIAL_SPEED 57600
#define SYSTEM_CLOCK F_CPU
#define TIMER_RESOLUTION_US 150UL
#endif
#include "utils.h"
#include "ring.h"
#include "timer.h"

#ifdef DEBUG
static int my_putchar(char c, FILE *stream)
{
	(void)stream;
	if (c == '\r') {
		usart0_put('\r');
		usart0_put('\n');
	}
	usart0_put(c);
	return 0;
}

static int my_getchar(FILE * stream)
{
	(void)stream;
	return  usart0_get();
}

/*
 * Define the input and output streams.
 * The stream implemenation uses pointers to functions.
 */
static FILE my_stream =
	FDEV_SETUP_STREAM (my_putchar, my_getchar, _FDEV_SETUP_RW);
#endif


void initADC()
{
	/* Select Vref=AVcc */
	ADMUX |= (1<<REFS0);

	/* set prescaller to 128 and enable ADC */
	/* ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN); */

	/* set prescaller to 64 and enable ADC */
	ADCSRA |= (1<<ADPS1)|(1<<ADPS0);
}

uint16_t analogRead(uint8_t ADCchannel)
{
	/* enable ADC */
	ADCSRA |= (1<<ADEN);

	/* select ADC channel with safety mask */
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);

	/* single conversion mode */
	ADCSRA |= (1<<ADSC);

	// wait until ADC conversion is complete
	while (ADCSRA & (1<<ADSC));

	/* shutdown the ADC */
	ADCSRA &= ~(1<<ADEN);
	return ADC;
}

typedef enum state {
	WAIT,
	STARTED,
} state_t;

#define START_FRAME = 0xAA

typedef struct pkt_header {
	unsigned char  start_frame; /* 0 */
	unsigned short from;	    /* 1 */
	unsigned char  len;	    /* 3 */
	unsigned short chksum;	    /* 4 */
	unsigned char  data[];      /* 6 */
} pkt_header_t;

#if 0
static void pkt_parse(ring_t *ring)
{
	unsigned short addr;
	int len, chksum, chksum2 = 0, i;

	if (ring_is_empty(ring))
		return;

	if (ring_len(ring) < sizeof(pkt_head)) {
		return;
	}

	if (ring->data[ring->tail] != START_FRAME) {
		ring_reset(ring); // maybe skip 1?
		return;
	}

	/* check if we have enough data */
	len = ring->data[ring->tail + 3];
	if (len + sizeof(pkt_header_t) > MAX_SIZE) {
		ring_reset(ring);
		return;
	}
	if (ring_len(ring) < len + sizeof(pkt_head_t)) {
		/* not enough data */
		return;
	}

	addr = (unsigned short)ring->data[ring->tail + 1];
	chksum = (unsigned short)ring->data[ring->tail + 4];
	for (i = 0; i < len; i++) {
		chksum2 += ring->data[ring->tail + 6 + i];
	}
	if (checksum != chksum) {
		ring_skip(ring, len);
	}
}
#endif
#if 0
void loop()
{
	ring_t ring;
	int start_frame_cpt = 0;

	memset(&ring, 0, sizeof(ring));

	while (1) {
		int v;
		// 180 716
	start:
		v = analogRead(0);

		if (v < 170) {
			start_frame_cpt++;
			continue;
		}
		if (start_frame_cpt < 52) {
			start_frame_cpt = 0;
			continue;
		}
		while (1) {
			if (v < 170) {
				/* low++; */
				/* state = STARTED; */
				/* hi = 0; */
				//printf("v:%d ");
				ring_add_bit(&ring, 0);
				//printf(" ");
			} else if (v > 690) {
				/* hi++; */
				/* state = STARTED; */
				/* low = 0; */
				//printf("v:%d ", v);
				ring_add_bit(&ring, 1);
				//printf("X");
			} else {
				/* noise */
				//low = hi = 0;
				/* if (state == STARTED) { */
				/* 	parse(&ring); */
				/* } */
				//state = WAIT;
				ring_print_bits(&ring);
				ring_reset(&ring);
				goto start;
#if 0
				{
					int pos = (ring.head - 1) & MASK;
					unsigned int i;

					for (i = 0; i < 4; i++) {
						if (ring.data[pos] != 0)
							continue;
						pos = (pos - 1) & MASK;
					}
					ring_print(&ring);
				}
				ring_reset(&ring);
#endif
			}
			v = analogRead(0);
		}
	}
}
#endif
void init_streams()
{
	// initialize the standard streams to the user defined one
	stdout = &my_stream;
	stdin = &my_stream;
	usart0_init(BAUD_RATE(SYSTEM_CLOCK, SERIAL_SPEED));
}

ring_t ring;

#define LED PD4
void tim_cb_led(void *arg)
{
	tim_t *timer = arg;

	PORTD ^= (1 << LED);
	timer_reschedule(timer, TIMER_RESOLUTION_US);
}

void tim_cb(void *arg)
{
	tim_t *timer = arg;
	int v = analogRead(0);

	if (v < 170) {
		PORTD &= ~(1 << LED);
	} else if (v > 690) {
		PORTD |= (1 << LED);
	}

	timer_reschedule(timer, TIMER_RESOLUTION_US);
}

int main(void)
{
	/* led */
	DDRB = 1<<3; // port B3, ATtiny13a pin 2

	/* siren */
	DDRB |= 1; // port B0, ATtiny13a pin 0

	//speaker_init(); // port B1 (0x2)
	//start_song();

	initADC();
	/* set PORTC (analog) for input */
	DDRC &= 0xFB; // pin 2 b1111 1011
	DDRC &= 0xFD; // pin 1 b1111 1101
	DDRC &= 0xFE; // pin 0 b1111 1110
	PORTC = 0xFF; // pullup resistors

#ifdef DEBUG
	init_streams();
	printf_P(PSTR("KW alarm v0.1\n"));
#endif
	timer_subsystem_init(TIMER_RESOLUTION_US);
	DDRD = (0x01 << LED); //Configure the PORTD4 as output
	{
		tim_t timer;
		memset(&timer, 0, sizeof(timer));
		timer_add(&timer, 300, tim_cb, &timer);
	}
	while (1) {}
	//	loop();

	return 0;
}
