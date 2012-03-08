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

#include <ieee1588_types.h> /* from ../include/pptp */
#include "decent_types.h"

#ifndef ETH_P_1588
#define ETH_P_1588     0x88F7
#endif

int dumpstruct(FILE *dest, char *prefix, char *name, void *ptr, int size)
{
	int ret, i;
	unsigned char *p = ptr;

	ret = fprintf(dest, "%s%s (size %i)\n", prefix, name, size);
	for (i = 0; i < size; ) {
		if ((i & 0xf) == 0)
			ret += fprintf(dest, "%s", prefix);
		ret += fprintf(dest, "%02x", p[i]);
		i++;
		ret += fprintf(dest, i & 3 ? " " : i & 0xf ? "  " : "\n");
	}
	if (i & 0xf)
		ret += fprintf(dest, "\n");
	return ret;
}


static void dump_eth(struct ethhdr *eth)
{
	struct timeval tv;
	static struct timeval prev;
	struct tm tm;
	unsigned char *d = eth->h_dest;
	unsigned char *s = eth->h_source;

	gettimeofday(&tv, NULL);
	if (prev.tv_sec) {
		int i;
		int diffms;

		diffms = (tv.tv_sec - prev.tv_sec) * 1000
			+ (signed)(tv.tv_usec + 500 - prev.tv_usec) / 1000;
		/* empty lines, one every .25 seconds, at most 10 of them */
		for (i = 250; i < 2500 && i < diffms; i += 250)
			printf("\n");
		printf("TIMEDELTA: %i ms\n", diffms);
	}
	prev = tv;
	localtime_r(&tv.tv_sec, &tm);
	printf("TIME: (%li - 0x%lx) %02i:%02i:%02i.%06li\n",
	       tv.tv_sec, tv.tv_sec,
	       tm.tm_hour, tm.tm_min, tm.tm_sec, (long)tv.tv_usec);
	printf("ETH: %04x (%02x:%02x:%02x:%02x:%02x:%02x -> "
	       "%02x:%02x:%02x:%02x:%02x:%02x)\n", ntohs(eth->h_proto),
	       s[0], s[1], s[2], s[3], s[4], s[5],
	       d[0], d[1], d[2], d[3], d[4], d[5]);
}

static void dump_ip(struct iphdr *ip)
{
	uint32_t s = ntohl(ip->saddr);
	uint32_t d = ntohl(ip->daddr);
	printf("IP: %i (%i.%i.%i.%i -> %i.%i.%i.%i) len %i\n",
	       ip->protocol,
	       (s >> 24) & 0xff, (s >> 16) & 0xff, (s >> 8) & 0xff, s & 0xff,
	       (d >> 24) & 0xff, (d >> 16) & 0xff, (d >> 8) & 0xff, d & 0xff,
	       ntohs(ip->tot_len));
}

static void dump_udp(struct udphdr *udp)
{
	printf("UDP: (%i -> %i) len %i\n",
	       ntohs(udp->source), ntohs(udp->dest), ntohs(udp->len));
}

/* Helpers for fucking data structures */
static void dump_1stamp(char *s, struct stamp *t)
{
	uint64_t  sec = (uint64_t)(ntohs(t->sec.msb)) << 32;

	sec |= (uint64_t)(ntohl(t->sec.lsb));
	printf("%s%lli.%09i\n", s, sec, ntohl(t->nsec));
}

static void dump_1quality(char *s, ClockQuality *q)
{
	printf("%s%02x-%02x-%04x\n", s, q->clockClass, q->clockAccuracy,
	       q->offsetScaledLogVariance);
}

static void dump_1clockid(char *s, ClockIdentity i)
{
	printf("%s%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n", s,
	       i[0], i[1], i[2], i[3], i[4], i[5], i[6], i[7]);
}

