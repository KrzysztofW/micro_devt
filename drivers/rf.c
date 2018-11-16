#include <stdio.h>
#include <string.h>
#include <sys/chksum.h>
#ifdef CONFIG_RND_SEED
#include <sys/random.h>
#endif
#include <net/swen.h>
#ifdef CONFIG_RF_CHECKS
#include <net/event.h>
#endif
#include <scheduler.h>
#include "rf.h"
#include "rf-cfg.h"
#ifndef X86
#include "adc.h"
#endif
#if RF_SAMPLING_US < CONFIG_TIMER_RESOLUTION_US
#error "RF sampling will not work with the current timer resolution"
#endif

/* #define RF_DEBUG */
#ifdef RF_DEBUG
extern iface_t *rf_debug_iface1;
extern iface_t *rf_debug_iface2;
#endif

#ifdef CONFIG_RF_RECEIVER
#ifndef RF_RCV_PIN_NB
#error "RF_RCV_PIN_NB not defined"
#endif
#endif

#ifdef CONFIG_RF_SENDER
#define FRAME_DELIMITER_LENGTH 29 /* loops */
#define START_FRAME_LENGTH 30	  /* loops */

#if !defined(RF_SND_PIN_NB) || !defined(RF_SND_PORT)
#error "RF_SND_PIN and RF_SND_PORT are not defined"
#endif
#ifdef CONFIG_RF_CHECKS
static pkt_t *pkt_recv;
static rf_ctx_t rf_check_recv_ctx;
#endif
#endif

#ifdef CONFIG_RF_RECEIVER

void rf_input(const iface_t *iface) {}

static int rf_add_bit(rf_ctx_t *ctx, uint8_t bit)
{
	int byte = byte_add_bit(&ctx->rcv_data.byte, bit);

	if (byte < 0)
		return 0;

	return buf_addc(&ctx->rcv_data.pkt->buf, byte);
}

#ifdef CONFIG_RF_SENDER
static void rf_snd_tim_cb(void *arg);

static void __rf_start_sending(iface_t *iface)
{
	if (ring_len(iface->tx)) {
		rf_ctx_t *ctx = iface->priv;

		timer_del(&ctx->timer);
		rf_snd_tim_cb(iface);
	}
}
#endif

static void rf_fill_data(const iface_t *iface, uint8_t bit)
{
	rf_ctx_t *ctx = iface->priv;
#ifdef CONFIG_RND_SEED
	static uint16_t rnd;

	rnd <<= 1;
	rnd |= bit;
	rnd_seed *= 16807 * rnd;
#endif
	if (ctx->rcv.receiving && ctx->rcv.cnt < 11) {
		if (ctx->rcv.receiving == 1) {
			/* first bit must be set to '1' */
			if (bit == 0)
				goto end;
			ctx->rcv.receiving = 2;
		}

		if (rf_add_bit(ctx, (ctx->rcv.cnt > 3)) < 0)
			goto end;
		return;
	}

	if (ctx->rcv.cnt >= 55 && ctx->rcv.cnt <= 62) {
		/* frame delimiter is only made of low values */
		if (bit)
			goto end;

		byte_reset(&ctx->rcv_data.byte);
		if (ctx->rcv.receiving) {
			if_schedule_receive(iface, ctx->rcv_data.pkt);
			ctx->rcv_data.pkt = NULL;
		}
		if (ctx->rcv_data.pkt == NULL) {
			if ((ctx->rcv_data.pkt = pkt_get(iface->pkt_pool))
			    == NULL) {
				DEBUG_LOG("%s: cannot alloc pkt\n", __func__);
				if_schedule_receive(iface, NULL);
				goto end;
			}
		} else
			buf_reset(&ctx->rcv_data.pkt->buf);
		ctx->rcv.receiving = 1;
		return;
	}
	if (ctx->rcv.receiving && ctx->rcv_data.pkt
	    && pkt_len(ctx->rcv_data.pkt)) {
		if_schedule_receive(iface, ctx->rcv_data.pkt);
		ctx->rcv_data.pkt = NULL;
	}
 end:
	ctx->rcv.receiving = 0;
	byte_reset(&ctx->rcv_data.byte);
#ifdef CONFIG_RF_SENDER
	__rf_start_sending((iface_t *)iface);
#endif
}

static void rf_rcv_tim_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;
	uint8_t v, not_v;
#ifdef RF_ANALOG_SAMPLING
	uint16_t v_analog;
#endif
	timer_reschedule(&ctx->timer, RF_SAMPLING_US);

