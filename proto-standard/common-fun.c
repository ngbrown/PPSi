/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "common-fun.h"

/* Called by listening, passive, slave, uncalibrated */
int st_com_execute_slave(struct pp_instance *ppi, int check_delayreq)
{
	int ret = 0;

	if (pp_hooks.execute_slave)
		ret = pp_hooks.execute_slave(ppi);
	if (ret == 1) /* done: just return */
		return 0;
	if (ret < 0)
		return ret;

	if (pp_timeout_z(ppi, PP_TO_ANN_RECEIPT)) {
		ppi->number_foreign_records = 0;
		ppi->foreign_record_i = 0;
		if (!DSDEF(ppi)->slaveOnly &&
			DSDEF(ppi)->clockQuality.clockClass != 255) {
			m1(ppi);
			ppi->next_state = PPS_MASTER;
		} else {
			ppi->next_state = PPS_LISTENING;
			pp_timeout_restart_annrec(ppi);
		}
	}

	if (!check_delayreq)
		return 0;

	if (pp_timeout_z(ppi, PP_TO_DELAYREQ)) {
		ret = msg_issue_delay_req(ppi);

		ppi->delay_req_send_time = ppi->last_snt_time;

		/* Add latency */
		add_TimeInternal(&ppi->delay_req_send_time,
				 &ppi->delay_req_send_time,
				 &OPTS(ppi)->outbound_latency);
	}
	return ret;
}

/* Called by listening, master, passive, slave */
int st_com_check_record_update(struct pp_instance *ppi)
{
	if (ppi->record_update) {
		PP_VPRINTF("event STATE_DECISION_EVENT\n");
		ppi->record_update = FALSE;
		ppi->next_state = bmc(ppi, ppi->frgn_master);

		if (ppi->next_state != ppi->state)
			return 1;
	}
	return 0;
}

/* Called by this file, basically when an announce is got, all states */
static void st_com_add_foreign(struct pp_instance *ppi, unsigned char *buf)
{
	int i, j;
	int found = 0;
	MsgHeader *hdr = &ppi->msg_tmp_header;

	j = ppi->foreign_record_best;

	/* Check if foreign master is already known */
	for (i = 0; i < ppi->number_foreign_records; i++) {
		if (!memcmp(hdr->sourcePortIdentity.clockIdentity,
			    ppi->frgn_master[j].port_identity.
			    clockIdentity,
			    PP_CLOCK_IDENTITY_LENGTH) &&
		    (hdr->sourcePortIdentity.portNumber ==
		     ppi->frgn_master[j].port_identity.portNumber)) {
			/* Foreign Master is already in Foreign master data set
			 */
			ppi->frgn_master[j].ann_messages++;
			found = 1;
			PP_VPRINTF("st_com_add_foreign: ann_messages: %d\n",
				ppi->frgn_master[j].ann_messages);

			msg_copy_header(&ppi->frgn_master[j].hdr, hdr);
			msg_unpack_announce(buf, &ppi->frgn_master[j].ann);
			break;
		}

		j = (j + 1) % ppi->number_foreign_records;
	}

	/* Old foreign master */
	if (found)
		return;

	/* New foreign master */
	if (ppi->number_foreign_records <
	    OPTS(ppi)->max_foreign_records) {
		ppi->number_foreign_records++;
	}

	j = ppi->foreign_record_i;

	/* Copy new foreign master data set from announce message */
	memcpy(ppi->frgn_master[j].port_identity.clockIdentity,
		hdr->sourcePortIdentity.clockIdentity,
		PP_CLOCK_IDENTITY_LENGTH);
	ppi->frgn_master[j].port_identity.portNumber =
		hdr->sourcePortIdentity.portNumber;
	ppi->frgn_master[j].ann_messages = 0;

	/*
	 * header and announce field of each Foreign Master are
	 * usefull to run Best Master Clock Algorithm
	 */
	msg_copy_header(&ppi->frgn_master[j].hdr, hdr);

	msg_unpack_announce(buf, &ppi->frgn_master[j].ann);

	PP_VPRINTF("New foreign Master added\n");

	ppi->foreign_record_i = (ppi->foreign_record_i+1) %
		OPTS(ppi)->max_foreign_records;
}


/* Called by slave and uncalibrated */
int st_com_slave_handle_announce(struct pp_instance *ppi, unsigned char *buf,
				 int len)
{
	MsgHeader *hdr = &ppi->msg_tmp_header;

	if (len < PP_ANNOUNCE_LENGTH)
		return -1;

	/*
	 * Valid announce message is received : BMC algorithm
	 * will be executed
	 */
	ppi->record_update = TRUE;

	if (ppi->is_from_cur_par) {
		msg_unpack_announce(buf, &ppi->msg_tmp.announce);
		s1(ppi, hdr, &ppi->msg_tmp.announce);
	} else {
		/* st_com_add_foreign takes care of announce unpacking */
		st_com_add_foreign(ppi, buf);
	}

	/*Reset Timer handling Announce receipt timeout*/
	pp_timeout_restart_annrec(ppi);

	if (pp_hooks.handle_announce)
		pp_hooks.handle_announce(ppi);

	return 0;
}

