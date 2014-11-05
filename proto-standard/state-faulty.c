/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

/*
 * Fault troubleshooting. Now only prints an error messages and comes back to
 * PTP_INITIALIZING state after a 4-seconds grace period
 */

int pp_faulty(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	if (ppi->is_new_state) {
		pp_timeout_set(ppi, PP_TO_FAULTY, 4000);
	}

	if (pp_timeout(ppi, PP_TO_FAULTY)) {
		ppi->next_state = PPS_INITIALIZING;
		return 0;
	}
	ppi->next_delay = pp_ms_to_timeout(ppi, PP_TO_FAULTY);
	return 0;
}
