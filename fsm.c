/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <ppsi/ppsi.h>

unsigned long pp_global_flags; /* This is the only "global" file in ppsi */

static void pp_timed_printf(struct pp_instance *ppi, char *fmt, ...)
{
	va_list args;
	TimeInternal t;
	unsigned long oflags = pp_global_flags;

	/* temporarily set NOTIMELOG, as we'll print the time ourselves */
	pp_global_flags |= PP_FLAG_NOTIMELOG;
	ppi->t_ops->get(&t);
	pp_global_flags = oflags;

	pp_printf("%09d.%03d ", (int)t.seconds,
		  (int)t.nanoseconds / 1000000);
	va_start(args, fmt);
	pp_vprintf(fmt, args);
	va_end(args);
}

/*
 * Diagnostics about state machine, enter, leave, remain
 */
enum {
	STATE_ENTER,
	STATE_LOOP,
	STATE_LEAVE
};

static void pp_diag_fsm(struct pp_instance *ppi, char *name, int sequence,
			int plen)
{
	if (sequence == STATE_ENTER) {
		/* enter with or without a packet len */
		pp_timed_printf(ppi, "fsm: ENTER %s, packet len %i\n",
			  name, plen);
		return;
	}
	if (sequence == STATE_LOOP) {
		pp_timed_printf(ppi, "fsm: %s: reenter in %i ms\n", name,
				ppi->next_delay);
		return;
	}
	/* leave has one \n more, so different states are separate */
	pp_timed_printf(ppi, "fsm: LEAVE %s (next: %3i in %i ms)\n\n",
		name, ppi->next_state, ppi->next_delay);
}

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
	int state, err = 0;

	if (plen && pp_verbose_frames) {
		PP_VPRINTF("RECV %02d bytes at %d.%09d (type %x)\n", plen,
			   (int)ppi->last_rcv_time.seconds,
			   (int)ppi->last_rcv_time.nanoseconds,
			   packet[0] & 0xf);
	}

	/*
	 * Since all ptp frames have the same header, parse it now.
	 * In case of error continue without a frame, so the current
	 * ptp state can update ppi->next_delay and return a proper value
	 */
	if (plen) {
		if (plen >= PP_HEADER_LENGTH)
			err = msg_unpack_header(ppi, packet);
		else
			err = 1;
		if (err) {
			plen = 0;
			packet = NULL;
		}
	}

	state = ppi->state;

	/* a linear search is affordable up to a few dozen items */
	for (ip = pp_state_table; ip->state != PPS_END_OF_TABLE; ip++) {
		if (ip->state != state)
			continue;
		/* found: handle this state */
		ppi->next_state = state;
		ppi->next_delay = 0;
		if (pp_diag_verbosity && ppi->is_new_state)
			pp_diag_fsm(ppi, ip->name, STATE_ENTER, plen);
		err = ip->f1(ppi, packet, plen);
		if (err)
			pp_printf("fsm for %s: Error %i in %s\n",
				  OPTS(ppi)->iface_name, err, ip->name);

		/* done: if new state mark it, and enter it now (0 ms) */
		if (ppi->state != ppi->next_state) {
			ppi->state = ppi->next_state;
			ppi->is_new_state = 1;
			if (pp_diag_verbosity)
				pp_diag_fsm(ppi, ip->name, STATE_LEAVE, 0);
			return 0;
		}
		ppi->is_new_state = 0;
		if (pp_diag_verbosity)
			pp_diag_fsm(ppi, ip->name, STATE_LOOP, 0);
		return ppi->next_delay;
	}
	/* Unknwon state, can't happen */
	pp_printf("fsm: Unknown state for iface %s\n", OPTS(ppi)->iface_name);
	return 10000; /* No way out. Repeat message every 10s */
}
