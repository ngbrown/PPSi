/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/*
 * These are the functions provided by the various posix files
 */

struct posix_arch_data {
	struct timeval tv;
};

extern int posix_net_init(struct pp_instance *ppi);
extern int posix_net_check_pkt(struct pp_instance *ppi, int delay_ms);

extern int posix_open_ch(struct pp_instance *ppi, char *name);

extern int posix_recv_packet(struct pp_instance *ppi, void *pkt, int len,
			     TimeInternal *t);
extern int posix_send_packet(struct pp_instance *ppi, void *pkt, int len,
			     int chtype);

extern void posix_main_loop(struct pp_instance *ppi);
