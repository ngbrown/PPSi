#ifndef __ARCH_H__
#define __ARCH_H__

/* Architecture-specific defines, included by top-level stuff */

/* please note that these have multiple evaluation of the argument */
#define	htons(x)	__bswap_16(x)
#define	__bswap_16(x)	((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))

#define htonl(x)	__bswap_32(x)
#define __bswap_32(x)	(					\
	(((uint32_t)(x) & (uint32_t)0x000000ffUL) << 24) |	\
	(((uint32_t)(x) & (uint32_t)0x0000ff00UL) <<  8) |	\
	(((uint32_t)(x) & (uint32_t)0x00ff0000UL) >>  8) |	\
	(((uint32_t)(x) & (uint32_t)0xff000000UL) >> 24))

#define ntohs htons
#define ntohl htonl

#define abs(x) ((x >= 0) ? x : -x)
#endif /* __ARCH_H__ */
