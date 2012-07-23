/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on ptp-noposix project (see AUTHORS for details)
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
int wr_calibrating_poll(struct pp_instance *ppi, int txrx, uint32_t *delta);
int wr_calibration_pattern_enable(struct pp_instance *ppi,
	unsigned int calibrationPeriod, unsigned int calibrationPattern,
	unsigned int calibrationPatternLen);
int wr_calibration_pattern_disable(struct pp_instance *ppi);
int wr_read_calibration_data(struct pp_instance *ppi,
	uint32_t *deltaTx, uint32_t *deltaRx, int32_t *fix_alpha,
	int32_t *clock_period);

int wr_enable_ptracker(struct pp_instance *ppi);

int wr_enable_timing_output(struct pp_instance *ppi, int enable);

uint32_t wr_timer_get_msec_tics(void);

int wr_adjust_in_progress(void);
int wr_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec);
int wr_adjust_phase(int32_t phase_ps);

/* wr_servo interface */
int wr_servo_init(struct pp_instance *ppi);
void wr_servo_reset();
int wr_servo_man_adjust_phase(int phase);
int wr_servo_got_sync(struct pp_instance *ppi, TimeInternal *t1,
		      TimeInternal *t2);
int wr_servo_got_delay(struct pp_instance *ppi, Integer32 cf);
int wr_servo_update(struct pp_instance *ppi);

struct wr_servo_state_t {
	char if_name[16];
	int state;
	int next_state;
	TimeInternal prev_t4;
	TimeInternal mu;
	TimeInternal nsec_offset;
	int32_t delta_tx_m;
	int32_t delta_rx_m;
	int32_t delta_tx_s;
	int32_t delta_rx_s;
	int32_t cur_setpoint;
	int64_t delta_ms;
	int64_t delta_ms_prev;
	TimeInternal t1, t2, t3, t4;
	uint64_t last_tics;
	int32_t fiber_fix_alpha;
	int32_t clock_period_ps;
	int missed_iters;
};

/* All data used as extension ppsi-wr must be put here */
struct wr_data_t {
	struct wr_servo_state_t servo_state;
};

#endif /* __WREXT_WR_API_H__ */
