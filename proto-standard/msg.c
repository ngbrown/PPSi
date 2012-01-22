/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <pptp/pptp.h>
#include <pptp/diag.h>


static inline void Integer64_display(const char *label, Integer64 *bigint)
{
	PP_VPRINTF("%s:\n", label);
	PP_VPRINTF("LSB: %u\n", bigint->lsb);
	PP_VPRINTF("MSB: %d\n", bigint->msb);
}

static inline void UInteger48_display(const char *label, UInteger48 *bigint)
{
	PP_VPRINTF("%s:\n", label);
	PP_VPRINTF("LSB: %u\n", bigint->lsb);
	PP_VPRINTF("MSB: %u\n", bigint->msb);
}

static inline void timestamp_display(const char *label, Timestamp *timestamp)
{
	PP_VPRINTF("%s:\n", label);
	UInteger48_display("seconds", &timestamp->secondsField);
	PP_VPRINTF("nanoseconds: %u\n", timestamp->nanosecondsField);
}

static inline void msg_display_header(MsgHeader *header)
{
	PP_VPRINTF("Message header: \n");
	PP_VPRINTF("\n");
	PP_VPRINTF("transportSpecific: %d\n", header->transportSpecific);
	PP_VPRINTF("messageType: %d\n", header->messageType);
	PP_VPRINTF("versionPTP: %d\n", header->versionPTP);
	PP_VPRINTF("messageLength: %d\n", header->messageLength);
	PP_VPRINTF("domainNumber: %d\n", header->domainNumber);
	PP_VPRINTF("FlagField %02hhx:%02hhx\n", header->flagField[0],
		   header->flagField[1]);
	Integer64_display("correctionfield",&header->correctionfield);
	/* FIXME diag portIdentity_display(&header->sourcePortIdentity); */
	PP_VPRINTF("sequenceId: %d\n", header->sequenceId);
	PP_VPRINTF("controlField: %d\n", header->controlField);
	PP_VPRINTF("logMessageInterval: %d\n", header->logMessageInterval);
	PP_VPRINTF("\n");
}

static inline void msg_display_announce(MsgAnnounce *announce)
{
        PP_VPRINTF("Message ANNOUNCE:\n");
        timestamp_display("Origin Timestamp", &announce->originTimestamp);
        PP_VPRINTF("currentUtcOffset: %d\n", announce->currentUtcOffset);
        PP_VPRINTF("grandMasterPriority1: %d\n",
		   announce->grandmasterPriority1);
        PP_VPRINTF("grandMasterClockQuality:\n");
        /* FIXME diag clockQuality_display(&announce->grandmasterClockQuality); */
        PP_VPRINTF("grandMasterPriority2: %d\n",
		   announce->grandmasterPriority2);
        PP_VPRINTF("grandMasterIdentity:\n");
        /* FIXME diag clockIdentity_display(announce->grandmasterIdentity); */
        PP_VPRINTF("stepsRemoved: %d\n", announce->stepsRemoved);
        PP_VPRINTF("timeSource: %d\n", announce->timeSource);
        PP_VPRINTF("\n");
}

/* Unpack header from in buffer to msg_tmp_header field */
void msg_unpack_header(struct pp_instance *ppi, void *buf)
{
	MsgHeader *hdr = &ppi->msg_tmp_header;

	hdr->transportSpecific = (*(Nibble *) (buf + 0)) >> 4;
	hdr->messageType = (*(Enumeration4 *) (buf + 0)) & 0x0F;
	hdr->versionPTP = (*(UInteger4 *) (buf + 1)) & 0x0F;

	/* force reserved bit to zero if not */
	hdr->messageLength = htons(*(UInteger16 *) (buf + 2));
	hdr->domainNumber = (*(UInteger8 *) (buf + 4));

	pp_memcpy(hdr->flagField, (buf + 6), PP_FLAG_FIELD_LENGTH);

	pp_memcpy(&hdr->correctionfield.msb, (buf + 8), 4);
	pp_memcpy(&hdr->correctionfield.lsb, (buf + 12), 4);
	hdr->correctionfield.msb = htonl(hdr->correctionfield.msb);
	hdr->correctionfield.lsb = htonl(hdr->correctionfield.lsb);
	pp_memcpy(hdr->sourcePortIdentity.clockIdentity, (buf + 20),
	       PP_CLOCK_IDENTITY_LENGTH);
	hdr->sourcePortIdentity.portNumber =
		htons(*(UInteger16 *) (buf + 28));
	hdr->sequenceId = htons(*(UInteger16 *) (buf + 30));
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

	msg_display_header(hdr);
}

