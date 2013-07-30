/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released to the public domain
 */
#include <ppsi/ppsi.h>
#include "wrpc.h"
#include "uart.h" /* wrpc-sw */

void pp_puts(const char *s)
{
	uart_write_string(s);
}
