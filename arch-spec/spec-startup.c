/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */
#include <syscon.h>
#include <uart.h>
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>
#include "../proto-ext-whiterabbit/wr-api.h" /* FIXME: ugly */
#include "spec.h"


static struct pp_instance ppi_static;
int pp_diag_verbosity = 0;

/*ppi fields*/
static UInteger16 sent_seq_id[16];
static DSDefault  defaultDS;
static DSCurrent  currentDS;
static DSParent   parentDS;
static DSPort     portDS;
static DSTimeProperties timePropertiesDS;
static struct pp_net_path net_path;
static struct pp_servo servo;
static struct pp_frgn_master frgn_master;

/* Calibration data (should be read from EEPROM, if available) */
#ifdef PPSI_MASTER
int32_t sfp_alpha = -73622176;
#else
int32_t sfp_alpha = 73622176;
#endif
int32_t sfp_deltaTx = 0;
int32_t sfp_deltaRx = 0;
uint32_t cal_phase_transition = 595; /* 7000 */

void ppsi_main(void)
{
	struct pp_instance *ppi = &ppi_static; /* no malloc, one instance */
	sdb_find_devices();
	uart_init();

	pp_puts("Spec: starting. Compiled on " __DATE__ "\n");

	/* leds are off and button is input */
	//gpio_dir(GPIO_PIN_BTN1, 0);
	//gpio_dir(GPIO_PIN_LED_LINK, 1);
	//gpio_dir(GPIO_PIN_LED_STATUS, 1);

	ppi->sent_seq_id = sent_seq_id;
	ppi->defaultDS   = &defaultDS;
	ppi->currentDS   = &currentDS;
	ppi->parentDS    = &parentDS;
	ppi->portDS      = &portDS;
	ppi->timePropertiesDS = &timePropertiesDS;
	ppi->net_path    = &net_path;
	ppi->servo       = &servo;
	ppi->frgn_master = &frgn_master;
	ppi->arch_data   = NULL;

	gpio_out(GPIO_PIN_LED_LINK, 0);
	gpio_out(GPIO_PIN_LED_STATUS, 0);

	if (spec_open_ch(ppi)) {
		pp_diag_error(ppi, spec_errno);
		pp_diag_fatal(ppi, "open_ch", "");
	}
	pp_open_instance(ppi, 0 /* no opts */);

#ifdef PPSI_SLAVE
	OPTS(ppi)->slave_only = 1;
	DSPOR(ppi)->wrConfig = WR_S_ONLY;
#endif

	spec_main_loop(ppi);
}


/* Our crt0.S is unchanged: it wants a "main" function, and "_irq_entry" too" */

int main(void) __attribute__((alias("ppsi_main")));
