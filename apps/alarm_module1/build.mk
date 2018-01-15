ifdef CONFIG_SERIAL_SPEED
CFLAGS += -DCONFIG_SERIAL_SPEED=$(CONFIG_SERIAL_SPEED)
endif

ifeq "$(or $(CONFIG_RF_RECEIVER), $(CONFIG_RF_SENDER))" "y"
SOURCES += ../../rf.c ../../sys/chksum.c
endif
ifdef CONFIG_RF_RECEIVER
CFLAGS += -DCONFIG_RF_RECEIVER
endif
ifdef CONFIG_RF_SENDER
CFLAGS += -DCONFIG_RF_SENDER
endif

ifdef CONFIG_AVR_SIMU
SOURCES += $(ARCH_DIR)/$(ARCH)/simu.c
CFLAGS += -DCONFIG_AVR_SIMU -DCONFIG_AVR_SIMU_PATH=$(CONFIG_AVR_SIMU_PATH)
endif

ifdef CONFIG_TIMER_CHECKS
CFLAGS += -DCONFIG_TIMER_CHECKS
endif
