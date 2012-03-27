#ifndef __MINIC_H
#define __MINIC_H

#include "inttypes.h"

#define ETH_HEADER_SIZE 14
#define ETH_ALEN 6
#define ETH_P_1588	0x88F7		/* IEEE 1588 Timesync */

struct ethhdr {
	unsigned char	h_dest[ETH_ALEN];	/* destination eth addr	*/
	unsigned char	h_source[ETH_ALEN];	/* source ether addr	*/
	uint16_t h_proto;		/* packet type ID field	*/
} __attribute__((packed));

void minic_init();
void minic_disable();
int minic_poll_rx();
void minic_get_stats(int *tx_frames, int *rx_frames);

int minic_rx_frame(uint8_t *hdr, uint8_t *payload, uint32_t buf_size, struct hw_timestamp *hwts);
int minic_tx_frame(uint8_t *hdr, uint8_t *payload, uint32_t size, struct hw_timestamp *hwts);



#endif
