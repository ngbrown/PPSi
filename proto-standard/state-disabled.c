/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

int pp_disabled(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	/* nothing to do */
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return 0;
}
