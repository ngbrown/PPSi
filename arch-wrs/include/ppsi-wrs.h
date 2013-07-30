/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */

/*
 * These are the functions provided by the various wrs files
 */

#include <minipc.h>

extern struct minipc_ch *hal_ch;
extern struct minipc_ch *ppsi_ch;

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

extern int unix_net_check_pkt(struct pp_globals *ppg, int delay_ms);

extern void wrs_main_loop(struct pp_globals *ppg);

extern void wrs_init_ipcserver(struct minipc_ch *ppsi_ch);
