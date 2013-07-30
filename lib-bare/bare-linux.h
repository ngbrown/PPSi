/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */
#include <string.h>
/*
 * These are the functions provided by the various bare files
 */

extern void bare_main_loop(struct pp_instance *ppi);

extern struct pp_network_operations bare_net_ops;
extern struct pp_time_operations bare_time_ops;

/* syscalls */
struct bare_sockaddr;

extern int sys_write(int fd, const void *buf, int count);
extern void sys_exit(int exitval);
extern int sys_time(int tz);
extern int sys_select(int max, void *in, void *out, void *exc, void *tout);
extern int sys_ioctl(int fd, int cmd, void *arg);
extern int sys_socket(int domain, int type, int proto);
extern int sys_bind(int fd, const struct bare_sockaddr *addr, int addrlen);
extern int sys_recv(int fd, void *pkt, int plen, int flags);
extern int sys_send(int fd, void *pkt, int plen, int flags);
extern int sys_shutdown(int fd, int flags);
extern int sys_close(int fd);
extern int sys_setsockopt(int fd, int level, int optname, const void *optval,
			  int optlen);
extern int sys_gettimeofday(void *tv, void *z);
extern int sys_settimeofday(void *tv, void *z);
extern int sys_adjtimex(void *tv);
extern int sys_clock_gettime(int clock, void *t);

extern int bare_errno;

/* structures passed to syscalls */
#define IFNAMSIZ 16

struct bare_sockaddr {
	uint16_t	sa_family;	/* address family, AF_xxx	*/
	char		sa_data[14];	/* 14 bytes of protocol address */
};

struct bare_ifreq {
	union {
		char	ifrn_name[IFNAMSIZ];
	} ifr_ifrn;

	union {
		struct	bare_sockaddr ifru_hwaddr;
		int	index;
	} ifr_ifru;
};

struct bare_sockaddr_ll {
	unsigned short int sll_family;
	unsigned short int sll_protocol;
	int sll_ifindex;
	unsigned short int sll_hatype;
	unsigned char sll_pkttype;
	unsigned char sll_halen;
	unsigned char sll_addr[8];
};

#define SIOCGIFINDEX	0x8933
#define SIOCGIFHWADDR	0x8927

#define SO_TIMESTAMP	29
#define SOL_SOCKET	1

/* from uapi/linux/if_ether.h */
#define ETH_ALEN	6	/* Octets in one ethernet addr */

/* start copy from linux/socket.h */

/* Setsockoptions(2) level. Thanks to BSD these must match IPPROTO_xxx */
#define SOL_IP		0
/* #define SOL_ICMP	1	No-no-no! We cannot use SOL_ICMP=1 */
#define SOL_TCP		6
#define SOL_UDP		17
#define SOL_IPV6	41
#define SOL_ICMPV6	58
#define SOL_SCTP	132
#define SOL_UDPLITE	136	/* UDP-Lite (RFC 3828) */
#define SOL_RAW		255
#define SOL_IPX		256
#define SOL_AX25	257
#define SOL_ATALK	258
#define SOL_NETROM	259
#define SOL_ROSE	260
#define SOL_DECNET	261
#define SOL_X25		262
#define SOL_PACKET	263
#define SOL_ATM		264	/* ATM layer (cell level) */
#define SOL_AAL		265	/* ATM Adaption Layer (packet level) */
#define SOL_IRDA	266
#define SOL_NETBEUI	267
#define SOL_LLC		268
#define SOL_DCCP	269
#define SOL_NETLINK	270
#define SOL_TIPC	271
#define SOL_RXRPC	272
#define SOL_PPPOL2TP	273
#define SOL_BLUETOOTH	274
#define SOL_PNPIPE	275
#define SOL_RDS		276
#define SOL_IUCV	277
#define SOL_CAIF	278
#define SOL_ALG		279

/* end copy from linux/socket.h */


/* start copy from uapi/linux/if_packet.h */
struct bare_packet_mreq {
	int		mr_ifindex;
	unsigned short	mr_type;
	unsigned short	mr_alen;
	unsigned char	mr_address[8];
};

#define PACKET_MR_MULTICAST	0
#define PACKET_MR_PROMISC	1
#define PACKET_MR_ALLMULTI	2
#define PACKET_MR_UNICAST	3

/* Packet types */
#define PACKET_HOST		0		/* To us		*/
#define PACKET_BROADCAST	1		/* To all		*/
#define PACKET_MULTICAST	2		/* To group		*/
#define PACKET_OTHERHOST	3		/* To someone else	*/
#define PACKET_OUTGOING		4		/* Outgoing of any type */
/* These ones are invisible by user level */
#define PACKET_LOOPBACK		5		/* MC/BRD frame looped back */
#define PACKET_FASTROUTE	6		/* Fastrouted frame	*/

