/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */

/*
 * This is the main loop for unix stuff.
 */
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <linux/if_ether.h>

#include <ppsi/ppsi.h>
#include "ppsi-unix.h"

/* Call pp_state_machine for each instance. To be called periodically,
 * when no packets are incoming */
static int run_all_state_machines(struct pp_globals *ppg)
{
	int j;
	int delay_ms = 0, delay_ms_j;

	for (j = 0; j < ppg->nlinks; j++) {
		struct pp_instance *ppi = INST(ppg, j);
		delay_ms_j = pp_state_machine(ppi, NULL, 0);

		/* delay_ms is the least delay_ms among all instances */
		if (j == 0)
			delay_ms = delay_ms_j;
		if (delay_ms_j < delay_ms)
			delay_ms = delay_ms_j;
	}

	return delay_ms;
}

void unix_main_loop(struct pp_globals *ppg)
{
	struct pp_instance *ppi;
	int delay_ms;
	int j;

	/* Initialize each link's state machine */
	for (j = 0; j < ppg->nlinks; j++) {

		ppi = INST(ppg, j);

		/*
		* If we are sending or receiving raw ethernet frames,
		* the ptp payload is one-eth-header bytes into the frame
		*/
		if (ppi->proto == PPSI_PROTO_RAW)
			NP(ppi)->ptp_offset = ETH_HLEN;

		/*
		* The main loop here is based on select. While we are not
		* doing anything else but the protocol, this allows extra stuff
		* to fit.
		*/
		ppi->is_new_state = 1;
	}

	delay_ms = run_all_state_machines(ppg);

	while (1) {
		int i;

		/*
		 * If Ebest was changed in previous loop, run best
		 * master clock before checking for new packets, which
		 * would affect port state again
		 */
		if (ppg->ebest_updated) {
			for (j = 0; j < ppg->nlinks; j++) {
				int new_state;
				struct pp_instance *ppi = INST(ppg, j);
				new_state = bmc(ppi);
				if (new_state != ppi->state) {
					ppi->state = new_state;
					ppi->is_new_state = 1;
				}
			}
			ppg->ebest_updated = 0;
		}

		i = unix_net_ops.check_packet(ppg, delay_ms);

		if (i < 0)
			continue;

		if (i == 0) {
			delay_ms = run_all_state_machines(ppg);
			continue;
		}

		/* If delay_ms is -1, the above ops.check_packet will continue
		 * consuming the previous timeout (see its implementation).
		 * This ensures that every state machine is called at least once
		 * every delay_ms */
		delay_ms = -1;

		for (j = 0; j < ppg->nlinks; j++) {
			int tmp_d;
			ppi = INST(ppg, j);

			if ((NP(ppi)->ch[PP_NP_GEN].pkt_present) ||
			    (NP(ppi)->ch[PP_NP_EVT].pkt_present)) {

				i = ppi->n_ops->recv(ppi, ppi->rx_frame,
						PP_MAX_FRAME_LENGTH - 4,
						&ppi->last_rcv_time);

				if (i == -2) {
					continue; /* dropped */
				}
				if (i == -1) {
					pp_diag(ppi, frames, 1,
						"Receive Error %i: %s\n",
						errno, strerror(errno));
					continue;
				}
				if (i < PP_MINIMUM_LENGTH) {
					pp_diag(ppi, frames, 1,
						"Short frame: %d < %d\n", i,
						PP_MINIMUM_LENGTH);
					continue;
				}

				tmp_d = pp_state_machine(ppi, ppi->rx_ptp,
					i - NP(ppi)->ptp_offset);

				if ((delay_ms == -1) || (tmp_d < delay_ms))
					delay_ms = tmp_d;
			}
		}
	}
}
