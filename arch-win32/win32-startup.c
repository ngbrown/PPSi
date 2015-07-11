
#include <ppsi/ppsi.h>
#include "ppsi-win32.h"

#include <stdio.h>

#pragma comment(linker, "/alternatename:_pp_printf=_printf")
#pragma comment(linker, "/alternatename:_pp_vprintf=_vprintf")
#pragma comment(linker, "/alternatename:_pp_sprintf=_sprintf")
#pragma comment(linker, "/alternatename:_pp_vsprintf=_vsprintf")

/* ppg and fields */
static struct pp_globals ppg_static;
static DSDefault defaultDS;
static DSCurrent currentDS;
static DSParent parentDS;
static DSTimeProperties timePropertiesDS;
static struct pp_servo servo;

int main(int argc, char **argv)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	int i;

	pp_printf("PPSi. Built on " __DATE__ "\n");

	ppg = &ppg_static;
	ppg->defaultDS = &defaultDS;
	ppg->currentDS = &currentDS;
	ppg->parentDS = &parentDS;
	ppg->timePropertiesDS = &timePropertiesDS;
	ppg->servo = &servo;
	ppg->rt_opts = &__pp_default_rt_opts;

	/* We are hosted, so we can allocate */
	ppg->max_links = PP_MAX_LINKS;
	ppg->arch_data = calloc(1, sizeof(struct win32_arch_data));
	ppg->pp_instances = calloc(ppg->max_links, sizeof(struct pp_instance));

	if ((!ppg->arch_data) || (!ppg->pp_instances)) {
		fprintf(stderr, "ppsi: out of memory\n");
		exit(1);
	}

	/* Before the configuration is parsed, set defaults */
	for (i = 0; i < ppg->max_links; i++) {
		ppi = INST(ppg, i);
		ppi->proto = PP_DEFAULT_PROTO;
		ppi->role = PP_DEFAULT_ROLE;
	}

	/* Set offset here, so config parsing can override it */
	// TODO
}