#include <ppsi/ppsi.h>
#include "wr-api.h"
#include "../proto-standard/common-fun.h"

/* ext-whiterabbit must offer its own hooks */

static int wr_init(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct wr_dsport *wp = WR_DSPOR(ppi);

	wp->wrStateTimeout = WR_DEFAULT_STATE_TIMEOUT_MS;
	wp->wrStateRetry = WR_DEFAULT_STATE_REPEAT;
	wp->calPeriod = WR_DEFAULT_CAL_PERIOD;
	wp->wrModeOn = 0;
	wp->parentWrConfig = 0;
	wp->calibrated = !WR_DEFAULT_PHY_CALIBRATION_REQUIRED;
	return 0;
}

/* This currently only works with one interface (i.e. WR Node) */
static int wr_open(struct pp_instance *ppi, struct pp_runtime_opts *rt_opts)
{
	static struct wr_data_t wr_data; /* WR-specific global data */

	rt_opts->iface_name = "wr1";
	ppi->ext_data = &wr_data;
	return 0;
}

static int wr_listening(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct wr_dsport *wp = WR_DSPOR(ppi);

	wp->wrMode = NON_WR;
	wp->wrPortState = WRS_IDLE;
	return 0;
}

static int wr_master_msg(struct pp_instance *ppi, unsigned char *pkt, int plen,
			 int msgtype)
{
	MsgHeader *hdr = &ppi->received_ptp_header;
	MsgSignaling wrsig_msg;
	TimeInternal *time = &ppi->last_rcv_time;

	switch (msgtype) {

	/* This case is modified from the default one */
	case PPM_DELAY_REQ:
		msg_copy_header(&ppi->delay_req_hdr, hdr);
		ppi->delay_req_hdr.correctionfield.msb = 0;
		ppi->delay_req_hdr.correctionfield.lsb =
			phase_to_cf_units(ppi->last_rcv_time.phase);
		msg_issue_delay_resp(ppi, time);
		msgtype = PPM_NOTHING_TO_DO;
		break;

	/* This is missing in the standard protocol */
	case PPM_SIGNALING:
		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
				 &(WR_DSPOR(ppi)->msgTmpWrMessageID));
		if ((WR_DSPOR(ppi)->msgTmpWrMessageID == SLAVE_PRESENT) &&
		    (WR_DSPOR(ppi)->wrConfig & WR_M_ONLY))
			ppi->next_state = WRS_M_LOCK;
		msgtype = PPM_NOTHING_TO_DO;
		break;
	}

	return msgtype;
}

static int wr_new_slave(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	wr_servo_init(ppi);
	return 0;
}

static int wr_update_delay(struct pp_instance *ppi)
{
	MsgHeader *hdr = &ppi->received_ptp_header;
	TimeInternal correction_field;

	int64_to_TimeInternal(hdr->correctionfield, &correction_field);

	/* If no WR mode is on, run normal code */
	if (!WR_DSPOR(ppi)->wrModeOn) {
		pp_update_delay(ppi, &correction_field);
		return 0;
	}
	wr_servo_got_delay(ppi, hdr->correctionfield.lsb);
	wr_servo_update(ppi);
	return 0;
}

static void wr_s1(struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann)
{
	WR_DSPOR(ppi)->parentIsWRnode =
		((ann->wrFlags & WR_NODE_MODE) != NON_WR);
	WR_DSPOR(ppi)->parentWrModeOn =
		(ann->wrFlags & WR_IS_WR_MODE) ? TRUE : FALSE;
	WR_DSPOR(ppi)->parentCalibrated =
			((ann->wrFlags & WR_IS_CALIBRATED) ? 1 : 0);
	WR_DSPOR(ppi)->parentWrConfig = ann->wrFlags & WR_NODE_MODE;
	DSCUR(ppi)->primarySlavePortNumber =
		DSPOR(ppi)->portIdentity.portNumber;
}

static int wr_execute_slave(struct pp_instance *ppi)
{
	if (!WR_DSPOR(ppi)->doRestart)
		return 0;

	ppi->next_state = PPS_INITIALIZING;
	st_com_restart_annrec_timer(ppi);
	WR_DSPOR(ppi)->doRestart = FALSE;
	return 1; /* the caller returns too */
}

static void wr_handle_announce(struct pp_instance *ppi)
{
	if ((WR_DSPOR(ppi)->wrConfig & WR_S_ONLY) &&
	    (1 /* FIXME: Recommended State, see page 33*/) &&
	    (WR_DSPOR(ppi)->parentWrConfig & WR_M_ONLY) &&
	    (!WR_DSPOR(ppi)->wrModeOn || !WR_DSPOR(ppi)->parentWrModeOn))
		ppi->next_state = WRS_PRESENT;
}

static int wr_handle_followup(struct pp_instance *ppi,
			      TimeInternal *precise_orig_timestamp,
			      TimeInternal *correction_field)
{
	if (!WR_DSPOR(ppi)->wrModeOn)
		return 0;

	precise_orig_timestamp->phase = 0;
	wr_servo_got_sync(ppi, precise_orig_timestamp,
			  &ppi->sync_receive_time);
	/* TODO Check: generates instability (Tx timestamp invalid) */
	/* return msg_issue_delay_req(ppi); */

	return 1; /* the caller returns too */
}

int wr_pack_announce(struct pp_instance *ppi)
{
	if (WR_DSPOR(ppi)->wrConfig != NON_WR &&
		WR_DSPOR(ppi)->wrConfig != WR_S_ONLY) {
		msg_pack_announce_wr_tlv(ppi);
		return WR_ANNOUNCE_LENGTH;
	}
	return PP_ANNOUNCE_LENGTH;
}

void wr_unpack_announce(void *buf, MsgAnnounce *ann)
{
	int msg_len = htons(*(UInteger16 *) (buf + 2));

	if (msg_len > PP_ANNOUNCE_LENGTH)
		msg_unpack_announce_wr_tlv(buf, ann);
}


struct pp_ext_hooks pp_hooks = {
	.init = wr_init,
	.open = wr_open,
	.listening = wr_listening,
	.master_msg = wr_master_msg,
	.new_slave = wr_new_slave,
	.update_delay = wr_update_delay,
	.s1 = wr_s1,
	.execute_slave = wr_execute_slave,
	.handle_announce = wr_handle_announce,
	.handle_followup = wr_handle_followup,
	.pack_announce = wr_pack_announce,
	.unpack_announce = wr_unpack_announce,
};
