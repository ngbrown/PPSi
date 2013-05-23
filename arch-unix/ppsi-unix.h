/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */

/*
 * These are the functions provided by the various unix files
 */

#define POSIX_ARCH(ppg) ((struct unix_arch_data *)(ppg->arch_data))
struct unix_arch_data {
	struct timeval tv;
};

extern int unix_net_check_pkt(struct pp_globals *ppg, int delay_ms);

extern void unix_main_loop(struct pp_globals *ppg);
