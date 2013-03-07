/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include "common-fun.h"

int pp_slave(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0; /* error var, to check errors in msg handling */
	TimeInternal correction_field;
	MsgHeader *hdr = &ppi->received_ptp_header;
	int d1, d2;

	if (ppi->is_new_state) {
		DSPOR(ppi)->portState = PPS_SLAVE;

		pp_init_clock(ppi);

		if (pp_hooks.new_slave)
			e = pp_hooks.new_slave(ppi, pkt, plen);
		if (e)
			goto out;

		ppi->waiting_for_follow = FALSE;

		pp_timeout_restart_annrec(ppi);

		pp_timeout_rand(ppi, PP_TO_DELAYREQ,
				DSPOR(ppi)->logMinDelayReqInterval);
	}

	if (st_com_check_record_update(ppi))
		goto state_updated;

	if (plen == 0)
		goto out;

	switch (hdr->messageType) {

	case PPM_ANNOUNCE:
		e = st_com_slave_handle_announce(ppi, pkt, plen);
		break;

	case PPM_SYNC:
		e = st_com_slave_handle_sync(ppi, pkt, plen);
		break;

	case PPM_FOLLOW_UP:
		e = st_com_slave_handle_followup(ppi, pkt, plen);
		break;

	case PPM_DELAY_REQ:
		/* Being slave, we are not waiting for a delay request */
		break;

	case PPM_DELAY_RESP:

		e = (plen < PP_DELAY_RESP_LENGTH);

		if (e)
			break;

		msg_unpack_delay_resp(pkt, &ppi->msg_tmp.resp);

		if ((memcmp(DSPOR(ppi)->portIdentity.clockIdentity,
			ppi->msg_tmp.resp.requestingPortIdentity.clockIdentity,
			PP_CLOCK_IDENTITY_LENGTH) == 0) &&
			((ppi->sent_seq[PPM_DELAY_REQ]) ==
				hdr->sequenceId) &&
			(DSPOR(ppi)->portIdentity.portNumber ==
			ppi->msg_tmp.resp.requestingPortIdentity.portNumber)
			&& ppi->is_from_cur_par) {

			to_TimeInternal(&ppi->delay_req_receive_time,
					&ppi->msg_tmp.resp.receiveTimestamp);

			int64_to_TimeInternal(
				hdr->correctionfield,
				&correction_field);

			if (pp_hooks.update_delay)
				e = pp_hooks.update_delay(ppi);
			else
				pp_update_delay(ppi, &correction_field);
			if (e)
				goto out;

			ppi->log_min_delay_req_interval =
				hdr->logMessageInterval;

		} else {
			PP_VPRINTF("pp_slave : "
			     "Delay Resp doesn't match Delay Req\n");
		}

		break;

	/*
	 * We are not supporting pdelay (not configured to, see
	 * 9.5.13.1, p 106), so all the code about pdelay is removed
	 * as a whole by one commit in our history. It can be recoverd
	 * and fixed if needed
	 */

	default:
		/* disregard, nothing to do */
		break;

	}

out:
	if (e == 0)
		e = st_com_execute_slave(ppi);

	if (pp_timeout_z(ppi, PP_TO_DELAYREQ)) {
		e = msg_issue_delay_req(ppi);

		ppi->delay_req_send_time = ppi->last_snt_time;

		/* Add latency */
		add_TimeInternal(&ppi->delay_req_send_time,
				 &ppi->delay_req_send_time,
				 &OPTS(ppi)->outbound_latency);
	}

	if (e) {
		ppi->next_state = PPS_FAULTY;
		return 0;
	}

state_updated:

	/* Leaving this state */
	if (ppi->next_state != ppi->state) {
		pp_timeout_clr(ppi, PP_TO_ANN_RECEIPT);
		pp_timeout_clr(ppi, PP_TO_DELAYREQ);

		pp_init_clock(ppi);
	}
	d1 = d2 = pp_ms_to_timeout(ppi, PP_TO_ANN_RECEIPT);
	if (ppi->timeouts[PP_TO_DELAYREQ])
		d2 = pp_ms_to_timeout(ppi, PP_TO_DELAYREQ);
	ppi->next_delay = d1 < d2 ? d1 : d2;
	return 0;
}
