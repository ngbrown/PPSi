/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */
#include <stdio.h>
#include <string.h>
#include <ppsi/ieee1588_types.h> /* from ../include */
#include "decent_types.h"
#include "ptpdump.h"

static int dumpstruct(char *p1, char *p2, char *name, void *ptr, int size)
{
	int ret, i;
	unsigned char *p = ptr;

	ret = printf("%s%s%s (size %i)\n", p1, p2, name, size);
	for (i = 0; i < size; ) {
		if ((i & 0xf) == 0)
			ret += printf("%s%s", p1, p2);
		ret += printf("%02x", p[i]);
		i++;
		ret += printf(i & 3 ? " " : i & 0xf ? "  " : "\n");
	}
	if (i & 0xf)
		ret += printf("\n");
	return ret;
}

#if __STDC_HOSTED__
static void dump_time(char *prefix, struct TimeInternal *ti)
{
	struct timeval tv;
	struct tm tm;

	tv.tv_sec = ti->seconds;
	tv.tv_usec = ti->nanoseconds / 1000;
	localtime_r(&tv.tv_sec, &tm);
	printf("%sTIME: (%li - 0x%lx) %02i:%02i:%02i.%06li%s\n", prefix,
	       tv.tv_sec, tv.tv_sec,
	       tm.tm_hour, tm.tm_min, tm.tm_sec, (long)tv.tv_usec,
	       !ti->correct ? " invalid" : "");
}
#else
static void dump_time(char *prefix, struct TimeInternal *ti)
{
	printf("%sTIME: (%li - 0x%lx) %li.%06li%s\n", prefix, (long)ti->seconds,
	       (long)ti->seconds, (long)ti->seconds, (long)ti->nanoseconds,
	       !ti->correct ? " invalid" : "");
}
#endif

static void dump_eth(char *prefix, struct ethhdr *eth)
{
	unsigned char *d = eth->h_dest;
	unsigned char *s = eth->h_source;

	printf("%sETH: %04x (%02x:%02x:%02x:%02x:%02x:%02x -> "
	       "%02x:%02x:%02x:%02x:%02x:%02x)\n", prefix, ntohs(eth->h_proto),
	       s[0], s[1], s[2], s[3], s[4], s[5],
	       d[0], d[1], d[2], d[3], d[4], d[5]);
}

static void dump_ip(char *prefix, struct iphdr *ip)
{
	unsigned int s = ntohl(ip->saddr);
	unsigned int d = ntohl(ip->daddr);
	printf("%sIP: %i (%i.%i.%i.%i -> %i.%i.%i.%i) len %i\n", prefix,
	       ip->protocol,
	       (s >> 24) & 0xff, (s >> 16) & 0xff, (s >> 8) & 0xff, s & 0xff,
	       (d >> 24) & 0xff, (d >> 16) & 0xff, (d >> 8) & 0xff, d & 0xff,
	       ntohs(ip->tot_len));
}

static void dump_udp(char *prefix, struct udphdr *udp)
{
	printf("%sUDP: (%i -> %i) len %i\n", prefix,
	       ntohs(udp->source), ntohs(udp->dest), ntohs(udp->len));
}

/* Helpers for fucking data structures */
static void dump_1stamp(char *prefix, char *s, struct stamp *t)
{
	uint64_t  sec = (uint64_t)(ntohs(t->sec.msb)) << 32;

	sec |= (uint64_t)(ntohl(t->sec.lsb));
	printf("%s%s%lu.%09i\n", prefix,
	       s, (unsigned long)sec, (int)ntohl(t->nsec));
}

static void dump_1quality(char *prefix, char *s, ClockQuality *q)
{
	printf("%s%s%02x-%02x-%04x\n", prefix, s, q->clockClass,
	       q->clockAccuracy, q->offsetScaledLogVariance);
}

static void dump_1clockid(char *prefix, char *s, ClockIdentity i)
{
	printf("%s%s%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n", prefix, s,
	       i.id[0], i.id[1], i.id[2], i.id[3],
	       i.id[4], i.id[5], i.id[6], i.id[7]);
}

static void dump_1port(char *prefix, char *s, unsigned char *p)
{
	printf("%s%s%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
	       prefix, s,
	       p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9]);
}


/* Helpers for each message types */
static void dump_msg_announce(char *prefix, struct ptp_announce *p)
{
	dump_1stamp(prefix, "MSG-ANNOUNCE: stamp ",
		    &p->originTimestamp);
	dump_1quality(prefix, "MSG-ANNOUNCE: grandmaster-quality ",
		      &p->grandmasterClockQuality);
	printf("%sMSG-ANNOUNCE: grandmaster-prio %i %i\n", prefix,
	       p->grandmasterPriority1, p->grandmasterPriority2);
	dump_1clockid(prefix, "MSG-ANNOUNCE: grandmaster-id ",
		      p->grandmasterIdentity);
}

static void dump_msg_sync_etc(char *prefix, char *s, struct ptp_sync_etc *p)
{
	dump_1stamp(prefix, s, &p->stamp);
}

static void dump_msg_resp_etc(char *prefix, char *s, struct ptp_sync_etc *p)
{
	dump_1stamp(prefix, s, &p->stamp);
	dump_1port(prefix, s, p->port);
}

