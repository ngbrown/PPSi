/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Pietro Fezzardi (pietrofezzardi@gmail.com)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

/* Socket interface for GNU/Linux (and most likely other posix systems) */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include <ppsi/ppsi.h>
#include "ptpdump.h"
#include "../arch-sim/ppsi-sim.h"

#define PP_MASTER_GEN_PORT	(10000 + PP_GEN_PORT)
#define PP_MASTER_EVT_PORT	(10000 + PP_EVT_PORT)
#define PP_SLAVE_GEN_PORT	(20000 + PP_GEN_PORT)
#define PP_SLAVE_EVT_PORT	(20000 + PP_EVT_PORT)

/* Returns 1 if p1 has higher priority  than p2 */
static int compare_pending(struct sim_pending_pkt *p1,
				struct sim_pending_pkt *p2)
{
	/* expires earlier ---> higher priority */
	if (p1->delay_ns < p2->delay_ns)
		return 1;

	/* same expire time ---> higher priority to the slave because it has to
	 * handle Sync and Follow_Up in a row. If both are for the slave handle
	 * PP_NP_EVT first */
	if (p1->delay_ns == p2->delay_ns) {
		if (p1->which_ppi > p2->which_ppi)
			return 1;
		if (p1->which_ppi == p2->which_ppi)
			return (p1->chtype > p2->chtype);
	}
	return 0;
}

static int insert_pending(struct sim_ppg_arch_data *data,
				struct sim_pending_pkt *new)
{
	struct sim_pending_pkt *pkt, tmp;
	int i = data->n_pending;

	data->pending[i] = *new;
	pkt = &data->pending[i - 1];
	while (compare_pending(new, pkt) && (i > 0)) {
		tmp = *pkt;
		*pkt = *new;
		*(pkt + 1) = tmp;
		pkt--;
		i--;
	}
	data->n_pending++;
	return 0;
}

static int pending_received(struct sim_ppg_arch_data *data)
{
	int i;

	if (data->n_pending == 0)
		return 0;
	for (i = 0; i < data->n_pending; i++)
		data->pending[i] = data->pending[i+1];
	data->n_pending--;
	return 0;
}

static int sim_recv_msg(struct pp_instance *ppi, int fd, void *pkt, int len,
			  TimeInternal *t)
{
	ssize_t ret;
	struct msghdr msg;
	struct iovec vec[1];

	union {
		struct cmsghdr cm;
		char control[512];
	} cmsg_un;

	vec[0].iov_base = pkt;
	vec[0].iov_len = PP_MAX_FRAME_LENGTH;

	memset(&msg, 0, sizeof(msg));
	memset(&cmsg_un, 0, sizeof(cmsg_un));

	/* msg_name, msg_namelen == 0: not used */
	msg.msg_iov = vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsg_un.control;
	msg.msg_controllen = sizeof(cmsg_un.control);

	ret = recvmsg(fd, &msg, 0);
	if (ret <= 0) {
		if (errno == EAGAIN || errno == EINTR)
			return 0;

		return ret;
	}
	if (msg.msg_flags & MSG_TRUNC) {
		pp_error("%s: truncated message\n", __func__);
		return 0;
	}
	/* get time stamp of packet */
	if (msg.msg_flags & MSG_CTRUNC) {
		pp_error("%s: truncated ancillary data\n", __func__);
		return 0;
	}

	ppi->t_ops->get(ppi, t);
	/* This is not really hw... */
	pp_diag(ppi, time, 2, "recv stamp: %i.%09i (%s)\n",
		(int)t->seconds, (int)t->nanoseconds, "user");
	return ret;
}

static int sim_net_recv(struct pp_instance *ppi, void *pkt, int len,
		   TimeInternal *t)
{
	struct sim_ppg_arch_data *data = SIM_PPG_ARCH(ppi->glbs);
	struct pp_channel *ch;
	int ret;
	/*
	 * only UDP
	 * We can return one frame only. Look in the global structure to know if
	 * the pending packet is on PP_NP_GEN or PP_NP_EVT
	 */
	if (data->n_pending <= 0)
		return 0;

	ch = &(NP(ppi)->ch[data->pending->chtype]);

	ret = -1;
	if (ch->pkt_present > 0) {
		ret = sim_recv_msg(ppi, ch->fd, pkt, len, t);
		if (ret > 0)
			ch->pkt_present--;
	}

	if (ret > 0 && pp_diag_allow(ppi, frames, 2))
		dump_payloadpkt("recv: ", pkt, ret, t);
	/* remove received packet from pending */
	pending_received(SIM_PPG_ARCH(ppi->glbs));
	return ret;
}

