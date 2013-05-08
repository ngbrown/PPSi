/*
 * Alessandro Rubini for CERN, 2011 -- public domain
 */
#include <ppsi/ppsi.h>
#include "wrpc.h"
#include "uart.h" /* wrpc-sw */

void pp_puts(const char *s)
{
	uart_write_string(s);
}
