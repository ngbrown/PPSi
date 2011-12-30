/*
 * FIXME: header
 */
#include <pproto/pproto.h>

int pp_pre_master(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	/*
	 * No need of PRE_MASTER state because of only ordinary clock
	 * implementation.
	 */
	return 0;
}
