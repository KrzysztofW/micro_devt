ifeq ($(CONFIG_ARCH),AVR)
CC = avr-gcc
EXECUTABLE = alarm
SOURCES += alarm.c enc28j60.c adc.c net_apps.c timer.c arch/avr/timer.c
ifeq ($(DEBUG), 1)
	SOURCES += usart0.c
endif
AVR_FLAGS = -DF_CPU=${CONFIG_AVR_F_CPU} -mmcu=${CONFIG_AVR_MCU} -Iarch/avr
AVR_FLAGS += -DF_CPU=$(CONFIG_AVR_F_CPU) -DCONFIG_AVR_MCU
AVR_FLAGS += -DCONFIG_AVR_F_CPU=$(CONFIG_AVR_F_CPU)
AVR_FLAGS += -Wno-deprecated-declarations -D__PROG_TYPES_COMPAT__
CFLAGS += $(AVR_FLAGS)
LDFLAGS += $(AVR_FLAGS)
OBJECTS = $(SOURCES:.c=.o)

ifdef CONFIG_TIMER_CHECKS
CFLAGS += -DCONFIG_TIMER_CHECKS
endif

ifdef CONFIG_RF
SOURCES += rf.c
CFLAGS += -DCONFIG_RF
endif

ifdef CONFIG_AVR_SIMU
CFLAGS += -DCONFIG_AVR_SIMU
endif

else ifeq ($(CONFIG_ARCH),X86_TUN_TAP)
ifndef CONFIG_TCP
$(error need CONFIG_TCP to compile the tun-driver)
endif
CC = gcc
EXECUTABLE = tun-driver
SOURCES := $(filter-out tests.c, $(SOURCES))
SOURCES := $(filter-out net/tests.c, $(SOURCES))
SOURCES += net/tun-driver.c net_apps.c timer.c
OBJECTS = $(SOURCES:.c=.o)
CFLAGS += -O0 -DX86
LDFLAGS += -lcap
else ifeq ($(CONFIG_ARCH),X86_TEST)
CC = gcc
EXECUTABLE = tests
SOURCES += tests.c net/tests.c $(COMMON) timer.c
# sys/hash-tables.c might be already in SOURCES
SOURCES := $(filter-out sys/hash-tables.c, $(SOURCES))
SOURCES += sys/hash-tables.c
OBJECTS = $(SOURCES:.c=.o)
CFLAGS += -O0 -DTEST -DX86
endif

ifdef NEED_ERRNO
COMMON += sys/errno.c
endif

CFLAGS += -DCONFIG_TIMER_RESOLUTION=$(CONFIG_TIMER_RESOLUTION)
