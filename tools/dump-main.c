/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include <netpacket/packet.h>

#include <ppsi/ieee1588_types.h> /* from ../include */
#include "decent_types.h"
#include "ptpdump.h"

#ifndef ETH_P_1588
#define ETH_P_1588     0x88F7
#endif

void print_spaces(struct TimeInternal *ti)
{
	static struct TimeInternal prev_ti;
	int i, diffms;

	if (prev_ti.seconds) {

		diffms = (ti->seconds - prev_ti.seconds) * 1000
			+ (ti->nanoseconds / 1000 / 1000)
			- (prev_ti.nanoseconds / 1000 / 1000);
		/* empty lines, one every .25 seconds, at most 10 of them */
		for (i = 250; i < 2500 && i < diffms; i += 250)
			printf("\n");
		printf("TIMEDELTA: %i ms\n", diffms);
	}
	prev_ti = *ti;
}


int main(int argc, char **argv)
{
	int sock, ret;
	struct packet_mreq req;
	struct ifreq ifr;
	char *ifname = "eth0";

	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sock < 0) {
		fprintf(stderr, "%s: socket(): %s\n", argv[0], strerror(errno));
		exit(1);
	}
	if (argc > 1)
		ifname = argv[1];

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		fprintf(stderr, "%s: ioctl(GETIFINDEX(%s)): %s\n", argv[0],
			ifname, strerror(errno));
		exit(1);
	}
	fprintf(stderr, "Dumping PTP packets from interface \"%s\"\n", ifname);

	memset(&req, 0, sizeof(req));
	req.mr_ifindex = ifr.ifr_ifindex;
	req.mr_type = PACKET_MR_PROMISC;

	if (setsockopt(3, SOL_PACKET, PACKET_ADD_MEMBERSHIP,
		       &req, sizeof(req)) < 0) {
		fprintf(stderr, "%s: set promiscuous(%s): %s\n", argv[0],
			ifname, strerror(errno));
		exit(1);
	}

	/* Ok, now we are promiscuous. Just read stuff forever */
	while(1) {
		struct ethhdr *eth;
		struct iphdr *ip;
		static unsigned char prev[1500];
		static int prevlen;
		unsigned char buf[1500];
		struct TimeInternal ti;
		struct timeval tv;

		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);
		int len;

		len = recvfrom(sock, buf, sizeof(buf), MSG_TRUNC,
			       (struct sockaddr *) &from, &fromlen);

		/* Get the receive time, copy it to TimeInternal */
		gettimeofday(&tv, NULL);
		ti.seconds = tv.tv_sec;
		ti.nanoseconds = tv.tv_usec * 1000;

		if (len > sizeof(buf))
			len = sizeof(buf);
		/* for some reasons, we receive it three times, check dups */
		if (len == prevlen && !memcmp(buf, prev, len))
			continue;
		memcpy(prev, buf, len); prevlen = len;

		/* now only print ptp packets */
		if (len < ETH_HLEN)
			continue;
		eth = (void *)buf;
		ip = (void *)(buf + ETH_HLEN);
		switch(ntohs(eth->h_proto)) {
		case ETH_P_IP:
		{
			struct udphdr *udp = (void *)(ip + 1);
			int udpdest = ntohs(udp->dest);

			/*
			 * Filter before calling the dump function, otherwise
			 * we'll report TIMEDELAY for not-relevant frames
			 */
			if (len < ETH_HLEN + sizeof(*ip) + sizeof(*udp))
				continue;
			if (ip->protocol != IPPROTO_UDP)
				continue;
			if (udpdest != 319 && udpdest != 320)
				continue;
			print_spaces(&ti);
			ret = dump_udppkt("", buf, len, &ti);
			break;
		}

		case ETH_P_1588:
			print_spaces(&ti);
			ret = dump_1588pkt("", buf, len, &ti);
			break;
		default:
			ret = -1;
			continue;
		}
		if (ret == 0)
			putchar('\n');
		fflush(stdout);
	}

	return 0;
}













