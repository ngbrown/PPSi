/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/*
 * This is the startup thing for hosted environments. It
 * defines main and then calls the main loop.
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define CONFIG_PPSI_RUNTIME_VERBOSITY 1
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "posix.h"

CONST_VERBOSITY int pp_diag_verbosity = 0;

int main(int argc, char **argv)
{
	struct pp_instance *ppi;
	char *ifname;

	/*
	 * Here, we may fork or whatever for each interface.
	 * To keep things simple, just run one thing for one interface.
	 */
	ifname = getenv("PPROTO_IF");
	if (!ifname)
		ifname = "eth0";

	/* We are hosted, so we can allocate */
	ppi = calloc(1, sizeof(*ppi));
	if (!ppi)
		exit(__LINE__);

	ppi->sent_seq_id = calloc(16, sizeof(*ppi->sent_seq_id));
	ppi->defaultDS = calloc(1, sizeof(*ppi->defaultDS));
	ppi->currentDS = calloc(1, sizeof(*ppi->currentDS));
	ppi->parentDS = calloc(1, sizeof(*ppi->parentDS));
	ppi->portDS = calloc(1, sizeof(*ppi->portDS));
	ppi->timePropertiesDS = calloc(1, sizeof(*ppi->timePropertiesDS));
	ppi->net_path = calloc(1, sizeof(*ppi->net_path));
	ppi->servo = calloc(1, sizeof(*ppi->servo));
	ppi->frgn_master = calloc(1, sizeof(*ppi->frgn_master));
	ppi->arch_data = calloc(1, sizeof(struct posix_arch_data));

	if ((!ppi->defaultDS) || (!ppi->currentDS) || (!ppi->parentDS)
	    || (!ppi->portDS) || (!ppi->timePropertiesDS) || (!ppi->sent_seq_id)
	    || (!ppi->net_path) || (!ppi->frgn_master) || (!ppi->arch_data)
	   )
		exit(__LINE__);

	pp_open_instance(ppi, NULL);

	OPTS(ppi)->iface_name = ifname;

	if (pp_parse_cmdline(ppi, argc, argv) != 0)
		return -1;

	posix_main_loop(ppi);
	return 0; /* never reached */
}
