/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

/*
 * This is the main loop for "freestanding" stuff under Linux.
 */
#include <ppsi/ppsi.h>
#include "bare-linux.h"

/* Define other hackish stuff */
struct bare_fd_set {
	unsigned long bits[1024/32];
};

#define FD_ZERO(p)	memset(p, 0, sizeof(p))
#define FD_SET(bit, p)	((p)->bits[0] |= (1 << (bit)))

void bare_main_loop(struct pp_instance *ppi)
{
	int delay_ms;

	NP(ppi)->ptp_offset = 14;

	/*
	 * The main loop here is based on select. While we are not
	 * doing anything else but the protocol, this allows extra stuff
	 * to fit.
	 */
	ppi->is_new_state = 1;
	delay_ms = pp_state_machine(ppi, NULL, 0);
	while (1) {
		struct bare_fd_set set;
		int i, maxfd;
		struct bare_timeval tv;

		/* Wait for a packet or for the timeout */
		tv.tv_sec = delay_ms / 1000;
		tv.tv_usec = (delay_ms % 1000) * 1000;

	again:
		FD_ZERO(&set);
		FD_SET(NP(ppi)->ch[PP_NP_GEN].fd, &set);
		FD_SET(NP(ppi)->ch[PP_NP_EVT].fd, &set);
		maxfd = NP(ppi)->ch[PP_NP_GEN].fd;
		if (NP(ppi)->ch[PP_NP_EVT].fd > maxfd)
			maxfd = NP(ppi)->ch[PP_NP_EVT].fd;

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
		i = ppi->n_ops->recv(ppi, ppi->rx_frame,
				     PP_MAX_FRAME_LENGTH - 4,
				     &ppi->last_rcv_time);

		/* we passed payload but it filled the ether header too */
		if (((struct bare_ethhdr *)(ppi->rx_frame))->h_proto
		     != htons(PP_ETHERTYPE))
			goto again;

		delay_ms = pp_state_machine(ppi, ppi->rx_ptp,
					    i - NP(ppi)->ptp_offset);
	}
}