#ifdef RF_ANALOG_SAMPLING
	v_analog = analog_read(RF_RCV_PIN_NB);

	if (v_analog < ANALOG_LOW_VAL)
		v = 0;
	else if (v_analog > ANALOG_HI_VAL)
		v = 1;
	else {
#ifdef CONFIG_RF_SENDER
		__rf_start_sending((iface_t *)iface);
#endif
		return;
	}
#else
	v = RF_RCV_PIN & (1 << RF_RCV_PIN_NB);
#endif
	not_v = !v;
	if (ctx->rcv.prev_val == not_v) {
		ctx->rcv.prev_val = v;
		rf_fill_data(iface, not_v);
		ctx->rcv.cnt = 0;
	} else
		ctx->rcv.cnt++;
}
#endif

#ifdef RF_DEBUG
static int rf_debug_send(rf_ctx_t *src, uint8_t byte)
{
	rf_ctx_t *dst;
	iface_t *iface;

	if (src == rf_debug_iface2->priv) {
		dst = rf_debug_iface1->priv;
		iface = rf_debug_iface1;
	}  else if (src == rf_debug_iface1->priv) {
		dst = rf_debug_iface2->priv;
		iface = rf_debug_iface2;
	} else
		__abort();

	if (dst->rcv_data.pkt == NULL) {
		if ((dst->rcv_data.pkt = pkt_get(iface->pkt_pool)) == NULL) {
			DEBUG_LOG("%s no pkts\n", __func__);
			if_schedule_receive(iface, NULL);
			return -1;
		}
	}
	if (buf_addc(&dst->rcv_data.pkt->buf, byte) < 0) {
		DEBUG_LOG("%s: buf len:%d\n", __func__,
			  dst->rcv_data.pkt->buf.len);
		return -1;
	}
	return 0;
}
#endif

#ifdef CONFIG_RF_SENDER
/* returns -1 if no data sent */
static int rf_snd(rf_ctx_t *ctx)
{
	if (ctx->snd.bit) {
		ctx->snd.bit--;
		return 0;
	}
	if (ctx->snd.frame_pos < START_FRAME_LENGTH) {
		/* send garbage data to calibrate the RF sender */
		RF_SND_PORT ^= 1 << RF_SND_PIN_NB;
		ctx->snd.frame_pos++;
		return 0;
	}

	if (ctx->snd.frame_pos == START_FRAME_LENGTH) {
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
		ctx->snd.frame_pos++;
		return 0;
	}
	if (ctx->snd.frame_pos
	    <= FRAME_DELIMITER_LENGTH + START_FRAME_LENGTH) {
		ctx->snd.frame_pos++;
		return 0;
	}

	if (byte_is_empty(&ctx->snd_data.byte)) {
		uint8_t c;

		if (buf_len(&ctx->snd_data.buf) == 0) {
			if ((RF_SND_PORT & (1 << RF_SND_PIN_NB)) == 0) {
				RF_SND_PORT |= 1 << RF_SND_PIN_NB;
				return 0;
			}
#ifndef CONFIG_RF_BURST
			return -1;
#else
			if (ctx->snd.burst_cnt == ctx->burst)
				return -1;
			ctx->snd.burst_cnt++;
			__buf_reset_keep(&ctx->snd_data.buf);
			ctx->snd.frame_pos = START_FRAME_LENGTH;
			return 0;
#endif
		}
		__buf_getc(&ctx->snd_data.buf, &c);
		byte_init(&ctx->snd_data.byte, c);
#ifdef RF_DEBUG
		if (rf_debug_send(ctx, c) < 0)
			return -1;
#endif
	}

	/* use 2 loops for '1' value */
	ctx->snd.bit = byte_get_bit(&ctx->snd_data.byte) ? 2 : 0;

#ifdef CONFIG_RF_CHECKS
	rf_add_bit(&rf_check_recv_ctx, ctx->snd.bit ? 1 : 0);
#endif

	/* send bit */
	if (ctx->snd.clk == 0)
		RF_SND_PORT |= 1 << RF_SND_PIN_NB;
	else
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
	ctx->snd.clk ^= 1;
	return 0;
}

static void rf_start_sending(const iface_t *iface)
{
	rf_ctx_t *ctx = iface->priv;

	if (ctx->rcv.receiving)
		return;
	timer_del(&ctx->timer);
	timer_add(&ctx->timer, RF_SAMPLING_US * 2, rf_snd_tim_cb,
		  (void *)iface);
}