/* Packet socket options */
#define PACKET_ADD_MEMBERSHIP		1
#define PACKET_DROP_MEMBERSHIP		2
#define PACKET_RECV_OUTPUT		3
/* Value 4 is still used by obsolete turbo-packet. */
#define PACKET_RX_RING			5
#define PACKET_STATISTICS		6
#define PACKET_COPY_THRESH		7
#define PACKET_AUXDATA			8
#define PACKET_ORIGDEV			9
#define PACKET_VERSION			10
#define PACKET_HDRLEN			11
#define PACKET_RESERVE			12
#define PACKET_TX_RING			13
#define PACKET_LOSS			14
#define PACKET_VNET_HDR			15
#define PACKET_TX_TIMESTAMP		16
#define PACKET_TIMESTAMP		17
#define PACKET_FANOUT			18
/* end copy from linux/if_packet.h */


/* other network stuff, bah.... */
struct bare_ethhdr {
	unsigned char	h_dest[6];
	unsigned char	h_source[6];
	uint16_t	h_proto;
} __attribute__((packed));

struct bare_timeval {
	long tv_sec;
	long tv_usec;
};

struct bare_timespec {
	long tv_sec;
	long tv_nsec;
};

#ifndef NULL
#define NULL 0
#endif

/* from linux/timex.h */

/*
 * Mode codes (timex.mode)
 */
#define ADJ_OFFSET		0x0001	/* time offset */
#define ADJ_FREQUENCY		0x0002	/* frequency offset */
#define ADJ_MAXERROR		0x0004	/* maximum time error */
#define ADJ_ESTERROR		0x0008	/* estimated time error */
#define ADJ_STATUS		0x0010	/* clock status */
#define ADJ_TIMECONST		0x0020	/* pll time constant */
#define ADJ_TAI			0x0080	/* set TAI offset */
#define ADJ_SETOFFSET		0x0100	/* add 'time' to current time */
#define ADJ_MICRO		0x1000	/* select microsecond resolution */
#define ADJ_NANO		0x2000	/* select nanosecond resolution */
#define ADJ_TICK		0x4000	/* tick value */

#define ADJ_OFFSET_SINGLESHOT	0x8001	/* old-fashioned adjtime */
#define ADJ_OFFSET_SS_READ	0xa001	/* read-only adjtime */

/* xntp 3.4 compatibility names */
#define MOD_OFFSET	ADJ_OFFSET
#define MOD_FREQUENCY	ADJ_FREQUENCY
#define MOD_MAXERROR	ADJ_MAXERROR
#define MOD_ESTERROR	ADJ_ESTERROR
#define MOD_STATUS	ADJ_STATUS
#define MOD_TIMECONST	ADJ_TIMECONST
#define MOD_TAI	ADJ_TAI
#define MOD_MICRO	ADJ_MICRO
#define MOD_NANO	ADJ_NANO

struct bare_timex {
	unsigned int modes;	/* mode selector */
	long offset;		/* time offset (usec) */
	long freq;		/* frequency offset (scaled ppm) */
	long maxerror;		/* maximum error (usec) */
	long esterror;		/* estimated error (usec) */
	int status;		/* clock command/status */
	long constant;		/* pll time constant */
	long precision;		/* clock precision (usec) (read only) */
	long tolerance;		/* clock frequency tolerance (ppm)
				 * (read only)
				 */
	struct bare_timeval time;	/* (RO, except for ADJ_SETOFFSET) */
	long tick;		/* (modified) usecs between clock ticks */

	long ppsfreq;		/* pps frequency (scaled ppm) (ro) */
	long jitter;		/* pps jitter (us) (ro) */
	int shift;		/* interval duration (s) (shift) (ro) */
	long stabil;		/* pps stability (scaled ppm) (ro) */
	long jitcnt;		/* jitter limit exceeded (ro) */
	long calcnt;		/* calibration intervals (ro) */
	long errcnt;		/* calibration errors (ro) */
	long stbcnt;		/* stability limit exceeded (ro) */

	int tai;		/* TAI offset (ro) */

	int  :32; int  :32; int  :32; int  :32;
	int  :32; int  :32; int  :32; int  :32;
	int  :32; int  :32; int  :32;
};


/*
 * Copy from <linux/time.h>
 *
 * The IDs of the various system clocks (for POSIX.1b interval timers):
 */
#define CLOCK_REALTIME			0
#define CLOCK_MONOTONIC			1
#define CLOCK_PROCESS_CPUTIME_ID	2
#define CLOCK_THREAD_CPUTIME_ID		3
#define CLOCK_MONOTONIC_RAW		4
#define CLOCK_REALTIME_COARSE		5
#define CLOCK_MONOTONIC_COARSE		6
