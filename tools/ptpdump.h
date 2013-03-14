#ifndef __PTPDUMP_H__
#define __PTPDUMP_H__

#if __STDC_HOSTED__
#include <time.h>
#include <sys/time.h>
#include <netinet/ip.h>		/* struct iphdr */
#include <netinet/udp.h>	/* struct udphdr */
#include <linux/if_ether.h>	/* struct ethhdr */
#else
#include <ppsi/ppsi.h>
#include "../lib/network_types.h"
#define printf pp_printf
#endif

int dump_udppkt(char *prefix, void *buf, int len, struct TimeInternal *ti);
int dump_payloadpkt(char *prefix, void *buf, int len, struct TimeInternal *ti);
int dump_1588pkt(char *prefix, void *buf, int len, struct TimeInternal *ti);

#endif /* __PTPDUMP_H__ */
