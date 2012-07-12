/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <ppsi/ppsi.h>

/* Contains all functions common to more than one state */

/* The following macros are used both by msg.c and by wr-msg.c */
#define MSG_SEND_AND_RET_VARLEN(x,y,z,w)\
	if (pp_send_packet(ppi, ppi->buf_out, w,\
		&ppi->last_snt_time, PP_NP_##y , z) < w) {\
		PP_PRINTF("%s(%d) Message can't be sent -> FAULTY state!\n",\
			pp_msg_names[PPM_##x], PPM_##x);\
		return -1;\
	}\
	PP_PRINTF("SENT %02d %d.%d %s \n", w,\
		ppi->last_snt_time.seconds,\
		ppi->last_snt_time.nanoseconds,pp_msg_names[PPM_##x]);\
	ppi->sent_seq_id[PPM_## x]++;\
	return 0;

#define MSG_SEND_AND_RET(x,y,z)\
	MSG_SEND_AND_RET_VARLEN(x,y,z,PP_## x ##_LENGTH)

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

int st_com_handle_pdelay_req(struct pp_instance *ppi, unsigned char *buf,
			     int len);
