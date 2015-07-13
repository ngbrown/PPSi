#ifndef __ARCH_H__
#define __ARCH_H__

/* Architecture-specific defines, included by top-level stuff */

#include <WinSock2.h> /* ntohs etc */
#pragma comment(lib, "Ws2_32.lib")

#include <stdlib.h>    /* abs */

#define DEFAULT_TIME_OPS win32_time_ops
#define DEFAULT_NET_OPS win32_net_ops

#endif /* __ARCH_H__ */
