/*
 * FIXME header
 */

#include <pproto/pproto.h>
#include <dep/dep.h>


/* Unpack header from in buffer to msg_tmp_header field */
void msg_unpack_header(void *buf, struct pp_instance *ppi)
{
	MsgHeader *hdr = &ppi->msg_tmp_header;

	hdr->transportSpecific = (*(Nibble *) (buf + 0)) >> 4;
	hdr->messageType = (*(Enumeration4 *) (buf + 0)) & 0x0F;
	hdr->versionPTP = (*(UInteger4 *) (buf + 1)) & 0x0F;

	/* force reserved bit to zero if not */
	hdr->messageLength = pp_htons(*(UInteger16 *) (buf + 2));
	hdr->domainNumber = (*(UInteger8 *) (buf + 4));

	/* pp_memcpy(hdr->flagField, (buf + 6), FIXME:FLAG_FIELD_LENGTH); */

	pp_memcpy(&hdr->correctionfield.msb, (buf + 8), 4);
	pp_memcpy(&hdr->correctionfield.lsb, (buf + 12), 4);
	hdr->correctionfield.msb = pp_htonl(hdr->correctionfield.msb);
	hdr->correctionfield.lsb = pp_htonl(hdr->correctionfield.lsb);
	pp_memcpy(hdr->sourcePortIdentity.clockIdentity, (buf + 20),
	       PP_CLOCK_IDENTITY_LENGTH);
	hdr->sourcePortIdentity.portNumber =
		pp_htons(*(UInteger16 *) (buf + 28));
	hdr->sequenceId = pp_htons(*(UInteger16 *) (buf + 30));
	hdr->controlField = (*(UInteger8 *) (buf + 32));
	hdr->logMessageInterval = (*(Integer8 *) (buf + 33));

	if (DSPOR(ppi)->portIdentity.portNumber ==
	    ppi->msg_tmp_header.sourcePortIdentity.portNumber
	    && !pp_memcmp(ppi->msg_tmp_header.sourcePortIdentity.clockIdentity,
			DSPOR(ppi)->portIdentity.clockIdentity,
			PP_CLOCK_IDENTITY_LENGTH))
		ppi->is_from_self = 1;
	else
		ppi->is_from_self = 0;

	if (!pp_memcmp(DSPAR(ppi)->parentPortIdentity.clockIdentity,
			hdr->sourcePortIdentity.clockIdentity,
			PP_CLOCK_IDENTITY_LENGTH) &&
			(DSPAR(ppi)->parentPortIdentity.portNumber ==
			hdr->sourcePortIdentity.portNumber))
		ppi->is_from_cur_par = 1;
	else
		ppi->is_from_cur_par = 0;

/* FIXME: diag
#ifdef PTPD_DBG
	msgHeader_display(header);
#endif
*/
}

/* Pack header message into out buffer of ppi */
void msg_pack_header(void *buf, struct pp_instance *ppi)
{
	Nibble transport = 0x80;

	/* (spec annex D) */
	*(UInteger8 *) (buf + 0) = transport;
	*(UInteger4 *) (buf + 1) = DSPOR(ppi)->versionNumber;
	*(UInteger8 *) (buf + 4) = DSDEF(ppi)->domainNumber;

	if (DSDEF(ppi)->twoStepFlag)
		*(UInteger8 *) (buf + 6) = PP_TWO_STEP_FLAG;

	pp_memset((buf + 8), 0, 8);
	pp_memcpy((buf + 20), DSPOR(ppi)->portIdentity.clockIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 28) =
				pp_htons(DSPOR(ppi)->portIdentity.portNumber);
	*(UInteger8 *) (buf + 33) = 0x7F;
	/* Default value(spec Table 24) */
}

void *msg_copy_header(MsgHeader *dest, MsgHeader *src)
{
	return pp_memcpy(dest,src,sizeof(MsgHeader));
}