/* Pack header message into out buffer of ppi */
void msg_pack_header(struct pp_instance *ppi, void *buf)
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
				htons(DSPOR(ppi)->portIdentity.portNumber);
	*(UInteger8 *) (buf + 33) = 0x7F;
	/* Default value(spec Table 24) */
}

void *msg_copy_header(MsgHeader *dest, MsgHeader *src)
{
	return pp_memcpy(dest, src, sizeof(MsgHeader));
}


/* Pack Sync message into out buffer of ppi */
void msg_pack_sync(struct pp_instance *ppi, Timestamp *orig_tstamp)
{
	void *buf;

	buf = ppi->buf_out;

	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x00;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = htons(PP_SYNC_LENGTH);
	*(UInteger16 *) (buf + 30) = htons(ppi->sent_seq_id[PPM_SYNC]);
	*(UInteger8 *) (buf + 32) = 0x00;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logSyncInterval;
	pp_memset((buf + 8), 0, 8);

	/* Sync message */
	*(UInteger16 *) (buf + 34) = htons(orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = htonl(orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = htonl(orig_tstamp->nanosecondsField);
}

/* Unpack Sync message from in buffer */
void msg_unpack_sync(void *buf, MsgSync *sync)
{
	sync->originTimestamp.secondsField.msb =
		htons(*(UInteger16 *) (buf + 34));
	sync->originTimestamp.secondsField.lsb =
		htonl(*(UInteger32 *) (buf + 36));
	sync->originTimestamp.nanosecondsField =
		htonl(*(UInteger32 *) (buf + 40));

	PP_VPRINTF("Message SYNC\n");
	timestamp_display("Origin Timestamp", &sync->originTimestamp);
	PP_VPRINTF("\n");
}

/* Pack Announce message into out buffer of ppi */
void msg_pack_announce(struct pp_instance *ppi)
{
	void *buf;

	buf = ppi->buf_out;
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x0B;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = htons(PP_ANNOUNCE_LENGTH);
	*(UInteger16 *) (buf + 30) = htons(ppi->sent_seq_id[PPM_ANNOUNCE]);
	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logAnnounceInterval;

	/* Announce message */
	pp_memset((buf + 34), 0, 10);
	*(Integer16 *) (buf + 44) = htons(DSPRO(ppi)->currentUtcOffset);
	*(UInteger8 *) (buf + 47) = DSPAR(ppi)->grandmasterPriority1;
	*(UInteger8 *) (buf + 48) = DSDEF(ppi)->clockQuality.clockClass;
	*(Enumeration8 *) (buf + 49) = DSDEF(ppi)->clockQuality.clockAccuracy;
	*(UInteger16 *) (buf + 50) =
		htons(DSDEF(ppi)->clockQuality.offsetScaledLogVariance);
	*(UInteger8 *) (buf + 52) = DSPAR(ppi)->grandmasterPriority2;
	pp_memcpy((buf + 53), DSPAR(ppi)->grandmasterIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 61) = htons(DSCUR(ppi)->stepsRemoved);
	*(Enumeration8 *) (buf + 63) = DSPRO(ppi)->timeSource;
}

/* Unpack Announce message from in buffer of ppi to msgtmp. Announce */
void msg_unpack_announce(void *buf, MsgAnnounce *ann)
{
	ann->originTimestamp.secondsField.msb =
		htons(*(UInteger16 *) (buf + 34));
	ann->originTimestamp.secondsField.lsb =
		htonl(*(UInteger32 *) (buf + 36));
	ann->originTimestamp.nanosecondsField =
		htonl(*(UInteger32 *) (buf + 40));
	ann->currentUtcOffset = htons(*(UInteger16 *) (buf + 44));
	ann->grandmasterPriority1 = *(UInteger8 *) (buf + 47);
	ann->grandmasterClockQuality.clockClass =
		*(UInteger8 *) (buf + 48);
	ann->grandmasterClockQuality.clockAccuracy =
		*(Enumeration8 *) (buf + 49);
	ann->grandmasterClockQuality.offsetScaledLogVariance =
		htons(*(UInteger16 *) (buf + 50));
	ann->grandmasterPriority2 = *(UInteger8 *) (buf + 52);
	pp_memcpy(ann->grandmasterIdentity, (buf + 53),
	       PP_CLOCK_IDENTITY_LENGTH);
	ann->stepsRemoved = htons(*(UInteger16 *) (buf + 61));
	ann->timeSource = *(Enumeration8 *) (buf + 63);

	msg_display_announce(ann);
}


/* Pack Follow Up message into out buffer of ppi*/
void msg_pack_follow_up(struct pp_instance *ppi, Timestamp *prec_orig_tstamp)
{
	void *buf;

	buf = ppi->buf_out;

	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x08;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = htons(PP_FOLLOW_UP_LENGTH);
	*(UInteger16 *) (buf + 30) = htons(ppi->sent_seq_id[PPM_SYNC] - 1);

	/* sentSyncSequenceId has already been incremented in msg_issue_sync */
	*(UInteger8 *) (buf + 32) = 0x02;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logSyncInterval;

	/* Follow Up message */
	*(UInteger16 *) (buf + 34) =
		htons(prec_orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) =
		htonl(prec_orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) =
		htonl(prec_orig_tstamp->nanosecondsField);
}

/* Unpack FollowUp message from in buffer of ppi to msgtmp.follow */
void msg_unpack_follow_up(void *buf, MsgFollowUp *flwup)
{
	flwup->preciseOriginTimestamp.secondsField.msb =
		htons(*(UInteger16 *) (buf + 34));
	flwup->preciseOriginTimestamp.secondsField.lsb =
		htonl(*(UInteger32 *) (buf + 36));
	flwup->preciseOriginTimestamp.nanosecondsField =
		htonl(*(UInteger32 *) (buf + 40));

	PP_VPRINTF("Message FOLLOW_UP\n");
	timestamp_display("Precise Origin Timestamp",
			  &flwup->preciseOriginTimestamp);
	PP_VPRINTF("\n");
}

/* pack PdelayReq message into out buffer of ppi */
void msg_pack_pdelay_req(struct pp_instance *ppi, Timestamp *orig_tstamp)
{
	void *buf;

	buf = ppi->buf_out;

	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */

	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x02;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = htons(PP_PDELAY_REQ_LENGTH);
	*(UInteger16 *) (buf + 30) = htons(ppi->sent_seq_id[PPM_PDELAY_REQ]);
	*(UInteger8 *) (buf + 32) = 0x05;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;

	/* Table 24 */
	pp_memset((buf + 8), 0, 8);

	/* Pdelay_req message */
	*(UInteger16 *) (buf + 34) = htons(orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = htonl(orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = htonl(orig_tstamp->nanosecondsField);

	pp_memset((buf + 44), 0, 10);
	/* RAZ reserved octets */
}


/* pack DelayReq message into out buffer of ppi */
void msg_pack_delay_req(struct pp_instance *ppi, Timestamp *orig_tstamp)
{
	void *buf;

	buf = ppi->buf_out;

	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x01;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = htons(PP_DELAY_REQ_LENGTH);
	*(UInteger16 *) (buf + 30) = htons(ppi->sent_seq_id[PPM_DELAY_REQ]);
	*(UInteger8 *) (buf + 32) = 0x01;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;

	/* Table 24 */
	pp_memset((buf + 8), 0, 8);

	/* Pdelay_req message */
	*(UInteger16 *) (buf + 34) = htons(orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = htonl(orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = htonl(orig_tstamp->nanosecondsField);
}


/* pack DelayResp message into OUT buffer of ppi */
void msg_pack_delay_resp(struct pp_instance *ppi,
			 MsgHeader *hdr, Timestamp *rcv_tstamp)
{
	void *buf;

	buf = ppi->buf_out;

	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x09;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = htons(PP_DELAY_RESP_LENGTH);
	*(UInteger8 *) (buf + 4) = hdr->domainNumber;
	pp_memset((buf + 8), 0, 8);

	/* Copy correctionField of PdelayReqMessage */
	*(Integer32 *) (buf + 8) = htonl(hdr->correctionfield.msb);
	*(Integer32 *) (buf + 12) = htonl(hdr->correctionfield.lsb);

	*(UInteger16 *) (buf + 30) = htons(hdr->sequenceId);

	*(UInteger8 *) (buf + 32) = 0x03;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logMinDelayReqInterval;

	/* Table 24 */

	/* Pdelay_resp message */
	*(UInteger16 *) (buf + 34) =
		htons(rcv_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = htonl(rcv_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = htonl(rcv_tstamp->nanosecondsField);
	pp_memcpy((buf + 44), hdr->sourcePortIdentity.clockIdentity,
		  PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) =
		htons(hdr->sourcePortIdentity.portNumber);
}

/* Pack PdelayResp message into out buffer of ppi */
void msg_pack_pdelay_resp(struct pp_instance *ppi, MsgHeader *hdr,
			  Timestamp *req_rec_tstamp)
{
	void *buf;

	buf = ppi->buf_out;

	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x03;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = htons(PP_PDELAY_RESP_LENGTH);
	*(UInteger8 *) (buf + 4) = hdr->domainNumber;
	pp_memset((buf + 8), 0, 8);

	*(UInteger16 *) (buf + 30) = htons(hdr->sequenceId);

	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;
	/* Table 24 */

	/* Pdelay_resp message */
	*(UInteger16 *) (buf + 34) = htons(req_rec_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = htonl(req_rec_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = htonl(req_rec_tstamp->nanosecondsField);
	pp_memcpy((buf + 44), hdr->sourcePortIdentity.clockIdentity,
		  PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) =
		htons(hdr->sourcePortIdentity.portNumber);

}

/* Unpack delayReq message from in buffer of ppi to msgtmp.req */
void msg_unpack_delay_req(void *buf, MsgDelayReq *delay_req)
{
	delay_req->originTimestamp.secondsField.msb =
		htons(*(UInteger16 *) (buf + 34));
	delay_req->originTimestamp.secondsField.lsb =
		htonl(*(UInteger32 *) (buf + 36));
	delay_req->originTimestamp.nanosecondsField =
		htonl(*(UInteger32 *) (buf + 40));

	PP_VPRINTF("Message DELAY_REQ\n");
	timestamp_display("Origin Timestamp",
			  &delay_req->originTimestamp);
	PP_VPRINTF("\n");
}



/* Unpack PdelayReq message from IN buffer of ppi to msgtmp.req */
void msg_unpack_pdelay_req(void *buf, MsgPDelayReq *pdelay_req)
{
	pdelay_req->originTimestamp.secondsField.msb =
		htons(*(UInteger16 *) (buf + 34));
	pdelay_req->originTimestamp.secondsField.lsb =
		htonl(*(UInteger32 *) (buf + 36));
	pdelay_req->originTimestamp.nanosecondsField =
		htonl(*(UInteger32 *) (buf + 40));

	PP_VPRINTF("Message PDELAY_REQ\n");
	timestamp_display("Origin Timestamp",
			  &pdelay_req->originTimestamp);
	PP_VPRINTF("\n");
}

/* Unpack delayResp message from IN buffer of ppi to msgtmp.presp */
void msg_unpack_delay_resp(void *buf, MsgDelayResp *resp)
{
	resp->receiveTimestamp.secondsField.msb =
		htons(*(UInteger16 *) (buf + 34));
	resp->receiveTimestamp.secondsField.lsb =
		htonl(*(UInteger32 *) (buf + 36));
	resp->receiveTimestamp.nanosecondsField =
		htonl(*(UInteger32 *) (buf + 40));
	pp_memcpy(resp->requestingPortIdentity.clockIdentity,
	       (buf + 44), PP_CLOCK_IDENTITY_LENGTH);
	resp->requestingPortIdentity.portNumber =
		htons(*(UInteger16 *) (buf + 52));

	PP_VPRINTF("Message DELAY_RESP\n");
	timestamp_display("Receive Timestamp",
			  &resp->receiveTimestamp);
	/* FIXME diag display requestingPortIdentity */
	PP_VPRINTF("\n");
}

/* Unpack PdelayResp message from IN buffer of ppi to msgtmp.presp */
void msg_unpack_pdelay_resp(void *buf, MsgPDelayResp *presp)
{
	presp->requestReceiptTimestamp.secondsField.msb =
		htons(*(UInteger16 *) (buf + 34));
	presp->requestReceiptTimestamp.secondsField.lsb =
		htonl(*(UInteger32 *) (buf + 36));
	presp->requestReceiptTimestamp.nanosecondsField =
		htonl(*(UInteger32 *) (buf + 40));
	pp_memcpy(presp->requestingPortIdentity.clockIdentity,
	       (buf + 44), PP_CLOCK_IDENTITY_LENGTH);
	presp->requestingPortIdentity.portNumber =
		htons(*(UInteger16 *) (buf + 52));

	PP_VPRINTF("Message PDELAY_RESP\n");
	timestamp_display("Request Receipt Timestamp",
			  &presp->requestReceiptTimestamp);
	/* FIXME diag display requestingPortIdentity */
	PP_VPRINTF("\n");
}

/* Pack PdelayRespFollowUp message into out buffer of ppi */
void msg_pack_pdelay_resp_followup(struct pp_instance *ppi,
				   MsgHeader *hdr,
				   Timestamp *resp_orig_tstamp)
{
	void *buf;

	buf = ppi->buf_out;

	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x0A;

	/* Table 19 */
	*(UInteger16 *) (buf + 2) = htons(PP_PDELAY_RESP_FOLLOW_UP_LENGTH);
	*(UInteger16 *) (buf + 30) = htons(ppi->pdelay_req_hdr.sequenceId);
	*(UInteger8 *) (buf + 32) = 0x05;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;

	/* Table 24 */

	/* Copy correctionField of PdelayReqMessage */
	*(Integer32 *) (buf + 8) = htonl(hdr->correctionfield.msb);
	*(Integer32 *) (buf + 12) = htonl(hdr->correctionfield.lsb);

	/* Pdelay_resp_follow_up message */
	*(UInteger16 *) (buf + 34) =
		htons(resp_orig_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) =
		htonl(resp_orig_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) =
		htonl(resp_orig_tstamp->nanosecondsField);
	pp_memcpy((buf + 44), hdr->sourcePortIdentity.clockIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 52) =
		htons(hdr->sourcePortIdentity.portNumber);
}

/* Unpack PdelayResp message from in buffer of ppi to msgtmp.presp */
void msg_unpack_pdelay_resp_followup(void *buf,
	MsgPDelayRespFollowUp *presp_follow)
{
	presp_follow->responseOriginTimestamp.secondsField.msb =
		htons(*(UInteger16 *) (buf + 34));
	presp_follow->responseOriginTimestamp.secondsField.lsb =
		htonl(*(UInteger32 *) (buf + 36));
	presp_follow->responseOriginTimestamp.nanosecondsField =
		htonl(*(UInteger32 *) (buf + 40));
	pp_memcpy(presp_follow->requestingPortIdentity.clockIdentity,
	       (buf + 44), PP_CLOCK_IDENTITY_LENGTH);
	presp_follow->requestingPortIdentity.portNumber =
		htons(*(UInteger16 *) (buf + 52));

	PP_VPRINTF("Message PDELAY_RESP_FOLLOW_UP\n");
	timestamp_display("Response Origin Timestamp",
			  &presp_follow->responseOriginTimestamp);
	/* FIXME diag display requestingPortIdentity */
	PP_VPRINTF("\n");
}

#define MSG_SEND_AND_RET(x,y,z)\
	if (pp_send_packet(ppi, ppi->buf_out, PP_## x ##_LENGTH, PP_NP_## y, z)\
		< PP_## x ##_LENGTH) {\
		PP_PRINTF("## x ## Message can't be sent -> FAULTY state!");\
		return -1;\
	}\
	PP_VPRINTF("## x ## Message sent");\
	ppi->sent_seq_id[PPM_## x]++;\
	return 0;

/* Pack and send on general multicast ip adress an Announce message */
int msg_issue_announce(struct pp_instance *ppi)
{
	msg_pack_announce(ppi);

	MSG_SEND_AND_RET(ANNOUNCE, GEN, 0);
}

/* Pack and send on event multicast ip adress a Sync message */
int msg_issue_sync(struct pp_instance *ppi)
{
	Timestamp orig_tstamp;
	TimeInternal now;
	pp_get_tstamp(&now);
	from_TimeInternal(&now, &orig_tstamp);

	msg_pack_sync(ppi,&orig_tstamp);

	MSG_SEND_AND_RET(SYNC, EVT, 0);
}

/* Pack and send on general multicast ip adress a FollowUp message */
int msg_issue_followup(struct pp_instance *ppi, TimeInternal *time)
{
	Timestamp prec_orig_tstamp;
	from_TimeInternal(time, &prec_orig_tstamp);

	msg_pack_follow_up(ppi, &prec_orig_tstamp);

	MSG_SEND_AND_RET(FOLLOW_UP, GEN, 0);
}

/* Pack and send on event multicast ip adress a DelayReq message */
int msg_issue_delay_req(struct pp_instance *ppi)
{
	Timestamp orig_tstamp;
	TimeInternal now;
	pp_get_tstamp(&now);
	from_TimeInternal(&now, &orig_tstamp);

	msg_pack_delay_req(ppi, &orig_tstamp);

	MSG_SEND_AND_RET(DELAY_REQ, EVT, 0);
}

/* Pack and send on event multicast ip adress a PDelayReq message */
int msg_issue_pdelay_req(struct pp_instance *ppi)
{
	Timestamp orig_tstamp;
	TimeInternal now;
	pp_get_tstamp(&now);
	from_TimeInternal(&now, &orig_tstamp);

	msg_pack_pdelay_req(ppi, &orig_tstamp);

	MSG_SEND_AND_RET(PDELAY_REQ, EVT, 1);
}

/* Pack and send on event multicast ip adress a PDelayResp message */
int msg_issue_pdelay_resp(struct pp_instance *ppi, TimeInternal *time,
			MsgHeader *hdr)
{
	Timestamp req_rec_tstamp;
	from_TimeInternal(time, &req_rec_tstamp);
	msg_pack_pdelay_resp(ppi, hdr, &req_rec_tstamp);

	MSG_SEND_AND_RET(PDELAY_RESP, EVT, 1);
}

/* Pack and send on event multicast ip adress a DelayResp message */
int msg_issue_delay_resp(struct pp_instance *ppi, TimeInternal *time)
{
	Timestamp rcv_tstamp;
	from_TimeInternal(time, &rcv_tstamp);

	msg_pack_delay_resp(ppi, &ppi->delay_req_hdr, &rcv_tstamp);

	MSG_SEND_AND_RET(PDELAY_RESP, GEN, 0);
}

/* Pack and send on event multicast ip adress a DelayResp message */
int msg_issue_pdelay_resp_follow_up(struct pp_instance *ppi, TimeInternal *time)
{
	Timestamp resp_orig_tstamp;
	from_TimeInternal(time, &resp_orig_tstamp);

	msg_pack_pdelay_resp_followup(ppi, &ppi->pdelay_req_hdr,
		&resp_orig_tstamp);

	MSG_SEND_AND_RET(PDELAY_RESP_FOLLOW_UP, GEN, 0);
}
