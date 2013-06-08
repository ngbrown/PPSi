/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/*
 * This is the main loop for unix stuff.
 */
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <linux/if_ether.h>

#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>

/* Call pp_state_machine for each instance. To be called periodically,
 * when no packets are incoming */
static int run_all_state_machines(struct pp_globals *ppg)
{
	int j;
	int delay_ms = 0, delay_ms_j;

	for (j = 0; j < ppg->nlinks; j++) {
		struct pp_instance *ppi = &ppg->pp_instances[j];
		delay_ms_j = pp_state_machine(ppi, NULL, 0);

		/* delay_ms is the least delay_ms among all instances */
		if (j == 0)
			delay_ms = delay_ms_j;
		else
			delay_ms = delay_ms_j < delay_ms ? delay_ms_j : delay_ms;
	}

	return delay_ms;
}

void wrs_main_loop(struct pp_globals *ppg)
{
	struct pp_instance *ppi;
	int delay_ms;
	int j;

	/* Initialize each link's state machine */
	for (j = 0; j < ppg->nlinks; j++) {

		ppi = &ppg->pp_instances[j];

		/*
		* If we are sending or receiving raw ethernet frames,
		* the ptp payload is one-eth-header bytes into the frame
		*/
		if (ppi->ethernet_mode)
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

		minipc_server_action(ppsi_ch, 10 /* ms */);

		if (ppg->ebest_updated) {
			/* If Ebest was changed in previous loop, run best master
			 * clock before checking for new packets, which would affect
			 * port state again */
			for (j = 0; j < ppg->nlinks; j++) {
				int new_state;
				struct pp_instance *ppi = &ppg->pp_instances[j];
				new_state = bmc(ppi);
				if (new_state != ppi->state) {
					ppi->state = new_state;
					ppi->is_new_state = 1;
				}
			}
			ppg->ebest_updated = 0;
		}

		i = unix_net_check_pkt(ppg, delay_ms);

		if (i < 0)
			continue;

		if (i == 0) {
			delay_ms = run_all_state_machines(ppg);
			continue;
		}

		/* If delay_ms is -1, the above unix_net_check_pkt will continue
		 * consuming the previous timeout (see its implementation).
		 * This ensures that every state machine is called at least once
		 * every delay_ms */
		delay_ms = -1;

		for (j = 0; j < ppg->nlinks; j++) {
			ppi = &ppg->pp_instances[j];

			if ((NP(ppi)->ch[PP_NP_GEN].pkt_present) ||
				(NP(ppi)->ch[PP_NP_EVT].pkt_present)) {

				/* We got some packets. If not ours, go on to the next ppi */

				i = ppi->n_ops->recv(ppi, ppi->rx_frame,
						PP_MAX_FRAME_LENGTH - 4,
						&ppi->last_rcv_time);

				ppi->last_rcv_time.seconds += DSPRO(ppi)->currentUtcOffset;

				if (i < PP_MINIMUM_LENGTH) {
					pp_diag(ppi, frames, 1, "Error or short packet: %d < %d\n", i,
						PP_MINIMUM_LENGTH
					);
					continue;
				}

				pp_state_machine(ppi, ppi->rx_ptp,
					i - NP(ppi)->ptp_offset);
			}
		}
	}
}
