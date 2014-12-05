/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */
#ifdef CONFIG_EXT_WR
#  define BUILT_WITH_WHITERABBIT 1
#else
#  define BUILT_WITH_WHITERABBIT 0
#endif

#define WRS_PPSI_SHMEM_VERSION 1

/*
 * These are the functions provided by the various wrs files
 */

#include <minipc.h>
#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>

extern struct minipc_ch *hal_ch;
extern struct minipc_ch *ppsi_ch;
extern struct hal_port_state *hal_ports;
extern int hal_nports;

static inline struct hal_port_state *pp_wrs_lookup_port(char *name)
{
	int i;

	for (i = 0; i < hal_nports; i++)
		if (hal_ports[i].in_use &&!strcmp(name, hal_ports[i].name))
                        return hal_ports + i;
	return NULL;
}

#define DEFAULT_TO 200000 /* ms */

/* FIXME return values, here copied from proto-ext-whiterabbit.
 * I do not include proto-ext-whiterabbit/wr-constants.h in order not to
 * have a dependency on ext when compiling wrs architecture. All the return
 * values mechanism of wrs hw should be reviewed in this src and in the
 * whole ppsi */

/* White Rabbit softpll status values */
#define WR_SPLL_OK	0
#define WR_SPLL_READY	1
#define WR_SPLL_ERROR	-1

/* White Rabbit calibration defines */
#define WR_HW_CALIB_TX	1
#define WR_HW_CALIB_RX	2
#define WR_HW_CALIB_OK	0
#define WR_HW_CALIB_READY	1
#define WR_HW_CALIB_ERROR	-1
#define WR_HW_CALIB_NOT_FOUND	-3

#define POSIX_ARCH(ppg) ((struct unix_arch_data *)(ppg->arch_data))
struct unix_arch_data {
	struct timeval tv;
};

extern void wrs_main_loop(struct pp_globals *ppg);

extern void wrs_init_ipcserver(struct minipc_ch *ppsi_ch);

/* wrs-calibration.c */
int wrs_read_calibration_data(struct pp_instance *ppi,
			      uint32_t *delta_tx, uint32_t *delta_rx,
			      int32_t *fix_alpha, int32_t *clock_period);
int wrs_calibrating_disable(struct pp_instance *ppi, int txrx);
int wrs_calibrating_enable(struct pp_instance *ppi, int txrx);
int wrs_calibrating_poll(struct pp_instance *ppi, int txrx, uint32_t *delta);
int wrs_calibration_pattern_enable(struct pp_instance *ppi,
				   unsigned int calib_period,
				   unsigned int calib_pattern,
				   unsigned int calib_pattern_len);
int wrs_calibration_pattern_disable(struct pp_instance *ppi);

/* wrs-time.c (some should moce to wrs-spll.c) */
int wrs_locking_enable(struct pp_instance *ppi);
int wrs_locking_poll(struct pp_instance *ppi, int grandmaster);
int wrs_locking_disable(struct pp_instance *ppi);
int wrs_enable_ptracker(struct pp_instance *ppi);
int wrs_adjust_in_progress(void);
int wrs_adjust_counters(int64_t adjust_sec, int32_t adjust_nsec);
int wrs_adjust_phase(int32_t phase_ps);
int wrs_enable_timing_output(struct pp_instance *ppi, int enable);