static int sim_net_send(struct pp_instance *ppi, void *pkt, int len,
			  TimeInternal *t, int chtype, int use_pdelay_addr)
{
	struct sockaddr_in addr;
	struct sim_pending_pkt pending;
	int ret;

	/* only UDP */
	addr.sin_family = AF_INET;
	if (ppi - ppi->glbs->pp_instances == SIM_SLAVE)
		addr.sin_port = htons(chtype == PP_NP_GEN ?
					PP_MASTER_GEN_PORT :
					PP_MASTER_EVT_PORT);
	else
		addr.sin_port = htons(chtype == PP_NP_GEN ?
					PP_SLAVE_GEN_PORT :
					PP_SLAVE_EVT_PORT);

	addr.sin_addr.s_addr = NP(ppi)->mcast_addr;

	if (t)
		ppi->t_ops->get(ppi, t);

	ret = sendto(NP(ppi)->ch[chtype].fd, pkt, len, 0,
		(struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (pp_diag_allow(ppi, frames, 2))
		dump_payloadpkt("send: ", pkt, len, t);

	/* store pending packets in global structure */
	pending.chtype = chtype;
	if (ppi - ppi->glbs->pp_instances == SIM_MASTER)
		pending.which_ppi = SIM_SLAVE;
	else
		pending.which_ppi = SIM_MASTER;
	pending.delay_ns = SIM_PPI_ARCH(ppi)->n_delay.t_prop_ns +
				SIM_PPI_ARCH(ppi)->n_delay.jit_ns;
	insert_pending(SIM_PPG_ARCH(ppi->glbs), &pending);
	NP(SIM_PPI_ARCH(ppi)->other_ppi)->ch[chtype].pkt_present++;
	return ret;
}

static int sim_net_exit(struct pp_instance *ppi)
{
	int fd;
	int i;

	/* only UDP */
	for (i = PP_NP_GEN; i <= PP_NP_EVT; i++) {
		fd = NP(ppi)->ch[i].fd;
		if (fd < 0)
			continue;
		close(fd);
		NP(ppi)->ch[i].fd = -1;
	}
	return 0;
}

static int sim_open_ch(struct pp_instance *ppi, char *ifname, int chtype)
{

	int sock = -1;
	int temp;
	struct sockaddr_in addr;
	char *context;

	/* only UDP */
	context = "socket()";
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
		goto err_out;

	NP(ppi)->ch[chtype].fd = sock;

	temp = 1; /* allow address reuse */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) < 0)
		pp_printf("%s: ioctl(SO_REUSEADDR): %s\n", __func__,
			  strerror(errno));

	/* bind sockets */
	/* need INADDR_ANY to allow receipt of multi-cast and uni-cast
	 * messages */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if ((ppi - ppi->glbs->pp_instances) == SIM_MASTER)
		addr.sin_port = htons(chtype == PP_NP_GEN ?
					PP_MASTER_GEN_PORT :
					PP_MASTER_EVT_PORT);
	else
		addr.sin_port = htons(chtype == PP_NP_GEN ?
					PP_SLAVE_GEN_PORT :
					PP_SLAVE_EVT_PORT);
	context = "bind()";
	if (bind(sock, (struct sockaddr *)&addr,
		 sizeof(struct sockaddr_in)) < 0)
		goto err_out;

	NP(ppi)->ch[chtype].fd = sock;
	/*
	 * Standard ppsi state machine is designed to drop packets coming from
	 * itself, based on the clockIdentity. This hack avoids this behaviour,
	 * changing the clockIdentity of the master.
	 */
	if (ppi - ppi->glbs->pp_instances == SIM_MASTER)
		memset(NP(ppi)->ch[chtype].addr, 111, 1);
	return 0;

err_out:
	pp_printf("%s: %s: %s\n", __func__, context, strerror(errno));
	if (sock >= 0)
		close(sock);
	NP(ppi)->ch[chtype].fd = -1;
	return -1;
}

static int sim_net_init(struct pp_instance *ppi)
{
	int i;

	if (NP(ppi)->ch[0].fd > 0)
		sim_net_exit(ppi);

	/* The buffer is inside ppi, but we need to set pointers and align */
	pp_prepare_pointers(ppi);

	/* only UDP, RAW is not supported */
	pp_diag(ppi, frames, 1, "sim_net_init UDP\n");
	for (i = PP_NP_GEN; i <= PP_NP_EVT; i++) {
		if (sim_open_ch(ppi, ppi->iface_name, i))
			return -1;
	}
	return 0;
}

struct pp_network_operations sim_net_ops = {
	.init = sim_net_init,
	.exit = sim_net_exit,
	.recv = sim_net_recv,
	.send = sim_net_send,
	.check_packet = NULL,
};
