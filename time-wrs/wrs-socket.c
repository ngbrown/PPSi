/*
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/net_tstamp.h>
#include <linux/errqueue.h>
#include <linux/sockios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <fcntl.h>
#include <errno.h>

#include <ptpdump.h>

#include <ppsi-wrs.h>
#include <hal_exports.h>
#include "../proto-ext-whiterabbit/wr-api.h"

extern struct minipc_pd __rpcdef_get_port_state;

#define ETHER_MTU 1518
#define DMTD_UPDATE_INTERVAL 500
#define PACKED __attribute__((packed))

struct scm_timestamping {
	struct timespec systime;
	struct timespec hwtimetrans;
	struct timespec hwtimeraw;
};

PACKED struct etherpacket {
	struct ethhdr ether;
	char data[ETHER_MTU];
};

typedef struct
{
	uint64_t start_tics;
	uint64_t timeout;
} timeout_t ;

struct wrs_socket {
	/* parameters for linearization of RX timestamps */
	uint32_t clock_period;
	uint32_t phase_transition;
	uint32_t dmtd_phase;
	int dmtd_phase_valid;
	timeout_t dmtd_update_tmo;
};

static uint64_t get_tics()
{
	struct timezone tz = {0, 0};
	struct timeval tv;
	gettimeofday(&tv, &tz);

	return (uint64_t) tv.tv_sec * 1000000ULL + (uint64_t) tv.tv_usec;
}

static inline int tmo_init(timeout_t *tmo, uint32_t milliseconds)
{
	tmo->start_tics = get_tics();
	tmo->timeout = (uint64_t) milliseconds * 1000ULL;
	return 0;
}

static inline int tmo_restart(timeout_t *tmo)
{
	tmo->start_tics = get_tics();
	return 0;
}

static inline int tmo_expired(timeout_t *tmo)
{
	return (get_tics() - tmo->start_tics > tmo->timeout);
}

/* checks if x is inside range <min, max> */
static inline int inside_range(int min, int max, int x)
{
	if(min < max)
		return (x>=min && x<=max);
	else
		return (x<=max || x>=min);
}

static void update_dmtd(struct wrs_socket *s, char *ifname)
{
	hexp_port_state_t pstate;

	if (tmo_expired(&s->dmtd_update_tmo))
	{
		minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_get_port_state,
			 &pstate, ifname);

		s->dmtd_phase = pstate.phase_val;
		s->dmtd_phase_valid = pstate.phase_val_valid;

		tmo_restart(&s->dmtd_update_tmo);
	}
}

static void wrs_linearize_rx_timestamp(TimeInternal *ts,
	int32_t dmtd_phase, int cntr_ahead, int transition_point,
	int clock_period)
{
	int trip_lo, trip_hi;
	int phase;

	phase = clock_period -1 -dmtd_phase;

	/* calculate the range within which falling edge timestamp is stable
	 * (no possible transitions) */
	trip_lo = transition_point - clock_period / 4;
	if(trip_lo < 0) trip_lo += clock_period;

	trip_hi = transition_point + clock_period / 4;
	if(trip_hi >= clock_period) trip_hi -= clock_period;

	if(inside_range(trip_lo, trip_hi, phase))
	{
		/* We are within +- 25% range of transition area of
		 * rising counter. Take the falling edge counter value as the
		 * "reliable" one. cntr_ahead will be 1 when the rising edge
		 * counter is 1 tick ahead of the falling edge counter */

		ts->nanoseconds -= cntr_ahead ? (clock_period / 1000) : 0;

		/* check if the phase is before the counter transition value
		 * and eventually increase the counter by 1 to simulate a
		 * timestamp transition exactly at s->phase_transition
		 * DMTD phase value */
		if(inside_range(trip_lo, transition_point, phase))
			ts->nanoseconds += clock_period / 1000;

	}

	ts->phase = phase - transition_point - 1;
	if(ts->phase  < 0) ts->phase += clock_period;
	ts->phase = clock_period - 1 -ts->phase;
}


static int wrs_recv_msg(struct pp_instance *ppi, int fd, void *pkt, int _len,
			  TimeInternal *t)
{
	struct wrs_socket *s;
	struct msghdr msg;
	struct iovec entry;
	struct sockaddr_ll from_addr;
	struct {
		struct cmsghdr cm;
		char control[1024];
	} control;
	struct cmsghdr *cmsg;
	struct scm_timestamping *sts = NULL;

	s = (struct wrs_socket*)NP(ppi)->ch[PP_NP_GEN].arch_data;
	size_t len = _len + sizeof(struct ethhdr);

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &entry;
	msg.msg_iovlen = 1;
	entry.iov_base = pkt;
	entry.iov_len = len;
	msg.msg_name = (caddr_t)&from_addr;
	msg.msg_namelen = sizeof(from_addr);
	msg.msg_control = &control;
	msg.msg_controllen = sizeof(control);

	int ret = recvmsg(fd, &msg, MSG_DONTWAIT);

	if (ret < 0 && errno == EAGAIN) return 0; // would be blocking

	if (ret == -EAGAIN) return 0;

	if (ret <= 0) return ret;

	/* FIXME Check ptp-noposix, commit d34f56f: if sender mac check
	 * is required. Should be added here */

	if (t)
		t->correct = 0;

	for (cmsg = CMSG_FIRSTHDR(&msg);
	     cmsg;
	     cmsg = CMSG_NXTHDR(&msg, cmsg)) {

		void *dp = CMSG_DATA(cmsg);

		if(cmsg->cmsg_level == SOL_SOCKET
		   && cmsg->cmsg_type == SO_TIMESTAMPING)
			sts = (struct scm_timestamping *) dp;

	}

	if(sts && t)
	{
		int cntr_ahead = sts->hwtimeraw.tv_sec & 0x80000000 ? 1: 0;
		t->nanoseconds = sts->hwtimeraw.tv_nsec;
		t->seconds =
			(uint64_t) sts->hwtimeraw.tv_sec & 0x7fffffff;

		t->raw_ahead = cntr_ahead;

		update_dmtd(s, ppi->iface_name);
		if (!WR_DSPOR(ppi)->wrModeOn) {
			/* for non-wr-mode any reported stamp is correct */
			t->correct = 1;
			return ret;
		}
		if (s->dmtd_phase_valid)
		{
			wrs_linearize_rx_timestamp(t, s->dmtd_phase,
				cntr_ahead, s->phase_transition, s->clock_period);
			t->correct = 1;
		}
	}

	return ret;
}

int wrs_net_recv(struct pp_instance *ppi, void *pkt, int len,
		   TimeInternal *t)
{
	int ret;

	if (ppi->ethernet_mode) {
		int fd = NP(ppi)->ch[PP_NP_GEN].fd;

		ret = wrs_recv_msg(ppi, fd, pkt, len, t);
		if (ret > 0 && pp_diag_allow(ppi, frames, 2))
			dump_1588pkt("recv: ", pkt, ret, t);
		pp_diag(ppi, time, 1, "recv stamp: (correct %i) %9li.%09li\n",
			t->correct, (long)t->seconds,
			(long)t->nanoseconds);
		return ret;
	}

	pp_error("udp is not supported in wrs arch yet\n");
	return -ENOTSUP;
}

/* Waits for the transmission timestamp and stores it in t (if not null). */
static void poll_tx_timestamp(struct wrs_socket *s, int fd,
							  TimeInternal *t)
{
	char data[16384];

	struct msghdr msg;
	struct iovec entry;
	struct sockaddr_ll from_addr;
	struct {
		struct cmsghdr cm;
		char control[1024];
	} control;
	struct cmsghdr *cmsg;
	int res;
	uint32_t rtag;

	struct sock_extended_err *serr = NULL;
	struct scm_timestamping *sts = NULL;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &entry;
	msg.msg_iovlen = 1;
	entry.iov_base = data;
	entry.iov_len = sizeof(data);
	msg.msg_name = (caddr_t)&from_addr;
	msg.msg_namelen = sizeof(from_addr);
	msg.msg_control = &control;
	msg.msg_controllen = sizeof(control);

	res = recvmsg(fd, &msg, MSG_ERRQUEUE);

	if (t)
		t->correct = 0;

	if (res <= 0)
		return;

	memcpy(&rtag, data + res - 4, 4);

	for (cmsg = CMSG_FIRSTHDR(&msg);
	     cmsg;
	     cmsg = CMSG_NXTHDR(&msg, cmsg)) {

			void *dp = CMSG_DATA(cmsg);

			if(cmsg->cmsg_level == SOL_PACKET
			   && cmsg->cmsg_type == PACKET_TX_TIMESTAMP)
				serr = (struct sock_extended_err *) dp;

			if(cmsg->cmsg_level == SOL_SOCKET
			   && cmsg->cmsg_type == SO_TIMESTAMPING)
				sts = (struct scm_timestamping *) dp;

			if(serr && sts && t)
			{
				t->correct = 1;
				t->phase = 0;
				t->nanoseconds = sts->hwtimeraw.tv_nsec;
				t->seconds = (uint64_t) sts->hwtimeraw.tv_sec & 0x7fffffff;
			}
	}
}

