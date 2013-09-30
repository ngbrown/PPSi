/*
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Aurelio Colosimo
 *
 * Released to the public domain
 */

/* Structs defined in IEEE Std 1588-2008 */

#ifndef __PPSI_IEEE_1588_TYPES_H__
#define __PPSI_IEEE_1588_TYPES_H__

#include <stdint.h>

/* See F.2, pag.223 */
#define PP_ETHERTYPE	0x88f7

typedef enum {FALSE=0, TRUE} Boolean;
typedef uint8_t		Octet;
typedef int8_t		Integer8;
typedef int16_t		Integer16;
typedef int32_t		Integer32;
typedef uint8_t		UInteger8;
typedef uint16_t	UInteger16;
typedef uint32_t	UInteger32;
/* Enumerations are unsigned, see 5.4.2, page 15 */
typedef uint16_t	Enumeration16;
typedef uint8_t		Enumeration8;
typedef uint8_t		Enumeration4;
typedef uint8_t		UInteger4;
typedef uint8_t		Nibble;

/* FIXME: each struct must be aligned for lower memory usage */

typedef struct UInteger48 {
	uint32_t	lsb;
	uint16_t	msb;
} UInteger48;

typedef struct Integer64 {
	uint32_t	lsb;
	int32_t		msb;
} Integer64;

typedef struct UInteger64 {
	uint32_t	lsb;
	uint32_t		msb;
} UInteger64;

struct TimeInterval { /* page 12 (32) -- never used */
	Integer64	scaledNanoseconds;
};

/* White Rabbit extension */
typedef struct FixedDelta {
	UInteger64 scaledPicoseconds;
} FixedDelta;

typedef struct Timestamp { /* page 13 (33) -- no typedef expected */
	UInteger48	secondsField;
	UInteger32	nanosecondsField;
} Timestamp;

typedef struct TimeInternal {
	Integer32	seconds;
	Integer32	nanoseconds;
	/* White Rabbit extension begin */
	Integer32	phase;		/* This is the set point */
	int 		correct;	/* 0 or 1 */
#if 0
	/*
	 * The following two fields may be used for diagnostics, but
	 * they cost space. So remove them but keep the code around just
	 * in case it is useful again (they are only set, never read)
	 */
	int32_t raw_phase;
	int32_t raw_nsec;
#endif
	int32_t raw_ahead;	/* raw_ahead is used during calibration */
	/* White Rabbit extension end */
} TimeInternal;

static inline void clear_TimeInternal(struct TimeInternal *t)
{
	memset(t, 0, sizeof(*t));
}

typedef struct ClockIdentity { /* page 13 (33) */
	Octet id[8];
} ClockIdentity;
#define PP_CLOCK_IDENTITY_LENGTH	sizeof(ClockIdentity)

typedef struct PortIdentity { /* page 13 (33) */
	ClockIdentity	clockIdentity;
	UInteger16	portNumber;
} PortIdentity;

typedef struct PortAdress { /* page 13 (33) */
	Enumeration16	networkProtocol;
	UInteger16	adressLength;
	Octet		*adressField;
} PortAdress;

typedef struct ClockQuality { /* page 14 (34) */
	UInteger8	clockClass;
	Enumeration8	clockAccuracy;
	UInteger16	offsetScaledLogVariance;
} ClockQuality;

struct TLV { /* page 14 (34) -- never used */
	Enumeration16	tlvType;
	UInteger16	lengthField;
	Octet		*valueField;
};

struct PTPText { /* page 14 (34) -- never used */
	UInteger8	lengthField;
	Octet		*textField;
};

struct FaultRecord { /* page 14 (34) -- never used */
	UInteger16	faultRecordLength;
	Timestamp	faultTime;
	Enumeration8	severityCode;
	struct PTPText	faultName;
	struct PTPText	faultValue;
	struct PTPText	faultDescription;
};


/* Common Message header (table 18, page 124) */
typedef struct MsgHeader {
	Nibble		transportSpecific;
	Enumeration4	messageType;
	UInteger4	versionPTP;
	UInteger16	messageLength;
	UInteger8	domainNumber;
	Octet		flagField[2];
	Integer64	correctionfield;
	PortIdentity	sourcePortIdentity;
	UInteger16	sequenceId;
	UInteger8	controlField;
	Integer8	logMessageInterval;
} MsgHeader;

/* Announce Message (table 25, page 129) */
typedef struct MsgAnnounce {
	Timestamp	originTimestamp;
	Integer16	currentUtcOffset;
	UInteger8	grandmasterPriority1;
	ClockQuality	grandmasterClockQuality;
	UInteger8	grandmasterPriority2;
	ClockIdentity	grandmasterIdentity;
	UInteger16	stepsRemoved;
	Enumeration8	timeSource;
	unsigned long	ext_specific;	/* used by extension */
} MsgAnnounce;

/* Sync Message (table 26, page 129) */
typedef struct MsgSync {
	Timestamp originTimestamp;
} MsgSync;

/* DelayReq Message (table 26, page 129) */
typedef struct MsgDelayReq {
	Timestamp	originTimestamp;
} MsgDelayReq;

/* DelayResp Message (table 27, page 130) */
typedef struct MsgFollowUp {
	Timestamp	preciseOriginTimestamp;
} MsgFollowUp;


/* DelayResp Message (table 28, page 130) */
typedef struct MsgDelayResp {
	Timestamp	receiveTimestamp;
	PortIdentity	requestingPortIdentity;
} MsgDelayResp;

/* PdelayReq Message (table 29, page 131) -- not used in ppsi */
struct MsgPDelayReq {
	Timestamp	originTimestamp;

};

