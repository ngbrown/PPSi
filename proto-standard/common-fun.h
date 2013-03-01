/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#ifndef __COMMON_FUN_H
#define __COMMON_FUN_H

#include <ppsi/ppsi.h>

/* Contains all functions common to more than one state */

/* returns -1 in case of error, see below */
int st_com_execute_slave(struct pp_instance *ppi, int check_delayreq);

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

#define MSG_SEND_AND_RET(_name, _chan, _size) \
	if (pp_net_ops.send(ppi, ppi->buf_out, _size,\
		&ppi->last_snt_time, PP_NP_ ## _chan , 0) < _size) { \
		if (pp_verbose_frames) \
			PP_PRINTF("%s(%d) Message can't be sent -> FAULTY\n", \
			pp_msg_names[PPM_ ## _name], PPM_ ## _name); \
		return -1; \
	} \
	if (pp_verbose_frames) \
		PP_VPRINTF("SENT %02d %d.%09d %s\n", _size, \
			ppi->last_snt_time.seconds, \
			ppi->last_snt_time.nanoseconds, \
			pp_msg_names[PPM_ ## _name]); \
	ppi->sent_seq_id[PPM_ ## _name]++; \
	return 0;

#endif /* __COMMON_FUN_H */
