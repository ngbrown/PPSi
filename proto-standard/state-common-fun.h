/*
 * FIXME: header
 */

#include <pptp/pptp.h>

/* Contains all functions common to more than one state */

void st_com_execute_slave(struct pp_instance *ppi);

void st_com_restart_annrec_timer(struct pp_instance *ppi);

int st_com_check_record_update(struct pp_instance *ppi);

/*
void st_com_add_foreign(unsigned char *buf, MsgHeader *header,
			struct pp_instance *ppi);
*/

int st_com_slave_handle_announce(unsigned char *buf, int len,
				struct pp_instance *ppi);

int st_com_master_handle_announce(unsigned char *buf, int len,
				struct pp_instance *ppi);

int st_com_slave_handle_sync(unsigned char *buf, int len, TimeInternal *time,
				struct pp_instance *ppi);

int st_com_master_handle_sync(unsigned char *buf, int len, TimeInternal *time,
				struct pp_instance *ppi);

int st_com_slave_handle_followup(unsigned char *buf, int len,
				 struct pp_instance *ppi);

int st_com_handle_pdelay_req(unsigned char *buf, int len, TimeInternal *time,
				struct pp_instance *ppi);
