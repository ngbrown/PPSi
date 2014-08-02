
/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 * Based on ptp-noposix project (see AUTHORS for details)
 *
 * Released according to the GNU LGPL, version 2.1 or any later version.
 */

#include <ppsi/ppsi.h>
#include "wr-api.h"
#include "../proto-standard/common-fun.h"

/*
 * WR way to handle little/big endianness
 * FIXME: check whether htons would be ok
 */
/* put 16 bit word in big endianess (from little endianess) */
static inline void put_be16(void *ptr, UInteger16 x)
{
	*(unsigned char *)(ptr++) = (x >> 8) & 0xff;
	*(unsigned char *)(ptr++) = (x) & 0xff;
}
/* put 32 bit word in big endianess (from little endianess) */
static inline void put_be32(void *ptr, UInteger32 x)
{
	*(unsigned char *)(ptr++) = (x >> 24) & 0xff;
	*(unsigned char *)(ptr++) = (x >> 16) & 0xff;
	*(unsigned char *)(ptr++) = (x >> 8) & 0xff;
	*(unsigned char *)(ptr++) = (x) & 0xff;
}

/* gets 32 bit word from big endianness (to little endianness) */
static inline UInteger32 get_be32(void *ptr)
{
	UInteger32 res = 0x0;

	res = res | ((*(unsigned char *)(ptr++) << 24) & 0xFF000000);
	res = res | ((*(unsigned char *)(ptr++) << 16) & 0x00FF0000);
	res = res | ((*(unsigned char *)(ptr++) << 8 ) & 0x0000FF00);
	res = res | ((*(unsigned char *)(ptr++) << 0 ) & 0x000000FF);
	return res;
}

/* gets 16 bit word from big endianess (to little endianess) */
static inline UInteger16 get_be16(void *ptr)
{
	UInteger16 res = 0x0;

	res = res | ((*(unsigned char *)(ptr++) << 8 ) & 0xFF00);
	res = res | ((*(unsigned char *)(ptr++) << 0 ) & 0x000FF);
	return res;
}

/* Pack White rabbit message in the suffix of PTP announce message */
void msg_pack_announce_wr_tlv(struct pp_instance *ppi)
{
	void *buf;
	UInteger16 wr_flags = 0;
	int locked, class = DSDEF(ppi)->clockQuality.clockClass;
	struct wr_dsport *wrp = WR_DSPOR(ppi);

	buf = ppi->tx_ptp;

	/* GM: update clock Class, according to whether we are locked or not */
	if (class < PP_CLASS_DEFAULT) {
		locked = wrp->ops->locking_poll(ppi, 1);
		if (locked > 0)
			class = PP_CLASS_WR_GM_LOCKED;
		else
			class = PP_CLASS_WR_GM_UNLOCKED;
		if (class != DSDEF(ppi)->clockQuality.clockClass) {
			pp_error("New class %i\n", class);
			DSDEF(ppi)->clockQuality.clockClass = class;
			*(UInteger8 *) (buf + 48) = class;
		}
	}

	/* Change length */
	*(UInteger16 *)(buf + 2) = htons(WR_ANNOUNCE_LENGTH);

	*(UInteger16 *)(buf + 64) = htons(TLV_TYPE_ORG_EXTENSION);
	*(UInteger16 *)(buf + 66) = htons(WR_ANNOUNCE_TLV_LENGTH);
	/* CERN's OUI: WR_TLV_ORGANIZATION_ID, how to flip bits? */
	*(UInteger16 *)(buf + 68) = htons((WR_TLV_ORGANIZATION_ID >> 8));
	*(UInteger16 *)(buf + 70) = htons((0xFFFF & (WR_TLV_ORGANIZATION_ID << 8
		| WR_TLV_MAGIC_NUMBER >> 8)));
	*(UInteger16 *)(buf + 72) = htons((0xFFFF & (WR_TLV_MAGIC_NUMBER << 8
		| WR_TLV_WR_VERSION_NUMBER)));
	/* wrMessageId */
	*(UInteger16 *)(buf + 74) = htons(ANN_SUFIX);
	wr_flags = wr_flags | WR_DSPOR(ppi)->wrConfig;

	if (WR_DSPOR(ppi)->calibrated)
		wr_flags = WR_IS_CALIBRATED | wr_flags;

	if (WR_DSPOR(ppi)->wrModeOn)
		wr_flags = WR_IS_WR_MODE | wr_flags;
	*(UInteger16 *)(buf + 76) = htons(wr_flags);
}

