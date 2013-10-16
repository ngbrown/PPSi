/*
 * Author: Pietro Fezzardi (pietrofezzardi@gmail.com)
 *
 * Released to the public domain
 */

/*
 * These are the functions provided by the various sim files
 */

/* the arch_data is the same as unix */
#define POSIX_ARCH(ppg) ((struct unix_arch_data *)(ppg->arch_data))
struct unix_arch_data {
	struct timeval tv;
};

extern void sim_main_loop(struct pp_globals *ppg);
