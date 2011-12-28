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
