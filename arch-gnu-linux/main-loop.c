/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/*
 * This is the main loop for posix stuff.
 */
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <linux/if_ether.h>

#include <pproto/pproto.h>
#include <pproto/diag.h>
#include "posix.h"

void posix_main_loop(struct pp_instance *ppi)
{
	int delay_ms;

	/*
	 * The main loop here is based on select. While we are not
	 * doing anything else but the protocol, this allows extra stuff
	 * to fit.
	 */
	delay_ms = pp_state_machine(ppi, NULL, 0);
	while (1) {
		fd_set set;
		int i;
		struct timeval tv;
		unsigned char packet[1500];

		/* Wait for a packet or for the timeout */
		tv.tv_sec = delay_ms / 1000;
		tv.tv_usec = (delay_ms % 1000) * 1000;

	again:
		FD_ZERO(&set);
		FD_SET(ppi->ch.fd, &set);
		i = select(ppi->ch.fd + 1, &set, NULL, NULL, &tv);
		if (i < 0 && errno != EINTR)
			exit(__LINE__);
		if (i < 0)
			continue;

		if (i == 0) {
			delay_ms = pp_state_machine(ppi, NULL, 0);
			continue;
		}

		/*
		 * We got a packet. If it's not ours, continue consuming
		 * the pending timeout
		 */
		i = posix_recv_packet(ppi, packet, sizeof(packet));
		/* FIXME
		if (i < sizeof(struct pp_packet))
			goto again;
		*/
		/* Warning: PP_PROTO_NR is endian-agnostic by design */
		if ( ((struct ethhdr *)packet)->h_proto != htons(PP_PROTO_NR))
			goto again;

		delay_ms = pp_state_machine(ppi, packet, i);
	}
}
