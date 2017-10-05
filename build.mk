ifeq ($(CONFIG_ARCH),AVR)
SOURCES += alarm.c
LDFLAGS += -DF_CPU=${CONFIG_AVR_F_CPU} -mmcu=${CONFIG_AVR_MCU}
CFLAGS += -DF_CPU=$(CONFIG_AVR_F_CPU) -DCONFIG_AVR_F_CPU=$(CONFIG_AVR_F_CPU)
CFLAGS += -Wno-deprecated-declarations -D__PROG_TYPES_COMPAT__
endif

ifdef NEED_ERRNO
COMMON += sys/errno.c
endif

ifdef CONFIG_RF
SOURCES += rf.c
CFLAGS += -DCONFIG_RF
endif
