#include <eeprom.h>
#include "module-common.h"

const uint32_t rf_enc_defkey[4] = {
	0xab9d6f04, 0xe6c82b9d, 0xefa78f03, 0xbc96f19c
};

#define MAGIC 0xA2
static uint8_t EEMEM eeprom_magic;

#define TX_QUEUE_SIZE 4
static RING_DECL(tx_queue, TX_QUEUE_SIZE);
static RING_DECL(urgent_tx_queue, TX_QUEUE_SIZE);

void module_init_iface(iface_t *iface, uint8_t *addr)
{
	iface->hw_addr = addr;
#ifdef CONFIG_RF_RECEIVER
	iface->recv = rf_input;
#endif
#ifdef CONFIG_RF_SENDER
	iface->send = rf_output;
#endif
	iface->flags = IF_UP|IF_RUNNING|IF_NOARP;

	if_init(iface, IF_TYPE_RF, CONFIG_PKT_NB_MAX, CONFIG_PKT_NB_MAX,
		CONFIG_PKT_DRIVER_NB_MAX, 1);
	rf_init(iface, 1);
}

int module_check_magic(void)
{
	uint8_t magic;

	eeprom_load(&magic, &eeprom_magic, sizeof(magic));
	return magic == MAGIC;
}

void module_update_magic(void)
{
	uint8_t magic = MAGIC;

	eeprom_update(&eeprom_magic, &magic, sizeof(magic));
}

static void module_task_cb(void *arg)
{
	uint8_t op = (uintptr_t)arg;

	module_add_op(op, 1);
}

int __module_add_op(ring_t *queue, uint8_t op)
{
	uint8_t cur_op;

	if (__module_get_op(queue, &cur_op) >= 0 && cur_op == op)
		return 0;
	return ring_addc(queue, op);
}

void module_add_op(uint8_t op, uint8_t urgent)
{
	ring_t *queue = urgent ? urgent_tx_queue : tx_queue;

	if (__module_add_op(queue, op) < 0) {
		assert(0);
		schedule_task(module_task_cb, (void *)(uintptr_t)op);
	}
}

int __module_get_op(ring_t *queue, uint8_t *op)
{
	if (ring_is_empty(queue))
		return -1;
	__ring_getc_at(queue, op, 0);
	return 0;
}

int module_get_op(uint8_t *op)
{
	if (__module_get_op(urgent_tx_queue, op) < 0)
		return __module_get_op(tx_queue, op);
	return 0;
}

void __module_skip_op(ring_t *queue)
{
	__ring_skip(queue, 1);
}

void __module_reset_op_queue(ring_t *queue)
{
	ring_reset(queue);
}

void module_reset_op_queues(void)
{
	ring_reset(urgent_tx_queue);
	ring_reset(tx_queue);
}

void module_skip_op(void)
{
	ring_t *ring;

	if (!ring_is_empty(urgent_tx_queue))
		ring = urgent_tx_queue;
	else
		ring = tx_queue;
	__module_skip_op(ring);
}

int
send_rf_msg(swen_l3_assoc_t *assoc, uint8_t cmd, const void *data, int len)
{
	buf_t buf = BUF(sizeof(uint8_t) + len);
	sbuf_t sbuf;

	__buf_addc(&buf, cmd);
	if (data)
		__buf_add(&buf, data, len);
	sbuf = buf2sbuf(&buf);
	return swen_l3_send(assoc, &sbuf);
}

void module_set_default_cfg(module_cfg_t *cfg)
{
	memset(cfg, 0, sizeof(module_cfg_t));
	cfg->state = MODULE_STATE_DISABLED;
	cfg->humidity_threshold = DEFAULT_HUMIDITY_THRESHOLD;
	cfg->siren_duration = DEFAULT_SIREN_ON_DURATION;
}
