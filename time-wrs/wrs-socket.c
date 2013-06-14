/*
 * Aurelio Colosimo for CERN, 2013 -- public domain
 */

#include <ppsi/ppsi.h>

static int wrs_net_init(struct pp_instance *ppi)
{
	/* FIXME */
	return unix_net_ops.init(ppi);
}

static int wrs_net_exit(struct pp_instance *ppi)
{
	/* FIXME */
	return unix_net_ops.exit(ppi);
}

static int wrs_net_recv(struct pp_instance *ppi, void *pkt, int len,
			TimeInternal *t)
{
	/* FIXME */
	return unix_net_ops.recv(ppi, pkt, len, t);
}

static int wrs_net_send(struct pp_instance *ppi, void *pkt, int len,
			TimeInternal *t, int chtype, int use_pdelay_addr)
{
	/* FIXME */
	return unix_net_ops.send(ppi, pkt, len, t, chtype, use_pdelay_addr);
}

struct pp_network_operations wrs_net_ops = {
	.init = wrs_net_init,
	.exit = wrs_net_exit,
	.recv = wrs_net_recv,
	.send = wrs_net_send,
};