static void dump_1port(char *s, unsigned char *p)
{
	printf("%s%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n", s,
	       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
}


/* Helpers for each message types */
static void dump_msg_announce(struct ptp_announce *p)
{
	dump_1stamp("MSG-ANNOUNCE: stamp ", &p->originTimestamp);
	dump_1quality("MSG-ANNOUNCE: grandmaster-quality ",
		      &p->grandmasterClockQuality);
	printf("MSG-ANNOUNCE: grandmaster-prio %i %i\n",
	       p->grandmasterPriority1, p->grandmasterPriority2);
	dump_1clockid("MSG-ANNOUNCE: grandmaster-id ",
		      p->grandmasterIdentity);
}

static void dump_msg_sync_etc(char *s, struct ptp_sync_etc *p)
{
	dump_1stamp(s, &p->stamp);
}

static void dump_msg_resp_etc(char *s, struct ptp_sync_etc *p)
{
	dump_1stamp(s, &p->stamp);
	dump_1port(s, p->port);
}

/* TLV dumper, not yet white-rabbit aware */
int dump_tlv(struct ptp_tlv *tlv, int totallen)
{
	/* the field includes 6 bytes of the header, ecludes 4 of them. Bah! */
	int explen = ntohs(tlv->len) + 4;

	printf("TLV: type %04x len %i oui %02x:%02x:%02x sub %02x:%02x:%02x\n",
	       ntohs(tlv->type), explen, tlv->oui[0], tlv->oui[1], tlv->oui[2],
	       tlv->subtype[0], tlv->subtype[1], tlv->subtype[2]);
	if (explen > totallen) {
		printf("TLV: too short (expected %i, total %i)\n",
		       explen, totallen);
		return totallen;
	}

	/* later:  if (memcmp(tlv->oui, "\x08\x00\x30", 3)) ... */

	/* Now dump non-wr tlv in binary, count only payload */
	dumpstruct(stdout, "TLV: ", "tlv-content", tlv->data,
		   explen - sizeof(*tlv));
	return explen;
}

/* A big function to dump the ptp information */
static void dump_payload(void *pl, int len)
{
	struct ptp_header *h = pl;
	void *msg_specific = (void *)(h + 1);
	int donelen = 34; /* packet length before tlv */

	if (h->versionPTP != 2) {
		printf("VERSION: unsupported (%i)\n", h->versionPTP);
		return;
	}
	printf("VERSION: %i (type %i, len %i, domain %i)\n",
	       h->versionPTP, h->messageType,
	       ntohs(h->messageLength), h->domainNumber);
	printf("FLAGS: 0x%04x (correction 0x%08llx)\n", h->flagField,
	       h->correctionField);
	dump_1port("PORT: ", h->sourcePortIdentity);
	printf("REST: seq %i, ctrl %i, log-interval %i\n",
	       ntohs(h->sequenceId), h->controlField, h->logMessageInterval);
#define CASE(t, x) case PPM_ ##x: printf("MESSAGE: (" #t ") " #x "\n")
	switch(h->messageType) {
		CASE(E, SYNC);
		dump_msg_sync_etc("MSG-SYNC: ", msg_specific);
		donelen = 44;
		break;

		CASE(E, DELAY_REQ);
		dump_msg_sync_etc("MSG-DELAY_REQ: ", msg_specific);
		donelen = 44;
		break;

		CASE(E, PDELAY_REQ);
		dump_msg_sync_etc("MSG-PDELAY_REQ: ", msg_specific);
		donelen = 54;
		break;

		CASE(E, PDELAY_RESP);
		dump_msg_resp_etc("MSG-PDELAY_RESP: ", msg_specific);
		donelen = 54;
		break;

		CASE(G, FOLLOW_UP);
		dump_msg_sync_etc("MSG-FOLLOW_UP: ", msg_specific);
		donelen = 44;
		break;

		CASE(G, DELAY_RESP);
		dump_msg_resp_etc("MSG-DELAY_RESP: ", msg_specific);
		donelen = 54;
		break;

		CASE(G, PDELAY_RESP_FOLLOW_UP);
		dump_msg_resp_etc("MSG-PDELAY_RESP_FOLLOWUP: ", msg_specific);
		donelen = 54;
		break;

		CASE(G, ANNOUNCE);
		dump_msg_announce(msg_specific);
		donelen = 64;
		break;

		CASE(G, SIGNALING);
		dump_1port("MSG-SIGNALING: target-port ", msg_specific);
		donelen = 44;
		break;

		CASE(G, MANAGEMENT);
		/* FIXME */
		break;
	}

	/* Dump any trailing TLV */
	while (donelen < len) {
		int n = len - donelen;
		if (n < sizeof(struct ptp_tlv)) {
			printf("TLV: too short (%i - %i = %i)\n", len,
			       donelen, n);
			break;
		}
		donelen += dump_tlv(pl + donelen, n);
	}

	/* Finally, binary dump of it all */
	dumpstruct(stdout, "DUMP: ", "payload", pl, len);
}

static int dump_udppkt(void *buf, int len)
{
	struct ethhdr *eth = buf;
	struct iphdr *ip = buf + ETH_HLEN;
	struct udphdr *udp = (void *)(ip + 1);
	void *payload = (void *)(udp + 1);
	int udpdest = ntohs(udp->dest);

	if (len < ETH_HLEN + sizeof(*ip) + sizeof(*udp))
		return -1;

	/* page 239 and following */

	if (udpdest != 319 && udpdest != 320)
		return -1;

	dump_eth(eth);
	dump_ip(ip);
	dump_udp(udp);
	dump_payload(payload, len - (payload - buf));
	putchar('\n');
	return 0;
}

static int dump_1588pkt(void *buf, int len)
{
	struct ethhdr *eth = buf;
	void *payload = (void *)(eth + 1);

	dump_eth(eth);
	dump_payload(payload, len - (payload - buf));
	putchar('\n');
	return 0;
}

int main(int argc, char **argv)
{
	int sock;
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

		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);
		int len;

		len = recvfrom(sock, buf, sizeof(buf), MSG_TRUNC,
			       (struct sockaddr *) &from, &fromlen);
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
			if (len < ETH_HLEN + sizeof(*ip))
				continue;
			if (ip->protocol != IPPROTO_UDP)
				continue;
			dump_udppkt(buf, len);
			break;
		case ETH_P_1588:
			dump_1588pkt(buf, len);
			break;
		default:
			continue;
		}
		fflush(stdout);
	}

	return 0;
}













