/*
 * Copyright (C) 2012 CERN (www.cern.ch)
 * Author: Alessandro Rubini
 *
 * Released according to the GNU GPL, version 2 or any later version.
 */

/*
 * I used to have bit fields, but endianness detection failed. Let's be
 * honest: bit fields must die. I use masks at run time instead
 */

/* Common Message header (table 18, page 124) -- was "MsgHeader */
struct ptp_header {
	/* 0 */
	uint8_t		type_and_transport_specific; /* LSB = type */
	/* 1 */
	uint8_t		versionPTP_and_reserved; /* LSB = version */
	/* 2 */
	uint16_t	messageLength;
	/* 4 */
	uint8_t		domainNumber;
	uint8_t		reserved5;
	uint16_t	flagField;
	/* 8 */
	uint64_t	correctionField;
	/* 16 */
	uint32_t	reserved16;
	uint8_t		sourcePortIdentity[10];
	/* 30 */
	uint16_t	sequenceId;
	uint8_t		controlField;
	uint8_t		logMessageInterval;
} __attribute__((packed));

/* Fucking fucking ieee specification! */
struct int48 {
	uint16_t	msb; /* big endian in packet! */
	uint32_t	lsb;
} __attribute__((packed));

struct stamp {
	struct int48	sec;
	uint32_t	nsec;
} __attribute__((packed));

/*
 * What follows is structures for the individual message types
 */


struct ptp_announce { /* page 129 (149 of pdf) */
	struct stamp	originTimestamp;		/* 34 */
	Integer16	currentUtcOffset;		/* 44 */
	uint8_t		reserved46;
	UInteger8	grandmasterPriority1;		/* 47 */
	ClockQuality	grandmasterClockQuality;	/* 48 */
	UInteger8	grandmasterPriority2;		/* 52 */
	ClockIdentity	grandmasterIdentity;		/* 53 */
	UInteger16	stepsRemoved;			/* 61 */
	Enumeration8	timeSource;			/* 63 */
} __attribute__((packed));

struct ptp_sync_etc { /* page 130 (150 of pdf) */
	/* doc uses different names in different packets: use simple names */
	struct stamp	stamp;		/* 34 */
	uint8_t		port[10];	/* 44: only for delay_resp etc */
};

struct ptp_tlv { /* page 135 (155) */
	uint16_t	type;
	uint16_t	len;
	uint8_t		oui[3];
	uint8_t		subtype[3];
	uint8_t		data[0];
} __attribute__((packed));
