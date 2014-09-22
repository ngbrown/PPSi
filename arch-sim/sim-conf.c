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
	SIM_PPI_ARCH(ppi_slave)->time.freq_ppb_real = arg->i * 1000;
	return 0;
}

static int f_ppm_servo(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	struct pp_instance *ppi_slave;

	/* master clock is supposed to be perfect. parameters about ppm are
	 * modifiable only for slave ppi */
	ppi_slave = pp_sim_get_slave(ppg);
	SIM_PPI_ARCH(ppi_slave)->time.freq_ppb_servo = arg->i * 1000;
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

static int f_fwd_t_prop(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	struct sim_ppi_arch_data *data;
	data = SIM_PPI_ARCH(pp_sim_get_master(ppg));
	data->n_delay.t_prop_ns = arg->i;
	return 0;
}

static int f_bckwd_t_prop(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	struct sim_ppi_arch_data *data;
	data = SIM_PPI_ARCH(pp_sim_get_slave(ppg));
	data->n_delay.t_prop_ns = arg->i;
	return 0;
}

static int f_t_prop(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	f_fwd_t_prop(lineno, ppg, arg);
	f_bckwd_t_prop(lineno, ppg, arg);
	return 0;
}

static int f_fwd_jit(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	struct sim_ppi_arch_data *data;
	data = SIM_PPI_ARCH(pp_sim_get_master(ppg));
	data->n_delay.jit_ns = arg->i;
	return 0;
}


static int f_bckwd_jit(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	struct sim_ppi_arch_data *data;
	data = SIM_PPI_ARCH(pp_sim_get_slave(ppg));
	data->n_delay.jit_ns = arg->i;
	return 0;
}

static int f_jit(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	f_fwd_jit(lineno, ppg, arg);
	f_bckwd_jit(lineno, ppg, arg);
	return 0;
}

static int f_iter(int lineno, struct pp_globals *ppg,
			union pp_cfg_arg *arg)
{
	SIM_PPG_ARCH(ppg)->sim_iter_max = arg->i;
	return 0;
}

struct pp_argline pp_arch_arglines[] = {
	{f_ppm_real,	"sim_ppm_real",		ARG_INT},
	{f_ppm_servo,	"sim_init_ppm_servo",	ARG_INT},
	{f_ofm,		"sim_init_ofm",		ARG_TIME},
	{f_init_time,	"sim_init_master_time",	ARG_TIME},
	{f_t_prop,	"sim_t_prop_ns",	ARG_INT},
	{f_fwd_t_prop,	"sim_fwd_t_prop_ns",	ARG_INT},
	{f_bckwd_t_prop,"sim_bckwd_t_prop_ns",	ARG_INT},
	{f_jit,		"sim_jit_ns",		ARG_INT},
	{f_fwd_jit,	"sim_fwd_jit_ns",	ARG_INT},
	{f_bckwd_jit,	"sim_bckwd_jit_ns",	ARG_INT},
	{f_iter,	"sim_iter_max",		ARG_TIME},
	{}
};


