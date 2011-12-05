/*
 * Basic printf based on vprintf based on vsprintf
 *
 * Alessandro Rubini for CERN, 2011 -- public domain
 * (please note that the vsprintf is not public domain but GPL)
 */
#include <stdarg.h>
#include <pproto/pproto.h>

#define PP_BUF 128		/* We prefer small targets */

static char print_buf[PP_BUF];

int pp_vprintf(const char *fmt, va_list args)
{
	int ret;

	ret = pp_vsprintf(print_buf, fmt, args);
	pp_puts(print_buf);
	return ret;
}

int pp_printf(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = pp_vprintf(fmt, args);
	va_end(args);

	return r;
}
