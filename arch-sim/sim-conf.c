/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Pietro Fezzardi (pietrofezzardi@gmail.com)
 *
 * Released according to GNU LGPL, version 2.1 or any later
 */

#include <ppsi/ppsi.h>
#include "ppsi-sim.h"

static int f_ppm_real(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	struct pp_instance *ppi_slave;

	/* master clock is supposed to be perfect. parameters about ppm are
	 * modifiable only for slave ppi */
	ppi_slave = pp_sim_get_slave(ppg);
	SIM_PPI_ARCH(ppi_slave)->time.freq_ppm_real = arg->i * 1000;
	return 0;
}

static int f_ppm_servo(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	struct pp_instance *ppi_slave;

	/* master clock is supposed to be perfect. parameters about ppm are
	 * modifiable only for slave ppi */
	ppi_slave = pp_sim_get_slave(ppg);
	SIM_PPI_ARCH(ppi_slave)->time.freq_ppm_servo = arg->i * 1000;
	return 0;
}

static int f_ofm(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	struct pp_sim_time_instance *t_master, *t_slave;

	t_master = &SIM_PPI_ARCH(pp_sim_get_master(ppg))->time;
	t_slave = &SIM_PPI_ARCH(pp_sim_get_slave(ppg))->time;
	t_slave->current_ns = t_master->current_ns + arg->ts.tv_nsec +
				arg->ts.tv_sec * (long long)PP_NSEC_PER_SEC;

	return 0;
}

static int f_init_time(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	struct pp_sim_time_instance *t_inst;

	t_inst = &SIM_PPI_ARCH(pp_sim_get_master(ppg))->time;
	t_inst->current_ns = arg->ts.tv_nsec +
				arg->ts.tv_sec * (long long)PP_NSEC_PER_SEC;

	return 0;
}

struct pp_argline pp_arch_arglines[] = {
	{f_ppm_real,	"sim_ppm_real",		ARG_INT},
	{f_ppm_servo,	"sim_init_ppm_servo",	ARG_INT},
	{f_ofm,		"sim_init_ofm",		ARG_TIME},
	{f_init_time,	"sim_init_master_time",	ARG_TIME},
	{}
};


