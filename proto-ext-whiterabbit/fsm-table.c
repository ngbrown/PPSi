/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 */

#include <ppsi/ppsi.h>
#include "wr_api.h"

/*
 * This is the default state machine table. It is weak so an extension
 * protocol can define its own stuff. It is in its own source file, so
 * the linker can avoid pulling this data space if another table is there.
 */

struct pp_state_table_item pp_state_table[] __weak = {
	{ PPS_INITIALIZING,	"initializing",	pp_initializing,},
	{ PPS_FAULTY,		"faulty",	pp_faulty,},
	{ PPS_DISABLED,		"disabled",	pp_disabled,},
	{ PPS_LISTENING,	"listening",	pp_listening,},
	{ PPS_PRE_MASTER,	"pre-master",	pp_pre_master,},
	{ PPS_MASTER,		"master",	pp_master,},
	{ PPS_PASSIVE,		"passive",	pp_passive,},
	{ PPS_UNCALIBRATED,	"uncalibrated",	pp_uncalibrated,},
	{ PPS_SLAVE,		"slave",	pp_slave,},
	{ WRS_PRESENT,		"wr-present",	wr_present,},
	{ WRS_M_LOCK,		"wr-m-lock",	wr_m_lock,},
	{ WRS_S_LOCK,		"wr-s-lock",	wr_s_lock,},
	{ WRS_LOCKED,		"wr-locked",	wr_locked,},
	{ WRS_CALIBRATION,	"wr-calibration",wr_calibration,},
	{ WRS_CALIBRATED,	"wr-calibrated",wr_calibrated,},
	{ WRS_RESP_CALIB_REQ,	"wr-resp-calib-req",wr_resp_calib_req,},
	{ WRS_WR_LINK_ON,	"wr-link-on",	wr_link_on,},
	{ PPS_END_OF_TABLE,}
};
