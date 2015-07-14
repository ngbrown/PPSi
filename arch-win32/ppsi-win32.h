/*
* These are the functions provided by the various win32 files
*/

#include <Winsock2.h>
#undef TRUE
#undef FALSE

#define WIN32_ARCH(ppg) ((struct win32_arch_data *)(ppg->arch_data))
typedef struct win32_arch_data {
	struct timeval tv;
};

void win32_main_loop(struct pp_globals *ppg);

