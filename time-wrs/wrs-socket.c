/*
 * Aurelio Colosimo for CERN, 2013 -- public domain
 */

#include <ppsi/ppsi.h>
#include <unix-time.h>

struct pp_network_operations wrs_net_ops = {
	.init = unix_net_init,
	.exit = unix_net_exit,
	.recv = unix_net_recv,
	.send = unix_net_send,
};
