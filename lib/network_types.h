/*
 * This header is a hack, meant to provide data structures for
 * tools/dump-funcs.c when it is built in a freestanding environment.
 */
#include <arch/arch.h>		/* for ntohs and ntohl */

/* From: /usr/include/linux/if_ether.h with s/__be16/uint16_t/ */

#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define ETH_HLEN	14		/* Total octets in header.	 */

#ifndef _MSC_VER
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#else
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#endif


PACK(struct ethhdr {
	unsigned char	h_dest[ETH_ALEN];	/* destination eth addr */
	unsigned char	h_source[ETH_ALEN];	/* source ether addr	*/
	uint16_t	h_proto;		/* packet type ID field */
});


/* From: /usr/include/netinet/ip.h, renaming the bitfield to avoid using them */

struct iphdr {
	unsigned int ihl_unused:4;
	unsigned int version_unused:4;
	uint8_t tos;
	uint16_t tot_len;
	uint16_t id;
	uint16_t frag_off;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t check;
	uint32_t saddr;
	uint32_t daddr;
	/*The options start here. */
};

/* From: /usr/include/netinet/udp.h */

struct udphdr {
	uint16_t source;
	uint16_t dest;
	uint16_t len;
	uint16_t check;
};

#ifndef __PPSI_PPSI_H__
/* from ppsi.h -- never defined elsewhere */
struct pp_vlanhdr {
	uint8_t h_dest[6];
	uint8_t h_source[6];
	uint16_t h_tpid;
	uint16_t h_tci;
	uint16_t h_proto;
};
#endif /* __PPSI_PPSI_H__ */
