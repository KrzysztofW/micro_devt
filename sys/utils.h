#ifndef _UTILS_H_
#define _UTILS_H_
#include <stdint.h>
#include <common.h>
#include <limits.h>

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({		      \
	const typeof(((type *)0)->member) *__mptr = (ptr);    \
	(type *)((char *)__mptr - offsetof(type, member));} )


#define POWEROF2(x) ((((x) - 1) & (x)) == 0)

#define __error__(msg) (void)({__asm__(".error \""msg"\"");})
#define STATIC_ASSERT(cond)                                             \
	__builtin_choose_expr(cond, (void)0,                            \
			      __error__("static assertion failed: "#cond""))

#define STATIC_IF(cond, if_statement, else_statement)			\
	__builtin_choose_expr(cond,					\
			      (void)({if_statement;}),			\
			      (void)({else_statement;}))

#define htons(x) (uint16_t)((x) >> 8 | (x) << 8)
#define ntohs(x) htons(x)
#define htonl(l) (uint32_t)(((l)<<24) | (((l)&0x00FF0000l)>>8) |	\
			    (((l)&0x0000FF00l)<<8) | ((l)>>24))
#define ntohl(x) htonl(x)

#define __STR(x) #x

#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define roundup_pwr2(x) (1 << ((sizeof(int) * 8) - __builtin_clz(x - 1)))

/* the checked value must be of type < (unsigned long long) */
#define IS_UNSIGNED(a) ((a) >= 0 && (typeof(a))~(a) >= 0)
#define MAX_VALUE_UNSIGNED(a)					\
	(((unsigned long long)1 << (sizeof(a) * CHAR_BIT)))
#define MAX_VALUE_SIGNED(a) (MAX_VALUE_UNSIGNED(a) >> 1)
#define MAX_VALUE(a)							\
	(IS_UNSIGNED(a) ? MAX_VALUE_UNSIGNED(a) : MAX_VALUE_SIGNED(a) - 1)

static inline long
map(long x, long in_min, long in_max, long out_min, long out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif
