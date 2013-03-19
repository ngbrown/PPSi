/*
 * This file is a "library" file which offers functions with the same
 * Interface as send() and recv() but that handle stamping as well.
 * The timestamp information is saved to global variables, so no
 * concurrency is allowed and so on.  In short: it's as crappy as
 * possible, but not crappier.
 *
 * Alessandro Rubini 2011, GPL2 or later
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "net_tstamp.h"
#include "misc-common.h"

/* This functions opens a socket and configures it for stamping */
int make_stamping_socket(FILE *errchan, char *argv0, char *ifname,
			 int tx_type, int rx_filter, int bits,
			 unsigned char *macaddr, int proto)
{
	struct ifreq ifr;
	struct sockaddr_ll addr;
	struct hwtstamp_config hwconfig;
	int sock, iindex, enable = 1;

	sock = socket(PF_PACKET, SOCK_RAW, proto);
	if (sock < 0 && errchan)
		fprintf(errchan, "%s: socket(): %s\n", argv0,
			strerror(errno));
	if (sock < 0)
		return sock;

	memset (&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);

	/* hw interface information */
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		if (errchan)
			fprintf(errchan, "%s: SIOCGIFINDEX(%s): %s\n", argv0,
				ifname, strerror(errno));
		close(sock);
		return -1;
	}

	iindex = ifr.ifr_ifindex;
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
		if (errchan)
			fprintf(errchan, "%s: SIOCGIFHWADDR(%s): %s\n", argv0,
				ifname, strerror(errno));
		close(sock);
		return -1;
	}
	memcpy(macaddr, ifr.ifr_hwaddr.sa_data, 6);

	/* Also, enable stamping for the hw interface */
	memset(&hwconfig, 0, sizeof(hwconfig));
	hwconfig.tx_type = tx_type;
	hwconfig.rx_filter = rx_filter;
	ifr.ifr_data = (void *)&hwconfig;
	if (ioctl(sock, SIOCSHWTSTAMP, &ifr) < 0) {
		if (errchan)
			fprintf(errchan, "%s: SIOCSHWSTAMP(%s): %s\n", argv0,
				ifname, strerror(errno));
		/* continue anyway, so any ether device can soft-stamp */
	}

	/* bind and setsockopt */
	memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(proto);
	addr.sll_ifindex = iindex;
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		if (errchan)
			fprintf(errchan, "%s: SIOCSHWSTAMP(%s): %s\n", argv0,
				ifname, strerror(errno));
		close(sock);
		return -1;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPNS,
			       &enable, sizeof(enable)) < 0) {
		if (errchan)
			fprintf(errchan, "%s: setsockopt(TIMESTAMPNS): %s\n",
				argv0, strerror(errno));
		close(sock);
		return -1;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING,
			       &bits, sizeof(bits)) < 0) {
		if (errchan)
			fprintf(errchan, "%s: setsockopt(TIMESTAMPING): %s\n",
				argv0, strerror(errno));
		close(sock);
		return -1;
	}
	return sock;
}


/* This static data is used to collect stamping information */
static struct timespec __stamp_ns;
static struct timespec __stamp_hw_info[3];
static int __stamp_errno;

/* We can print such stamp info */
int print_stamp(FILE *out, char *prefix, FILE *err)
{
	int i;

	if (__stamp_errno) {
		if (err)
			fprintf(err, "get_timestamp: %s\n", strerror(errno));
		errno = __stamp_errno;
		return -1;
	}
	fprintf(out, "%s  ns: %10li.%09li\n", prefix, __stamp_ns.tv_sec,
	       __stamp_ns.tv_nsec);
	for (i = 0; i < 3; i++)
		fprintf(out, "%s ns%i: %10li.%09li\n", prefix, i,
			__stamp_hw_info[i].tv_sec,
			__stamp_hw_info[i].tv_nsec);
	fprintf(out, "\n");
	return 0;
}

/* Or we can return it to the caller -- FIXME */
int get_stamp(struct timespec ts[4])
{
	if (__stamp_errno) {
		errno = __stamp_errno;
		memset(ts, 0, 4 * sizeof(ts[0]));
		return -1;
	}
	ts[0] = __stamp_ns;
	memcpy(ts + 1, __stamp_hw_info, sizeof(__stamp_hw_info));
	return 0;
}


/* This collecting of data is in common between send and recv */
static void __collect_data(struct msghdr *msgp)
{
	struct cmsghdr *cmsg;
	int i;

	__stamp_errno = 0;
	memset(&__stamp_ns, 0, sizeof(__stamp_ns));
	memset(__stamp_hw_info, 0, sizeof(__stamp_hw_info));
	/* Ok, got the tx stamp. Now collect stamp data */
	for (cmsg = CMSG_FIRSTHDR(msgp); cmsg;
	     cmsg = CMSG_NXTHDR(msgp, cmsg)) {
		struct timespec *stamp_ptr;
		if (0) {
			unsigned char *data;
			printf("level %i, type %i, len %i\n", cmsg->cmsg_level,
			       cmsg->cmsg_type, (int)cmsg->cmsg_len);
			data = CMSG_DATA(cmsg);
			for (i = 0; i < cmsg->cmsg_len; i++)
				printf (" %02x", data[i]);
			putchar('\n');
		}
		if (cmsg->cmsg_level != SOL_SOCKET)
			continue;
		stamp_ptr = (struct timespec *)CMSG_DATA(cmsg);
		if (cmsg->cmsg_type == SO_TIMESTAMPNS)
			__stamp_ns = *stamp_ptr;
		if (cmsg->cmsg_type != SO_TIMESTAMPING)
			continue;
		for (i = 0; i < 3; i++, stamp_ptr++)
			__stamp_hw_info[i] = *stamp_ptr;
	}
}

/*
 * These functions are like send/recv but handle stamping too.
 */
ssize_t send_and_stamp(int sock, void *buf, size_t len, int flags)
{
	struct msghdr msg; /* this line and more from timestamping.c */
	struct iovec entry;
	struct sockaddr_ll from_addr;
	struct {
		struct cmsghdr cm;
		char control[512];
	} control;
	char data[3*1024];
	int i, j, ret;

	ret = send(sock, buf, len, flags);
	if (ret < 0)
		return ret;

	/* Then, get back from the error queue */
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &entry;
	msg.msg_iovlen = 1;
	entry.iov_base = data;
	entry.iov_len = sizeof(data);
	msg.msg_name = (caddr_t)&from_addr;
	msg.msg_namelen = sizeof(from_addr);
	msg.msg_control = &control;
	msg.msg_controllen = sizeof(control);

	j = 10; /* number of trials */
	while ( (i = recvmsg(sock, &msg, MSG_ERRQUEUE | MSG_DONTWAIT)) < 0
		&& j--)
		usleep(1000); /* retry... */
	if (i < 0) {
		__stamp_errno = ETIMEDOUT;
		memset(&__stamp_ns, 0, sizeof(__stamp_ns));
		memset(__stamp_hw_info, 0, sizeof(__stamp_hw_info));
		return ret;
	}
	if (getenv("STAMP_VERBOSE")) {
		int b;
		printf("send %i =", i);
		for (b = 0; b < i && b < 20; b++)
			printf(" %02x", data[b] & 0xff);
		putchar('\n');
	}
	if (0) { /* print the data buffer */
		int j;
		printf("sent: here is the data back (%i bytes):\n", i);
		for (j = 0; j < i; j++)
			printf(" %02x", data[j] & 0xff);
		putchar('\n');
	}

	__collect_data(&msg);
	return ret;
}

ssize_t recv_and_stamp(int sock, void *buf, size_t len, int flags)
{
	int ret;
	struct msghdr msg;
	struct iovec entry;
	struct sockaddr_ll from_addr;
	struct {
		struct cmsghdr cm;
		char control[512];
	} control;

	if (0) {
		/* we can't really call recv, do it with cmsg alone */
		ret = recv(sock, buf, len, flags);
	} else {
		memset(&msg, 0, sizeof(msg));
		msg.msg_iov = &entry;
		msg.msg_iovlen = 1;
		entry.iov_base = buf;
		entry.iov_len = len;
		msg.msg_name = (caddr_t)&from_addr;
		msg.msg_namelen = sizeof(from_addr);
		msg.msg_control = &control;
		msg.msg_controllen = sizeof(control);

		ret = recvmsg(sock, &msg, 0);
		if (ret < 0)
			return ret;

		if (getenv("STAMP_VERBOSE")) {
			int b;
			printf("recv %i =", ret);
			for (b = 0; b < ret && b < 20; b++)
				printf(" %02x", ((char *)buf)[b] & 0xff);
			putchar('\n');
		}
		__collect_data(&msg);
	}
	return ret;
}
