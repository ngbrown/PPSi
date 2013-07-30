/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

int pp_pre_master(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	/*
	 * No need of PRE_MASTER state because of only ordinary clock
	 * implementation.
	 */
	return 0;
}
