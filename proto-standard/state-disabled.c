/*
 * FIXME: header
 */
#include <pproto/pproto.h>

int pp_disabled(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	/* nothing to do */
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;
	return 0;
}
