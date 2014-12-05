/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */

/*
 * This is the startup thing for hosted environments. It
 * defines main and then calls the main loop.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/timex.h>

#include <minipc.h>
#include <hal_exports.h>

#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>
#include <libwr/shmem.h>

#if BUILT_WITH_WHITERABBIT
#  define WRSW_HAL_RETRIES 1000
#else
#  define WRSW_HAL_RETRIES 0
#endif

#define WRSW_HAL_TIMEOUT 2000000 /* us */

static struct wr_operations wrs_wr_operations = {
	.locking_enable = wrs_locking_enable,
	.locking_poll = wrs_locking_poll,
	.locking_disable = wrs_locking_disable,
	.enable_ptracker = wrs_enable_ptracker,

	.adjust_in_progress = wrs_adjust_in_progress,
	.adjust_counters = wrs_adjust_counters,
	.adjust_phase = wrs_adjust_phase,

	.read_calib_data = wrs_read_calibration_data,
	.calib_disable = wrs_calibrating_disable,
	.calib_enable = wrs_calibrating_enable,
	.calib_poll = wrs_calibrating_poll,
	.calib_pattern_enable = wrs_calibration_pattern_enable,
	.calib_pattern_disable = wrs_calibration_pattern_disable,

	.enable_timing_output = wrs_enable_timing_output,
};

struct minipc_ch *hal_ch;
struct minipc_ch *ppsi_ch;
struct hal_port_state *hal_ports;
int hal_nports;

/*
 * we need to call calloc, to reset all stuff that used to be static,
 * but we'd better have a simple prototype, compatilble with wrs_shm_alloc()
 */
