#ifndef __ARCH_H__
#define __ARCH_H__

/* This arch exports wr functions, so include this for consistency checking */
#include "../proto-ext-whiterabbit/wr-api.h"

/* Architecture-specific defines, included by top-level stuff */

#define htons(x)      (x)
#define htonl(x)      (x)

#define ntohs htons
#define ntohl htonl

#define abs(x) ((x >= 0) ? x : -x)
#endif /* __ARCH_H__ */
