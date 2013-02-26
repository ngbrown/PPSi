/*
 * Aurelio Colosimo for CERN, 2011 -- public domain
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

#define PP_CLOCK_IDENTITY_LENGTH 8

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

typedef struct TimeInterval {
	Integer64	scaledNanoseconds;
} TimeInterval;

/* White Rabbit extension */
typedef struct FixedDelta {
	UInteger64 scaledPicoseconds;
} FixedDelta;

typedef struct  {
	UInteger48	secondsField;
	UInteger32	nanosecondsField;
} Timestamp;

typedef struct TimeInternal {
	Integer32	seconds;
	Integer32	nanoseconds;
	/* White Rabbit extension begin */
	Integer32	phase;
	int 		correct;
	/* raw time (non-linearized) for debugging purposes */
	int32_t raw_phase;
	int32_t raw_nsec;
	int32_t raw_ahead;
	/* White Rabbit extension end */
} TimeInternal;

typedef Octet		ClockIdentity[PP_CLOCK_IDENTITY_LENGTH];

typedef struct PortIdentity {
	ClockIdentity	clockIdentity;
	UInteger16	portNumber;
} PortIdentity;

typedef struct PortAdress {
	Enumeration16	networkProtocol;
	UInteger16	adressLength;
	Octet		*adressField;
} PortAdress;

typedef struct ClockQuality {
	UInteger8	clockClass;
	Enumeration8	clockAccuracy;
	UInteger16	offsetScaledLogVariance;
} ClockQuality;

typedef struct TLV {
	Enumeration16	tlvType;
	UInteger16	lengthField;
	Octet		*valueField;
} TLV;

typedef struct PTPText {
	UInteger8	lengthField;
	Octet		*textField;
} PTPText;

typedef struct FaultRecord {
	UInteger16	faultRecordLength;
	Timestamp	faultTime;
	Enumeration8	severityCode;
	PTPText		faultName;
	PTPText		faultValue;
	PTPText		faultDescription;
} FaultRecord;


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

	/* White Rabbit extension begin */
	UInteger8 wrFlags;
	/* White Rabbit extension end */
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

/* PdelayReq Message (table 29, page 131) */
typedef struct MsgPDelayReq {
	Timestamp	originTimestamp;

} MsgPDelayReq;

/* PdelayResp Message (table 30, page 131) */
typedef struct MsgPDelayResp {
	Timestamp	requestReceiptTimestamp;
	PortIdentity	requestingPortIdentity;
} MsgPDelayResp;

/* PdelayRespFollowUp Message (table 31, page 132) */
typedef struct MsgPDelayRespFollowUp {
	Timestamp	responseOriginTimestamp;
	PortIdentity	requestingPortIdentity;
} MsgPDelayRespFollowUp;

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
typedef struct DSDefault {
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
typedef struct DSCurrent {
	/* Dynamic */
	UInteger16	stepsRemoved;
	TimeInternal	offsetFromMaster;
	TimeInternal	meanPathDelay;
	/* White Rabbit extension begin */
	UInteger16	primarySlavePortNumber;
	/* White Rabbit extension end */
} DSCurrent;

/* Parent Data Set */
typedef struct DSParent {
	/* Dynamic */
	PortIdentity	parentPortIdentity;
	Boolean		parentStats;
	UInteger16	observedParentOffsetScaledLogVariance;
	Integer32	observedParentClockPhaseChangeRate;
	ClockIdentity	grandmasterIdentity;
	ClockQuality	grandmasterClockQuality;
	UInteger8	grandmasterPriority1;
	UInteger8	grandmasterPriority2;
} DSParent;

/* Port Data set */
typedef struct DSPort {
	/* Static */
	PortIdentity	portIdentity;
	/* Dynamic */
	Enumeration8	portState;
	Integer8	logMinDelayReqInterval;
	TimeInternal	peerMeanPathDelay;
	/* Configurable */
	Integer8	logAnnounceInterval;
	UInteger8	announceReceiptTimeout;
	Integer8	logSyncInterval;
	Enumeration8	delayMechanism;
	Integer8	logMinPdelayReqInterval;
	UInteger4	versionNumber;

	/* White Rabbit extension begin (see wrspec.v2-06-07-2011, page 17) */
	Enumeration8 wrConfig;
	Enumeration8 wrMode;
	Boolean wrModeOn;
	Enumeration8  wrPortState;
	/* FIXME check doc: knownDeltaTx, knownDeltaRx, deltasKnown?) */
	Boolean calibrated;
	FixedDelta deltaTx;
	FixedDelta deltaRx;
	UInteger32 wrStateTimeout;
	UInteger8 wrStateRetry;
	UInteger32 calPeriod;
	UInteger8 calRetry;
	Enumeration8 parentWrConfig;
	Boolean parentIsWRnode; /* FIXME Not in the doc */
	/* FIXME check doc: (parentWrMode?) */
	Enumeration16 msgTmpWrMessageID; /* FIXME Not in the doc */
	Boolean parentWrModeOn;
	Boolean parentCalibrated;

	/* FIXME: are they in the doc? */
	UInteger16 otherNodeCalSendPattern;
	UInteger32 otherNodeCalPeriod;
	UInteger8 otherNodeCalRetry;
	FixedDelta otherNodeDeltaTx;
	FixedDelta otherNodeDeltaRx;
	Boolean doRestart;
	Boolean linkUP;
	/* White Rabbit extension end */
} DSPort;

/* Time Properties Data Set */
typedef struct DSTimeProperties {
	/* Dynamic */
	Integer16	currentUtcOffset;
	Boolean	currentUtcOffsetValid;
	Boolean	leap59;
	Boolean	leap61;
	Boolean	timeTraceable;
	Boolean	frequencyTraceable;
	Boolean	ptpTimescale;
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
