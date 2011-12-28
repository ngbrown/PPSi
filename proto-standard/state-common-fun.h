/*
 * FIXME: header
 */

#include <pproto/pproto.h>

/* Contains all functions common to more than one state */

void st_com_execute_slave(struct pp_instance *ppi);

void st_com_restart_annrec_timer(struct pp_instance *ppi);

int st_com_check_record_update(struct pp_instance *ppi);

void st_com_add_foreign(unsigned char *buf, MsgHeader *header,
			struct pp_instance *ppi);

int st_com_slave_handle_announce(unsigned char *buf, int len,
				struct pp_instance *ppi);
