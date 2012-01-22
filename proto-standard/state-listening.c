/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <pptp/pptp.h>
#include <pptp/diag.h>
#include "common-fun.h"

int pp_listening(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0; /* error var, to check errors in msg handling */

	if (ppi->is_new_state)
		st_com_restart_annrec_timer(ppi);

	if (st_com_check_record_update(ppi))
		goto state_updated;

	if (plen == 0)
		goto no_incoming_msg;

	switch (ppi->msg_tmp_header.messageType) {

	case PPM_ANNOUNCE:
		e = st_com_master_handle_announce(ppi, pkt, plen);
		break;

	case PPM_SYNC:
		e = st_com_master_handle_sync(ppi, pkt, plen);
		break;

	default:
		/* disreguard, nothing to do */
		break;
	}

no_incoming_msg:
	if (e == 0)
		e = st_com_execute_slave(ppi);

	if (e != 0)
		ppi->next_state = PPS_FAULTY;

state_updated:
	/* Leaving this state */
	if (ppi->next_state != ppi->state)
		pp_timer_stop(ppi->timers[PP_TIMER_ANN_RECEIPT]);

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return 0;
}
