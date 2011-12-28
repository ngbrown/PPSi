/*
 * FIXME: header
 */
#include <pproto/pproto.h>
#include "state-common-fun.h"

int pp_passive(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	if (ppi->is_new_state) {
		pp_timer_start(1 << DSPOR(ppi)->logMinPdelayReqInterval,
			ppi->timers[PP_TIMER_PDELAYREQ_INTERVAL]);

		st_com_restart_annrec_timer(ppi);
	}

	if (st_com_check_record_update(ppi))
		goto state_updated;

	if (ppi->msg_tmp_header.messageType == PPM_PDELAY_REQ) {
#ifdef _FROM_PTPD_2_1_0_
		/* TODO "translate" it into ptp-wr structs*/
		if (isFromSelf) {
			/*
			 * Get sending timestamp from IP stack
			 * with So_TIMESTAMP
			 */
			ptpClock->pdelay_req_send_time.seconds =
				time->seconds;
			ptpClock->pdelay_req_send_time.nanoseconds =
				time->nanoseconds;

			/*Add latency*/
			addTime(&ptpClock->pdelay_req_send_time,
				&ptpClock->pdelay_req_send_time,
				&rtOpts->outboundLatency);
			break;
		} else {
			msgUnpackHeader(ptpClock->msgIbuf,
					&ptpClock->PdelayReqHeader);
			issuePDelayResp(time, header, rtOpts,
					ptpClock);
			break;
		}
#endif /* _FROM_PTPD_2_1_0_ */
	}

	st_com_execute_slave(ppi);

state_updated:
	/* Leaving this state */
	if (ppi->next_state != ppi->state) {
		pp_timer_stop(ppi->timers[PP_TIMER_ANNOUNCE_RECEIPT]);
		pp_timer_stop(ppi->timers[PP_TIMER_PDELAYREQ_INTERVAL]);
	}

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY;

	return 0;
}
