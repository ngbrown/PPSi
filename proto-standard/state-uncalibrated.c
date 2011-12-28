/*
 * FIXME: header
 */
#include <pproto/pproto.h>
#include "state-common-fun.h"

int pp_uncalibrated(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;

	switch (ppi->msg_tmp_header.messageType) {

	case PPM_ANNOUNCE:
		e = st_com_slave_handle_announce(pkt, plen, ppi);
		break;

	case PPM_SYNC:
		/* TODO */
		break;

	case PPM_FOLLOW_UP:
		/* TODO */
		break;

	default:
		/* disreguard, nothing to do */
		break;
	}

	if (e == 0)
		st_com_execute_slave(ppi);
	else
		ppi->next_state = PPS_FAULTY;

	ppi->next_delay = PP_DEFAULT_NEXT_DELAY_MS;

	return 0;
}
