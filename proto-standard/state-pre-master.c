/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <pptp/pptp.h>

int pp_pre_master(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	/*
	 * No need of PRE_MASTER state because of only ordinary clock
	 * implementation.
	 */
	return 0;
}
