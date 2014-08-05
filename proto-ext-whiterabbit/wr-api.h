/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on ptp-noposix project (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#ifndef __WREXT_WR_API_H__
#define __WREXT_WR_API_H__

#include <ppsi/lib.h>
#include "wr-constants.h"

/*
 * This structure is used as extension-specific data in the DSPort
 * (see wrspec.v2-06-07-2011, page 17)
 */
struct wr_dsport {
	struct wr_operations *ops; /* hardware-dependent, see below */
	Enumeration8 wrConfig;
	Enumeration8 wrMode;
	Boolean wrModeOn;
	Boolean ppsOutputOn;
	Enumeration8  wrPortState; /* used for sub-states during calibration */
	/* FIXME check doc: knownDeltaTx, knownDeltaRx, deltasKnown?) */
	Boolean calibrated;
	FixedDelta deltaTx;
	FixedDelta deltaRx;
	UInteger32 wrStateTimeout;
	UInteger8 wrStateRetry;
	UInteger32 calPeriod;		/* microseconsds, never changed */
	UInteger8 calRetry;
	Enumeration8 parentWrConfig;
	Boolean parentIsWRnode; /* FIXME Not in the doc */
	/* FIXME check doc: (parentWrMode?) */
	Enumeration16 msgTmpWrMessageID; /* FIXME Not in the doc */
	Boolean parentWrModeOn;
	Boolean parentCalibrated;

	/* FIXME: are they in the doc? */
	UInteger16 otherNodeCalSendPattern;
	UInteger32 otherNodeCalPeriod;/* microseconsds, never changed */
	UInteger8 otherNodeCalRetry;
	FixedDelta otherNodeDeltaTx;
	FixedDelta otherNodeDeltaRx;
	Boolean doRestart;
	Boolean linkUP;
};

/* This uppercase name matches "DSPOR(ppi)" used by standard protocol */
static inline struct wr_dsport *WR_DSPOR(struct pp_instance *ppi)
{
	return ppi->portDS->ext_dsport;
}

static inline Integer32 phase_to_cf_units(Integer32 phase)
{
	uint64_t ph64;
	if (phase >= 0) {
		ph64 = phase * 65536LL;
		__div64_32(&ph64, 1000);
		return ph64;
	} else {
		ph64 = -phase * 65536LL;
		__div64_32(&ph64, 1000);
		return -ph64;
	}
}

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

/* Common functions, used by various states and hooks */
void wr_handshake_init(struct pp_instance *ppi, int mode);
void wr_handshake_fail(struct pp_instance *ppi); /* goto non-wr */
int wr_handshake_retry(struct pp_instance *ppi); /* 1 == retry; 0 == failed */

/* White Rabbit hw-dependent functions (code in arch-wrpc and arch-wrs) */
struct wr_operations {
	int (*locking_enable)(struct pp_instance *ppi);
	int (*locking_poll)(struct pp_instance *ppi, int grandmaster);
	int (*locking_disable)(struct pp_instance *ppi);
	int (*enable_ptracker)(struct pp_instance *ppi);

	int (*adjust_in_progress)(void);
	int (*adjust_counters)(int64_t adjust_sec, int32_t adjust_nsec);
	int (*adjust_phase)(int32_t phase_ps);

	int (*read_calib_data)(struct pp_instance *ppi,
			      uint32_t *deltaTx, uint32_t *deltaRx,
			      int32_t *fix_alpha, int32_t *clock_period);
	int (*calib_disable)(struct pp_instance *ppi, int txrx);
	int (*calib_enable)(struct pp_instance *ppi, int txrx);
	int (*calib_poll)(struct pp_instance *ppi, int txrx, uint32_t *delta);
	int (*calib_pattern_enable)(struct pp_instance *ppi,
				    unsigned int calibrationPeriod,
				    unsigned int calibrationPattern,
				    unsigned int calibrationPatternLen);
	int (*calib_pattern_disable)(struct pp_instance *ppi);
	int (*enable_timing_output)(struct pp_instance *ppi, int enable);
};


/* wr_servo interface */
int wr_servo_init(struct pp_instance *ppi);
void wr_servo_reset();
int wr_servo_man_adjust_phase(int phase);
void wr_servo_enable_tracking(int enable);
int wr_servo_got_sync(struct pp_instance *ppi, TimeInternal *t1,
		      TimeInternal *t2);
int wr_servo_got_delay(struct pp_instance *ppi, Integer32 cf);
int wr_servo_update(struct pp_instance *ppi);

struct wr_servo_state_t {
	char if_name[16];
	int state;
	int next_state;
	TimeInternal prev_t4;
	TimeInternal mu;		/* half of the RTT */
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

/* FIXME: what is the difference with the above? */
typedef struct{
	int valid;
	char slave_servo_state[32];
	char sync_source[32];
	int tracking_enabled;
	int64_t mu;
	int64_t delay_ms;
	int64_t delta_tx_m;
	int64_t delta_rx_m;
	int64_t delta_tx_s;
	int64_t delta_rx_s;
	int64_t fiber_asymmetry;
	int64_t total_asymmetry;
	int64_t cur_offset;
	int64_t cur_setpoint;
	int64_t cur_skew;
	int64_t update_count;
}  ptpdexp_sync_state_t ;

/* All data used as extension ppsi-wr must be put here */
struct wr_data_t {
	struct wr_servo_state_t servo_state;
};

#endif /* __WREXT_WR_API_H__ */
