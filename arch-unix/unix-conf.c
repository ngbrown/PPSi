/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to GNU LGPL, version 2.1 or any later
 */

#include <ppsi/ppsi.h>


static int f_rxdrop(int lineno, struct pp_globals *ppg, union pp_cfg_arg *arg)
{
	ppg->rxdrop = arg->i;
	return 0;
}

static int f_txdrop(int lineno, struct pp_globals *ppg, union pp_cfg_arg *arg)
{
	ppg->txdrop = arg->i;
	return 0;
}

struct pp_argline pp_arch_arglines[] = {
	{ f_rxdrop,	"rx-drop",	ARG_INT},
	{ f_txdrop,	"tx-drop",	ARG_INT},
	{}
};
