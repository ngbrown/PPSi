/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "common-fun.h"

int pp_listening(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0; /* error var, to check errors in msg handling */

	if (pp_hooks.listening)
		e = pp_hooks.listening(ppi, pkt, plen);
	if (e)
		goto out;

	if (ppi->is_new_state) {
		DSPOR(ppi)->portState = PPS_LISTENING;
		pp_timeout_restart_annrec(ppi);
	}

	if (st_com_check_record_update(ppi))
		goto state_updated;

	if (plen == 0)
		goto out;

	switch (ppi->msg_tmp_header.messageType) {

	case PPM_ANNOUNCE:
		e = st_com_master_handle_announce(ppi, pkt, plen);
		break;

	case PPM_SYNC:
		e = st_com_master_handle_sync(ppi, pkt, plen);
		break;

	default:
		/* disregard, nothing to do */
		break;
	}

out:
	if (e == 0)
		e = st_com_execute_slave(ppi, 0);

	if (e != 0)
		ppi->next_state = PPS_FAULTY;

state_updated:
	/* Leaving this state */
	if (ppi->next_state != ppi->state)
		pp_timeout_clr(ppi, PP_TO_ANN_RECEIPT);

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return 0;
}
