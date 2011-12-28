/*
 * FIXME: header
 */
#include <pproto/pproto.h>
#include <dep/dep.h>

/*
 * Fault troubleshooting. Now only prints an error messages and comes back to
 * PTP_INITIALIZING state
 */

int pp_faulty(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	pp_printf("event FAULT_CLEARED\n");
	ppi->next_state = PPS_INITIALIZING;
	ppi->next_delay = PP_DEFAULT_NEXT_DELAY;
	return 0;
}
