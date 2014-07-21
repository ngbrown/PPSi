/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Authors: Alessandro Rubini
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */
#include <ppsi/ppsi.h>

/* Always rx before tx (alphabetic) */
struct ppsi_drop {
	unsigned long rxrand, txrand;
	int rxrate, txrate;
};

static struct ppsi_drop d;

static inline unsigned long drop_this(unsigned long *p, int rate)
{
	/* hash the value, according to the TYPE_0 rule of glibc */
	*p = ((*p * 1103515245) + 12345) & 0x7fffffff;

	return (*p % 1000) < rate;
}

void ppsi_drop_init(struct pp_globals *ppg, unsigned long seed)
{
	/* lazily, avoid passing globals every time */
	d.rxrand = seed;
	d.txrand = seed + 1;
	d.rxrate = ppg->rxdrop;
	d.txrate = ppg->txdrop;
}

int ppsi_drop_rx(void)
{
	if (d.rxrate)
		return drop_this(&d.rxrand, d.rxrate);
	return 0;
}

int ppsi_drop_tx(void)
{
	if (d.txrate)
		return drop_this(&d.txrand, d.txrate);
	return 0;
}

