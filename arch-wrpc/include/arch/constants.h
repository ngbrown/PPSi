
#ifndef __PPSI_ARCH_CONSTANTS_H__
#define __PPSI_ARCH_CONSTANTS_H__

#ifndef __PPSI_CONSTANTS_H__
#Warning "Please include <ppsi/constants.h> before <arch/constants.h>"
#endif

#undef PP_DEFAULT_PROTO
#define PP_DEFAULT_PROTO PPSI_PROTO_RAW /* We only use raw ethernet */


#endif /* __PPSI_ARCH_CONSTANTS_H__ */
