/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on ptp-noposix
 */

#include "wr-constants.h"

/* Pack/Unkpack White rabbit message in the suffix of PTP announce message */
void msg_pack_announce_wr_tlv(struct pp_instance *ppi);
void msg_unpack_announce_wr_tlv(void *buf, MsgAnnounce *ann);

/* Pack/Unkpack White rabbit message signaling msg */
int msg_pack_wrsig(struct pp_instance *ppi, Enumeration16 wr_msg_id);
void msg_unpack_wrsig(struct pp_instance *ppi, void *buf,
		      MsgSignaling *wrsig_msg, Enumeration16 *wr_msg_id);

/* White rabbit state functions */
int wr_present(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_m_lock(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_s_lock(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_locked(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_calibration(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_calibrated(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_resp_calib_req(struct pp_instance *ppi, unsigned char *pkt, int plen);
int wr_link_on(struct pp_instance *ppi, unsigned char *pkt, int plen);
