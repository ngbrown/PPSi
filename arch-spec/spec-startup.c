/*
 * Alessandro Rubini for CERN, 2011 -- GNU LGPL v2.1 or later
 */
#include <pptp/pptp.h>
#include "spec.h"
#include "include/gpio.h"

static struct pp_instance ppi_static;

void pptp_main(void)
{
	struct pp_instance *ppi = &ppi_static; /* no malloc, one instance */

	spec_uart_init();

	pp_puts("Spec: starting. Compiled on " __DATE__ "\n");

	/* leds are off and button is input */
	gpio_dir(GPIO_PIN_BTN1, 0);
	gpio_dir(GPIO_PIN_LED_LINK, 1);
	gpio_dir(GPIO_PIN_LED_STATUS, 1);

	gpio_out(GPIO_PIN_LED_LINK, 0);
	gpio_out(GPIO_PIN_LED_STATUS, 0);

	if (spec_open_ch(ppi)) {
		pp_diag_error(ppi, spec_errno);
		pp_diag_fatal(ppi, "open_ch", "");
	}
	pp_open_instance(ppi);

	spec_main_loop(ppi);
}


/* Our crt0.S is unchanged: it wants a "main" function, and "_irq_entry" too" */

int main(void) __attribute__((alias("pptp_main")));

void _irq_entry(void)
{
	return;
}

