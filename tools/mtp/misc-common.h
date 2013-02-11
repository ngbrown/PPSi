/* Common stuff for these misc tools */

#include <sys/types.h>

#ifndef SO_TIMESTAMPING
# define SO_TIMESTAMPING         37
# define SCM_TIMESTAMPING        SO_TIMESTAMPING
#endif

#ifndef SIOCSHWTSTAMP
# define SIOCSHWTSTAMP 0x89b0
#endif

#ifndef ETH_P_1588
# define ETH_P_1588   0x88F7
#endif

extern int make_stamping_socket(FILE *errchan, char *argv0, char *ifname,
				int tx_type, int rx_filter, int bits,
				unsigned char *macaddr, int proto);

extern ssize_t send_and_stamp(int sock, void *buf, size_t len, int flags);
extern ssize_t recv_and_stamp(int sock, void *buf, size_t len, int flags);

extern int print_stamp(FILE *out, char *prefix, FILE *err /* may be NULL */);

extern int get_stamp(struct timespec ts[4]);


/* Functions to make the difference between timevals and timespecs */
static inline int tvdiff(struct timeval *tv2, struct timeval *tv1)
{
    return (tv2->tv_sec - tv1->tv_sec) * 1000 * 1000
	+ tv2->tv_usec - tv1->tv_usec;
}

static inline int tsdiff(struct timespec *tv2, struct timespec *tv1)
{
    return (tv2->tv_sec - tv1->tv_sec) * 1000 * 1000 * 1000
	+ tv2->tv_nsec - tv1->tv_nsec;
}
