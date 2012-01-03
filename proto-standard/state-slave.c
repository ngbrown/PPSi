/*
 * FIXME: header
 */
#include <pptp/pptp.h>
#include "state-common-fun.h"

int pp_slave(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	TimeInternal time; /* TODO: handle it, see handle(...) in protocol.c */
	TimeInternal req_rec_tstamp;
	TimeInternal correction_field;
	TimeInternal resp_orig_tstamp;
	MsgHeader *hdr = &ppi->msg_tmp_header;

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

		e = (plen < PP_DELAY_REQ_LENGTH);

		if (!e && ppi->is_from_self) {
			/* Get sending timestamp from IP stack
			 * with So_TIMESTAMP */

			ppi->delay_req_send_time.seconds =
				time.seconds;
			ppi->delay_req_send_time.nanoseconds =
				time.nanoseconds;

			/* Add latency */
			add_TimeInternal(&ppi->delay_req_send_time,
					 &ppi->delay_req_send_time,
					 &ppi->rt_opts->outbound_latency);
		}
		break;

	case PPM_DELAY_RESP:

		e = (plen < PP_DELAY_RESP_LENGTH);

		if (e)
			break;

		if (!ppi->rt_opts->e2e_mode)
			break;

		msg_unpack_delay_resp(pkt, &ppi->msg_tmp.resp);

		if ((pp_memcmp(DSPOR(ppi)->portIdentity.clockIdentity,
			ppi->msg_tmp.resp.requestingPortIdentity.clockIdentity,
			PP_CLOCK_IDENTITY_LENGTH) == 0) &&
			((ppi->sent_seq_id[PPM_DELAY_REQ] - 1) ==
				hdr->sequenceId) &&
			(DSPOR(ppi)->portIdentity.portNumber ==
			ppi->msg_tmp.resp.requestingPortIdentity.portNumber)
			&& ppi->is_from_cur_par) {

			to_TimeInternal(&req_rec_tstamp,
				       &ppi->msg_tmp.resp.receiveTimestamp);
			ppi->delay_req_receive_time.seconds =
				req_rec_tstamp.seconds;
			ppi->delay_req_receive_time.nanoseconds =
				req_rec_tstamp.nanoseconds;

			int64_to_TimeInternal(
				hdr->correctionfield,
				&correction_field);

			/* TODO servo
			updateDelay(&ptpClock->owd_filt,
				    rtOpts,ptpClock, &correctionField);
			*/

			ppi->log_min_delay_req_interval =
				hdr->logMessageInterval;

		} else {
			/* FIXME diag
			DBGV("HandledelayResp : "
			     "delayResp doesn't match with the delayReq. \n");
			*/
		}

		break;

	case PPM_PDELAY_REQ:
		e = st_com_handle_pdelay_req(pkt, plen, &time, ppi);
		break;

	case PPM_PDELAY_RESP:
		if (ppi->rt_opts->e2e_mode)
			break;

		e = (plen < PP_PDELAY_RESP_LENGTH);

		if (e)
			break;

		if (ppi->is_from_self)	{
			add_TimeInternal(&time, &time,
					&ppi->rt_opts->outbound_latency);
			/* TODO issuePDelayRespFollowUp(time,
						&ptpClock->PdelayReqHeader,
						rtOpts,ptpClock);
			*/
			break;
		}
		msg_unpack_pdelay_resp(pkt,
					&ppi->msg_tmp.presp);

		if (!((ppi->sent_seq_id[PPM_PDELAY_REQ] ==
				hdr->sequenceId)
			&& (!pp_memcmp(DSPOR(ppi)->portIdentity.clockIdentity,
				  ppi->msg_tmp.presp.requestingPortIdentity.
				  clockIdentity, PP_CLOCK_IDENTITY_LENGTH))
			&& (DSPOR(ppi)->portIdentity.portNumber ==
				 ppi->msg_tmp.presp.
					 requestingPortIdentity.portNumber))) {

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

				int64_to_TimeInternal(hdr->correctionfield,
					&correction_field);
				ppi->last_pdelay_resp_corr_field.seconds =
					correction_field.seconds;
				ppi->last_pdelay_resp_corr_field.nanoseconds =
					correction_field.nanoseconds;
			} else {
				/* One step Clock */
				/* Store t4 (Fig 35) */
				ppi->pdelay_resp_receive_time.seconds =
					time.seconds;
				ppi->pdelay_resp_receive_time.nanoseconds =
					time.nanoseconds;

				int64_to_TimeInternal(hdr->correctionfield,
					&correction_field);
				/* TODO
				updatePeerDelay(&ptpClock->owd_filt,rtOpts,
					ptpClock,&correctionField,FALSE);
				*/
			}
		} else {
			/* FIXME diag
			DBGV("HandlePdelayResp : Pdelayresp doesn't "
			     "match with the PdelayReq. \n");
			*/
		}
		break;

	case PPM_PDELAY_RESP_FOLLOW_UP:

		if (ppi->rt_opts->e2e_mode)
			break;

		e = (plen < PP_PDELAY_RESP_FOLLOW_UP_LENGTH);

		if (e)
			break;

		if (hdr->sequenceId ==
		    ppi->sent_seq_id[PPM_PDELAY_REQ]) {

			msg_unpack_pdelay_resp_followup(
				pkt,
				&ppi->msg_tmp.prespfollow);

			to_TimeInternal(
				&resp_orig_tstamp,
				&ppi->msg_tmp.prespfollow.
					responseOriginTimestamp);

			ppi->pdelay_resp_send_time.seconds =
				resp_orig_tstamp.seconds;
			ppi->pdelay_resp_send_time.nanoseconds =
				resp_orig_tstamp.nanoseconds;

			int64_to_TimeInternal(
				hdr->correctionfield,
				&correction_field);
			add_TimeInternal(&correction_field, &correction_field,
				&ppi->last_pdelay_req_corr_field);

			/* TODO servo
			updatePeerDelay (&ptpClock->owd_filt,
					 rtOpts, ptpClock,
					 &correctionField,TRUE);
			*/
		}
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
