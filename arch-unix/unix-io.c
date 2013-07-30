/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */
#include <stdio.h>
#include <ppsi/ppsi.h>

void pp_puts(const char *s)
{
	fputs(s, stdout);
}
