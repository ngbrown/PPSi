/*
 * Author: Pietro Fezzardi (pietrofezzardi@gmail.com)
 *
 * Released to the public domain
 */

/*
 * This structure represents a clock abstraction. You can use it to
 * have different pp_instances with different notions of time.
 * When you try to get the time the sim_get_time function is performing the
 * calculations needed to convert the raw data from the "real" clock to the
 * timescale of the simulated one.
 * When you set the time you're not actually setting it in the hardware but
 * only in the parameters in this structure.
 */
struct pp_sim_time_instance {
	int64_t current_ns; // with nsecs it's enough for ~300 years
	int64_t freq_ppm_real; // drift of the simulated hw clock
	int64_t freq_ppm_servo; // drift applied from servo to correct the hw
	// Future parameters can be added
};

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

/*
 * Structure holding parameters and informations restricted to just a single
 * instance, like timing informations. Infact in the simulator, even if both
 * master and slave are ppi inside the same ppg, they act like every one of
 * them had its own clock.
 * Some more data we need in every instance are Data Sets, runtime options,
 * servo and TimeProperties. We need them because, since we have two ppi
 * acting like two different machines, we have to use different copies of
 * these data for every one of them. So we just put the per-instance data
 * here and we will the pointers in ppg according to our needs every time
 * we will need to use them.
 * Even more stuff can be added if needed
 */
struct sim_ppi_arch_data {
	struct pp_sim_time_instance time;
	/* servo */
	struct pp_servo *servo;
	/* Runtime options */
	struct pp_runtime_opts *rt_opts;
	/* Data sets */
	DSDefault *defaultDS;
	DSCurrent *currentDS;
	DSParent *parentDS;
	DSTimeProperties *timePropertiesDS;
	/* other pp_instance, used in net ops */
	struct pp_instance *other_ppi;
};
/* symbolic names to address master and slave in ppg->pp_instances */
#define SIM_SLAVE	1
#define SIM_MASTER	0

static inline struct sim_ppi_arch_data *SIM_PPI_ARCH(struct pp_instance *ppi)
{
	return (struct sim_ppi_arch_data *)(ppi->arch_data);
}

extern int sim_set_global_DS(struct pp_instance *ppi);
extern void sim_main_loop(struct pp_globals *ppg);
