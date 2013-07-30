/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */
#include <ppsi/ppsi.h>
#include "bare-linux.h"

void pp_puts(const char *s)
{
	sys_write(0, s, strnlen(s, 300));
}
