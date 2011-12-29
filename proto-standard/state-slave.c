/*
 * FIXME: header
 */
#include <pproto/pproto.h>
#include "state-common-fun.h"

int pp_slave(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	TimeInternal time; /* TODO: handle it, see handle(...) in protocol.c */

	if (ppi->is_new_state) {
		pp_init_clock(ppi);

		ppi->waiting_for_follow = FALSE;

		ppi->pdelay_req_send_time.seconds = 0;
		ppi->pdelay_req_send_time.nanoseconds = 0;
		ppi->pdelay_req_receive_time.seconds = 0;
		ppi->pdelay_req_receive_time.nanoseconds = 0;
		ppi->pdelay_resp_send_time.seconds = 0;
		ppi->pdelay_resp_send_time.nanoseconds = 0;
		ppi->pdelay_resp_receive_time.seconds = 0;
		ppi->pdelay_resp_receive_time.nanoseconds = 0;

		st_com_restart_annrec_timer(ppi);

		if (ppi->rt_opts->e2e_mode)
			pp_timer_start(1 << DSPOR(ppi)->logMinDelayReqInterval,
				ppi->timers[PP_TIMER_DELAYREQ_INTERVAL]);
		else
			pp_timer_start(1 << DSPOR(ppi)->logMinPdelayReqInterval,
				ppi->timers[PP_TIMER_PDELAYREQ_INTERVAL]);
	}

	if (st_com_check_record_update(ppi))
		goto state_updated;

	switch (ppi->msg_tmp_header.messageType) {

	case PPM_ANNOUNCE:
		e = st_com_slave_handle_announce(pkt, plen, ppi);
		break;

	case PPM_SYNC:
		e = st_com_slave_handle_sync(pkt, plen, &time, ppi);
		break;

	case PPM_FOLLOW_UP:
		e = st_com_slave_handle_followup(pkt, plen, ppi);
		break;

	case PPM_DELAY_REQ:
		/* TODO */
		break;

	case PPM_DELAY_RESP:
		/* TODO */
		break;

	case PPM_PDELAY_REQ:
		/* TODO */
		break;

	case PPM_PDELAY_RESP:
		/* TODO */
		break;

	case PPM_PDELAY_RESP_FOLLOW_UP:
		/* TODO */
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

		if (ppi->rt_opts->e2e_mode)
			pp_timer_stop(ppi->timers[PP_TIMER_DELAYREQ_INTERVAL]);
		else
			pp_timer_stop(ppi->timers[PP_TIMER_PDELAYREQ_INTERVAL]);

		pp_init_clock(ppi);
	}

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return 0;
}
