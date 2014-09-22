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
	int64_t freq_ppb_real; // drift of the simulated hw clock
	int64_t freq_ppb_servo; // drift applied from servo to correct the hw
	// Future parameters can be added
};

/*
 * This structure holds the parameter representing the delays on the outgoing
 * link of every pp_instance. All the values are expressed in the *absolute*
 * timescale, which is represented by the master time.
 */
struct pp_sim_net_delay {
	unsigned int t_prop_ns; // propagation delay on outgoing link
	uint64_t jit_ns; // jitter in nsec on outgoing link
	uint64_t last_outgoing_jit_ns;
};

/*
 * This structure represets a pending packet. which_ppi is the destination ppi,
 * chtype is the channel and delay_ns is the time that should pass from when the
 * packet is sent until it will be received from the destination.
 * Every time a packet is sent a structure like this is filled and stored in the
 * ppg->arch_data so that we always know which is the first packet that has to
 * be received and when.
 */
struct sim_pending_pkt {
	int64_t delay_ns;
	int which_ppi;
	int chtype;
};
/*
 * Structure holding an array of pending packets. 64 does not have a special
 * meaning. They could be less if you look inside the ptp specification, but
 * I put 64 just to be sure.
 * The aim of this structure is to store information on flying packets and when
 * the'll be received, because the standard state machine timeouts are not
 * enough for this. Infact the main loop need to know if there are some packets
 * arriving and when, otherwise it will not know how much fast forwarding is
 * needed. If you fast forward based on timeouts they will expire before any
 * packet has arrived and the state machine will do nothing.
 */
struct sim_ppg_arch_data {
	int n_pending;
	struct sim_pending_pkt pending[64];
	int64_t sim_iter_max;
	int64_t sim_iter_n;
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
	struct pp_sim_net_delay n_delay;
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

static inline struct pp_instance *pp_sim_get_master(struct pp_globals *ppg)
{
	return INST(ppg, SIM_MASTER);
}

static inline struct pp_instance *pp_sim_get_slave(struct pp_globals *ppg)
{
	return INST(ppg, SIM_SLAVE);
}

static inline int pp_sim_is_master(struct pp_instance *ppi)
{
	return ((ppi - ppi->glbs->pp_instances) == SIM_MASTER);
}

static inline int pp_sim_is_slave(struct pp_instance *ppi)
{
	return ((ppi - ppi->glbs->pp_instances) == SIM_SLAVE);
}

extern int sim_fast_forward_ns(struct pp_globals *ppg, int64_t ff_ns);
extern int sim_set_global_DS(struct pp_instance *ppi);
extern void sim_main_loop(struct pp_globals *ppg);