/* Pack Sync message into out buffer of ppi */
void msg_pack_sync(void *buf, Timestamp *orig_tstamp, struct pp_instance *ppi)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x00;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = pp_htons(PP_SYNC_LENGTH);
	*(UInteger16 *) (buf + 30) = pp_htons(ppi->sent_seq_id[PPM_SYNC]);
	*(UInteger8 *) (buf + 32) = 0x00;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logSyncInterval;
	pp_memset((buf + 8), 0, 8);

	/* Sync message */
	*(UInteger16 *) (buf + 34) = pp_htons(orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = pp_htonl(orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = pp_htonl(orig_tstamp->nanosecondsField);
}

/* Unpack Sync message from in buffer */
void msg_unpack_sync(void *buf, MsgSync *sync)
{
	sync->originTimestamp.secondsField.msb = 
		pp_htons(*(UInteger16 *) (buf + 34));
	sync->originTimestamp.secondsField.lsb = 
		pp_htonl(*(UInteger32 *) (buf + 36));
	sync->originTimestamp.nanosecondsField = 
		pp_htonl(*(UInteger32 *) (buf + 40));

/* FIXME: diag
#ifdef PTPD_DBG
	msgSync_display(sync);
#endif
*/
}

/* Pack Announce message into out buffer of ppi */
void msg_pack_announce(void *buf, struct pp_instance *ppi)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x0B;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = pp_htons(PP_ANNOUNCE_LENGTH);
	*(UInteger16 *) (buf + 30) = pp_htons(ppi->sent_seq_id[PPM_ANNOUNCE]);
	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logAnnounceInterval;

	/* Announce message */
	pp_memset((buf + 34), 0, 10);
	*(Integer16 *) (buf + 44) = pp_htons(DSPRO(ppi)->currentUtcOffset);
	*(UInteger8 *) (buf + 47) = DSPAR(ppi)->grandmasterPriority1;
	*(UInteger8 *) (buf + 48) = DSDEF(ppi)->clockQuality.clockClass;
	*(Enumeration8 *) (buf + 49) = DSDEF(ppi)->clockQuality.clockAccuracy;
	*(UInteger16 *) (buf + 50) = 
		pp_htons(DSDEF(ppi)->clockQuality.offsetScaledLogVariance);
	*(UInteger8 *) (buf + 52) = DSPAR(ppi)->grandmasterPriority2;
	pp_memcpy((buf + 53), DSPAR(ppi)->grandmasterIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 61) = pp_htons(DSCUR(ppi)->stepsRemoved);
	*(Enumeration8 *) (buf + 63) = DSPRO(ppi)->timeSource;
}

/* Unpack Announce message from in buffer of ppi to msgtmp. Announce */
void msg_unpack_announce(void *buf, MsgAnnounce *ann)
{
	ann->originTimestamp.secondsField.msb =
		pp_htons(*(UInteger16 *) (buf + 34));
	ann->originTimestamp.secondsField.lsb =
		pp_htonl(*(UInteger32 *) (buf + 36));
	ann->originTimestamp.nanosecondsField =
		pp_htonl(*(UInteger32 *) (buf + 40));
	ann->currentUtcOffset = pp_htons(*(UInteger16 *) (buf + 44));
	ann->grandmasterPriority1 = *(UInteger8 *) (buf + 47);
	ann->grandmasterClockQuality.clockClass =
		*(UInteger8 *) (buf + 48);
	ann->grandmasterClockQuality.clockAccuracy =
		*(Enumeration8 *) (buf + 49);
	ann->grandmasterClockQuality.offsetScaledLogVariance =
		pp_htons(*(UInteger16 *) (buf + 50));
	ann->grandmasterPriority2 = *(UInteger8 *) (buf + 52);
	pp_memcpy(ann->grandmasterIdentity, (buf + 53),
	       PP_CLOCK_IDENTITY_LENGTH);
	ann->stepsRemoved = pp_htons(*(UInteger16 *) (buf + 61));
	ann->timeSource = *(Enumeration8 *) (buf + 63);

/* FIXME: diag
#ifdef PTPD_DBG
	msgAnnounce_display(announce);
#endif
*/
}


