/*
 * FIXME: header
 */
#include <pproto/pproto.h>


static void restart_annrec_timer(struct pp_instance *ppi)
{
	/* 0 <= logAnnounceInterval <= 4, see pag. 237 of spec */
	/* FIXME: if (logAnnounceInterval < 0), error? Or handle a right
	 * shift?*/
	pp_timer_start((DSPOR(ppi)->announceReceiptTimeout) <<
			DSPOR(ppi)->logAnnounceInterval,
			ppi->timers[PP_TIMER_ANNOUNCE_RECEIPT]);
}

int pp_listening(struct pp_instance *ppi, unsigned char *pkt, int plen)
{

	if (ppi->is_new_state) {
		restart_annrec_timer(ppi);
	}

	if (pp_timer_expired(ppi->timers[PP_TIMER_ANNOUNCE_RECEIPT])) {
		/* FIXME diag
		 * DBGV("event ANNOUNCE_RECEIPT_TIMEOUT_EXPIRES\n");
		 */
		ppi->number_foreign_records = 0;
		ppi->foreign_record_i = 0;
		if (!DSDEF(ppi)->slaveOnly &&
			DSDEF(ppi)->clockQuality.clockClass != 255) {
			m1(ppi);
			ppi->next_state = PPS_MASTER;
		}
		else {
			restart_annrec_timer(ppi);
		}
	}

	if (ppi->rt_opts->e2e_mode) {
		if (pp_timer_expired(ppi->timers[PP_TIMER_DELAYREQ_INTERVAL])) {
			/* FIXME diag
			 * DBGV("event DELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
			 */
			/* TODO issueDelayReq(rtOpts,ptpClock); */
		}
	} else {
		if (pp_timer_expired(ppi->timers[PP_TIMER_PDELAYREQ_INTERVAL]))
		{
			/* FIXME diag
			DBGV("event PDELAYREQ_INTERVAL_TIMEOUT_EXPIRES\n");
			*/
			/* TODO issuePDelayReq(rtOpts,ptpClock); */
		}
	}

	if (ppi->msg_tmp_header.messageType == PPM_ANNOUNCE) {
		/* TODO check isFromSelf?
		if (isFromSelf) {
			DBGV("HandleAnnounce : Ignore message from self \n");
			return;
		}
		*/

		/* FIXME diag
		 * DBGV("Announce message from another foreign master");
		 */

		/* TODO addForeign(ptpClock->msgIbuf,header,ptpClock); */

		ppi->record_update = TRUE;
	}

	if(ppi->record_update) {
		/* FIXME diag DBGV("event STATE_DECISION_EVENT\n"); */
		ppi->record_update = FALSE;
		ppi->next_state = bmc(ppi->frgn_master,
				      ppi->rt_opts, ppi);
	}

	/* Leaving this state */
	if (ppi->next_state != ppi->state) {
		pp_timer_stop(ppi->timers[PP_TIMER_ANNOUNCE_RECEIPT]);
	}

	return 0;
}
