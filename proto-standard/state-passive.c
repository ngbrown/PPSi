/*
 * FIXME: header
 */
#include <pptp/pptp.h>
#include "state-common-fun.h"

int pp_passive(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	TimeInternal time; /* TODO: handle it, see handle(...) in protocol.c */
	int e = 0;

	if (ppi->is_new_state) {
		pp_timer_start(1 << DSPOR(ppi)->logMinPdelayReqInterval,
			ppi->timers[PP_TIMER_PDELAYREQ_INTERVAL]);

		st_com_restart_annrec_timer(ppi);
	}

	if (st_com_check_record_update(ppi))
		goto state_updated;

	switch (ppi->msg_tmp_header.messageType) {

	case PPM_ANNOUNCE:
		e = st_com_master_handle_announce(pkt, plen, ppi);
		break;

	case PPM_PDELAY_REQ:
		e = st_com_handle_pdelay_req(pkt, plen, &time, ppi);
		break;

	default:
		/* disreguard, nothing to do */
		break;

	}

	if (e == 0)
		st_com_execute_slave(ppi);
	else
		ppi->next_state = PPS_FAULTY;


state_updated:
	/* Leaving this state */
	if (ppi->next_state != ppi->state) {
		pp_timer_stop(ppi->timers[PP_TIMER_ANNOUNCE_RECEIPT]);
		pp_timer_stop(ppi->timers[PP_TIMER_PDELAYREQ_INTERVAL]);
	}

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return 0;
}