/* PdelayResp Message (table 30, page 131) -- not used in ppsi */
struct MsgPDelayResp {
	Timestamp	requestReceiptTimestamp;
	PortIdentity	requestingPortIdentity;
};

/* PdelayRespFollowUp Message (table 31, page 132) -- not used in ppsi */
struct MsgPDelayRespFollowUp {
	Timestamp	responseOriginTimestamp;
	PortIdentity	requestingPortIdentity;
};

/* Signaling Message (table 33, page 133) */
typedef struct MsgSignaling {
	PortIdentity	targetPortIdentity;
	char		*tlv;
} MsgSignaling;

/* Management Message (table 37, page 137) */
typedef struct MsgManagement{
	PortIdentity	targetPortIdentity;
	UInteger8	startingBoundaryHops;
	UInteger8	boundaryHops;
	Enumeration4	actionField;
	char		*tlv;
} MsgManagement;

/* Default Data Set */
typedef struct DSDefault {		/* page 65 */
	/* Static */
	Boolean		twoStepFlag;
	ClockIdentity	clockIdentity;
	UInteger16	numberPorts;
	/* Dynamic */
	ClockQuality	clockQuality;
	/* Configurable */
	UInteger8	priority1;
	UInteger8	priority2;
	UInteger8	domainNumber;
	Boolean		slaveOnly;
} DSDefault;

/* Current Data Set */
typedef struct DSCurrent {		/* page 67 */
	/* Dynamic */
	UInteger16	stepsRemoved;
	TimeInternal	offsetFromMaster;
	TimeInternal	meanPathDelay; /* oneWayDelay */
	/* White Rabbit extension begin */
	UInteger16	primarySlavePortNumber;
	/* White Rabbit extension end */
} DSCurrent;

/* Parent Data Set */
typedef struct DSParent {		/* page 68 */
	/* Dynamic */
	PortIdentity	parentPortIdentity;
	/* Boolean		parentStats; -- not used */
	UInteger16	observedParentOffsetScaledLogVariance;
	Integer32	observedParentClockPhaseChangeRate;
	ClockIdentity	grandmasterIdentity;
	ClockQuality	grandmasterClockQuality;
	UInteger8	grandmasterPriority1;
	UInteger8	grandmasterPriority2;
} DSParent;

/* Port Data set */
typedef struct DSPort {			/* page 72 */
	/* Static */
	PortIdentity	portIdentity;
	/* Dynamic */
	/* Enumeration8	portState; -- not used */
	Integer8	logMinDelayReqInterval; /* note: never changed */
	/* TimeInternal	peerMeanPathDelay; -- not used */
	/* Configurable */
	Integer8	logAnnounceInterval;
	UInteger8	announceReceiptTimeout;
	Integer8	logSyncInterval;
	/* Enumeration8	delayMechanism; -- not used */
	UInteger4	versionNumber;

	void		*ext_dsport;
} DSPort;

/* Time Properties Data Set */
typedef struct DSTimeProperties {	/* page 70 */
	/* Dynamic */
	Integer16	currentUtcOffset;
	Boolean		currentUtcOffsetValid;
	Boolean		leap59;
	Boolean		leap61;
	Boolean		timeTraceable;
	Boolean		frequencyTraceable;
	Boolean		ptpTimescale;
	Enumeration8	timeSource;
} DSTimeProperties;

/* Enumeration States (table 8, page 73) */
enum pp_std_states {
	PPS_END_OF_TABLE	= 0,
	PPS_INITIALIZING,
	PPS_FAULTY,
	PPS_DISABLED,
	PPS_LISTENING,
	PPS_PRE_MASTER,
	PPS_MASTER,
	PPS_PASSIVE,
	PPS_UNCALIBRATED,
	PPS_SLAVE,
};

enum pp_std_messages {
	PPM_SYNC		= 0x0,
	PPM_DELAY_REQ,
	PPM_PDELAY_REQ,
	PPM_PDELAY_RESP,
	PPM_FOLLOW_UP		= 0x8,
	PPM_DELAY_RESP,
	PPM_PDELAY_RESP_FOLLOW_UP,
	PPM_ANNOUNCE,
	PPM_SIGNALING,
	PPM_MANAGEMENT,
	__PP_NR_MESSAGES_TYPES,

	PPM_NOTHING_TO_DO	= 0x100, /* for hooks.master_msg() */
};

extern const char const * pp_msg_names[];

/* Enumeration Domain Number (table 2, page 41) */
enum ENDomainNumber {
	DFLT_DOMAIN_NUMBER	= 0,
	ALT1_DOMAIN_NUMBER,
	ALT2_DOMAIN_NUMBER,
	ALT3_DOMAIN_NUMBER
};

/* Enumeration Network Protocol (table 3, page 46) */
enum ENNetworkProtocol {
	UDP_IPV4	= 1,
	UDP_IPV6,
	IEEE_802_3,
	DeviceNet,
	ControlNet,
	PROFINET
};

/* Enumeration Time Source (table 7, page 57) */
enum ENTimeSource {
	ATOMIC_CLOCK		= 0x10,
	GPS			= 0x20,
	TERRESTRIAL_RADIO	= 0x30,
	PTP			= 0x40,
	NTP			= 0x50,
	HAND_SET		= 0x60,
	OTHER			= 0x90,
	INTERNAL_OSCILLATOR	= 0xA0
};

/* Enumeration Delay mechanism (table 9, page 74) */
enum ENDelayMechanism {
	E2E		= 1,
	P2P		= 2,
	DELAY_DISABLED	= 0xFE
};

#endif /* __PPSI_IEEE_1588_TYPES_H__ */
