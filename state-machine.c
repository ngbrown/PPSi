/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <pproto/pproto.h>
#include <pproto/diag.h>

/*
 * This is the state machine code. i.e. the extension-independent
 * function that runs the machine. Errors are managed and reported
 * here (based on the diag module). The returned value is the time
 * in milliseconds to wait before reentering the state machine.
 * the normal protocol. If an extended protocol is used, the table used
 * is that of the extension, otherwise the one in state-table-default.c
 */

int pp_state_machine(struct pp_instance *ppi, uint8_t *packet, int plen)
{
	struct pp_state_table_item *ip;
	int state, err;

	if (packet) {
		msg_unpack_header(packet, ppi);
	}

	state = ppi->state;

	/* a linear search is affordable up to a few dozen items */
	for (ip = pp_state_table; ip->state != PPS_END_OF_TABLE; ip++) {
		if (ip->state != state)
			continue;
		/* found: handle this state */
		ppi->next_state = state;
		ppi->next_delay = 0;
		ppi->is_new_state = 0;
		pp_diag_fsm(ppi, 0 /* enter */, plen);
		err = ip->f1(ppi, packet, plen);
		if (!err && ip->f2)
			err = ip->f2(ppi, packet, plen);
		if (err)
			pp_diag_error(ppi, err);
		pp_diag_fsm(ppi, 1 /* leave */, 0 /* unused */);

		/* done: accept next state and delay */
		if (ppi->state != ppi->next_state) {
			ppi->state = ppi->next_state;
			ppi->is_new_state = 1;
		}
		return ppi->next_delay;
	}
	/* Unknwon state, can't happen */
	pp_diag_error_str2(ppi, "Unknown state in FSM", "");
	return 10000; /* No way out. Repeat message every 10s */
}
