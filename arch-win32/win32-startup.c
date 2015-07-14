
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

int main(int argc, char *argv[])
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	unsigned long seed;
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
	timePropertiesDS.currentUtcOffset = 36;	// TAI = UTC + ~36 seconds, must be manually set as Windows doesn't keep track

	if (pp_parse_cmdline(ppg, argc, argv) != 0)
		return -1;

	if (ppg->cfg.cfg_items == 0) {
		pp_config_file(ppg, 0, PP_DEFAULT_CONFIGFILE);
	}

	if (ppg->cfg.cfg_items == 0) {
		pp_config_string(ppg, _strdup("link 0; iface Local Area Connection; proto udp; role slave"));
		//pp_config_string(ppg, _strdup("link 0; iface Wireless Network Connection; proto udp; role slave"));
	}

	for (i = 0; i < ppg->nlinks; i++) {
		ppi = INST(ppg, i);
		ppi->ch[PP_NP_EVT].fd = -1;
		ppi->ch[PP_NP_GEN].fd = -1;

		ppi->glbs = ppg;
		ppi->vlans_array_len = CONFIG_VLAN_ARRAY_SIZE,
			ppi->iface_name = ppi->cfg.iface_name;
		ppi->port_name = ppi->cfg.port_name;

		/* The following default names depend on TIME= at build time */
		ppi->n_ops = &DEFAULT_NET_OPS;
		ppi->t_ops = &DEFAULT_TIME_OPS;

		ppi->portDS = calloc(1, sizeof(*ppi->portDS));
		ppi->__tx_buffer = malloc(PP_MAX_FRAME_LENGTH);
		ppi->__rx_buffer = malloc(PP_MAX_FRAME_LENGTH);

		if (!ppi->portDS || !ppi->__tx_buffer || !ppi->__rx_buffer) {
			fprintf(stderr, "ppsi: out of memory\n");
			exit(1);
		}
	}

	pp_init_globals(ppg, &__pp_default_rt_opts);

	seed = time(NULL);
	if (getenv("PPSI_DROP_SEED"))
		seed = atoi(getenv("PPSI_DROP_SEED"));
	ppsi_drop_init(ppg, seed);

	win32_main_loop(ppg);
	return 0; /* never reached */
}