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


#include <ppsi/ppsi.h>
#include "ppsi-unix.h"

CONST_VERBOSITY int pp_diag_verbosity = 0;

/* FIXME: make MAX_LINKS and conf_path definable at compile time */
#define MAX_LINKS 32
#define CONF_PATH "/etc/ppsi.conf"

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	int i, ret;
	struct stat conf_fs;
	char *conf_buf;
	FILE *f;
	int conf_len = 0;

	setbuf(stdout, NULL);

	/* We are hosted, so we can allocate */
	ppg = calloc(1, sizeof(*ppg));
	if (!ppg)
		exit(__LINE__);

	ppg->max_links = MAX_LINKS;
	ppg->links = calloc(ppg->max_links, sizeof(struct pp_link));

	f = fopen(CONF_PATH, "r");

	if (!f || stat(CONF_PATH, &conf_fs) < 0) {
		fprintf(stderr, "%s: Warning: %s: %s -- default to eth0 only\n",
			argv[0], CONF_PATH, strerror(errno));
		conf_buf = "link 0\niface eth0\n";
		conf_len = strlen(conf_buf);
	} else {
		/* The parser needs a trailing newline, add one to be sure */
		conf_len = conf_fs.st_size + 1;
		conf_buf = calloc(1, conf_len + 1); /* trailing \0 */
		i = fread(conf_buf, 1, conf_len, f);
		if (i > 0) conf_buf[i] = '\n';
		fclose(f);
	}

	ppg->rt_opts = &default_rt_opts;

	if ((ret = pp_parse_conf(ppg, conf_buf, conf_len)) < 0) {
		fprintf(stderr, "%s: %s:%d: parse error\n", argv[0],
			CONF_PATH, -ret);
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

	for (i = 0; i < ppg->nlinks; i++) {

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

		/* FIXME set ppi ext enable as defined in its pp_link */

		ppi->portDS = calloc(1, sizeof(*ppi->portDS));

		/* The following default names depend on TIME= at build time */
		ppi->n_ops = &DEFAULT_NET_OPS;
		ppi->t_ops = &DEFAULT_TIME_OPS;

		if (!ppi->portDS)
			exit(__LINE__);
	}

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	pp_open_globals(ppg);

	unix_main_loop(ppg);
	return 0; /* never reached */
}
