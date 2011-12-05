/*
 * FIXME header
 * Structs defined in IEEE Std 1588-2008
 */

#ifndef IEEE_1588_TYPES_H_
#define IEEE_1588_TYPES_H_

/* FIXME needed? #include <stdio.h> */
#include <stdint.h>


typedef enum {FALSE=0, TRUE} Boolean; /* FIXME really needed? */
typedef char Octet;
typedef int8_t Integer8;
typedef int16_t Integer16;
typedef int32_t Integer32;
typedef uint8_t UInteger8;
typedef uint16_t UInteger16;
typedef uint32_t UInteger32;
/* Enumerations are unsigned, see 5.4.2, page 15 */
typedef uint16_t Enumeration16;
typedef uint8_t Enumeration8;
typedef uint8_t Enumeration4;
typedef uint8_t UInteger4;
typedef uint8_t Nibble;

typedef struct {
	uint32_t lsb;
	uint16_t msb;
} UInteger48;

typedef struct {
	uint32_t lsb;
	int32_t msb;
} Integer64;

typedef struct {
	Integer64 scaledNanoseconds;
} TimeInterval;

typedef struct  {
	UInteger48 secondsField;
	UInteger32 nanosecondsField;
} Timestamp;

typedef Octet ClockIdentity[CLOCK_IDENTITY_LENGTH];

typedef struct {
	ClockIdentity clockIdentity;
	UInteger16 portNumber;
} PortIdentity;

typedef struct {
	Enumeration16 networkProtocol;
	UInteger16 adressLength;
	Octet* adressField;
} PortAdress;

typedef struct {
	UInteger8 clockClass;
	Enumeration8 clockAccuracy;
	UInteger16 offsetScaledLogVariance;
} ClockQuality;

typedef struct {
	Enumeration16 tlvType;
	UInteger16 lengthField;
	Octet* valueField;
} TLV;

typedef struct {
	UInteger8 lengthField;
	Octet* textField;
} PTPText;

typedef struct {
	UInteger16 faultRecordLength;
	Timestamp faultTime;
	Enumeration8 severityCode;
	PTPText faultName;
	PTPText faultValue;
	PTPText faultDescription;
} FaultRecord;


/* Common Message header (table 18, page 124) */
typedef struct {
 	Nibble transportSpecific;
 	Enumeration4 messageType;
 	UInteger4 versionPTP;
 	UInteger16 messageLength;
 	UInteger8 domainNumber;
 	Octet flagField[2];
 	Integer64 correctionfield;
	PortIdentity sourcePortIdentity;
 	UInteger16 sequenceId;
 	UInteger8 controlField;
 	Integer8 logMessageInterval;
} MsgHeader;

/* Announce Message (table 25, page 129) */
typedef struct {
	Timestamp originTimestamp;
	Integer16 currentUtcOffset;
	UInteger8 grandmasterPriority1;
	ClockQuality grandmasterClockQuality;
	UInteger8 grandmasterPriority2;
	ClockIdentity grandmasterIdentity;
	UInteger16 stepsRemoved;
	Enumeration8 timeSource;
} MsgAnnounce;

/* Sync Message (table 26, page 129) */
typedef struct {
	Timestamp originTimestamp;
} MsgSync;

/* DelayReq Message (table 26, page 129) */
typedef struct {
	Timestamp originTimestamp;
} MsgDelayReq;

/* DelayResp Message (table 27, page 130) */
typedef struct {
	Timestamp preciseOriginTimestamp;
} MsgFollowUp;


/* DelayResp Message (table 28, page 130) */
typedef struct {
	Timestamp receiveTimestamp;
	PortIdentity requestingPortIdentity;
} MsgDelayResp;

/* PdelayReq Message (table 29, page 131) */
typedef struct {
	Timestamp originTimestamp;

} MsgPDelayReq;

/* PdelayResp Message (table 30, page 131) */
typedef struct {
	Timestamp requestReceiptTimestamp;
	PortIdentity requestingPortIdentity;
} MsgPdelayResp;

/* PdelayRespFollowUp Message (table 31, page 132) */
typedef struct {
	Timestamp responseOriginTimestamp;
	PortIdentity requestingPortIdentity;
} MsgPdelayRespFollowUp;

/* Signaling Message (table 33, page 133) */
typedef struct {
	PortIdentity targetPortIdentity;
	char* tlv;
} MsgSignaling;

/* Management Message (table 37, page 137) */
typedef struct {
	PortIdentity targetPortIdentity;
	UInteger8 startingBoundaryHops;
	UInteger8 boundaryHops;
	Enumeration4 actionField;
	char* tlv;
} MsgManagement;

#endif /*IEEE_1588_TYPES_H_*/
