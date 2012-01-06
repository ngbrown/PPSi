/*
 * FIXME: header
 */
#include <pptp/pptp.h>
#include <pptp/diag.h>
#include "common-fun.h"

int pp_master(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	TimeInternal time; /* TODO: handle it, see handle(...) in protocol.c */
	TimeInternal req_rec_tstamp;
	TimeInternal correction_field;
	TimeInternal resp_orig_tstamp;

	int e = 0;
	MsgHeader *hdr = &ppi->msg_tmp_header;

	if (ppi->is_new_state) {
		pp_timer_start(1 << DSPOR(ppi)->logSyncInterval,
			ppi->timers[PP_TIMER_SYNC]);

		pp_timer_start(1 << DSPOR(ppi)->logAnnounceInterval,
			ppi->timers[PP_TIMER_ANN_INTERVAL]);

		pp_timer_start(1 << DSPOR(ppi)->logMinPdelayReqInterval,
			ppi->timers[PP_TIMER_PDELAYREQ]);
	}

	if (st_com_check_record_update(ppi))
		goto state_updated;

	if (pp_timer_expired(ppi->timers[PP_TIMER_SYNC])) {
		DBGV("TODO: event SYNC_INTERVAL_TIMEOUT_EXPIRES\n");
		/* TODO issueSync(rtOpts, ptpClock); */
	}

	if (pp_timer_expired(ppi->timers[PP_TIMER_ANN_INTERVAL])) {
		DBGV("TODO: event ANNOUNCE_INTERVAL_TIMEOUT_EXPIRES\n");
		/* TODO issueAnnounce(rtOpts, ptpClock); */
	}

	if (!ppi->rt_opts->e2e_mode) {
		if (pp_timer_expired(ppi->timers[PP_TIMER_PDELAYREQ])) {
			DBGV("TODO: event PDELAYREQ_INTERVAL_TOUT_EXPIRES\n");
			/* TODO issuePDelayReq(rtOpts,ptpClock); */
		}
	}

	switch (ppi->msg_tmp_header.messageType) {

	case PPM_ANNOUNCE:
		e = st_com_master_handle_announce(ppi, pkt, plen);
		break;

	case PPM_SYNC:
		e = st_com_master_handle_sync(ppi, pkt, plen, &time);
		break;

	case PPM_DELAY_REQ:
		msg_copy_header(&ppi->delay_req_hdr, hdr);
		/*TODO issueDelayResp(time,&ptpClock->delayReqHeader,rtOpts,ptpClock);
		*/
		break;

	case PPM_PDELAY_REQ:
		e = st_com_handle_pdelay_req(ppi, pkt, plen, &time);
		break;

	case PPM_PDELAY_RESP:
		/* Loopback Timestamp */
		if (ppi->rt_opts->e2e_mode)
			break;

		if (ppi->is_from_self) {
			/*Add latency*/
			add_TimeInternal(&time, &time,
					 &ppi->rt_opts->outbound_latency);

			/* TODO issuePDelayRespFollowUp(
				time,
				&ptpClock->PdelayReqHeader,
				rtOpts, ptpClock);
			*/
			break;
		}
		msg_unpack_pdelay_resp(pkt, &ppi->msg_tmp.presp);

		if (!((ppi->sent_seq_id[PPM_PDELAY_REQ] ==
		       hdr->sequenceId)
			&& (!pp_memcmp(DSPOR(ppi)->portIdentity.clockIdentity,
			       ppi->msg_tmp.presp.requestingPortIdentity.
				       clockIdentity,
			       PP_CLOCK_IDENTITY_LENGTH))
			&& (DSPOR(ppi)->portIdentity.portNumber ==
			ppi->msg_tmp.presp.requestingPortIdentity.portNumber))
		   ) {
			if ((hdr->flagField[0] & 0x02) ==
			    PP_TWO_STEP_FLAG) {
				/* Two Step Clock */
				/* Store t4 (Fig 35) */
				ppi->pdelay_resp_receive_time.seconds =
					time.seconds;
				ppi->pdelay_resp_receive_time.nanoseconds =
					time.nanoseconds;
				/* Store t2 (Fig 35) */
				to_TimeInternal(&req_rec_tstamp,
					       &ppi->msg_tmp.presp.
					       requestReceiptTimestamp);
				ppi->pdelay_req_receive_time.seconds =
					req_rec_tstamp.seconds;
				ppi->pdelay_req_receive_time.nanoseconds =
					req_rec_tstamp.nanoseconds;
				int64_to_TimeInternal(
					hdr->correctionfield,
					&correction_field);
				ppi->last_pdelay_resp_corr_field.seconds =
					correction_field.seconds;
				ppi->last_pdelay_resp_corr_field.nanoseconds =
					correction_field.nanoseconds;
				break;
			} else {
				/* One step Clock */
				/* Store t4 (Fig 35)*/
				ppi->pdelay_resp_receive_time.seconds =
				time.seconds;
				ppi->pdelay_resp_receive_time.nanoseconds =
					time.nanoseconds;

				int64_to_TimeInternal(
					hdr->correctionfield,
					&correction_field);
				/* TODO servo
				updatePeerDelay(&ptpClock->owd_filt,
						rtOpts,ptpClock,
						&correctionField,FALSE);
				*/
				break;
			}
		}
		break; /* XXX added by gnn for safety */


	case PPM_PDELAY_RESP_FOLLOW_UP:
		if (hdr->sequenceId == ppi->sent_seq_id[PPM_PDELAY_REQ] - 1) {
			msg_unpack_pdelay_resp_followup(pkt,
				&ppi->msg_tmp.prespfollow);
			to_TimeInternal(&resp_orig_tstamp,
				       &ppi->msg_tmp.prespfollow.
					       responseOriginTimestamp);
			ppi->pdelay_resp_send_time.seconds =
				resp_orig_tstamp.seconds;
			ppi->pdelay_resp_send_time.nanoseconds =
				resp_orig_tstamp.nanoseconds;

			int64_to_TimeInternal(
				ppi->msg_tmp_header.correctionfield,
				&correction_field);
			add_TimeInternal(&correction_field,
				&correction_field,
				&ppi->last_pdelay_resp_corr_field);

			/* TODO servo
			updatePeerDelay(&ptpClock->owd_filt,
					rtOpts, ptpClock,
					&correctionField,TRUE);
			*/
			break;
		}

		break;

	default:
		/* disreguard, nothing to do */
		break;
	}

	if (e == 0) {
		if (DSDEF(ppi)->slaveOnly ||
			DSDEF(ppi)->clockQuality.clockClass == 255)
			ppi->next_state = PPS_LISTENING;
	} else {
		ppi->next_state = PPS_FAULTY;
	}

state_updated:
	/* Leaving this state */
	if (ppi->next_state != ppi->state) {
		pp_timer_stop(ppi->timers[PP_TIMER_SYNC]);
		pp_timer_stop(ppi->timers[PP_TIMER_ANN_INTERVAL]);
		pp_timer_stop(ppi->timers[PP_TIMER_PDELAYREQ]);
	}

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return 0;

}