int wrs_net_send(struct pp_instance *ppi, void *pkt, int len,
			  TimeInternal *t, int chtype, int use_pdelay_addr)
{
	struct ethhdr *hdr = pkt;
	struct wrs_socket *s;
	int ret;

	s = (struct wrs_socket *)NP(ppi)->ch[PP_NP_GEN].arch_data;

	if (ppi->ethernet_mode) {
		int fd = NP(ppi)->ch[PP_NP_GEN].fd;
		hdr->h_proto = htons(ETH_P_1588);

		memcpy(hdr->h_dest, PP_MCAST_MACADDRESS, ETH_ALEN);
		/* raw socket implementation always uses gen socket */
		memcpy(hdr->h_source, NP(ppi)->ch[PP_NP_GEN].addr, ETH_ALEN);

		if (t)
			ppi->t_ops->get(ppi, t);

		ret = send(fd, hdr, len, 0);
		poll_tx_timestamp(s, fd, t);
		if (pp_diag_allow(ppi, frames, 2))
			dump_1588pkt("send: ", pkt, len, t);
		pp_diag(ppi, time, 1, "send stamp: (correct %i) %9li.%09li\n",
			t->correct, (long)t->seconds,
			(long)t->nanoseconds);
		return ret;
	}

	pp_error("udp is not supported in wrs arch yet\n");
	return -ENOTSUP;

}

/* Helper for setting up hardware timestamps */
static int wrs_enable_timestamps(struct pp_instance *ppi, int fd)
{
	int so_timestamping_flags = SOF_TIMESTAMPING_TX_HARDWARE |
		SOF_TIMESTAMPING_RX_HARDWARE |
		SOF_TIMESTAMPING_RAW_HARDWARE;
	struct ifreq ifr;
	struct hwtstamp_config hwconfig;

	if (fd < 0)
		return 0; /* nothing to do */

	strncpy(ifr.ifr_name, ppi->iface_name, sizeof(ifr.ifr_name));
	hwconfig.tx_type = HWTSTAMP_TX_ON;
	hwconfig.rx_filter = HWTSTAMP_FILTER_PTP_V2_L2_EVENT;
	ifr.ifr_data = &hwconfig;

	if (ioctl(fd, SIOCSHWTSTAMP, &ifr) < 0) {
		pp_diag(ppi, frames, 1,
			"%s: ioctl(SIOCSHWTSTAMP): %s\n",
			ppi->iface_name, strerror(errno));
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING,
		       &so_timestamping_flags, sizeof(int)) < 0) {
		pp_diag(ppi, frames, 1,
			"socket: setsockopt(TIMESTAMPING): %s\n",
			strerror(errno));
		return -1;
	}
	return 0;
}

static int wrs_net_exit(struct pp_instance *ppi);

static int wrs_net_init(struct pp_instance *ppi)
{
	int r, i;
	hexp_port_state_t pstate;

	if (NP(ppi)->ch[PP_NP_GEN].arch_data)
		wrs_net_exit(ppi);

	r = unix_net_ops.init(ppi);
	if (r)
		return r;

	if (!ppi->ethernet_mode) {
		pp_diag(ppi, frames, 1, "%s: Can't do White Rabbit over UDP,"
			" falling back to normal PTP\n", __func__);
		ppi->n_ops = &unix_net_ops;
		return 0;
	}

	r = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_get_port_state,
		 &pstate, ppi->iface_name);

	if (r)
		return r;

	/* Only one wrs_socket is created for each pp_instance, because
	 * wrs_socket is related to the interface and not to the pp_channel */
	struct wrs_socket *s = calloc(0, sizeof(struct wrs_socket));

	if (!s)
		return -1;

	s->clock_period = pstate.clock_period;
	s->phase_transition = pstate.t2_phase_transition;
	s->dmtd_phase_valid = 0;
	s->dmtd_phase = pstate.phase_val;

	NP(ppi)->ch[PP_NP_GEN].arch_data = s;
	NP(ppi)->ch[PP_NP_EVT].arch_data = s;
	tmo_init(&s->dmtd_update_tmo, DMTD_UPDATE_INTERVAL);

	for (i = PP_NP_GEN, r = 0; i <= PP_NP_EVT && r == 0; i++)
		r = wrs_enable_timestamps(ppi, NP(ppi)->ch[i].fd);
	if (r) {
		NP(ppi)->ch[PP_NP_GEN].arch_data = NULL;
		NP(ppi)->ch[PP_NP_EVT].arch_data = NULL;
		free(s);
	}
	return r;
}

static int wrs_net_exit(struct pp_instance *ppi)
{
	unix_net_ops.exit(ppi);
	free(NP(ppi)->ch[PP_NP_GEN].arch_data);
	NP(ppi)->ch[PP_NP_GEN].arch_data = NULL;
	return 0;
}

struct pp_network_operations wrs_net_ops = {
	.init = wrs_net_init,
	.exit = wrs_net_exit,
	.recv = wrs_net_recv,
	.send = wrs_net_send,
};