void msg_unpack_announce_wr_tlv(void *buf, MsgAnnounce *ann)
{
	UInteger16 tlv_type;
	UInteger32 tlv_organizationID;
	UInteger16 tlv_magicNumber;
	UInteger16 tlv_versionNumber;
	UInteger16 tlv_wrMessageID;

	tlv_type = (UInteger16)get_be16(buf+64);
	tlv_organizationID = htons(*(UInteger16 *)(buf+68)) << 8;
	tlv_organizationID = htons(*(UInteger16 *)(buf+70)) >> 8
		| tlv_organizationID;
	tlv_magicNumber = 0xFF00 & (htons(*(UInteger16 *)(buf+70)) << 8);
	tlv_magicNumber = htons(*(UInteger16 *)(buf+72)) >> 8
		| tlv_magicNumber;
	tlv_versionNumber = 0xFF & htons(*(UInteger16 *)(buf+72));
	tlv_wrMessageID = htons(*(UInteger16 *)(buf+74));

	if (tlv_type == TLV_TYPE_ORG_EXTENSION &&
		tlv_organizationID == WR_TLV_ORGANIZATION_ID &&
		tlv_magicNumber == WR_TLV_MAGIC_NUMBER &&
		tlv_versionNumber == WR_TLV_WR_VERSION_NUMBER &&
		tlv_wrMessageID == ANN_SUFIX) {
		ann->ext_specific = (UInteger16)get_be16(buf+76);
	}
}

/* White Rabbit: packing WR Signaling messages*/
int msg_pack_wrsig(struct pp_instance *ppi, Enumeration16 wr_msg_id)
{
	void *buf;
	UInteger16 len = 0;

	if ((WR_DSPOR(ppi)->wrMode == NON_WR) || (wr_msg_id == ANN_SUFIX)) {
		pp_diag(ppi, frames, 1,
			  "BUG: Trying to send invalid wr_msg mode=%x id=%x",
			  WR_DSPOR(ppi)->wrMode, wr_msg_id);
		return 0;
	}

	buf = ppi->tx_ptp;

	/* Changes in header */
	*(char *)(buf+0) = *(char *)(buf+0) & 0xF0; /* RAZ messageType */
	*(char *)(buf+0) = *(char *)(buf+0) | 0x0C; /* Table 19 -> signaling */

	*(UInteger8 *)(buf+32) = 0x05; //Table 23 -> all other

	/* target portIdentity */
	memcpy((buf+34), &DSPAR(ppi)->parentPortIdentity.clockIdentity,
		PP_CLOCK_IDENTITY_LENGTH);
	put_be16(buf + 42, DSPAR(ppi)->parentPortIdentity.portNumber);

	/* WR TLV */
	*(UInteger16 *)(buf+44) = htons(TLV_TYPE_ORG_EXTENSION);
	/* leave lenght free */
	*(UInteger16 *)(buf+48) = htons((WR_TLV_ORGANIZATION_ID >> 8));
	*(UInteger16 *)(buf+50) = htons((0xFFFF &
		(WR_TLV_ORGANIZATION_ID << 8 | WR_TLV_MAGIC_NUMBER >> 8)));
	*(UInteger16 *)(buf+52) = htons((0xFFFF &
		(WR_TLV_MAGIC_NUMBER    << 8 | WR_TLV_WR_VERSION_NUMBER)));
	/* wrMessageId */
	*(UInteger16 *)(buf+54) = htons(wr_msg_id);

	switch (wr_msg_id) {
	case CALIBRATE:
		if (WR_DSPOR(ppi)->calibrated) {
			put_be16(buf+56,
				 (WR_DSPOR(ppi)->calRetry << 8 | 0x0000));
		} else {
			put_be16(buf+56,
				 (WR_DSPOR(ppi)->calRetry << 8 | 0x0001));
		}
		put_be32(buf+58, WR_DSPOR(ppi)->calPeriod);
		len = 20;
		break;

	case CALIBRATED: /* new fsm */
		/* delta TX */
		put_be32(buf+56, WR_DSPOR(ppi)->deltaTx.scaledPicoseconds.msb);
		put_be32(buf+60, WR_DSPOR(ppi)->deltaTx.scaledPicoseconds.lsb);

		/* delta RX */
		put_be32(buf+64, WR_DSPOR(ppi)->deltaRx.scaledPicoseconds.msb);
		put_be32(buf+68, WR_DSPOR(ppi)->deltaRx.scaledPicoseconds.lsb);
		len = 24;

		break;

	default:
		/* only WR TLV "header" and wrMessageID */
		len = 8;
		break;
	}
	/* header len */
	put_be16(buf + 2, WR_SIGNALING_MSG_BASE_LENGTH + len);
	/* TLV len */
	*(Integer16 *)(buf+46) = htons(len);

	/* FIXME diagnostic */
	return WR_SIGNALING_MSG_BASE_LENGTH + len;
}

