#include <ppsi/ppsi.h>
#include "wr-api.h"

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
	MsgHeader *hdr = &ppi->msg_tmp_header;
	MsgSignaling wrsig_msg;
	TimeInternal *time = &ppi->last_rcv_time;
;

	switch(msgtype) {

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

struct pp_ext_hooks pp_hooks = {
	.init = wr_init,
	.open = wr_open,
	.listening = wr_listening,
	.master_msg = wr_master_msg,
};
