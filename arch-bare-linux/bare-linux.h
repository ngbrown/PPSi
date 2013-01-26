/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */

/*
 * These are the functions provided by the various bare files
 */
extern int bare_open_ch(struct pp_instance *ppi, char *name);

extern int bare_recv_packet(struct pp_instance *ppi, void *pkt, int len,
			    TimeInternal *t);
extern int bare_send_packet(struct pp_instance *ppi, void *pkt, int len,
			int chtype, int use_pdelay_addr);

extern void bare_main_loop(struct pp_instance *ppi);

/* basics */
extern void *memset(void *s, int c, int count);
extern char *strcpy(char *dest, const char *src);
extern void *memcpy(void *dest, const void *src, int count);

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

extern int sys_gettimeofday(void *tv, void *z);
extern int sys_settimeofday(void *tv, void *z);
extern int sys_adjtimex(void *tv);

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

/* other network stuff, bah.... */
struct bare_ethhdr {
	unsigned char	h_dest[6];
	unsigned char	h_source[6];
	uint16_t	h_proto;
} __attribute__((packed));

struct bare_timeval {
	unsigned long tv_sec;
	unsigned long tv_usec;
};

#ifndef NULL
#define NULL 0
#endif
