
/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */

/*
 * This is the main loop for the Spec board
 */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include <syscon.h>
#include "spec.h"

void spec_main_loop(struct pp_instance *ppi)
{
	int i, delay_ms;

	const int eth_ofst = sizeof(struct spec_ethhdr);
	/*pp_diag_verbosity = 1;*/
	/* SPEC is raw ethernet by default */
	NP(ppi)->proto_ofst = eth_ofst;

	/* FIXME now it is end-to-end mode by default */
	OPTS(ppi)->e2e_mode = 1;
	/*
	 * The main loop here is polling every ms. While we are not
	 * doing anything else but the protocol, this allows extra stuff
	 * to fit.
	 */

	delay_ms = pp_state_machine(ppi, NULL, 0);
	while (1) {
		unsigned char _packet[1500];
		/* FIXME Alignment */
		unsigned char *packet = _packet + 2;

		/* Wait for a packet or for the timeout */
		while (delay_ms && !minic_poll_rx()) {
			timer_delay(1000);
			delay_ms--;
		}
		if (!minic_poll_rx()) {
			delay_ms = pp_state_machine(ppi, NULL, 0);
			continue;
		}
		/*
		 * We got a packet. If it's not ours, continue consuming
		 * the pending timeout
		 */
		i = spec_recv_packet(ppi, packet, sizeof(_packet), &ppi->last_rcv_time);
		if (0) {
			int j;
			pp_printf("recvd: %i\n", i);
			for (j = 0; j < i - eth_ofst; j++) {
				pp_printf("%02x ", packet[j + eth_ofst]);
				if( (j+1)%16==0 )
					pp_printf("\n");
			}
			pp_printf("\n");
		}
		/* Warning: PP_ETHERTYPE is endian-agnostic by design */
		if (((struct spec_ethhdr *)packet)->h_proto !=
		     htons(PP_ETHERTYPE))
			continue;
		delay_ms = pp_state_machine(ppi, packet + eth_ofst, i - eth_ofst);
	}
	pp_printf("OUT MAIN LOOP\n");
}
