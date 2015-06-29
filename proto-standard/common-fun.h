/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#ifndef __COMMON_FUN_H
#define __COMMON_FUN_H

#include <ppsi/ppsi.h>

/* Contains all functions common to more than one state */

/* returns -1 in case of error, see below */
int st_com_execute_slave(struct pp_instance *ppi);

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
		return PP_SEND_ERROR;
	}
	/* FIXME: diagnosticst should be looped back in the send method */
	pp_diag(ppi, frames, 1, "SENT %02d bytes at %d.%09d (%s)\n", msglen,
		(int)(ppi->last_snt_time.seconds),
		(int)(ppi->last_snt_time.nanoseconds),
		pp_msg_names[msgtype]);
	if (chtype == PP_NP_EVT && ppi->last_snt_time.correct == 0)
		return PP_SEND_NO_STAMP;

	/* count sent packets */
	ppi->ptp_tx_count++;

	return 0;
}

/* Count successfully received PTP packets */
static inline int __recv_and_count(struct pp_instance *ppi, void *pkt, int len,
		   TimeInternal *t)
{
	int ret;
	ret = ppi->n_ops->recv(ppi, pkt, len, t);
	if (ret >= 0)
		ppi->ptp_rx_count++;
	return ret;
}

#endif /* __COMMON_FUN_H */
