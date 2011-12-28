/*
 * FIXME: header
 */
#include <pproto/pproto.h>
#include "state-common-fun.h"


int pp_listening(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	if (ppi->is_new_state) {
		st_com_restart_annrec_timer(ppi);
	}

	if (st_com_check_record_update(ppi))
		goto state_updated;

	switch (ppi->msg_tmp_header.messageType) {

	case PPM_ANNOUNCE:

		if (plen < PP_ANNOUNCE_LENGTH) {
			ppi->next_state = PPS_FAULTY;
			goto state_updated;
		}

		if (ppi->is_from_self) {
			/* FIXME diag
			DBGV("HandleAnnounce : Ignore message from self \n");
			*/
			goto state_done;
		}

		/* FIXME diag
		 * DBGV("Announce message from another foreign master");
		 */

		st_com_add_foreign(pkt, &ppi->msg_tmp_header, ppi);

		ppi->record_update = TRUE;
		break;

	default:
		/* disreguard, nothing to do */
		break;
	}

	st_com_execute_slave(ppi);

state_updated:
	/* Leaving this state */
	if (ppi->next_state != ppi->state) {
		pp_timer_stop(ppi->timers[PP_TIMER_ANNOUNCE_RECEIPT]);
	}

state_done:
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return 0;
}
