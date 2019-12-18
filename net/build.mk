#
# microdevt - Microcontroller Development Toolkit
#
# Copyright (c) 2017, Krzysztof Witek
# All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#
# The full GNU General Public License is included in this distribution in
# the file called "LICENSE".
#

ifdef CONFIG_TIMER_CHECKS
CFLAGS += -DCONFIG_TIMER_CHECKS
endif

SOURCES = ../sys/timer.c ../arch/$(ARCH)/timer.c ../sys/scheduler.c ../crypto/xtea.c

ifdef CONFIG_ETHERNET
SOURCES += eth.c
SOURCES += arp.c
CFLAGS += -DCONFIG_ETHERNET -DCONFIG_ARP
endif

SOURCES += if.c

ifdef CONFIG_IP
SOURCES += ip.c
CFLAGS += -DCONFIG_IP
ifdef CONFIG_IP_TTL
CFLAGS += -DCONFIG_IP_TTL=$(CONFIG_IP_TTL)
endif
endif

SOURCES += tr-chksum.c ../sys/chksum.c route.c

ifdef CONFIG_PKT_NB_MAX
CFLAGS += -DCONFIG_PKT_NB_MAX=$(CONFIG_PKT_NB_MAX)
CFLAGS += -DCONFIG_PKT_SIZE=$(CONFIG_PKT_SIZE)
CFLAGS += -DCONFIG_PKT_DRIVER_NB_MAX=$(CONFIG_PKT_DRIVER_NB_MAX)
SOURCES += pkt-mempool.c
ifdef CONFIG_PKT_MEM_POOL_EMERGENCY_PKT
CFLAGS += -DCONFIG_PKT_MEM_POOL_EMERGENCY_PKT=$(CONFIG_PKT_MEM_POOL_EMERGENCY_PKT)
endif
endif

ifdef CONFIG_ICMP
SOURCES += icmp.c
CFLAGS += -DCONFIG_ICMP
endif

ifdef CONFIG_UDP
SOURCES += udp.c
CFLAGS += -DCONFIG_UDP
endif

ifdef CONFIG_DNS
ifeq ($(CONFIG_UDP),)
$(error CONFIG_UDP is required for DNS)
endif
ifeq ($(CONFIG_EVENT),)
$(error CONFIG_EVENT is required for DNS)
endif
SOURCES += dns.c
CFLAGS += -DCONFIG_DNS
endif

ifdef CONFIG_TCP
SOURCES += tcp.c
CFLAGS += -DCONFIG_TCP
ifdef CONFIG_TCP_CLIENT
CFLAGS += -DCONFIG_TCP_CLIENT
endif
CFLAGS += -DCONFIG_TCP_MAX_CONNS=$(CONFIG_TCP_MAX_CONNS)
endif
ifdef CONFIG_TCP_RETRANSMIT
CFLAGS += -DCONFIG_TCP_RETRANSMIT
ifdef CONFIG_TCP_RETRANSMIT_TIMEOUT
CFLAGS += -DCONFIG_TCP_RETRANSMIT_TIMEOUT=$(CONFIG_TCP_RETRANSMIT_TIMEOUT)
else
CFLAGS += -DCONFIG_TCP_RETRANSMIT_TIMEOUT=3000
endif
endif

ifdef CONFIG_ARP_TABLE_SIZE
CFLAGS += -DCONFIG_ARP_TABLE_SIZE=$(CONFIG_ARP_TABLE_SIZE)
endif

ifdef CONFIG_ARP_EXPIRY
CFLAGS += -DCONFIG_ARP_EXPIRY
endif

ifeq "$(or $(CONFIG_UDP), $(CONFIG_TCP))" "y"
SOURCES += socket.c
CFLAGS += -DCONFIG_TRANSPORT_MAX_HT=$(CONFIG_TRANSPORT_MAX_HT)
CFLAGS += -DCONFIG_TCP_SYN_TABLE_SIZE=$(CONFIG_TCP_SYN_TABLE_SIZE)
CFLAGS += -DCONFIG_EPHEMERAL_PORT_START=$(CONFIG_EPHEMERAL_PORT_START)
CFLAGS += -DCONFIG_EPHEMERAL_PORT_END=$(CONFIG_EPHEMERAL_PORT_END)
endif

ifdef CONFIG_HT_STORAGE
CFLAGS += -DCONFIG_HT_STORAGE
ifeq ($(CONFIG_MAX_SOCK_HT_SIZE),)
$(error CONFIG_MAX_SOCK_HT_SIZE not set)
endif
CFLAGS += -DCONFIG_MAX_SOCK_HT_SIZE=$(CONFIG_MAX_SOCK_HT_SIZE)
SOURCES += ../sys/hash-tables.c
endif

ifdef CONFIG_BSD_COMPAT
CFLAGS += -DCONFIG_BSD_COMPAT
endif

ifdef CONFIG_EVENT
CFLAGS += -DCONFIG_EVENT
SOURCES += event.c
endif

ifdef CONFIG_SWEN
SOURCES += swen.c
CFLAGS += -DCONFIG_SWEN
ifdef CONFIG_SWEN_ROLLING_CODES
SOURCES += swen-rc.c
CFLAGS += -DCONFIG_SWEN_ROLLING_CODES
endif
ifdef CONFIG_RF_RECEIVER
CFLAGS += -DCONFIG_RF_RECEIVER
ifdef CONFIG_RF_GENERIC_COMMANDS
CFLAGS += -DCONFIG_RF_GENERIC_COMMANDS -DCONFIG_RF_GENERIC_COMMANDS_SIZE=$(CONFIG_RF_GENERIC_COMMANDS_SIZE)
endif
endif
ifdef CONFIG_RF_SENDER
CFLAGS += -DCONFIG_RF_SENDER
endif
ifdef CONFIG_SWEN_L3
SOURCES += swen-l3.c
CFLAGS += -DCONFIG_SWEN_L3
endif
endif

ifdef TEST
CFLAGS += -DTEST
SOURCES += tests.c
endif
