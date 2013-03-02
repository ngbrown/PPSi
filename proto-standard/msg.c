/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "common-fun.h"

static void Integer64_display(const char *label, Integer64 *bigint)
{
	if (pp_verbose_dump) {
		pp_Vprintf("%s:\n", label);
		pp_Vprintf("LSB: %u\n", bigint->lsb);
		pp_Vprintf("MSB: %d\n", bigint->msb);
	}
}

static void UInteger48_display(const char *label, UInteger48 *bigint)
{
	if (pp_verbose_dump) {
		pp_Vprintf("%s:\n", label);
		pp_Vprintf("LSB: %u\n", bigint->lsb);
		pp_Vprintf("MSB: %u\n", bigint->msb);
	}
}

static void timestamp_display(const char *label, Timestamp *timestamp)
{
	if (pp_verbose_dump) {
		pp_Vprintf("%s:\n", label);
		UInteger48_display("seconds", &timestamp->secondsField);
		pp_Vprintf("nanoseconds: %u\n", timestamp->nanosecondsField);
	}
}

static void msg_display_header(MsgHeader *header)
{
	if (pp_verbose_dump) {
		pp_Vprintf("Message header:\n");
		pp_Vprintf("\n");
		pp_Vprintf("transportSpecific: %d\n",
			   header->transportSpecific);
		pp_Vprintf("messageType: %d\n", header->messageType);
		pp_Vprintf("versionPTP: %d\n", header->versionPTP);
		pp_Vprintf("messageLength: %d\n", header->messageLength);
		pp_Vprintf("domainNumber: %d\n", header->domainNumber);
		pp_Vprintf("FlagField %02x:%02x\n", header->flagField[0],
			   header->flagField[1]);
		Integer64_display("correctionfield", &header->correctionfield);
		/* FIXME diag portIdentity_display(&h->sourcePortIdentity); */
		pp_Vprintf("sequenceId: %d\n", header->sequenceId);
		pp_Vprintf("controlField: %d\n", header->controlField);
		pp_Vprintf("logMessageInterval: %d\n",
			   header->logMessageInterval);
		pp_Vprintf("\n");
	}
}

static void msg_display_announce(MsgAnnounce *announce)
{
	if (pp_verbose_dump) {
		pp_Vprintf("Message ANNOUNCE:\n");
		timestamp_display("Origin Timestamp",
				  &announce->originTimestamp);
		pp_Vprintf("currentUtcOffset: %d\n",
			   announce->currentUtcOffset);
		pp_Vprintf("grandMasterPriority1: %d\n",
			   announce->grandmasterPriority1);
		pp_Vprintf("grandMasterClockQuality:\n");
		/* FIXME diag clockQuality_display(&ann->gmasterQuality); */
		pp_Vprintf("grandMasterPriority2: %d\n",
			   announce->grandmasterPriority2);
		pp_Vprintf("grandMasterIdentity:\n");
		/* FIXME diag clockIdentity_display(ann->gmasterIdentity); */
		pp_Vprintf("stepsRemoved: %d\n", announce->stepsRemoved);
		pp_Vprintf("timeSource: %d\n", announce->timeSource);
		pp_Vprintf("\n");
		/* FIXME: diagnostic for extension */
	}
}

/* Unpack header from in buffer to msg_tmp_header field */
int msg_unpack_header(struct pp_instance *ppi, void *buf)
{
	MsgHeader *hdr = &ppi->msg_tmp_header;

	hdr->transportSpecific = (*(Nibble *) (buf + 0)) >> 4;
	hdr->messageType = (*(Enumeration4 *) (buf + 0)) & 0x0F;
	hdr->versionPTP = (*(UInteger4 *) (buf + 1)) & 0x0F;

	/* force reserved bit to zero if not */
	hdr->messageLength = htons(*(UInteger16 *) (buf + 2));
	hdr->domainNumber = (*(UInteger8 *) (buf + 4));

	memcpy(hdr->flagField, (buf + 6), PP_FLAG_FIELD_LENGTH);

	memcpy(&hdr->correctionfield.msb, (buf + 8), 4);
	memcpy(&hdr->correctionfield.lsb, (buf + 12), 4);
	hdr->correctionfield.msb = htonl(hdr->correctionfield.msb);
	hdr->correctionfield.lsb = htonl(hdr->correctionfield.lsb);
	memcpy(hdr->sourcePortIdentity.clockIdentity, (buf + 20),
	       PP_CLOCK_IDENTITY_LENGTH);
	hdr->sourcePortIdentity.portNumber =
		htons(*(UInteger16 *) (buf + 28));
	hdr->sequenceId = htons(*(UInteger16 *) (buf + 30));
	hdr->controlField = (*(UInteger8 *) (buf + 32));
	hdr->logMessageInterval = (*(Integer8 *) (buf + 33));

	/*
	 * If the message is from us, we should discard it.
	 * The best way to do that is comparing the mac address,
	 * but it's easier to check the clock identity (we refuse
	 * any port, not only the same port, as we can't sync with
	 * ourself even when we'll run in multi-port mode.
	 */
	if (!memcmp(ppi->msg_tmp_header.sourcePortIdentity.clockIdentity,
			DSPOR(ppi)->portIdentity.clockIdentity,
		    PP_CLOCK_IDENTITY_LENGTH))
		return -1;

	if (!memcmp(DSPAR(ppi)->parentPortIdentity.clockIdentity,
			hdr->sourcePortIdentity.clockIdentity,
			PP_CLOCK_IDENTITY_LENGTH) &&
			(DSPAR(ppi)->parentPortIdentity.portNumber ==
			hdr->sourcePortIdentity.portNumber))
		ppi->is_from_cur_par = 1;
	else
		ppi->is_from_cur_par = 0;

	msg_display_header(hdr);
	return 0;
}

