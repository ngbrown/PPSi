/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on ptp-noposix
 */

#ifndef __WREXT_WR_API_H__
#define __WREXT_WR_API_H__

#include "wr-constants.h"

/* Pack/Unkpack White rabbit message in the suffix of PTP announce message */
void msg_pack_announce_wr_tlv(struct pp_instance *ppi);
void msg_unpack_announce_wr_tlv(void *buf, MsgAnnounce *ann);

/* Pack/Unkpack/Issue White rabbit message signaling msg */
int msg_pack_wrsig(struct pp_instance *ppi, Enumeration16 wr_msg_id);
void msg_unpack_wrsig(struct pp_instance *ppi, void *buf,
		      MsgSignaling *wrsig_msg, Enumeration16 *wr_msg_id);
int msg_issue_wrsig(struct pp_instance *ppi, Enumeration16 wr_msg_id);

/* White rabbit state functions */
int wr_present(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_m_lock(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_s_lock(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_locked(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_calibration(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_calibrated(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_resp_calib_req(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_link_on(struct pp_instance *ppi, unsigned char *pkt, int plen);

/* White Rabbit hw-dependent functions (they are indeed implemented in
 * arch-spec or any other arch- */
int wr_locking_enable(struct pp_instance *ppi);
int wr_locking_poll(struct pp_instance *ppi);
int wr_locking_disable(struct pp_instance *ppi);


int wr_calibrating_disable(struct pp_instance *ppi, int txrx);
int wr_calibrating_enable(struct pp_instance *ppi, int txrx);
int wr_calibrating_poll(struct pp_instance *ppi, int txrx, uint64_t *delta);
int wr_calibration_pattern_enable(struct pp_instance *ppi,
	unsigned int calibrationPeriod, unsigned int calibrationPattern,
	unsigned int calibrationPatternLen);
int wr_calibration_pattern_disable(struct pp_instance *ppi);

#endif /* __WREXT_WR_API_H__ */
