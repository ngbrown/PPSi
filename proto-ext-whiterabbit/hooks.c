#include <ppsi/ppsi.h>
#include "wr-api.h"
#include "../proto-standard/common-fun.h"

/* ext-whiterabbit must offer its own hooks */

static int wr_init(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);

	wrp->wrStateTimeout = WR_DEFAULT_STATE_TIMEOUT_MS;
	wrp->calPeriod = WR_DEFAULT_CAL_PERIOD;
	wrp->wrModeOn = 0;
	wrp->parentWrConfig = NON_WR;
	wrp->parentWrModeOn = 0;
	wrp->calibrated = !WR_DEFAULT_PHY_CALIBRATION_REQUIRED;

	if ((wrp->wrConfig & WR_M_AND_S) == WR_M_ONLY)
		wrp->ops->enable_timing_output(ppi, 1);
	else
		wrp->ops->enable_timing_output(ppi, 0);
	return 0;
}

static int wr_open(struct pp_globals *ppg, struct pp_runtime_opts *rt_opts)
{
	static struct wr_data_t wr_data; /* WR-specific global data */
	int i;

	/* If current arch (e.g. wrpc) is not using the 'pp_links style'
	 * configuration, just assume there is one ppi instance,
	 * already configured properly by the arch's main loop */
	if (ppg->nlinks == 0) {
		INST(ppg, 0)->ext_data = &wr_data;
		return 0;
	}

	for (i = 0; i < ppg->nlinks; i++) {
		struct pp_instance *ppi = INST(ppg, i);

		/* FIXME check if correct: assign to each instance the same
		 * wr_data. May I move it to pp_globals? */
		INST(ppg, i)->ext_data = &wr_data;

		if (ppi->cfg.ext == PPSI_EXT_WR) {
			switch (ppi->role) {
				case PPSI_ROLE_MASTER:
					WR_DSPOR(ppi)->wrConfig = WR_M_ONLY;
					break;
				case PPSI_ROLE_SLAVE:
					WR_DSPOR(ppi)->wrConfig = WR_S_ONLY;
					break;
				default:
					WR_DSPOR(ppi)->wrConfig = WR_M_AND_S;
			}
		}
		else
			WR_DSPOR(ppi)->wrConfig = NON_WR;
	}

	return 0;
}

static int wr_listening(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	struct wr_dsport *wrp = WR_DSPOR(ppi);

	wrp->wrMode = NON_WR;
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
		hdr->correctionfield.msb = 0;
		hdr->correctionfield.lsb =
			phase_to_cf_units(ppi->last_rcv_time.phase);
		msg_issue_delay_resp(ppi, time);
		msgtype = PPM_NOTHING_TO_DO;
		break;

	/* This is missing in the standard protocol */
	case PPM_SIGNALING:
		msg_unpack_wrsig(ppi, pkt, &wrsig_msg,
				 &(WR_DSPOR(ppi)->msgTmpWrMessageID));
		if ((WR_DSPOR(ppi)->msgTmpWrMessageID == SLAVE_PRESENT) &&
		    (WR_DSPOR(ppi)->wrConfig & WR_M_ONLY)) {
			/* We must start the handshake as a WR master */
			wr_handshake_init(ppi, PPS_MASTER);
		}
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

static int wr_handle_resp(struct pp_instance *ppi)
{
	MsgHeader *hdr = &ppi->received_ptp_header;
	TimeInternal correction_field;
	TimeInternal *ofm = &DSCUR(ppi)->offsetFromMaster;
	struct wr_dsport *wrp = WR_DSPOR(ppi);

	/* FIXME: check sub-nano relevance of correction filed */
	cField_to_TimeInternal(&correction_field, hdr->correctionfield);

	/*
	 * If no WR mode is on, run normal code, if T2/T3 are valid.
	 * After we adjusted the pps counter, stamps are invalid, so
	 * we'll have the Unix time instead, marked by "correct"
	 */
	if (!wrp->wrModeOn) {
		if (!ppi->t2.correct || !ppi->t3.correct) {
			pp_diag(ppi, servo, 1,
				"T2 or T3 incorrect, discarding tuple\n");
			return 0;
		}
		pp_servo_got_resp(ppi);
		/*
		 * pps always on if offset less than 1 second,
		 * until ve have a configurable threshold */
		if (ofm->seconds)
			wrp->ops->enable_timing_output(ppi, 0);
		else
			wrp->ops->enable_timing_output(ppi, 1);

	}
	wr_servo_got_delay(ppi, hdr->correctionfield.lsb);
	wr_servo_update(ppi);
	return 0;
}

static void wr_s1(struct pp_instance *ppi, MsgHeader *hdr, MsgAnnounce *ann)
{
	WR_DSPOR(ppi)->parentIsWRnode =
		((ann->ext_specific & WR_NODE_MODE) != NON_WR);
	WR_DSPOR(ppi)->parentWrModeOn =
		(ann->ext_specific & WR_IS_WR_MODE) ? TRUE : FALSE;
	WR_DSPOR(ppi)->parentCalibrated =
			((ann->ext_specific & WR_IS_CALIBRATED) ? 1 : 0);
	WR_DSPOR(ppi)->parentWrConfig = ann->ext_specific & WR_NODE_MODE;
	DSCUR(ppi)->primarySlavePortNumber =
		DSPOR(ppi)->portIdentity.portNumber;
}

static int wr_execute_slave(struct pp_instance *ppi)
{
	if (!WR_DSPOR(ppi)->doRestart)
		return 0;

	ppi->next_state = PPS_INITIALIZING;
	pp_timeout_restart_annrec(ppi);
	WR_DSPOR(ppi)->doRestart = FALSE;
	return 1; /* the caller returns too */
}

static void wr_handle_announce(struct pp_instance *ppi)
{
	if ((WR_DSPOR(ppi)->wrConfig & WR_S_ONLY) &&
	    (1 /* FIXME: Recommended State, see page 33*/) &&
	    (WR_DSPOR(ppi)->parentWrConfig & WR_M_ONLY) &&
	    (!WR_DSPOR(ppi)->wrModeOn || !WR_DSPOR(ppi)->parentWrModeOn)) {
		/* We must start the handshake as a WR slave */
		wr_handshake_init(ppi, PPS_SLAVE);
	}
}

static int wr_handle_followup(struct pp_instance *ppi,
			      TimeInternal *precise_orig_timestamp,
			      TimeInternal *correction_field)
{
	if (!WR_DSPOR(ppi)->wrModeOn)
		return 0;

	precise_orig_timestamp->phase = 0;
	wr_servo_got_sync(ppi, precise_orig_timestamp,
			  &ppi->t2);
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
	.handle_resp = wr_handle_resp,
	.s1 = wr_s1,
	.execute_slave = wr_execute_slave,
	.handle_announce = wr_handle_announce,
	.handle_followup = wr_handle_followup,
	.pack_announce = wr_pack_announce,
	.unpack_announce = wr_unpack_announce,
};
