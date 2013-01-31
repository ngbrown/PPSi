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
			    TimeInternal *t, int chtype, int use_pdelay_addr);

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

/* other network stuff, bah.... */
struct bare_ethhdr {
	unsigned char	h_dest[6];
	unsigned char	h_source[6];
	uint16_t	h_proto;
} __attribute__((packed));

struct bare_timeval {
	unsigned long tv_sec;
	unsigned long tv_usec;
  	unsigned long tv_nsec;
};

#ifndef NULL
#define NULL 0
#endif

/* from linux/timex.h */

/*
 * Mode codes (timex.mode)
 */
#define ADJ_OFFSET              0x0001  /* time offset */
#define ADJ_FREQUENCY           0x0002  /* frequency offset */
#define ADJ_MAXERROR            0x0004  /* maximum time error */
#define ADJ_ESTERROR            0x0008  /* estimated time error */
#define ADJ_STATUS              0x0010  /* clock status */
#define ADJ_TIMECONST           0x0020  /* pll time constant */
#define ADJ_TAI                 0x0080  /* set TAI offset */
#define ADJ_MICRO               0x1000  /* select microsecond resolution */
#define ADJ_NANO                0x2000  /* select nanosecond resolution */
#define ADJ_TICK                0x4000  /* tick value */

#define ADJ_OFFSET_SINGLESHOT   0x8001  /* old-fashioned adjtime */
#define ADJ_OFFSET_SS_READ      0xa001  /* read-only adjtime */

/* xntp 3.4 compatibility names */
#define MOD_OFFSET      ADJ_OFFSET
#define MOD_FREQUENCY   ADJ_FREQUENCY
#define MOD_MAXERROR    ADJ_MAXERROR
#define MOD_ESTERROR    ADJ_ESTERROR
#define MOD_STATUS      ADJ_STATUS
#define MOD_TIMECONST   ADJ_TIMECONST

struct bare_timex {
  unsigned int modes;     /* mode selector */
  long offset;            /* time offset (usec) */
  long freq;              /* frequency offset (scaled ppm) */
  long maxerror;          /* maximum error (usec) */
  long esterror;          /* estimated error (usec) */
  int status;             /* clock command/status */
  long constant;          /* pll time constant */
  long precision;         /* clock precision (usec) (read only) */
  long tolerance;         /* clock frequency tolerance (ppm)
			   * (read only)
			   */
  struct bare_timeval time;    /* (read only) */
  long tick;              /* (modified) usecs between clock ticks */

  long ppsfreq;           /* pps frequency (scaled ppm) (ro) */
  long jitter;            /* pps jitter (us) (ro) */
  int shift;              /* interval duration (s) (shift) (ro) */
  long stabil;            /* pps stability (scaled ppm) (ro) */
  long jitcnt;            /* jitter limit exceeded (ro) */
  long calcnt;            /* calibration intervals (ro) */
  long errcnt;            /* calibration errors (ro) */
  long stbcnt;            /* stability limit exceeded (ro) */

  int tai;                /* TAI offset (ro) */

  int  :32; int  :32; int  :32; int  :32;
  int  :32; int  :32; int  :32; int  :32;
  int  :32; int  :32; int  :32;
};


/*                                                                                 
 * Copy from <linux/time.h>                                                        
 *                                                                                 
 * The IDs of the various system clocks (for POSIX.1b interval timers):            
 */
#define CLOCK_REALTIME                  0
#define CLOCK_MONOTONIC                 1
#define CLOCK_PROCESS_CPUTIME_ID        2
#define CLOCK_THREAD_CPUTIME_ID         3
#define CLOCK_MONOTONIC_RAW             4
#define CLOCK_REALTIME_COARSE           5
#define CLOCK_MONOTONIC_COARSE          6
