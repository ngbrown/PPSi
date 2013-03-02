/*
 * Alessandro Rubini for CERN, 2013 -- GNU LGPL v2.1 or later
 */
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include <ppsi/diag-macros.h>

#define N(n) [n] = #n

static char *timeout_names[__PP_TO_ARRAY_SIZE] __attribute__((used)) = {
	N(PP_TO_DELAYREQ),
	N(PP_TO_SYNC),
	N(PP_TO_ANN_RECEIPT),
	N(PP_TO_ANN_INTERVAL),
	N(PP_TO_EXT_0),
	N(PP_TO_EXT_1),
	N(PP_TO_EXT_2),
	N(PP_TO_EXT_3),
};

/*
 * Log means messages
 */
void pp_timeout_log(struct pp_instance *ppi, int index)
{
	PP_VPRINTF("timeout expire event: %s\n", timeout_names[index]);
}

