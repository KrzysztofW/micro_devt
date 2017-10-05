NET_SRC = net/eth.c net/if.c net/arp.c net/ip.c
NET_SRC += net/chksum.c net/pkt-mempool.c net/route.c

CFLAGS += -DCONFIG_PKT_NB_MAX=$(CONFIG_PKT_NB_MAX)
CFLAGS += -DCONFIG_PKT_SIZE=$(CONFIG_PKT_SIZE)

ifdef CONFIG_IP_TTL
CFLAGS += -DCONFIG_IP_TTL=$(CONFIG_IP_TTL)
endif

ifdef CONFIG_ICMP
NET_SRC += net/icmp.c
CFLAGS += -DCONFIG_ICMP
endif

ifdef CONFIG_UDP
NET_SRC += net/udp.c
CFLAGS += -DCONFIG_UDP
endif

ifdef CONFIG_TCP
NET_SRC += net/tcp.c
CFLAGS += -DCONFIG_TCP
endif

ifdef CONFIG_ARP_TABLE_SIZE
CFLAGS += -DCONFIG_ARP_TABLE_SIZE
endif

ifdef CONFIG_ARP_EXPIRY
CFLAGS += -DCONFIG_ARP_EXPIRY
endif

ifeq "$(or $(CONFIG_UDP), $(CONFIG_TCP))" "y"
NET_SRC += net/socket.c
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
NET_SRC += sys/hash-tables.c
endif

ifdef CONFIG_BSD_COMPAT
CFLAGS += -DCONFIG_BSD_COMPAT
endif
