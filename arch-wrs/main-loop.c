/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */

/*
 * This is the main loop for the wr-switch architecture. It's amost
 * the same as the unix main loop, but we must serve RPC calls too
 */
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <linux/if_ether.h>

#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>
#include <wr-api.h>
#include <hal_exports.h>

/* Call pp_state_machine for each instance. To be called periodically,
 * when no packets are incoming */
static int run_all_state_machines(struct pp_globals *ppg)
{
	int j;
	int delay_ms = 0, delay_ms_j;

	for (j = 0; j < ppg->nlinks; j++) {
		struct pp_instance *ppi = INST(ppg, j);
		int old_lu = WR_DSPOR(ppi)->linkUP;
		struct hal_port_state *p;

		/* FIXME: we should save this pointer in the ppi itself */
		p = pp_wrs_lookup_port(ppi->iface_name);
		if (!p) {
			fprintf(stderr, "ppsi: can't find %s in shmem\n",
				ppi->iface_name);
			continue;
		}

		WR_DSPOR(ppi)->linkUP =
			(p->state != HAL_PORT_STATE_LINK_DOWN &&
			 p->state != HAL_PORT_STATE_DISABLED);

		if (old_lu != WR_DSPOR(ppi)->linkUP) {

			pp_diag(ppi, fsm, 1, "iface %s went %s\n",
				ppi->iface_name, WR_DSPOR(ppi)->linkUP ? "up":"down");

			if (WR_DSPOR(ppi)->linkUP) {
				ppi->state = PPS_INITIALIZING;
			}
			else {
				ppi->n_ops->exit(ppi);
				ppi->frgn_rec_num = 0;
				ppi->frgn_rec_best = -1;
				if (BUILT_WITH_WHITERABBIT
				    && ppg->ebest_idx == ppi->port_idx)
					wr_servo_reset();
			}
		}

		/* Do not call state machine if link is down */
		if (WR_DSPOR(ppi)->linkUP)
			delay_ms_j = pp_state_machine(ppi, NULL, 0);
		else
			delay_ms_j = PP_DEFAULT_NEXT_DELAY_MS;

		/* delay_ms is the least delay_ms among all instances */
		if (j == 0)
			delay_ms = delay_ms_j;
		if (delay_ms_j < delay_ms)
			delay_ms = delay_ms_j;
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

		minipc_server_action(ppsi_ch, 10 /* ms */);

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

		i = wrs_net_ops.check_packet(ppg, delay_ms);

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
