/*
* These are the functions provided by the various win32 files
*/

#include <Windows.h>
#undef TRUE
#undef FALSE

#define WIN32_ARCH(ppg) ((struct win32_arch_data *)(ppg->arch_data))
struct win32_arch_data {
	FILETIME time;
};

void win32_main_loop(struct pp_globals *ppg);

