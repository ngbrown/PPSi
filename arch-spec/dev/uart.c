/*
 * Copyright 2011 Tomasz Wlostowski <tomasz.wlostowski@cern.ch> for CERN
 * Modified by Alessandro Rubini for ptp-proposal/proto
 *
 * GNU LGPL 2.1 or later versions
 */
#include <stdint.h>
#include "../spec.h"

#include <hw/wb_uart.h>

#define CALC_BAUD(baudrate) \
	( ((( (unsigned long long)baudrate * 8ULL) << (16 - 7)) +	\
	   (CPU_CLOCK >> 8)) / (CPU_CLOCK >> 7) )

static volatile struct UART_WB *uart;

void spec_uart_init(void)
{
	uart = (volatile struct UART_WB *) BASE_UART;
	uart->BCR = CALC_BAUD(UART_BAUDRATE);
}

void spec_putc(int c)
{
	if(c == '\n')
		spec_putc('\r');
	while(uart->SR & UART_SR_TX_BUSY)
		;
	uart->TDR = c;
}

void spec_puts(const char *s)
{
	while (*s)
		spec_putc(*(s++));
}

int spec_testc(void)
{
	return uart->SR & UART_SR_RX_RDY;
}

int spec_uart_poll()
{
 	return uart->SR & UART_SR_RX_RDY;
}

int spec_getc(void)
{
	if(!spec_uart_poll())
		return -1;

	return uart ->RDR & 0xff;
}