/* Pack Follow Up message into out buffer of ppi*/
void msg_pack_follow_up(void *buf, Timestamp *prec_orig_tstamp,
			struct pp_instance *ppi)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x08;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = pp_htons(PP_FOLLOW_UP_LENGTH);
	*(UInteger16 *) (buf + 30) = pp_htons(ppi->sent_seq_id[PPM_SYNC] - 1);

	/* sentSyncSequenceId has already been incremented in "issueSync" */
	*(UInteger8 *) (buf + 32) = 0x02;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logSyncInterval;

	/* Follow Up message */
	*(UInteger16 *) (buf + 34) = 
		pp_htons(prec_orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = 
		pp_htonl(prec_orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = 
		pp_htonl(prec_orig_tstamp->nanosecondsField);
}

/* Unpack FollowUp message from in buffer of ppi to msgtmp.follow */
void msg_unpack_follow_up(void *buf, MsgFollowUp *flwup)
{
	flwup->preciseOriginTimestamp.secondsField.msb =
		pp_htons(*(UInteger16 *) (buf + 34));
	flwup->preciseOriginTimestamp.secondsField.lsb =
		pp_htonl(*(UInteger32 *) (buf + 36));
	flwup->preciseOriginTimestamp.nanosecondsField =
		pp_htonl(*(UInteger32 *) (buf + 40));

/* FIXME: diag
#ifdef PTPD_DBG
	msgFollowUp_display(follow);
#endif
*/
}

/* pack PdelayReq message into out buffer of ppi */
void msg_pack_pdelay_req(void *buf, Timestamp *orig_tstamp,
		    struct pp_instance *ppi)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */

	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x02;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = pp_htons(PP_PDELAY_REQ_LENGTH);
	*(UInteger16 *) (buf + 30) = pp_htons(ppi->sent_seq_id[PPM_PDELAY_REQ]);
	*(UInteger8 *) (buf + 32) = 0x05;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;

	/* Table 24 */
	pp_memset((buf + 8), 0, 8);

	/* Pdelay_req message */
	*(UInteger16 *) (buf + 34) = pp_htons(orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = pp_htonl(orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = pp_htonl(orig_tstamp->nanosecondsField);

	pp_memset((buf + 44), 0, 10);
	/* RAZ reserved octets */
}


/*pack DelayReq message into out buffer of ppi*/
void msg_pack_delay_req(void *buf, Timestamp *orig_tstamp,
			struct pp_instance *ppi)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x01;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = pp_htons(PP_DELAY_REQ_LENGTH);
	*(UInteger16 *) (buf + 30) = pp_htons(ppi->sent_seq_id[PPM_DELAY_REQ]);
	*(UInteger8 *) (buf + 32) = 0x01;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;

	/* Table 24 */
	pp_memset((buf + 8), 0, 8);

	/* Pdelay_req message */
	*(UInteger16 *) (buf + 34) = pp_htons(orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = pp_htonl(orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = pp_htonl(orig_tstamp->nanosecondsField);
}


/*pack delayResp message into OUT buffer of ppi*/
void msg_pack_delay_resp(void *buf, MsgHeader *hdr, Timestamp *rcv_tstamp,
			 struct pp_instance *ppi)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x09;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = pp_htons(PP_DELAY_RESP_LENGTH);
	*(UInteger8 *) (buf + 4) = hdr->domainNumber;
	pp_memset((buf + 8), 0, 8);

	/* Copy correctionField of PdelayReqMessage */
	*(Integer32 *) (buf + 8) = pp_htonl(hdr->correctionfield.msb);
	*(Integer32 *) (buf + 12) = pp_htonl(hdr->correctionfield.lsb);

	*(UInteger16 *) (buf + 30) = pp_htons(hdr->sequenceId);
	
	*(UInteger8 *) (buf + 32) = 0x03;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logMinDelayReqInterval;

	/* Table 24 */

	/* Pdelay_resp message */
	*(UInteger16 *) (buf + 34) = 
		pp_htons(rcv_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = pp_htonl(rcv_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = pp_htonl(rcv_tstamp->nanosecondsField);
	pp_memcpy((buf + 44), hdr->sourcePortIdentity.clockIdentity,
		  PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) = 
		pp_htons(hdr->sourcePortIdentity.portNumber);
}



/* Pack PdelayResp message into out buffer of ppi */
void msg_pack_pdelay_resp(void *buf, MsgHeader *hdr,
			  Timestamp *req_rec_tstamp, struct pp_instance *ppi)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x03;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = pp_htons(PP_PDELAY_RESP_LENGTH);
	*(UInteger8 *) (buf + 4) = hdr->domainNumber;
	pp_memset((buf + 8), 0, 8);

	*(UInteger16 *) (buf + 30) = pp_htons(hdr->sequenceId);

	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */

	/* Pdelay_resp message */
	*(UInteger16 *) (buf + 34) = pp_htons(req_rec_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = pp_htonl(req_rec_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = pp_htonl(req_rec_tstamp->nanosecondsField);
	pp_memcpy((buf + 44), hdr->sourcePortIdentity.clockIdentity,
		  PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) =
		pp_htons(hdr->sourcePortIdentity.portNumber);

}


/* Unpack delayReq message from in buffer of ppi to msgtmp.req */
void msg_unpack_delay_req(void *buf, MsgDelayReq *delay_req)
{
	delay_req->originTimestamp.secondsField.msb =
		pp_htons(*(UInteger16 *) (buf + 34));
	delay_req->originTimestamp.secondsField.lsb =
		pp_htonl(*(UInteger32 *) (buf + 36));
	delay_req->originTimestamp.nanosecondsField =
		pp_htonl(*(UInteger32 *) (buf + 40));
/* FIXME: diag
#ifdef PTPD_DBG
	msgDelayReq_display(delayreq);
#endif
*/
}



/* Unpack PdelayReq message from IN buffer of ppi to msgtmp.req */
void msg_unpack_pdelay_req(void *buf, MsgPDelayReq *pdelay_req)
{
	pdelay_req->originTimestamp.secondsField.msb =
		pp_htons(*(UInteger16 *) (buf + 34));
	pdelay_req->originTimestamp.secondsField.lsb =
		pp_htonl(*(UInteger32 *) (buf + 36));
	pdelay_req->originTimestamp.nanosecondsField =
		pp_htonl(*(UInteger32 *) (buf + 40));
/* FIXME: diag
#ifdef PTPD_DBG
	msgPDelayReq_display(pdelayreq);
#endif
*/
}

/* Unpack delayResp message from IN buffer of ppi to msgtmp.presp */
void msg_unpack_delay_resp(void *buf, MsgDelayResp *resp)
{
	resp->receiveTimestamp.secondsField.msb = 
		pp_htons(*(UInteger16 *) (buf + 34));
	resp->receiveTimestamp.secondsField.lsb = 
		pp_htonl(*(UInteger32 *) (buf + 36));
	resp->receiveTimestamp.nanosecondsField = 
		pp_htonl(*(UInteger32 *) (buf + 40));
	pp_memcpy(resp->requestingPortIdentity.clockIdentity, 
	       (buf + 44), PP_CLOCK_IDENTITY_LENGTH);
	resp->requestingPortIdentity.portNumber = 
		pp_htons(*(UInteger16 *) (buf + 52));

/* FIXME: diag
#ifdef PTPD_DBG
	msgDelayResp_display(resp);
#endif
*/
}

/* Unpack PdelayResp message from IN buffer of ppi to msgtmp.presp */
void msg_unpack_pdelay_resp(void *buf, MsgPDelayResp *presp)
{
	presp->requestReceiptTimestamp.secondsField.msb =
		pp_htons(*(UInteger16 *) (buf + 34));
	presp->requestReceiptTimestamp.secondsField.lsb =
		pp_htonl(*(UInteger32 *) (buf + 36));
	presp->requestReceiptTimestamp.nanosecondsField =
		pp_htonl(*(UInteger32 *) (buf + 40));
	pp_memcpy(presp->requestingPortIdentity.clockIdentity,
	       (buf + 44), PP_CLOCK_IDENTITY_LENGTH);
	presp->requestingPortIdentity.portNumber =
		pp_htons(*(UInteger16 *) (buf + 52));

/* FIXME: diag
#ifdef PTPD_DBG
	msgPDelayResp_display(presp);
#endif
*/
}

/* Pack PdelayRespFollowUp message into out buffer of ppi */
void msg_pack_pdelay_resp_followup(void *buf, MsgHeader *hdr,
	Timestamp *resp_orig_tstamp, struct pp_instance* ppi)
{
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x0A;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = pp_htons(PP_PDELAY_RESP_FOLLOW_UP_LENGTH);
	*(UInteger16 *) (buf + 30) = pp_htons(ppi->pdelay_req_hdr.sequenceId);
	*(UInteger8 *) (buf + 32) = 0x05;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;

	/* Table 24 */

	/* Copy correctionField of PdelayReqMessage */
	*(Integer32 *) (buf + 8) = pp_htonl(hdr->correctionfield.msb);
	*(Integer32 *) (buf + 12) = pp_htonl(hdr->correctionfield.lsb);

	/* Pdelay_resp_follow_up message */
	*(UInteger16 *) (buf + 34) =
		pp_htons(resp_orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) =
		pp_htonl(resp_orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) =
		pp_htonl(resp_orig_tstamp->nanosecondsField);
	pp_memcpy((buf + 44), hdr->sourcePortIdentity.clockIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) =
		pp_htons(hdr->sourcePortIdentity.portNumber);
}

/* Unpack PdelayResp message from in buffer of ppi to msgtmp.presp */
void msg_unpack_pdelay_resp_followup(void *buf,
	MsgPDelayRespFollowUp *presp_follow)
{
	presp_follow->responseOriginTimestamp.secondsField.msb =
		pp_htons(*(UInteger16 *) (buf + 34));
	presp_follow->responseOriginTimestamp.secondsField.lsb =
		pp_htonl(*(UInteger32 *) (buf + 36));
	presp_follow->responseOriginTimestamp.nanosecondsField =
		pp_htonl(*(UInteger32 *) (buf + 40));
	pp_memcpy(presp_follow->requestingPortIdentity.clockIdentity,
	       (buf + 44), PP_CLOCK_IDENTITY_LENGTH);
	presp_follow->requestingPortIdentity.portNumber =
		pp_htons(*(UInteger16 *) (buf + 52));
}
