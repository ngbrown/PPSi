
#include <stdint.h>

enum mtp_ptype {
	MTP_FORWARD = 0xb8,
	MTP_BACKWARD,
	MTP_BACKSTAMP
};

struct mtp_packet {
	uint32_t tragic;
	uint32_t ptype;
	struct timespec t[4][3];
};

static inline void tv_to_ts(struct timeval *tv, struct timespec *ts)
{
	ts->tv_sec = tv->tv_sec;
	ts->tv_nsec = tv->tv_usec * 1000;
}

extern void mtp_result(struct mtp_packet *pkt);
