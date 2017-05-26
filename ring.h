#ifndef _RING_H_
#define _RING_H_

#define MAX_SIZE 128
#define MASK (MAX_SIZE - 1)

typedef struct byte {
	unsigned char c;
	int pos;
} byte_t;

typedef struct ring {
	int head;
	int tail;
	byte_t byte;
	unsigned char data[MAX_SIZE];
} ring_t;

/*
      0 1 2 3
head    |
tail  |
size = 3

*/

static inline int ring_is_full(const ring_t *ring)
{
	if (((MASK + ring->tail - ring->head) & MASK) == 0)
		return 1;
	return 0;
}

static inline int ring_is_empty(const ring_t *ring)
{
	return ring->tail == ring->head;
}

static inline int ring_is_empty2(const ring_t *ring)
{
	return ring->tail == ring->head && ring->byte.pos == 0;
}

static inline int ring_len(const ring_t *ring)
{
	return MASK - ((MASK + ring->tail - ring->head) & MASK);
}

static inline void reset_byte(byte_t *byte)
{
	/* byte->c = 0; */
	byte->pos = 0;
}

static inline void ring_reset(ring_t *ring)
{
	ring->tail = ring->head;
	reset_byte(&ring->byte);
}

static inline int ring_addc(ring_t *ring, unsigned char c)
{
	if (ring_is_full(ring))
		return -1;
	ring->data[ring->head] = c;
	ring->head = (ring->head + 1) & MASK;
	return 0;
}

static inline int ring_getc(ring_t *ring, unsigned char *c)
{
	if (ring_is_empty(ring)) {
		return -1;
	}
	*c = (unsigned char)ring->data[ring->tail];
	ring->tail = (ring->tail + 1) & MASK;
	return 0;
}

/* static inline void ring_skip(ring_t *ring, int skip) */
/* { */

/* } */

static inline void print_byte(const byte_t *byte)
{
	int j;

	for (j = 0; j < byte->pos; j++) {
		printf("%s", ((byte->c  >> (7 - j)) & 1) ? "X" : " ");
	}
}

static inline void ring_print(const ring_t *ring)
{
	int i;

	if (ring_is_empty(ring))
		return;

	for (i = 0; i < ring_len(ring); i++) {
		int pos = (ring->tail + i) & MASK;

		printf("0x%X ", ring->data[pos]);
	}
	puts("");
}

static inline void ring_print_bits(const ring_t *ring)
{
	int i;

	if (ring_is_empty(ring))
		return;

	for (i = 0; i < ring_len(ring); i++) {
		int pos = (ring->tail + i) & MASK;
		unsigned char byte = ring->data[pos];
		int j;

		for (j = 0; j < 8; j++) {
			printf("%s", ((byte >> (7 - j)) & 1) ? "X" : " ");
		}
	}
	if (ring->byte.pos)
		printf("byte:");
	print_byte(&ring->byte);
	puts("");
}

static inline int ring_add_bit(ring_t *ring, int bit)
{
	if (ring_is_full(ring))
		return -1;

	ring->byte.c = (ring->byte.c << 1) | bit;
	ring->byte.pos++;

	if (ring->byte.pos >= 8) {
		ring->byte.pos = 0;
		ring_addc(ring, ring->byte.c);
		ring->byte.c = 0;
		ring->byte.pos = 0;
	}
	return 0;
}

static inline int ring_cmp(const ring_t *ring, const unsigned char *str, int len)
{
	int i;

	if (len == 0 || ring_len(ring) != len)
		return -1;

	for (i = 0; i < len; i++) {
		int pos = (ring->tail + i) & MASK;

		if (ring->data[pos] != str[i])
			return -1;
	}
	return 0;
}
#endif
