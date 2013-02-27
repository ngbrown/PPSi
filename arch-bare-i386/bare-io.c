/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <ppsi/ppsi.h>
#include "bare-i386.h"

void pp_puts(const char *s)
{
	sys_write(0, s, strnlen(s, 300));
}

