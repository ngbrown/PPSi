/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <pptp/pptp.h>
#include "common-fun.h"

int pp_passive(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	TimeInternal time; /* TODO: handle it, see handle(...) in protocol.c */
	int e = 0; /* error var, to check errors in msg handling */

	if (ppi->is_new_state) {
		pp_timer_start(1 << DSPOR(ppi)->logMinPdelayReqInterval,
			ppi->timers[PP_TIMER_PDELAYREQ]);

		st_com_restart_annrec_timer(ppi);
	}

	if (st_com_check_record_update(ppi))
		goto state_updated;

	switch (ppi->msg_tmp_header.messageType) {

	case PPM_ANNOUNCE:
		e = st_com_master_handle_announce(ppi, pkt, plen);
		break;

	case PPM_PDELAY_REQ:
		e = st_com_handle_pdelay_req(ppi, pkt, plen, &time);
		break;

	default:
		/* disreguard, nothing to do */
		break;

	}

	if (e == 0)
		e = st_com_execute_slave(ppi);

	if (e != 0)
		ppi->next_state = PPS_FAULTY;


state_updated:
	/* Leaving this state */
	if (ppi->next_state != ppi->state) {
		pp_timer_stop(ppi->timers[PP_TIMER_ANN_RECEIPT]);
		pp_timer_stop(ppi->timers[PP_TIMER_PDELAYREQ]);
	}

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return 0;
}
