/*
 * FIXME: header
 */
#include <pproto/pproto.h>
#include <dep/dep.h>
#include "state-common-fun.h"

void st_com_execute_slave(struct pp_instance *ppi)
{
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
			st_com_restart_annrec_timer(ppi);
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
}

void st_com_restart_annrec_timer(struct pp_instance *ppi)
{
	/* 0 <= logAnnounceInterval <= 4, see pag. 237 of spec */
	/* FIXME: if (logAnnounceInterval < 0), error? Or handle a right
	 * shift?*/
	pp_timer_start((DSPOR(ppi)->announceReceiptTimeout) <<
			DSPOR(ppi)->logAnnounceInterval,
			ppi->timers[PP_TIMER_ANNOUNCE_RECEIPT]);
}


int st_com_check_record_update(struct pp_instance *ppi)
{
	if(ppi->record_update) {
		/* FIXME diag DBGV("event STATE_DECISION_EVENT\n"); */
		ppi->record_update = FALSE;
		ppi->next_state = bmc(ppi->frgn_master,
				      ppi->rt_opts, ppi);

		if (ppi->next_state != ppi->state)
			return 1;
	}
	return 0;
}

void st_com_add_foreign(unsigned char *buf, MsgHeader *hdr,
			struct pp_instance *ppi)
{
/*TODO "translate" it into ptp-wr structs*/
#ifdef _FROM_PTPD_2_1_0_
	int i,j;
	Boolean found = FALSE;

	j = ptpClock->foreign_record_best;

	/*Check if Foreign master is already known*/
	for (i=0;i<ptpClock->number_foreign_records;i++) {
		if (!memcmp(header->sourcePortIdentity.clockIdentity,
			    ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity,
			    CLOCK_IDENTITY_LENGTH) &&
		    (header->sourcePortIdentity.portNumber ==
		     ptpClock->foreign[j].foreignMasterPortIdentity.portNumber))
		{
			/*Foreign Master is already in Foreignmaster data set*/
			ptpClock->foreign[j].foreignMasterAnnounceMessages++;
			found = TRUE;
			DBGV("addForeign : AnnounceMessage incremented \n");
			msgUnpackHeader(buf,&ptpClock->foreign[j].header);
			msgUnpackAnnounce(buf,&ptpClock->foreign[j].announce);
			break;
		}

		j = (j+1)%ptpClock->number_foreign_records;
	}

	/*New Foreign Master*/
	if (!found) {
		if (ptpClock->number_foreign_records <
		    ptpClock->max_foreign_records) {
			ptpClock->number_foreign_records++;
		}
		j = ptpClock->foreign_record_i;

		/*Copy new foreign master data set from Announce message*/
		memcpy(ptpClock->foreign[j].foreignMasterPortIdentity.clockIdentity,
		       header->sourcePortIdentity.clockIdentity,
		       CLOCK_IDENTITY_LENGTH);
		ptpClock->foreign[j].foreignMasterPortIdentity.portNumber =
			header->sourcePortIdentity.portNumber;
		ptpClock->foreign[j].foreignMasterAnnounceMessages = 0;

		/*
		 * header and announce field of each Foreign Master are
		 * usefull to run Best Master Clock Algorithm
		 */
		msgUnpackHeader(buf,&ptpClock->foreign[j].header);
		msgUnpackAnnounce(buf,&ptpClock->foreign[j].announce);
		DBGV("New foreign Master added \n");

		ptpClock->foreign_record_i =
			(ptpClock->foreign_record_i+1) %
			ptpClock->max_foreign_records;
	}
#endif  /* _FROM_PTPD_2_1_0_ */
}


int st_com_slave_handle_announce(unsigned char *buf, int len,
				  struct pp_instance *ppi)
{
	MsgHeader *hdr = &ppi->msg_tmp_header;

	if (len < PP_ANNOUNCE_LENGTH)
		return -1;

	if (ppi->is_from_self)
		return 0;

	/*
	 * Valid announce message is received : BMC algorithm
	 * will be executed
	 */
	ppi->record_update = TRUE;

