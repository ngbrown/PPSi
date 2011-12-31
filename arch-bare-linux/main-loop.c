
/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */

/*
 * This is the main loop for "freestanding" stuff under Linux.
 */
#include <pproto/pproto.h>
#include "bare-linux.h"

/* Define other hackish stuff */
typedef struct {unsigned long bits[1024/32];} bare_fd_set;
#define FD_ZERO(p) memset(p, 0, sizeof(p))
#define FD_SET(bit, p) ((p)->bits[0] |= (1 << (bit)))

struct bare_timeval {unsigned long tv_sec; unsigned long tv_usec;};

#define NULL 0

void bare_main_loop(struct pp_instance *ppi)
{
	int delay_ms;

	/*
	 * The main loop here is based on select. While we are not
	 * doing anything else but the protocol, this allows extra stuff
	 * to fit.
	 */
	delay_ms = pp_state_machine(ppi, NULL, 0);
	while (1) {
		bare_fd_set set;
		int i, maxfd;
		struct bare_timeval tv;
		unsigned char packet[1500];

		/* Wait for a packet or for the timeout */
		tv.tv_sec = delay_ms / 1000;
		tv.tv_usec = (delay_ms % 1000) * 1000;

	again:
		FD_ZERO(&set);
		FD_SET(ppi->net_path->gen_ch.fd, &set);
		FD_SET(ppi->net_path->evt_ch.fd, &set);
		maxfd = ppi->net_path->gen_ch.fd;
		if (ppi->net_path->evt_ch.fd > maxfd)
			maxfd = ppi->net_path->evt_ch.fd;

		i = sys_select(maxfd + 1, &set, NULL, NULL, &tv);
		if (i < 0 && bare_errno != 4 /* EINTR */)
			sys_exit(__LINE__);
		if (i < 0)
			continue;

		if (i == 0) {
			delay_ms = pp_state_machine(ppi, NULL, 0);
			continue;
		}

		/*
		 * We got a packet. If it's not ours, continue consuming
		 * the pending timeout.
		 *
		 * FIXME: we don't know which socket to receive from
		 */
		i = bare_recv_packet(ppi, packet, sizeof(packet));
		/* FIXME: PP_PROTO_NR is a legacy number */
		if ( ((struct bare_ethhdr *)packet)->h_proto
		     != htons(PP_PROTO_NR))
			goto again;

		delay_ms = pp_state_machine(ppi, packet, i);
	}
}
