/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#ifndef __COMMON_FUN_H
#define __COMMON_FUN_H

#include <ppsi/ppsi.h>

/* Contains all functions common to more than one state */

/* returns -1 in case of error, see below */
int st_com_execute_slave(struct pp_instance *ppi);

void st_com_restart_annrec_timer(struct pp_instance *ppi);

int st_com_check_record_update(struct pp_instance *ppi);

/* Each of the following "handle" functions" return 0 in case of correct
 * message, -1 in case the message contained in buf is not proper (e.g. size
 * is not the expected one
 */
int st_com_slave_handle_announce(struct pp_instance *ppi, unsigned char *buf,
				 int len);

int st_com_master_handle_announce(struct pp_instance *ppi, unsigned char *buf,
				  int len);

int st_com_slave_handle_sync(struct pp_instance *ppi, unsigned char *buf,
			     int len);

int st_com_master_handle_sync(struct pp_instance *ppi, unsigned char *buf,
			      int len);

int st_com_slave_handle_followup(struct pp_instance *ppi, unsigned char *buf,
				 int len);

static inline int __send_and_log(struct pp_instance *ppi, int msglen,
				 int msgtype, int chtype)
{
	if (ppi->n_ops->send(ppi, ppi->tx_frame, msglen + NP(ppi)->ptp_offset,
			    &ppi->last_snt_time, chtype, 0) < msglen) {
		pp_diag(ppi, frames, 1, "%s(%d) Message can't be sent\n",
			pp_msg_names[msgtype], msgtype);
		return -1;
	}
	/* FIXME: diagnosticst should be looped back in the send method */
	pp_diag(ppi, frames, 1, "SENT %02d bytes at %d.%09d (%s)\n", msglen,
			ppi->last_snt_time.seconds,
			ppi->last_snt_time.nanoseconds,
			pp_msg_names[msgtype]);
	return 0;
}

#endif /* __COMMON_FUN_H */
