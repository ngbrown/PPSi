/*
 * Alessandro Rubini for CERN, 2013 -- GNU LGPL v2.1 or later
 */
#include <stdarg.h>
#include <ppsi/ppsi.h>

static char *thing_name[] = {
	[pp_dt_fsm]	= "diag-fsm",
	[pp_dt_time]	= "diag-time",
	[pp_dt_frames]	= "diag-frames",
	[pp_dt_servo]	= "diag-servo",
	[pp_dt_bmc]	= "diag-bmc",
	[pp_dt_ext]	= "diag-extension",
};


void __pp_diag(struct pp_instance *ppi, enum pp_diag_things th,
		      int level, char *fmt, ...)
{
	va_list args;

	if (!__PP_DIAG_ALLOW(ppi, th, level))
		return;

	pp_printf("%s-%i-%s: ", thing_name[th], level, OPTS(ppi)->iface_name);
	va_start(args, fmt);
	pp_vprintf(fmt, args);
	va_end(args);
}

unsigned long pp_diag_parse(char *diaglevel)
{
	unsigned long res = 0;
	int i = 28;

#ifdef VERB_LOG_MSGS /* compatible with older way: this enables all */
	return ~0xf;
#endif

	while (*diaglevel && i >= 4) {
		if (*diaglevel < '0' || *diaglevel > '3')
			break;
		res |= ((*diaglevel - '0') << i);
		i -= 4;
		diaglevel++;
	}
	if (*diaglevel)
		pp_printf("%s: error parsing \"%s\"\n", __func__, diaglevel);

	return res;
}
