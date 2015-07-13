

#include <ppsi/ppsi.h>
#include "../arch-win32/ppsi-win32.h"

/*
* Inits all the network stuff
*/

/* This function must be able to be called twice, and clean-up internally */
static int win32_net_init(struct pp_instance *ppi)
{
	if (ppi->proto == PPSI_PROTO_UDP) {
		if (ppi->nvlans) {
			/* If "proto udp" is set after setting vlans... */
			pp_printf("Error: can't use UDP with VLAN support\n");
			exit(1);
		}
		pp_diag(ppi, frames, 1, "win32_net_init UDP\n");


		return -1;
	}

	// Don't handle RAW, VLAN
	pp_diag(ppi, frames, 1, "win32_net_init not UDP\n");
	return -1;
}

/*
* Shutdown all the network stuff
*/
static int win32_net_exit(struct pp_instance *ppi)
{
	return -1;
}

/* Receive and send is *not* so trivial */
static int win32_net_recv(struct pp_instance *ppi, void *pkt, int len,
	TimeInternal *t)
{
	int ret;

	if (ppi->proto == PPSI_PROTO_UDP) {


		return ret;
	}

	// Don't handle RAW, VLAN
	pp_diag(ppi, frames, 1, "win32_net_recv not UDP\n");
	return -1;
}

static int win32_net_send(struct pp_instance *ppi, void *pkt, int len,
	TimeInternal *t, int chtype, int use_pdelay_addr)
{
	int ret;

	if (ppi->proto == PPSI_PROTO_UDP) {


		return ret;
	}

	// Don't handle RAW, VLAN
	pp_diag(ppi, frames, 1, "win32_net_send not UDP\n");
	return -1;
}

static int win32_net_check_packet(struct pp_globals *ppg, int delay_ms)
{
	int ret = 0;

	return ret;
}

struct pp_network_operations win32_net_ops = {
	.init = win32_net_init,
	.exit = win32_net_exit,
	.recv = win32_net_recv,
	.send = win32_net_send,
	.check_packet = win32_net_check_packet,
};

