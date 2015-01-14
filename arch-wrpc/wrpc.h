/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */
#ifndef __WRPC_H
#define __WRPC_H

#include <ppsi/ppsi.h>
#include <hw/memlayout.h>
#include <libwr/hal_shmem.h>

/* This part is exactly wrpc-sw::wrc_ptp.h */
#define WRC_MODE_UNKNOWN 0
#define WRC_MODE_GM 1
#define WRC_MODE_MASTER 2
#define WRC_MODE_SLAVE 3
extern int ptp_mode;

int wrc_ptp_init(void);
int wrc_ptp_set_mode(int mode);
int wrc_ptp_get_mode(void);
int wrc_ptp_start(void);
int wrc_ptp_stop(void);
int wrc_ptp_update(void);
/* End of wrc-ptp.h */

extern struct pp_network_operations wrpc_net_ops;
extern struct pp_time_operations wrpc_time_ops;

/* other network stuff, bah.... */

struct wrpc_ethhdr {
	unsigned char	h_dest[6];
	unsigned char	h_source[6];
	uint16_t	h_proto;
} __attribute__((packed));

/* wrpc-spll.c (some should move to time-wrpc/) */
int wrpc_spll_locking_enable(struct pp_instance *ppi);
int wrpc_spll_locking_poll(struct pp_instance *ppi, int grandmaster);
int wrpc_spll_locking_disable(struct pp_instance *ppi);
int wrpc_spll_enable_ptracker(struct pp_instance *ppi);
int wrpc_adjust_in_progress(void);
int wrpc_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec);
int wrpc_adjust_phase(int32_t phase_ps);
int wrpc_enable_timing_output(struct pp_instance *ppi, int enable);

/* wrpc-calibration.c */
int wrpc_read_calibration_data(struct pp_instance *ppi,
			       uint32_t *deltaTx, uint32_t *deltaRx,
			       int32_t *fix_alpha, int32_t *clock_period);
int wrpc_calibrating_disable(struct pp_instance *ppi, int txrx);
int wrpc_calibrating_enable(struct pp_instance *ppi, int txrx);
int wrpc_calibrating_poll(struct pp_instance *ppi, int txrx, uint32_t *delta);
int wrpc_calibration_pattern_enable(struct pp_instance *ppi,
				    unsigned int calibrationPeriod,
				    unsigned int calibrationPattern,
				    unsigned int calibrationPatternLen);
int wrpc_calibration_pattern_disable(struct pp_instance *ppi);
int wrpc_get_port_state(struct hal_port_state *port, const char *port_name);

#endif /* __WRPC_H */
