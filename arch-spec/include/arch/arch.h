#ifndef __ARCH_H__
#define __ARCH_H__

/* Architecture-specific defines, included by top-level stuff */

#define htons(x)      (x)
#define htonl(x)      (x)

#define ntohs htons
#define ntohl htonl

#define abs(x) ((x >= 0) ? x : -x)
#endif /* __ARCH_H__ */
