/*
 * FIXME: header
 */
#include <pptp/pptp.h>
#include "state-common-fun.h"

int pp_uncalibrated(struct pp_instance *ppi, unsigned char *pkt, int plen)
{
	int e = 0;
	TimeInternal time; /* TODO: handle it, see handle(...) in protocol.c */

	switch (ppi->msg_tmp_header.messageType) {

	case PPM_ANNOUNCE:
		e = st_com_slave_handle_announce(pkt, plen, ppi);
		break;

	case PPM_SYNC:
		e = st_com_slave_handle_sync(pkt, plen, &time, ppi);
		break;

	case PPM_FOLLOW_UP:
		e = st_com_slave_handle_followup(pkt, plen, ppi);
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
