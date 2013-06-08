/*
 * Alessandro Rubini for CERN, 2011 -- public domain
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

#include <minipc.h>
#include <hal_exports.h>

#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>
#include "../proto-ext-whiterabbit/wr-api.h"

CONST_VERBOSITY int pp_diag_verbosity = 0;

/* FIXME: make MAX_LINKS and conf_path definable at compile time */
#define MAX_LINKS 32
#define CONF_PATH "/etc/ppsi.conf"

struct minipc_ch *hal_ch;
struct minipc_ch *ppsi_ch;

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	int i = 0, ret;
	struct stat conf_fs;
	char *conf_buf;
	int conf_fd;
	int conf_len = 0;

	setbuf(stdout, NULL);

	hal_ch = minipc_client_create(WRSW_HAL_SERVER_ADDR, MINIPC_FLAG_VERBOSE);
	if (!hal_ch) /* FIXME should we retry with minipc_client_create? */
		pp_printf("Fatal: could not connect to HAL");

	ppsi_ch = minipc_server_create("ptpd", 0);
	if (!ppsi_ch) /* FIXME should we retry with minipc_server_create? */
		pp_printf("Fatal: could not create minipc server");

	wrs_init_ipcserver(ppsi_ch);

	/* We are hosted, so we can allocate */
	ppg = calloc(1, sizeof(*ppg));
	if (!ppg)
		exit(__LINE__);

	ppg->max_links = MAX_LINKS;
	ppg->links = calloc(ppg->max_links, sizeof(struct pp_link));

	conf_fd = open(CONF_PATH, O_RDONLY);

	if ((stat(CONF_PATH, &conf_fs) < 0) ||
	    (conf_fd < 0)) {
		pp_printf("Warning: could not open %s, default to one-link built-in "
					"config\n", CONF_PATH);
		conf_buf = "link 0\niface eth0";
		conf_len = strlen(conf_buf);
	}
	else {
		int r = 0, next_r;
		conf_buf = calloc(1, conf_fs.st_size + 1);

		do {
			next_r = conf_fs.st_size - r;
			r = read(conf_fd, &conf_buf[conf_len], next_r);
			if (r <= 0)
				break;
			conf_len = strlen(conf_buf);
		} while (conf_len < conf_fs.st_size);

		close(conf_fd);
	}

	ppg->rt_opts = &default_rt_opts;

	if ((ret = pp_parse_conf(ppg, conf_buf, conf_len)) < 0) {
		pp_printf("Fatal: Error in %s file at line %d\n", CONF_PATH, -ret);
		exit(__LINE__);
	}

	ppg->defaultDS = calloc(1, sizeof(*ppg->defaultDS));
	ppg->currentDS = calloc(1, sizeof(*ppg->currentDS));
	ppg->parentDS = calloc(1, sizeof(*ppg->parentDS));
	ppg->timePropertiesDS = calloc(1, sizeof(*ppg->timePropertiesDS));
	ppg->arch_data = calloc(1, sizeof(struct unix_arch_data));
	ppg->pp_instances = calloc(ppg->nlinks, sizeof(struct pp_instance));

	if ((!ppg->defaultDS) || (!ppg->currentDS) || (!ppg->parentDS)
		|| (!ppg->timePropertiesDS) || (!ppg->arch_data)
		|| (!ppg->pp_instances))
		exit(__LINE__);

	ppg->servo = calloc(1, sizeof(*ppg->servo));

	for (; i < ppg->nlinks; i++) {

		struct pp_link *lnk = &ppg->links[i];

		ppi = &ppg->pp_instances[i];
		ppi->glbs = ppg;
		ppi->iface_name = lnk->iface_name;
		ppi->ethernet_mode = (lnk->proto == 0) ? 1 : 0;
		if (lnk->role == 1) {
			ppi->master_only = 1;
			ppi->slave_only = 0;
		}
		else if (lnk->role == 2) {
			ppi->master_only = 0;
			ppi->slave_only = 1;
		}

		ppi->portDS = calloc(1, sizeof(*ppi->portDS));
		if (!ppi->portDS)
			exit(__LINE__);

		ppi->portDS->ext_dsport = calloc(1, sizeof(struct wr_dsport));
		if (!ppi->portDS->ext_dsport)
			exit(__LINE__);

		/* The following default names depend on TIME= at build time */
		ppi->n_ops = &DEFAULT_NET_OPS;
		ppi->t_ops = &DEFAULT_TIME_OPS;

	}

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	pp_open_globals(ppg);

	wrs_main_loop(ppg);
	return 0; /* never reached */
}
