/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 * Aurelio Colosimo for CERN, 2012 -- GNU LGPL v2.1 or later
 */
#ifndef __WRPC_H
#define __WRPC_H

#include <ppsi/ppsi.h>
#include <hw/memlayout.h>

/* This part is exactly wrpc-sw::wrc_ptp.h */
#define WRC_MODE_UNKNOWN 0
#define WRC_MODE_GM 1
#define WRC_MODE_MASTER 2
#define WRC_MODE_SLAVE 3
extern int ptp_mode;

int wrc_ptp_init(void);
int wrc_ptp_set_mode(int mode);
int wrc_ptp_get_mode(void);
int wrc_ptp_start(void);
int wrc_ptp_stop(void);
int wrc_ptp_update(void);
/* End of wrc-ptp.h */

extern struct pp_network_operations wrpc_net_ops;
extern struct pp_time_operations wrpc_time_ops;

/* other network stuff, bah.... */

struct wrpc_ethhdr {
	unsigned char	h_dest[6];
	unsigned char	h_source[6];
	uint16_t	h_proto;
} __attribute__((packed));

#endif /* __WRPC_H */