/* Pack header message into out buffer of ppi */
void msg_pack_header(struct pp_instance *ppi, void *buf)
{
	Nibble transport = 0x80;

	/* (spec annex D) */
	*(UInteger8 *) (buf + 0) = transport;
	*(UInteger4 *) (buf + 1) = DSPOR(ppi)->versionNumber;
	*(UInteger8 *) (buf + 4) = DSDEF(ppi)->domainNumber;

	*(UInteger8 *) (buf + 6) = PP_TWO_STEP_FLAG;

	memset((buf + 8), 0, 8);
	memcpy((buf + 20), DSPOR(ppi)->portIdentity.clockIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 28) =
				htons(DSPOR(ppi)->portIdentity.portNumber);
	*(UInteger8 *) (buf + 33) = 0x7F;
	/* Default value(spec Table 24) */
}

void *msg_copy_header(MsgHeader *dest, MsgHeader *src)
{
	return memcpy(dest, src, sizeof(MsgHeader));
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
	*(UInteger16 *) (buf + 30) = htons(ppi->sent_seq[PPM_SYNC]);
	*(UInteger8 *) (buf + 32) = 0x00;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logSyncInterval;
	memset((buf + 8), 0, 8);

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

	if (pp_verbose_dump) {
		PP_VPRINTF("Message SYNC\n");
		timestamp_display("Origin Timestamp", &sync->originTimestamp);
		PP_VPRINTF("\n");
	}
}

