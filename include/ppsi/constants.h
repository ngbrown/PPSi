/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#ifndef __PPSI_CONSTANTS_H__
#define __PPSI_CONSTANTS_H__

/* general purpose constants */
#define PP_NSEC_PER_SEC (1000*1000*1000)

/* implementation specific constants */
#define PP_DEFAULT_INBOUND_LATENCY		0	/* in nsec */
#define PP_DEFAULT_OUTBOUND_LATENCY		0	/* in nsec */
#define PP_DEFAULT_NO_RESET_CLOCK		0
#define PP_DEFAULT_ETHERNET_MODE		0
#define PP_DEFAULT_GPTP_MODE			0
#define PP_DEFAULT_DOMAIN_NUMBER		0
#define PP_DEFAULT_DELAY_MECHANISM		P2P
#define PP_DEFAULT_AP				10
#define PP_DEFAULT_AI				120
#define PP_DEFAULT_DELAY_S			6
#define PP_DEFAULT_ANNOUNCE_INTERVAL		1	/* 0 in 802.1AS, 3 is ok */
#define PP_DEFAULT_UTC_OFFSET			0
#define PP_DEFAULT_UTC_VALID			0
#define PP_DEFAULT_PDELAYREQ_INTERVAL		1	/* -4 in 802.1AS */
#define PP_DEFAULT_DELAYREQ_INTERVAL		0	/* 3 is ok */
#define PP_DEFAULT_SYNC_INTERVAL		0	/* -7 in 802.1AS, 1 is ok */
#define PP_DEFAULT_SYNC_RECEIPT_TIMEOUT		3
#define PP_DEFAULT_ANNOUNCE_RECEIPT_TIMEOUT	6	/* 3 by default, 3 is ok */
#define PP_DEFAULT_QUALIFICATION_TIMEOUT	2
#define PP_DEFAULT_FOREIGN_MASTER_TIME_WINDOW	4
#define PP_DEFAULT_FOREIGN_MASTER_THRESHOLD	2
#define PP_DEFAULT_CLOCK_CLASS			248
#define PP_DEFAULT_CLOCK_ACCURACY		0xFE
#define PP_DEFAULT_PRIORITY1			248
#define PP_DEFAULT_PRIORITY2			248
#define PP_DEFAULT_CLOCK_VARIANCE		-4000 /* To be determined in
						       * 802.1AS. We use the
						       * same value as in ptpdv1
						       */
#define PP_DEFAULT_MAX_FOREIGN_RECORDS		5
#define PP_DEFAULT_PARENTS_STATS		0
#define PP_DEFAULT_MAX_RESET			0
#define PP_DEFAULT_MAX_DELAY			0
#define PP_DEFAULT_NO_ADJUST			0
#define PP_DEFAULT_E2E_MODE			0
#define PP_DEFAULT_TTL				1

/* We use an array of timers, with these indexes */
#define PP_TIMER_PDELAYREQ	0
#define PP_TIMER_DELAYREQ	1
#define PP_TIMER_SYNC		2
#define PP_TIMER_ANN_RECEIPT	3
#define PP_TIMER_ANN_INTERVAL	4
/* White Rabbit timers */
#define PP_TIMER_WRS_PRESENT	5
#define PP_TIMER_WRS_S_LOCK	6
#define PP_TIMER_WRS_M_LOCK	7
#define PP_TIMER_WRS_LOCKED	8
#define PP_TIMER_WRS_CALIBRATION 9
#define PP_TIMER_WRS_CALIBRATED 10
#define PP_TIMER_WRS_RESP_CALIB_REQ 11
#define PP_TIMER_WRS_WR_LINK_ON 12
#define PP_TIMER_ARRAY_SIZE	13

#define PP_TWO_STEP_FLAG		2
#define PP_VERSION_PTP			2

#define PP_HEADER_LENGTH		34
#define PP_ANNOUNCE_LENGTH		64
#define PP_SYNC_LENGTH			44
#define PP_FOLLOW_UP_LENGTH		44
#define PP_PDELAY_REQ_LENGTH		54
#define PP_DELAY_REQ_LENGTH		44
#define PP_DELAY_RESP_LENGTH		54
#define PP_PDELAY_RESP_LENGTH		54
#define PP_PDELAY_RESP_FOLLOW_UP_LENGTH 54
#define PP_MANAGEMENT_LENGTH		48

#define PP_MINIMUM_LENGTH 44

#define PP_DEFAULT_NEXT_DELAY_MS	1000

/* UDP/IPv4 dependent */
#define PP_SUBDOMAIN_ADDRESS_LENGTH	4
#define PP_PORT_ADDRESS_LENGTH		2
#define PP_UUID_LENGTH			6
#define PP_CLOCK_IDENTITY_LENGTH	8
#define PP_FLAG_FIELD_LENGTH		2
#define PP_PACKET_SIZE			300
#define PP_EVT_PORT			319
#define PP_GEN_PORT			320
#define PP_DEFAULT_DOMAIN_ADDRESS	"224.0.1.129"
#define PP_PEER_DOMAIN_ADDRESS		"224.0.0.107"
#define PP_MM_STARTING_BOUNDARY_HOPS	0x7fff

/* Raw ethernet dependent */
#ifndef ETH_P_1588
#define ETH_P_1588			0x88F7
#endif

#define PP_MCAST_MACADDRESS		"\x01\x1B\x19\x00\x00\x00"
#define PP_PEER_MACADDRESS		"\x01\x80\xC2\x00\x00\x0E"

#include <arch/constants.h> /* architectures may override the defaults */

#endif /* __PPSI_CONSTANTS_H__ */
