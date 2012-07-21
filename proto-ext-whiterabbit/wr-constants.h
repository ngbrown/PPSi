/*
 * Aurelio Colosimo for CERN, 2012 -- GNU LGPL v2.1 or later
 * Based on ptp-noposix
 */

/* White Rabbit stuff
 * if this defined, WR uses new implementation of timeouts (not using interrupt)
 */

#ifndef __WREXT_WR_CONSTANTS_H__
#define __WREXT_WR_CONSTANTS_H__

#define WR_NODE				0x80
#define WR_IS_CALIBRATED		0x04
#define WR_IS_WR_MODE			0x08
#define WR_NODE_MODE			0x03

# define WR_TLV_TYPE			0x2004

#define WR_MASTER_PRIORITY1		6
#define WR_DEFAULT_CAL_PERIOD		3000	/* [us] */
#define WR_DEFAULT_CAL_PATTERN		0x3E0	/* 1111100000 */
#define WR_DEFAULT_CAL_PATTERN_LEN	0xA	/* 10 bits */

#define WR_DEFAULT_STATE_TIMEOUT_MS	2000 	/* [ms] (was 300) */
#define WR_M_LOCK_TIMEOUT_MS		10000
#define WR_S_LOCK_TIMEOUT_MS		10000
#define WR_DEFAULT_STATE_REPEAT		3
#define WR_DEFAULT_INIT_REPEAT		3

/* White Rabbit package Size */
#define WR_ANNOUNCE_TLV_LENGTH		0x0A

#define WR_ANNOUNCE_LENGTH    (PP_ANNOUNCE_LENGTH + WR_ANNOUNCE_TLV_LENGTH + 4)
#define WR_MANAGEMENT_TLV_LENGTH	 6
#define WR_MANAGEMENT_LENGTH  (MANAGEMENT_LENGTH + WR_MANAGEMENT_TLV_LENGTH)

/* memory footprint tweak for WRPC */
#ifdef WRPC_EXTRA_SLIM
#define MAX_PORT_NUMBER			1
#else
#define MAX_PORT_NUMBER			16
#endif

#define MIN_PORT_NUMBER			1

#define WR_PORT_NUMBER			10

#define WR_SLAVE_CLOCK_CLASS		248
#define WR_MASTER_CLOCK_CLASS		5

#define WR_MASTER_ONLY_CLOCK_CLASS	70

/* new stuff for WRPTPv2 */

#define TLV_TYPE_ORG_EXTENSION 		0x0003 /* organization specific */

#define WR_PRIORITY1                    64

#define WR_TLV_ORGANIZATION_ID		0x080030
#define WR_TLV_MAGIC_NUMBER		0xDEAD
#define WR_TLV_WR_VERSION_NUMBER	0x01

/* WR_SIGNALING_MSG_BASE_LENGTH Computation:
 * = LEN(header) + LEN(targetPortId) + LEN(tlvType) + LEN(lenghtField)
 *      34       +      10           +     2        +     2 */
#define WR_SIGNALING_MSG_BASE_LENGTH	48

#define WR_DEFAULT_PHY_CALIBRATION_REQUIRED FALSE

#define     SEND_CALIBRATION_PATTERN 	0X0001
#define NOT_SEND_CALIBRATION_PATTERN 	0X0000

/* White Rabbit softpll status values */
#define WR_SPLL_OK	0
#define WR_SPLL_READY	1
#define WR_SPLL_ERROR	-1

/* Indicates if a port is configured as White Rabbit, and what kind
 * (master/slave) */
enum {
	NON_WR      = 0x0,
	WR_S_ONLY   = 0x2,
	WR_M_ONLY   = 0x1,
	WR_M_AND_S  = 0x3,
	WR_MODE_AUTO= 0x4, /* only for ptpx - not in the spec */
};

/* White Rabbit node */
enum{
	WR_SLAVE  = 2,
	WR_MASTER = 1,
};

/* Values of Management Actions (extended for WR), see table 38
 */
enum {
	GET,
	SET,
	RESPONSE,
	COMMAND,
	ACKNOWLEDGE,
	WR_CMD, /* White Rabbit */
};

/* brief WR PTP states (new, single FSM) */
enum {
	/* WR states start from 16 in order not to be confused with PTP states,
	 * since our application FSM is flat */
	WRS_PRESENT = 16,  WRS_S_LOCK, WRS_M_LOCK,  WRS_LOCKED,
	WRS_CALIBRATION,  WRS_CALIBRATED,  WRS_RESP_CALIB_REQ ,WRS_WR_LINK_ON,
	/* Each WR main state (except IDLE) has an associated timetout
	 * we use state names to manage timeouts as well */
	WR_TIMER_ARRAY_SIZE, /* number of states which has timeouts */
	WRS_IDLE,
	/* here are substates*/
	WRS_S_LOCK_1,
	WRS_S_LOCK_2,
	WRS_CALIBRATION_1,
	WRS_CALIBRATION_2,
	WRS_CALIBRATION_3,
	WRS_CALIBRATION_4,
	WRS_CALIBRATION_5,
	WRS_CALIBRATION_6,
	WRS_CALIBRATION_7,
	WRS_CALIBRATION_8,
	WRS_RESP_CALIB_REQ_1,
	WRS_RESP_CALIB_REQ_2,
	WRS_RESP_CALIB_REQ_3,
};

/* White Rabbit commands (for new implementation, single FSM), see table 38 */
enum {

	NULL_WR_TLV = 0x0000,
	SLAVE_PRESENT	= 0x1000,
	LOCK,
	LOCKED,
	CALIBRATE,
	CALIBRATED,
	WR_MODE_ON,
	ANN_SUFIX = 0x2000,
};

/* White Rabbit slave port's role */
enum {
	NON_SLAVE	= 0x0,
	PRIMARY_SLAVE 	,
	SECONDARY_SLAVE ,
};

/* White Rabbit data initialization mode */
enum {
	INIT,
	RE_INIT,
};

#endif /* __WREXT_WR_CONSTANTS_H__ */
