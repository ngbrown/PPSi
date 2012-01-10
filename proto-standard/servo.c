/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <pptp/pptp.h>


void pp_init_clock(struct pp_instance *ppi)
{
	/* TODO servo implement function*/
}

void pp_update_offset(struct pp_instance *ppi, TimeInternal *send_time,
		      TimeInternal *recv_time,
		      TimeInternal *correctionField)
			/* FIXME: offset_from_master_filter: put it in ppi */
{
	/* TODO servo implement function*/
}

void pp_update_clock(struct pp_instance *ppi)
{
	/* TODO servo implement function*/
}