int rf_output(const iface_t *iface, pkt_t *pkt)
{
	if (pkt_put(iface->tx, pkt) < 0)
		return -1;
	rf_start_sending(iface);
	return 0;
}
#endif

#ifdef CONFIG_RF_SENDER
static void rf_snd_tim_cb(void *arg)
{
	iface_t *iface = arg;
	rf_ctx_t *ctx = iface->priv;
#ifdef RF_DEBUG
	rf_ctx_t *debug_ctx1 = rf_debug_iface1->priv;
	rf_ctx_t *debug_ctx2 = rf_debug_iface2->priv;
#endif
	while (1) {
		if (ctx->snd_data.pkt == NULL) {
			pkt_t *pkt;

			if ((ctx->snd_data.pkt = pkt_get(iface->tx)) == NULL)
				break;
			pkt = ctx->snd_data.pkt;
			buf_init(&ctx->snd_data.buf, pkt->buf.data,
				 pkt->buf.len);
		}
		if (rf_snd(ctx) >= 0) {
			timer_add(&ctx->timer, RF_SAMPLING_US * 2,
				  rf_snd_tim_cb, arg);
			return;
		}
		RF_SND_PORT &= ~(1 << RF_SND_PIN_NB);
		if_schedule_tx_pkt_free(ctx->snd_data.pkt);
		memset(&ctx->snd, 0, sizeof(ctx->snd));
		ctx->snd_data.pkt = NULL;
		ctx->snd.frame_pos = START_FRAME_LENGTH;
#ifdef RF_DEBUG
		if (iface == rf_debug_iface1) {
			if_schedule_receive(rf_debug_iface2,
					    debug_ctx2->rcv_data.pkt);
			debug_ctx2->rcv_data.pkt = NULL;
		} else {
			if_schedule_receive(rf_debug_iface1,
					    debug_ctx1->rcv_data.pkt);
			debug_ctx1->rcv_data.pkt = NULL;
		}
#endif
	}
	ctx->snd.frame_pos = 0;
#ifdef CONFIG_RF_RECEIVER
#ifndef RF_DEBUG
	timer_add(&ctx->timer, RF_SAMPLING_US, rf_rcv_tim_cb, arg);
#endif
#endif
}
#endif

#ifdef CONFIG_RF_CHECKS
#ifdef CONFIG_RF_RECEIVER
static uint8_t received;

static void
rf_checks_event_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	if (events & EV_READ)
		received = 1;
}

static void
rf_checks_send_data(const iface_t *iface, uint8_t cnt, uint8_t bit)
{
	rf_ctx_t *ctx = iface->priv;

	ctx->rcv.cnt = cnt;
	rf_fill_data(iface, bit);
}

static void rf_simulate_garbage_data(const iface_t *iface)
{
	unsigned i;

	for (i = 0; i < 0xFF; i++) {
		uint8_t cnt = rand();
		uint8_t bit = rand() & 0x1;

		rf_checks_send_data(iface, cnt, bit);
	}
}

static void
rf_simulate_sending_data(const iface_t *iface, const sbuf_t *sbuf)
{
	int i;
	uint8_t clk = 1, bit;

	rf_simulate_garbage_data(iface);

	/* send frame delimiter */
	rf_checks_send_data(iface, 60, 0);

	for (i = 0; i < sbuf->len; i++) {
		int j;
		uint8_t d = sbuf->data[i];

		for (j = 0; j < 8; j++) {
			uint8_t cnt;

			bit = (d & 0x80) >> 7;
			d <<= 1;
			cnt = bit ? 4 : 2;
			rf_checks_send_data(iface, cnt, clk);
			clk ^= 1;
		}
	}
	if (clk == 0)
		rf_checks_send_data(iface, 2, 1);

	/* send frame delimiter */
	rf_checks_send_data(iface, 60, 0);
}

static void rf_receive_checks(iface_t *iface)
{
	sbuf_t sbuf;
	uint8_t data[] = {
		0x69, 0x68, 0x03, 0xAF, 0xB9, 0x74, 0x65, 0x73, 0x74,
	};
	uint8_t addr = 0x69;
	uint8_t *cur_addr = iface->hw_addr;

	/* save current address */
	iface->hw_addr = &addr;

	swen_ev_set(rf_checks_event_cb);
	sbuf_init(&sbuf, data, sizeof(data));
	rf_simulate_sending_data(iface, &sbuf);
	while (received == 0)
		scheduler_run_tasks();

	/* restore address */
	iface->hw_addr = cur_addr;
}
#endif

