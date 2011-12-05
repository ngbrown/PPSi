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

#include <pproto/pproto.h>
#include <pproto/diag.h>
#include "posix.h"

int main(int argc, char **argv)
{
	struct pp_instance *ppi;
	char *ifname;

	/*
	 * Here, we may fork or whatever for each interface.
	 * To keep things simple, just run one thing for one interface.
	 */
	ifname = getenv("PPROTO_IF");
	if (!ifname) ifname = "eth0";

	/* We are hosted, so we can allocate */
	ppi = calloc(1, sizeof(*ppi));
	if (!ppi)
		exit(__LINE__);

	if (posix_open_ch(ppi, ifname)) {
		pp_diag_fatal(ppi, "open_ch", strerror(errno));
	}
	pp_open_instance(ppi, NULL);

	pp_parse_cmdline(ppi,argc,argv);

	posix_main_loop(ppi);
	return 0; /* never reached */
}
