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
		int i;
		unsigned char packet[1500];

	again:

		i = posix_net_check_pkt(ppi, delay_ms);

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

		if (i < PP_PACKET_SIZE) {
			delay_ms = -1;
			goto again;
		}

		/* Warning: PP_PROTO_NR is endian-agnostic by design */
		if ( ((struct ethhdr *)packet)->h_proto != htons(PP_PROTO_NR)) {
			delay_ms = -1;
			goto again;
		}

		delay_ms = pp_state_machine(ppi, packet, i);
	}
}