/* TLV dumper, not yet white-rabbit aware */
static int dump_tlv(char *prefix, struct ptp_tlv *tlv, int totallen)
{
	/* the field includes 6 bytes of the header, ecludes 4 of them. Bah! */
	int explen = ntohs(tlv->len) + 4;

	printf("%sTLV: type %04x len %i oui %02x:%02x:%02x "
	       "sub %02x:%02x:%02x\n", prefix, ntohs(tlv->type), explen,
	       tlv->oui[0], tlv->oui[1], tlv->oui[2],
	       tlv->subtype[0], tlv->subtype[1], tlv->subtype[2]);
	if (explen > totallen) {
		printf("%sTLV: too short (expected %i, total %i)\n", prefix,
		       explen, totallen);
		return totallen;
	}

	/* later:  if (memcmp(tlv->oui, "\x08\x00\x30", 3)) ... */

	/* Now dump non-wr tlv in binary, count only payload */
	dumpstruct(prefix, "TLV: ", "tlv-content", tlv->data,
		   explen - sizeof(*tlv));
	return explen;
}

/* A big function to dump the ptp information */
static void dump_payload(char *prefix, void *pl, int len)
{
	struct ptp_header *h = pl;
	void *msg_specific = (void *)(h + 1);
	int donelen = 34; /* packet length before tlv */
	int version = h->versionPTP_and_reserved & 0xf;
	int messageType = h->type_and_transport_specific & 0xf;

	if (version != 2) {
		printf("%sVERSION: unsupported (%i)\n", prefix, version);
		return;
	}
	printf("%sVERSION: %i (type %i, len %i, domain %i)\n", prefix,
	       version, messageType,
	       ntohs(h->messageLength), h->domainNumber);
	printf("%sFLAGS: 0x%04x (correction %08lu)\n", prefix, h->flagField,
	       (unsigned long)h->correctionField);
	dump_1port(prefix, "PORT: ", h->sourcePortIdentity);
	printf("%sREST: seq %i, ctrl %i, log-interval %i\n", prefix,
	       ntohs(h->sequenceId), h->controlField, h->logMessageInterval);
#define CASE(t, x) case PPM_ ##x: printf("%sMESSAGE: (" #t ") " #x "\n", prefix)
	switch(messageType) {
		CASE(E, SYNC);
		dump_msg_sync_etc(prefix, "MSG-SYNC: ", msg_specific);
		donelen = 44;
		break;

		CASE(E, DELAY_REQ);
		dump_msg_sync_etc(prefix, "MSG-DELAY_REQ: ", msg_specific);
		donelen = 44;
		break;

		CASE(G, FOLLOW_UP);
		dump_msg_sync_etc(prefix, "MSG-FOLLOW_UP: ", msg_specific);
		donelen = 44;
		break;

		CASE(G, DELAY_RESP);
		dump_msg_resp_etc(prefix, "MSG-DELAY_RESP: ", msg_specific);
		donelen = 54;
		break;

		CASE(G, ANNOUNCE);
		dump_msg_announce(prefix, msg_specific);
		donelen = 64;
		break;

		CASE(G, SIGNALING);
		dump_1port(prefix, "MSG-SIGNALING: target-port ", msg_specific);
		donelen = 44;
		break;

#if __STDC_HOSTED__ /* Avoid pdelay dump withing ppsi, we don't use it */
		CASE(E, PDELAY_REQ);
		dump_msg_sync_etc(prefix, "MSG-PDELAY_REQ: ", msg_specific);
		donelen = 54;
		break;

		CASE(E, PDELAY_RESP);
		dump_msg_resp_etc(prefix, "MSG-PDELAY_RESP: ", msg_specific);
		donelen = 54;
		break;

		CASE(G, PDELAY_RESP_FOLLOW_UP);
		dump_msg_resp_etc(prefix, "MSG-PDELAY_RESP_FOLLOWUP: ",
				  msg_specific);
		donelen = 54;
		break;

		CASE(G, MANAGEMENT);
		/* FIXME */
		break;
#endif
	}

	/*
	 * Dump any trailing TLV, but ignore a trailing 2-long data hunk.
	 * The trailing zeroes appear with less-than-minimum Eth messages.
	 */
	while (donelen < len && len - donelen > 2) {
		int n = len - donelen;
		if (n < sizeof(struct ptp_tlv)) {
			printf("%sTLV: too short (%i - %i = %i)\n", prefix,
			       len, donelen, n);
			break;
		}
		donelen += dump_tlv(prefix, pl + donelen, n);
	}

	/* Finally, binary dump of it all */
	dumpstruct(prefix, "DUMP: ", "payload", pl, len);
}

/* This dumps a complete udp frame, starting from the eth header */
int dump_udppkt(char *prefix, void *buf, int len, struct TimeInternal *ti)
{
	struct ethhdr *eth = buf;
	struct iphdr *ip = buf + ETH_HLEN;
	struct udphdr *udp = (void *)(ip + 1);
	void *payload = (void *)(udp + 1);

	if (ti)
		dump_time(prefix, ti);
	dump_eth(prefix, eth);
	dump_ip(prefix, ip);
	dump_udp(prefix, udp);
	dump_payload(prefix, payload, len - (payload - buf));
	return 0;
}

/* This dumps the payload only, used for udp frames without headers */
int dump_payloadpkt(char *prefix, void *buf, int len, struct TimeInternal *ti)
{
	if (ti)
		dump_time(prefix, ti);
	dump_payload(prefix, buf, len);
	return 0;
}

/* This dumps everything, used for raw frames with headers and ptp payload */
int dump_1588pkt(char *prefix, void *buf, int len, struct TimeInternal *ti)
{
	struct ethhdr *eth = buf;
	void *payload = (void *)(eth + 1);

	if (ti)
		dump_time(prefix, ti);
	dump_eth(prefix, eth);
	dump_payload(prefix, payload, len - (payload - buf));
	return 0;
}