#ifdef CONFIG_RF_SENDER
static int rf_buffer_checks(rf_ctx_t *ctx, pkt_t *pkt)
{
	int i;
	int data_len = pkt->buf.len;
	int burst = 0;

#ifdef CONFIG_RF_BURST
	burst = ctx->burst;
#endif
	ctx->snd_data.pkt = pkt;
	ctx->snd_data.buf = pkt->buf;
	rf_check_recv_ctx.rcv_data.pkt = pkt_recv;

	/* send bytes */
	while (rf_snd(ctx) >= 0) {}

	if (pkt_recv->buf.len == 0) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}

	if (pkt_recv->buf.len != data_len * (burst + 1)) {
		DEBUG_LOG("%s:%d failed (len:%d, expected: %d)\n",
			  __func__, __LINE__, pkt_recv->buf.len,
			  data_len * burst + 1);
		return -1;
	}
	for (i = 0; i < burst; i++) {
		buf_t tmp;

		buf_init(&tmp, pkt_recv->buf.data + (pkt->buf.len * i),
			 pkt->buf.len);
		if (buf_cmp(&pkt->buf, &tmp) != 0) {
			DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
			return -1;
		}
	}

	return 0;
}

static int rf_send_checks(const iface_t *iface)
{
	uint8_t data[] = {
		0x69, 0x70, 0x00, 0x10, 0xC8, 0xA0, 0x4B, 0xF7,
		0x17, 0x7F, 0xE7, 0x81, 0x92, 0xE4, 0x0E, 0xA3,
		0x83, 0xE7, 0x29, 0x74, 0x34, 0x03
	};
	uint8_t data2[] = {
		0x69, 0x70, 0x00, 0x10, 0xC8, 0xA0, 0x4B, 0xF7,
		0x17, 0x7F, 0xE7, 0x81, 0x92, 0xE4, 0x0E, 0xA3,
		0x00
	};
	rf_ctx_t *ctx = iface->priv;
	pkt_t *pkt;

	pkt = pkt_alloc();
	pkt_recv = pkt_alloc();

	if (pkt == NULL || pkt_recv == NULL) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}

	buf_init(&pkt->buf, data, sizeof(data));
	if (rf_buffer_checks(ctx, pkt) < 0) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}
	memset(&ctx->snd, 0, sizeof(ctx->snd));
	buf_reset(&pkt_recv->buf);
	buf_init(&pkt->buf, data2, sizeof(data2));

	if (rf_buffer_checks(ctx, pkt) < 0) {
		DEBUG_LOG("%s:%d failed\n", __func__, __LINE__);
		return -1;
	}

	memset(&ctx->snd, 0, sizeof(ctx->snd));
	pkt_free(pkt_recv);
	pkt_free(pkt);

	return 0;
}
#endif

int rf_checks(iface_t *iface)
{
	rf_ctx_t *ctx = iface->priv;

#ifdef CONFIG_RF_SENDER
	if (rf_send_checks(iface) < 0)
		return -1;
#endif
#ifdef CONFIG_RF_RECEIVER
	timer_del(&ctx->timer);
	rf_receive_checks(iface);
	timer_reschedule(&ctx->timer, RF_SAMPLING_US);
#endif
#ifndef TEST
	DEBUG_LOG("RF checks succeeded\n");
#endif
	return 0;
}
#endif

void rf_init(iface_t *iface, rf_ctx_t *ctx, uint8_t burst)
{
	timer_init(&ctx->timer);
#ifdef CONFIG_RF_RECEIVER
#if !defined(X86) && !defined(RF_DEBUG)
	timer_add(&ctx->timer, RF_SAMPLING_US, rf_rcv_tim_cb, iface);
#else
	(void)rf_rcv_tim_cb;
#endif
#ifndef RF_ANALOG_SAMPLING
	RF_RCV_PORT &= ~(1 << RF_RCV_PIN_NB);
#endif
#endif

#if defined (CONFIG_RF_SENDER) && defined (CONFIG_RF_BURST)
#ifdef RF_DEBUG
	ctx->burst = 0;
#else
	if (burst)
		ctx->burst = burst - 1;
#endif
#endif
	iface->priv = ctx;
}

/* XXX Will never be used in an infinite loop. Save space, don't compile it */
#ifdef TEST
void rf_shutdown(const iface_t *iface)
{
	rf_ctx_t *ctx = iface->priv;

	timer_del(&ctx->timer);
}
#endif