/* White Rabbit: unpacking wr signaling messages */
void msg_unpack_wrsig(struct pp_instance *ppi, void *buf,
		      MsgSignaling *wrsig_msg, Enumeration16 *pwr_msg_id)
{
	UInteger16 tlv_type;
	UInteger32 tlv_organizationID;
	UInteger16 tlv_magicNumber;
	UInteger16 tlv_versionNumber;
	Enumeration16 wr_msg_id;

	memcpy(&wrsig_msg->targetPortIdentity.clockIdentity, (buf + 34),
	       PP_CLOCK_IDENTITY_LENGTH);
	wrsig_msg->targetPortIdentity.portNumber = (UInteger16)get_be16(buf+42);

	tlv_type           = (UInteger16)get_be16(buf + 44);
	tlv_organizationID = htons(*(UInteger16 *)(buf + 48)) << 8;
	tlv_organizationID = htons(*(UInteger16 *)(buf + 50)) >> 8
				| tlv_organizationID;
	tlv_magicNumber = 0xFF00 & (htons(*(UInteger16 *)(buf + 50)) << 8);
	tlv_magicNumber = htons(*(UInteger16 *)(buf + 52)) >>  8
				| tlv_magicNumber;
	tlv_versionNumber = 0xFF & htons(*(UInteger16 *)(buf + 52));

	if (tlv_type != TLV_TYPE_ORG_EXTENSION) {
		pp_diag(ppi, frames, 1, "handle Signaling msg, failed, This is not "
			"organization extension TLV = 0x%x\n", tlv_type);
		return;
	}

	if (tlv_organizationID != WR_TLV_ORGANIZATION_ID) {
		pp_diag(ppi, frames, 1, "handle Signaling msg, failed, not CERN's "
			"OUI = 0x%x\n", tlv_organizationID);
		return;
	}

	if (tlv_magicNumber != WR_TLV_MAGIC_NUMBER) {
		pp_diag(ppi, frames, 1, "handle Signaling msg, failed, "
		"not White Rabbit magic number = 0x%x\n", tlv_magicNumber);
		return;
	}

	if (tlv_versionNumber  != WR_TLV_WR_VERSION_NUMBER ) {
		pp_diag(ppi, frames, 1, "handle Signaling msg, failed, not supported "
			"version number = 0x%x\n", tlv_versionNumber);
		return;
	}

	wr_msg_id = htons(*(UInteger16 *)(buf + 54));

	if (pwr_msg_id) {
		*pwr_msg_id = wr_msg_id;
	}

	switch (wr_msg_id) {
	case CALIBRATE:
		WR_DSPOR(ppi)->otherNodeCalSendPattern =
			0x00FF & get_be16(buf+56);
		WR_DSPOR(ppi)->otherNodeCalRetry =
			0x00FF & (get_be16(buf+56) >> 8);
		WR_DSPOR(ppi)->otherNodeCalPeriod = get_be32(buf+58);
		break;

	case CALIBRATED:
		/* delta TX */
		WR_DSPOR(ppi)->otherNodeDeltaTx.scaledPicoseconds.msb =
			get_be32(buf+56);
		WR_DSPOR(ppi)->otherNodeDeltaTx.scaledPicoseconds.lsb =
			get_be32(buf+60);

		/* delta RX */
		WR_DSPOR(ppi)->otherNodeDeltaRx.scaledPicoseconds.msb =
			get_be32(buf+64);
		WR_DSPOR(ppi)->otherNodeDeltaRx.scaledPicoseconds.lsb =
			get_be32(buf+68);
		break;

	default:
		/* no data */
		break;
	}
	/* FIXME diagnostic */
}

/* Pack and send a White Rabbit signalling message */
int msg_issue_wrsig(struct pp_instance *ppi, Enumeration16 wr_msg_id)
{
	int len = msg_pack_wrsig(ppi, wr_msg_id);

	return __send_and_log(ppi, len, PPM_SIGNALING, PP_NP_GEN);
}