/* Called by slave and uncalibrated */
int st_com_slave_handle_sync(struct pp_instance *ppi, unsigned char *buf,
			     int len)
{
	TimeInternal *time;
	TimeInternal origin_tstamp;
	TimeInternal correction_field;
	MsgHeader *hdr = &ppi->msg_tmp_header;

	if (len < PP_SYNC_LENGTH)
		return -1;

	time = &ppi->last_rcv_time;

	if (ppi->is_from_cur_par) {
		ppi->sync_receive_time = *time;

		/* FIXME diag to file? will we ever handle it?
		if (OPTS(ppi)->recordFP)
			fprintf(rtOpts->recordFP, "%d %llu\n",
				header->sequenceId,
				((time->seconds * 1000000000ULL) +
					time->nanoseconds));
		*/

		if ((hdr->flagField[0] & PP_TWO_STEP_FLAG) != 0) {
			ppi->waiting_for_follow = TRUE;
			ppi->recv_sync_sequence_id = hdr->sequenceId;
			/* Save correctionField of Sync message */
			int64_to_TimeInternal(
				hdr->correctionfield,
				&correction_field);
			ppi->last_sync_corr_field.seconds =
				correction_field.seconds;
			ppi->last_sync_corr_field.nanoseconds =
				correction_field.nanoseconds;
		} else {
			msg_unpack_sync(buf, &ppi->msg_tmp.sync);
			int64_to_TimeInternal(
				ppi->msg_tmp_header.correctionfield,
				&correction_field);

			display_TimeInternal("Correction field",
					     &correction_field);

			ppi->waiting_for_follow = FALSE;
			to_TimeInternal(&origin_tstamp,
					&ppi->msg_tmp.sync.originTimestamp);
			pp_update_offset(ppi, &origin_tstamp,
					&ppi->sync_receive_time,
					&correction_field);
			pp_update_clock(ppi);
		}
	}
	return 0;
}

/* Called by slave and uncalibrated */
int st_com_slave_handle_followup(struct pp_instance *ppi, unsigned char *buf,
				 int len)
{
	TimeInternal precise_orig_timestamp;
	TimeInternal correction_field;
	int ret = 0;

	MsgHeader *hdr = &ppi->msg_tmp_header;

	if (len < PP_FOLLOW_UP_LENGTH)
		return -1;

	if (!ppi->is_from_cur_par) {
		pp_error("%s: SequenceID doesn't match last Sync message\n",
			__func__);
		return 0;
	}

	if (!ppi->waiting_for_follow) {
		pp_error("%s: Slave was not waiting a follow up message\n",
			__func__);
		return 0;
	}

	if (ppi->recv_sync_sequence_id != hdr->sequenceId) {
		pp_error("%s: Follow up message is not from current parent\n",
			__func__);
		return 0;
	}

	msg_unpack_follow_up(buf, &ppi->msg_tmp.follow);
	ppi->waiting_for_follow = FALSE;
	to_TimeInternal(&precise_orig_timestamp,
			&ppi->msg_tmp.follow.preciseOriginTimestamp);

	int64_to_TimeInternal(ppi->msg_tmp_header.correctionfield,
					&correction_field);

	add_TimeInternal(&correction_field, &correction_field,
		&ppi->last_sync_corr_field);

	/* Call the extension; it may do it all and ask to return */
	if (pp_hooks.handle_followup)
		ret = pp_hooks.handle_followup(ppi, &precise_orig_timestamp,
					       &correction_field);
	if (ret == 1)
		return 0;
	if (ret < 0)
		return ret;

	pp_update_offset(ppi, &precise_orig_timestamp,
			&ppi->sync_receive_time,
			&correction_field);

	pp_update_clock(ppi);
	return 0;
}

/* Called by master, listenting, passive. */
int st_com_master_handle_announce(struct pp_instance *ppi, unsigned char *buf,
				  int len)
{
	if (len < PP_ANNOUNCE_LENGTH)
		return -1;

	PP_VPRINTF("Announce message from another foreign master\n");

	st_com_add_foreign(ppi, buf);

	ppi->record_update = TRUE;

	return 0;
}

/*
 * Called by master, listenting, passive.
 * FIXME: this must be implemented to support one-step masters
 */
int st_com_master_handle_sync(struct pp_instance *ppi, unsigned char *buf,
			      int len)
{
	/* No more used: follow up is sent right after the corresponding sync */
	return 0;
}
