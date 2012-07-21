/*
 * Aurelio Colosimo for CERN, 2012 -- public domain
 */

#include <stdint.h>
#include <ppsi/ppsi.h>
#include "dev/softpll_ng.h"
#include "../proto-ext-whiterabbit/wr-constants.h"

int spec_spll_locking_enable(struct pp_instance *ppi)
{
	spll_init(SPLL_MODE_SLAVE, 0, 1);
	spll_enable_ptracker(0, 1);
	return WR_SPLL_OK;
}

int spec_spll_locking_poll(struct pp_instance *ppi)
{
	return spll_check_lock(0) ? WR_SPLL_READY : WR_SPLL_ERROR;
}

int spec_spll_locking_disable(struct pp_instance *ppi)
{
	/* softpll_disable(); */
	return WR_SPLL_OK;
}

int wr_locking_enable(struct pp_instance *ppi)
	__attribute__((alias("spec_spll_locking_enable")));

int wr_locking_poll(struct pp_instance *ppi)
	__attribute__((alias("spec_spll_locking_poll")));

int wr_locking_disable(struct pp_instance *ppi)
	__attribute__((alias("spec_spll_locking_disable")));