/* Pack Announce message into out buffer of ppi */
int msg_pack_announce(struct pp_instance *ppi)
{
	void *buf;

	buf = ppi->buf_out;
	/* changes in header */
	*(char *)(buf + 0) = *(char *)(buf + 0) & 0xF0;
	/* RAZ messageType */
	*(char *)(buf + 0) = *(char *)(buf + 0) | 0x0B;
	/* Table 19 */
	*(UInteger16 *) (buf + 2) = htons(PP_ANNOUNCE_LENGTH);
	*(UInteger16 *) (buf + 30) = htons(ppi->sent_seq[PPM_ANNOUNCE]);
	*(UInteger8 *) (buf + 32) = 0x05;
	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logAnnounceInterval;

	/* Announce message */
	memset((buf + 34), 0, 10);
	*(Integer16 *) (buf + 44) = htons(DSPRO(ppi)->currentUtcOffset);
	*(UInteger8 *) (buf + 47) = DSPAR(ppi)->grandmasterPriority1;
	*(UInteger8 *) (buf + 48) = DSDEF(ppi)->clockQuality.clockClass;
	*(Enumeration8 *) (buf + 49) = DSDEF(ppi)->clockQuality.clockAccuracy;
	*(UInteger16 *) (buf + 50) =
		htons(DSDEF(ppi)->clockQuality.offsetScaledLogVariance);
	*(UInteger8 *) (buf + 52) = DSPAR(ppi)->grandmasterPriority2;
	memcpy((buf + 53), DSPAR(ppi)->grandmasterIdentity,
	       PP_CLOCK_IDENTITY_LENGTH);
	*(UInteger16 *) (buf + 61) = htons(DSCUR(ppi)->stepsRemoved);
	*(Enumeration8 *) (buf + 63) = DSPRO(ppi)->timeSource;

	if (pp_hooks.pack_announce)
		return pp_hooks.pack_announce(ppi);
	return PP_ANNOUNCE_LENGTH;
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
	memcpy(ann->grandmasterIdentity, (buf + 53),
	       PP_CLOCK_IDENTITY_LENGTH);
	ann->stepsRemoved = htons(*(UInteger16 *) (buf + 61));
	ann->timeSource = *(Enumeration8 *) (buf + 63);

	if (pp_hooks.unpack_announce)
		pp_hooks.unpack_announce(buf, ann);
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
	*(UInteger16 *) (buf + 30) = htons(ppi->sent_seq[PPM_SYNC] - 1);

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

	if (pp_verbose_dump) {
		PP_VPRINTF("Message FOLLOW_UP\n");
		timestamp_display("Precise Origin Timestamp",
				  &flwup->preciseOriginTimestamp);
		PP_VPRINTF("\n");
	}
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
	*(UInteger16 *) (buf + 30) = htons(ppi->sent_seq[PPM_DELAY_REQ]);
	*(UInteger8 *) (buf + 32) = 0x01;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = 0x7F;

	/* Table 24 */
	memset((buf + 8), 0, 8);

	/* Delay_req message */
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
	memset((buf + 8), 0, 8);

	/* Copy correctionField of delayReqMessage */
	*(Integer32 *) (buf + 8) = htonl(hdr->correctionfield.msb);
	*(Integer32 *) (buf + 12) = htonl(hdr->correctionfield.lsb);

	*(UInteger16 *) (buf + 30) = htons(hdr->sequenceId);

	*(UInteger8 *) (buf + 32) = 0x03;

	/* Table 23 */
	*(Integer8 *) (buf + 33) = DSPOR(ppi)->logMinDelayReqInterval;

	/* Table 24 */

	/* Delay_resp message */
	*(UInteger16 *) (buf + 34) =
		htons(rcv_tstamp->secondsField.msb);
	*(UInteger32 *) (buf + 36) = htonl(rcv_tstamp->secondsField.lsb);
	*(UInteger32 *) (buf + 40) = htonl(rcv_tstamp->nanosecondsField);
	memcpy((buf + 44), hdr->sourcePortIdentity.clockIdentity,
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

	if (pp_verbose_dump) {
		PP_VPRINTF("Message DELAY_REQ\n");
		timestamp_display("Origin Timestamp",
				  &delay_req->originTimestamp);
		PP_VPRINTF("\n");
	}
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
	memcpy(resp->requestingPortIdentity.clockIdentity,
	       (buf + 44), PP_CLOCK_IDENTITY_LENGTH);
	resp->requestingPortIdentity.portNumber =
		htons(*(UInteger16 *) (buf + 52));

	if (pp_verbose_dump) {
		PP_VPRINTF("Message DELAY_RESP\n");
		timestamp_display("Receive Timestamp",
				  &resp->receiveTimestamp);
		/* FIXME diag display requestingPortIdentity */
		PP_VPRINTF("\n");
	}
}

const char const *pp_msg_names[] = {
	[PPM_SYNC] =			"sync",
	[PPM_DELAY_REQ] =		"delay_req",
	[PPM_PDELAY_REQ] =		"pdelay_req",
	[PPM_PDELAY_RESP] =		"pdelay_resp",

	[PPM_FOLLOW_UP] =		"follow_up",
	[PPM_DELAY_RESP] =		"delay_resp",
	[PPM_PDELAY_RESP_FOLLOW_UP] =	"pdelay_resp_follow_up",
	[PPM_ANNOUNCE] =		"announce",
	[PPM_SIGNALING] =		"signaling",
	[PPM_MANAGEMENT] =		"management",
};

/* Pack and send on general multicast ip adress an Announce message */
int msg_issue_announce(struct pp_instance *ppi)
{
	int len = msg_pack_announce(ppi);

	return __send_and_log(ppi, len, PPM_ANNOUNCE, PP_NP_GEN);
}

/* Pack and send on event multicast ip adress a Sync message */
int msg_issue_sync(struct pp_instance *ppi)
{
	Timestamp orig_tstamp;
	TimeInternal now;
	pp_t_ops.get(&now);
	from_TimeInternal(&now, &orig_tstamp);

	msg_pack_sync(ppi, &orig_tstamp);

	return __send_and_log(ppi, PP_SYNC_LENGTH, PPM_SYNC, PP_NP_EVT);
}

/* Pack and send on general multicast ip address a FollowUp message */
int msg_issue_followup(struct pp_instance *ppi, TimeInternal *time)
{
	Timestamp prec_orig_tstamp;
	from_TimeInternal(time, &prec_orig_tstamp);

	msg_pack_follow_up(ppi, &prec_orig_tstamp);

	return __send_and_log(ppi, PP_FOLLOW_UP_LENGTH, PPM_FOLLOW_UP,
			      PP_NP_GEN);
}

/* Pack and send on event multicast ip adress a DelayReq message */
int msg_issue_delay_req(struct pp_instance *ppi)
{
	Timestamp orig_tstamp;
	TimeInternal now;
	pp_t_ops.get(&now);
	from_TimeInternal(&now, &orig_tstamp);

	msg_pack_delay_req(ppi, &orig_tstamp);

	return __send_and_log(ppi, PP_DELAY_REQ_LENGTH, PPM_DELAY_REQ,
			      PP_NP_EVT);
}

/* Pack and send on event multicast ip adress a DelayResp message */
int msg_issue_delay_resp(struct pp_instance *ppi, TimeInternal *time)
{
	Timestamp rcv_tstamp;
	from_TimeInternal(time, &rcv_tstamp);

	msg_pack_delay_resp(ppi, &ppi->delay_req_hdr, &rcv_tstamp);

	return __send_and_log(ppi, PP_DELAY_RESP_LENGTH, PPM_DELAY_RESP,
			      PP_NP_GEN);
}