void *local_malloc(void *headptr, size_t size)
{
	void *retval = malloc(size);

	if (retval)
		memset(retval, 0, size);
	return retval;
}

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	struct wr_dsport *wrp;
	unsigned long seed;
	struct timex t;
	int i, hal_retries;
	struct wrs_shm_head *hal_head, *ppsi_head;
	struct hal_shmem_header *h;
	void *(*alloc_fn)(void *headptr, size_t size) = local_malloc;

	setbuf(stdout, NULL);

	pp_printf("PPSi. Commit %s, built on " __DATE__ "\n",
		PPSI_VERSION);

	/* try connecting to HAL multiple times in case it's still not ready */
	hal_retries = WRSW_HAL_RETRIES;
	while (hal_retries) { /* may be never, if built without WR extension */
		hal_ch = minipc_client_create(WRSW_HAL_SERVER_ADDR,
					      MINIPC_FLAG_VERBOSE);
		if (hal_ch)
			break;
		hal_retries--;
		usleep(WRSW_HAL_TIMEOUT);
	}

	if (BUILT_WITH_WHITERABBIT && !hal_ch) {
		pp_printf("ppsi: could not connect to HAL RPC");
		exit(1);
	}

	if (BUILT_WITH_WHITERABBIT) {
		/* If we connected, we also know "for sure" shmem is there */
		hal_head = wrs_shm_get(wrs_shm_hal,"", WRS_SHM_READ);
		if (!hal_head || !hal_head->data_off) {
			pp_printf("ppsi: Can't connect with HAL "
				  "shared memory\n");
			exit(1);
		}
		h = (void *)hal_head + hal_head->data_off;
		hal_nports = h->nports;
		hal_ports = wrs_shm_follow(hal_head, h->ports);

		/* And create your own channel, until we move to shmem too */
		ppsi_ch = minipc_server_create("ptpd", 0);
		if (!ppsi_ch) { /* FIXME should we retry ? */
			pp_printf("ppsi: could not create minipc server");
			exit(1);
		}
		wrs_init_ipcserver(ppsi_ch);

		ppsi_head = wrs_shm_get(wrs_shm_ptp, "ppsi", WRS_SHM_WRITE);
		if (!ppsi_head) {
			fprintf(stderr, "Fatal: could not create shmem: %s\n",
				strerror(errno));
			exit(1);
		}
		alloc_fn = wrs_shm_alloc;
		ppsi_head->version = WRS_PPSI_SHMEM_VERSION;
	}

	ppg = alloc_fn(ppsi_head, sizeof(*ppg));
	ppg->defaultDS = alloc_fn(ppsi_head, sizeof(*ppg->defaultDS));
	ppg->currentDS = alloc_fn(ppsi_head, sizeof(*ppg->currentDS));
	ppg->parentDS =  alloc_fn(ppsi_head, sizeof(*ppg->parentDS));
	ppg->timePropertiesDS = alloc_fn(ppsi_head,
					 sizeof(*ppg->timePropertiesDS));
	ppg->servo = alloc_fn(ppsi_head, sizeof(*ppg->servo));
	ppg->rt_opts = &__pp_default_rt_opts;

	ppg->max_links = PP_MAX_LINKS;
	ppg->arch_data = malloc( sizeof(struct unix_arch_data));
	ppg->pp_instances = alloc_fn(ppsi_head,
				     ppg->max_links * sizeof(*ppi));

	if ((!ppg->arch_data) || (!ppg->pp_instances)) {
		fprintf(stderr, "ppsi: out of memory\n");
		exit(1);
	}

	/* Set offset here, so config parsing can override it */
	if (adjtimex(&t) >= 0) {
		int *p;
		/*
		 * Our WRS kernel has tai support, but our compiler does not.
		 * We are 32-bit only, and we know for sure that tai is
		 * exactly after stbcnt. It's a bad hack, but it works
		 */
		p = (int *)(&t.stbcnt) + 1;
		ppg->timePropertiesDS->currentUtcOffset = *p;
	}

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	/* If no item has been parsed, provide a default file or string */
	if (ppg->cfg.cfg_items == 0)
		pp_config_file(ppg, 0, "/wr/etc/ppsi.conf");
	if (ppg->cfg.cfg_items == 0)
		pp_config_file(ppg, 0, PP_DEFAULT_CONFIGFILE);
	if (ppg->cfg.cfg_items == 0) {
		/* Default configuration for WR switch is all ports */
		char s[128];
		int i;

		for (i = 0; i < 18; i++) {
			sprintf(s, "port wr%i; iface wr%i; proto raw;"
				"extension whiterabbit; role auto", i, i);
			pp_config_string(ppg, s);
		}
	}
	for (i = 0; i < ppg->nlinks; i++) {

		ppi = INST(ppg, i);
		NP(ppi)->ch[PP_NP_EVT].fd = -1;
		NP(ppi)->ch[PP_NP_GEN].fd = -1;

		ppi->glbs = ppg;
		ppi->iface_name = ppi->cfg.iface_name;
		ppi->port_name = ppi->cfg.port_name;
		ppi->portDS = calloc(1, sizeof(*ppi->portDS));
		if (ppi->portDS)
			ppi->portDS->ext_dsport =
				calloc(1, sizeof(struct wr_dsport));
		if (!ppi->portDS || !ppi->portDS->ext_dsport) {
			fprintf(stderr, "ppsi: out of memory\n");
			exit(1);
		}
		wrp = WR_DSPOR(ppi); /* just allocated above */
		wrp->ops = &wrs_wr_operations;

		/* The following default names depend on TIME= at build time */
		ppi->n_ops = &DEFAULT_NET_OPS;
		ppi->t_ops = &DEFAULT_TIME_OPS;

		ppi->__tx_buffer = malloc(PP_MAX_FRAME_LENGTH);
		ppi->__rx_buffer = malloc(PP_MAX_FRAME_LENGTH);

		if (!ppi->__tx_buffer || !ppi->__rx_buffer) {
			fprintf(stderr, "ppsi: out of memory\n");
			exit(1);
		}
	}

	pp_init_globals(ppg, &__pp_default_rt_opts);

	seed = time(NULL);
	if (getenv("PPSI_DROP_SEED"))
		seed = atoi(getenv("PPSI_DROP_SEED"));
	ppsi_drop_init(ppg, seed);

	wrs_main_loop(ppg);
	return 0; /* never reached */
}
