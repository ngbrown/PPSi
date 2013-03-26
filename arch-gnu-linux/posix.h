/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/*
 * These are the functions provided by the various posix files
 */

#define POSIX_ARCH(ppi) ((struct posix_arch_data *)(ppi->arch_data))
struct posix_arch_data {
	struct timeval tv;
};

extern int posix_net_check_pkt(struct pp_instance *ppi, int delay_ms);

extern void posix_main_loop(struct pp_globals *ppg);

extern struct pp_network_operations posix_net_ops;
extern struct pp_time_operations posix_time_ops;
