/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"

/*
 * This is the WR state machine table.
 */

struct pp_state_table_item pp_state_table[] = {
	{ PPS_INITIALIZING,	"initializing",	pp_initializing,},
	{ PPS_FAULTY,		"faulty",	pp_faulty,},
	{ PPS_DISABLED,		"disabled",	pp_disabled,},
	{ PPS_LISTENING,	"listening",	pp_listening,},
	/*{ PPS_PRE_MASTER,	"pre-master",	pp_pre_master,},*/
	{ PPS_MASTER,		"master",	pp_master,},
	/*{ PPS_PASSIVE,	"passive",	pp_passive,},*/
	{ PPS_UNCALIBRATED,	"uncalibrated",	pp_uncalibrated,},
	{ PPS_SLAVE,		"slave",	pp_slave,},
	{ WRS_PRESENT,		"uncalibrated/wr-present",	wr_present,},
	{ WRS_M_LOCK,		"master/wr-m-lock",	wr_m_lock,},
	{ WRS_S_LOCK,		"uncalibrated/wr-s-lock",	wr_s_lock,},
	{ WRS_LOCKED,		"uncalibrated/wr-locked",	wr_locked,},
	{ WRS_CALIBRATION,	"wr-calibration",	wr_calibration,},
	{ WRS_CALIBRATED,	"wr-calibrated",	wr_calibrated,},
	{ WRS_RESP_CALIB_REQ,	"wr-resp-calib-req",	wr_resp_calib_req,},
	{ WRS_WR_LINK_ON,	"wr-link-on",		wr_link_on,},
	{ PPS_END_OF_TABLE,}
};
