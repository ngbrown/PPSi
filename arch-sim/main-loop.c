/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Pietro Fezzardi (pietrofezzardi@gmail.com)
 *
 * Released to the public domain
 */

/*
 * This is the main loop for the simulator.
 */
#include <stdlib.h>
#include <errno.h>
#include <sys/select.h>
#include <linux/if_ether.h>

#include <ppsi/ppsi.h>
#include "ppsi-sim.h"

/* Call pp_state_machine for each instance. To be called periodically,
 * when no packets are incoming */
static int run_all_state_machines(struct pp_globals *ppg)
{
	int j;
	int delay_ms = 0, delay_ms_j;

	for (j = 0; j < ppg->nlinks; j++) {
		struct pp_instance *ppi = INST(ppg, j);
		sim_set_global_DS(ppi);
		delay_ms_j = pp_state_machine(ppi, NULL, 0);

		/* delay_ms is the least delay_ms among all instances */
		if (j == 0)
			delay_ms = delay_ms_j;
		if (delay_ms_j < delay_ms)
			delay_ms = delay_ms_j;
	}

	return delay_ms;
}


void sim_main_loop(struct pp_globals *ppg)
{
	struct pp_instance *ppi;
	struct sim_ppg_arch_data *data = SIM_PPG_ARCH(ppg);
	int64_t delay_ns, tmp_ns;
	int j, i;

	/* Initialize each link's state machine */
	for (j = 0; j < ppg->nlinks; j++) {
		ppi = INST(ppg, j);
		ppi->is_new_state = 1;
	}

	delay_ns = run_all_state_machines(ppg) * 1000LL * 1000LL;

	while (data->sim_iter_n <= data->sim_iter_max) {
		/*
		 * If Ebest was changed in previous loop, run best
		 * master clock before checking for new packets, which
		 * would affect port state again
		 */
		if (ppg->ebest_updated) {
			for (j = 0; j < ppg->nlinks; j++) {
				int new_state;
				struct pp_instance *ppi = INST(ppg ,j);
				new_state = bmc(ppi);
				if (new_state != ppi->state) {
					ppi->state = new_state;
					ppi->is_new_state = 1;
				}
			}
			ppg->ebest_updated = 0;
		}

		while (data->n_pending && data->pending->delay_ns <= delay_ns) {
			ppi = INST(ppg, data->pending->which_ppi);

			sim_fast_forward_ns(ppg, data->pending->delay_ns);
			delay_ns -= data->pending->delay_ns;

			i = ppi->n_ops->recv(ppi, ppi->rx_frame,
						PP_MAX_FRAME_LENGTH - 4,
						&ppi->last_rcv_time);

			if (i < PP_MINIMUM_LENGTH) {
				pp_diag(ppi, frames, 1,	"Error or short frame: "
					"%d < %d\n", i,	PP_MINIMUM_LENGTH);
				continue;
			}

			sim_set_global_DS(ppi);
			tmp_ns = 1000LL * 1000LL * pp_state_machine(ppi,
					ppi->rx_ptp, i - NP(ppi)->ptp_offset);

			if (tmp_ns < delay_ns)
				delay_ns = tmp_ns;
		}
		/* here we have no pending packets or the timeout for a state
		 * machine is expired (so delay_ns == 0). If the timeout is not
		 * expired we just fast forward till it's not expired, since we
		 * know that there are no packets pending. */
		sim_fast_forward_ns(ppg, delay_ns);
		delay_ns = run_all_state_machines(ppg) * 1000LL * 1000LL;
	}
	return;
}
