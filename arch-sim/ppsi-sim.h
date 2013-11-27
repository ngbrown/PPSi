/*
 * Author: Pietro Fezzardi (pietrofezzardi@gmail.com)
 *
 * Released to the public domain
 */

/*
 * This structure holds the lowest timeout of all the state machines in the
 * ppg, namely the master and slave state machines in the simulator. All the
 * future configuration parameters needed from both master and slave ppi
 * can be added here.
 */
struct sim_ppg_arch_data {
	struct timeval tv;
};

static inline struct sim_ppg_arch_data *SIM_PPG_ARCH(struct pp_globals *ppg)
{
	return (struct sim_ppg_arch_data *)(ppg->arch_data);
}

extern void sim_main_loop(struct pp_globals *ppg);
