/*
 * FIXME: header
 */
#include <pproto/pproto.h>
#include <dep/dep.h>

/*
 * Initializes network and other stuff
 */

int pp_initializing(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	
	/* FIXME 
	if (pp_net_init(ppi) < 0)
		goto failure;
	*/
	/* TODO initialize other stuff */
	/* TODO copy &ppi->rt_opts to &ppi->ppc. Do it here, no real need for a
			initData function	*/
	/* TODO ARCH initTimer(); define a "timer" abstract object, define it for
			all the archs and initialize it here */
	/* TODO CHECK initClock(rtOpts, ptpClock); check what is it for */
	/* TODO CHECK m1(ptpClock);*/
	/* TODO initializes header of a packet msgPackHeader(ptpClock->msgObuf,
			ptpClock); */

	if (1) /* FIXME: implement above */
		goto failure;

	ppi->next_state = PPS_LISTENING;
	return 0;

failure:
	pp_printf("Failed to initialize network\n");
	ppi->next_state = PPS_FAULTY;
	return 0;
}