	if (!ppi->is_from_cur_par) {
		msg_unpack_announce(buf, &ppi->msg_tmp.announce);
		s1(hdr, &ppi->msg_tmp.announce, ppi);
	}
	else {
		/* st_com_add_foreign takes care of announce unpacking */
		st_com_add_foreign(buf, hdr, ppi);
	}

	/*Reset Timer handling Announce receipt timeout*/
	st_com_restart_annrec_timer(ppi);

	return 0;
}

int st_com_slave_handle_sync(unsigned char *buf, int len, TimeInternal *time,
				struct pp_instance *ppi)
{
	TimeInternal origin_tstamp;
	TimeInternal correction_field;
	MsgHeader *hdr = &ppi->msg_tmp_header;

	if (len < PP_SYNC_LENGTH)
		return -1;

	if (ppi->is_from_self) {
		return 0;
	}

	if (ppi->is_from_cur_par) {
		ppi->sync_receive_time.seconds = time->seconds;
		ppi->sync_receive_time.nanoseconds = time->nanoseconds;

		/* FIXME diag check. Delete it?
		if (ppi->rt_opts->recordFP)
			fprintf(rtOpts->recordFP, "%d %llu\n",
				header->sequenceId,
				((time->seconds * 1000000000ULL) +
					time->nanoseconds));
		*/

		if ((hdr->flagField[0] & 0x02) == PP_TWO_STEP_FLAG) {
			ppi->waiting_for_follow = TRUE;
			ppi->recv_sync_sequence_id = hdr->sequenceId;
			/* Save correctionField of Sync message */
			int64_to_TimeInternal(
				hdr->correctionfield,
				&correction_field);
			ppi->last_sync_correction_field.seconds =
				correction_field.seconds;
			ppi->last_sync_correction_field.nanoseconds =
				correction_field.nanoseconds;
		} else {
			msg_unpack_sync(buf, &ppi->msg_tmp.sync);
			int64_to_TimeInternal(
				ppi->msg_tmp_header.correctionfield,
				&correction_field);
			/* FIXME diag check
			 * timeInternal_display(&correctionfield);
			 */
			ppi->waiting_for_follow = FALSE;
			to_TimeInternal(&origin_tstamp,
					&ppi->msg_tmp.sync.originTimestamp);
			pp_update_offset(&origin_tstamp,
					&ppi->sync_receive_time,
					&correction_field,ppi);
			pp_update_clock(ppi);
		}
	}
	return 0;
}

int st_com_slave_handle_followup(unsigned char *buf, int len,
				 struct pp_instance *ppi)
{
	TimeInternal precise_orig_timestamp;
	TimeInternal correction_field;

	MsgHeader *hdr = &ppi->msg_tmp_header;

	if (len < PP_FOLLOW_UP_LENGTH)
		return -1;

	if (!ppi->is_from_cur_par) {
		/* FIXME diag
		DBGV("SequenceID doesn't match with "
			"last Sync message \n");
		*/
		return 0;
	}

	if (!ppi->waiting_for_follow) {
		/* FIXME diag
		DBGV("Slave was not waiting a follow up "
			"message \n");
		*/
		return 0;
	}

	if (ppi->recv_sync_sequence_id != hdr->sequenceId) {
		/* FIXME diag
		DBGV("Follow up message is not from current parent \n");
		*/
		return 0;
	}

	msg_unpack_follow_up(buf, &ppi->msg_tmp.follow);
	ppi->waiting_for_follow = FALSE;
	to_TimeInternal(&precise_orig_timestamp,
			&ppi->msg_tmp.follow.preciseOriginTimestamp);

	int64_to_TimeInternal(ppi->msg_tmp_header.correctionfield,
					&correction_field);

	add_TimeInternal(&correction_field,&correction_field,
		&ppi->last_sync_correction_field);

	pp_update_offset(&precise_orig_timestamp,
			&ppi->sync_receive_time,
			&correction_field, ppi);

	pp_update_clock(ppi);
	return 0;
}
