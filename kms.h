#ifndef __kms_h
#define __kms_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include <sys/time.h>
#include <stdlib.h>
#include "types.h"
//
// REQUEST... types are actually fixed size
// RESPONSE... size may vary, defined here is max possible size
//

#define MAX_RESPONSE_SIZE 384
#define PID_BUFFER_SIZE 64
#define MAX_REQUEST_SIZE sizeof(REQUEST_V6)
#define WORKSTATION_NAME_BUFFER 64

// Constants for V6 time stamp interval
#define TIME_C1 0x00000022816889BDULL
#define TIME_C2 0x000000208CBAB5EDULL
#define TIME_C3 0x3156CD5AC628477AULL

#define VERSION_INFO union \
{ \
	DWORD Version;\
	struct { \
		WORD MinorVer; \
		WORD MajorVer; \
	} /*__packed*/; \
} /*__packed*/

// Aliases for various KMS struct members
#define IsClientVM VMInfo
#define GraceTime BindingExpiration
#define MinutesRemaingInCurrentStatus BindingExpiration
#define ID ActID
#define ApplicationID AppID
#define SkuId ActID
#define KmsId KMSID
#define ClientMachineId CMID
#define MinimumClients N_Policy
#define TimeStamp ClientTime
#define PreviousCLientMachineId CMID_prev
#define Salt IV
#define XorSalt XoredIVs
#define ActivationInterval VLActivationInterval
#define RenewalInterval VLRenewalInterval


typedef struct {
	VERSION_INFO;
	DWORD VMInfo;					// 0 = client is bare metal / 1 = client is VM
	DWORD LicenseStatus;			// 0 = Unlicensed, 1 = Licensed (Activated), 2 = OOB grace, 3 = OOT grace, 4 = NonGenuineGrace, 5 = Notification, 6 = extended grace
	DWORD BindingExpiration;		// Expiration of the current status in minutes (e.g. when KMS activation or OOB grace expires).
	GUID AppID;						// Can currently be Windows, Office2010 or Office2013 (see kms.c, table AppList).
	GUID ActID;						// Most detailed product list. One product key per ActID (see kms.c, table ExtendedProductList). Is ignored by KMS server.
	GUID KMSID;						// This is actually what the KMS server uses to grant or refuse activation (see kms.c, table BasicProductList).
	GUID CMID;						// Client machine id. Used by the KMS server for counting minimum clients.
	DWORD N_Policy;					// Minimum clients required for activation.
	FILETIME ClientTime;			// Current client time.
	GUID CMID_prev;					// previous client machine id. All zeros, if it never changed.
	WCHAR WorkstationName[64];		// Workstation name. FQDN if available, NetBIOS otherwise.
} /*__packed*/ REQUEST;

typedef struct {
	VERSION_INFO;					
	DWORD PIDSize;					// Size of PIDData in bytes.
	WCHAR KmsPID[PID_BUFFER_SIZE];	// ePID (must include terminating zero)
	GUID CMID;						// Client machine id. Must be the same as in request.
	FILETIME ClientTime;			// Current client time. Must be the same as in request.
	DWORD Count;					// Current activated machines. KMS server counts up to N_Policy << 1 then stops
	DWORD VLActivationInterval;		// Time in minutes when clients should retry activation if it was unsuccessful (default 2 hours)
	DWORD VLRenewalInterval;        // Time in minutes when clients should renew KMS activation (default 7 days)
} /*__packed*/ RESPONSE;

#ifdef _DEBUG
typedef struct {
	VERSION_INFO;
	DWORD PIDSize;
	WCHAR KmsPID[49]; 				// Set this to the ePID length you want to debug
	GUID CMID;
	FILETIME ClientTime;
	DWORD Count;
	DWORD VLActivationInterval;
	DWORD VLRenewalInterval;
} __packed RESPONSE_DEBUG;
#endif


typedef struct {
	REQUEST RequestBase;			// Base request
	BYTE MAC[16];					// Aes 160 bit CMAC
} /*__packed*/ REQUEST_V4;

typedef struct {
	RESPONSE ResponseBase;			// Base response
	BYTE MAC[16];					// Aes 160 bit CMAC
} /*__packed*/ RESPONSE_V4;


typedef struct {
	VERSION_INFO;					// unencrypted version info
	BYTE IV[16];					// IV
	REQUEST RequestBase;			// Base Request
	BYTE Pad[4];					// since this struct is fixed, we use fixed PKCS pad bytes
} /*__packed*/ REQUEST_V5;

typedef REQUEST_V5 REQUEST_V6;		// v5 and v6 requests are identical

typedef struct {
	VERSION_INFO;
	BYTE IV[16];
	RESPONSE ResponseBase;
	BYTE RandomXoredIVs[16];		// If RequestIV was used for decryption: Random ^ decrypted Request IV ^ ResponseIV. If NULL IV was used for decryption: Random ^ decrypted Request IV
	BYTE Hash[32];					// SHA256 of Random used in RandomXoredIVs
	BYTE HwId[8];					// HwId from the KMS server
	BYTE XoredIVs[16];				// If RequestIV was used for decryption: decrypted Request IV ^ ResponseIV. If NULL IV was used for decryption: decrypted Request IV.
	BYTE HMAC[16];					// V6 Hmac (low 16 bytes only), see kms.c CreateV6Hmac
	//BYTE Pad[10];					// Pad is variable sized. So do not include in struct
} /*__packed*/ RESPONSE_V6;

typedef struct {					// not used except for sizeof(). Fields are the same as RESPONSE_V6
	VERSION_INFO;
	BYTE IV[16];
	RESPONSE ResponseBase;
	BYTE RandomXoredIVs[16];
	BYTE Hash[32];
} /*__packed*/ RESPONSE_V5;

#ifdef _DEBUG
typedef struct {					// Debug structure for direct casting of RPC data in debugger
	VERSION_INFO;
	BYTE IV[16];
	RESPONSE_DEBUG ResponseBase;
	BYTE RandomXoredIVs[16];
	BYTE MAC[32];
	BYTE Unknown[8];
	BYTE XorSalts[16];
	BYTE HMAC[16];
	BYTE Pad[16];
} __packed RESPONSE_V6_DEBUG;
#endif

#define V4_PRE_EPID_SIZE 	( \
								sizeof(((RESPONSE*)0)->Version) + \
								sizeof(((RESPONSE*)0)->PIDSize) \
							)

#define V4_POST_EPID_SIZE 	( \
								sizeof(((RESPONSE*)0)->CMID) + \
								sizeof(((RESPONSE*)0)->ClientTime) + \
								sizeof(((RESPONSE*)0)->Count) + \
								sizeof(((RESPONSE*)0)->VLActivationInterval) + \
								sizeof(((RESPONSE*)0)->VLRenewalInterval) \
							)

#define V6_DECRYPT_SIZE		( \
								sizeof(((REQUEST_V6*)0)->IV) + \
								sizeof(((REQUEST_V6*)0)->RequestBase) + \
								sizeof(((REQUEST_V6*)0)->Pad) \
							)

#define V6_UNENCRYPTED_SIZE	( \
								sizeof(((RESPONSE_V6*)0)->Version) + \
								sizeof(((RESPONSE_V6*)0)->IV) \
							)

#define V6_PRE_EPID_SIZE 	( \
								V6_UNENCRYPTED_SIZE + \
								sizeof(((RESPONSE*)0)->Version) + \
								sizeof(((RESPONSE*)0)->PIDSize) \
							)

#define V5_POST_EPID_SIZE 	( \
								V4_POST_EPID_SIZE + \
								sizeof(((RESPONSE_V6*)0)->RandomXoredIVs) + \
								sizeof(((RESPONSE_V6*)0)->Hash) \
							)

#define V6_POST_EPID_SIZE 	( \
								V5_POST_EPID_SIZE + \
								sizeof(((RESPONSE_V6*)0)->HwId) + \
								sizeof(((RESPONSE_V6*)0)->XoredIVs) + \
								sizeof(((RESPONSE_V6*)0)->HMAC) \
							)

#define RESPONSE_RESULT_OK ((1 << 10) - 1) //(9 bits)
typedef union
{
	DWORD mask;
	struct
	{
		BOOL HashOK : 1;
		BOOL TimeStampOK : 1;
		BOOL ClientMachineIDOK : 1;
		BOOL VersionOK : 1;
		BOOL IVsOK : 1;
		BOOL DecryptSuccess : 1;
		BOOL HmacSha256OK : 1;
		BOOL PidLengthOK : 1;
		BOOL RpcOK : 1;
		BOOL IVnotSuspicious : 1;
		BOOL reserved3 : 1;
		BOOL reserved4 : 1;
		BOOL reserved5 : 1;
		BOOL reserved6 : 1;
		uint32_t effectiveResponseSize : 9;
		uint32_t correctResponseSize : 9;
	};
} RESPONSE_RESULT;

typedef BYTE hwid_t[8];

typedef struct
{
	GUID guid;
	const char* name;
	const char* pid;
	uint8_t AppIndex;
	uint8_t KmsIndex;
} KmsIdList;

#define KMS_PARAM_MAJOR AppIndex
#define KMS_PARAM_REQUIREDCOUNT KmsIndex

#define APP_ID_WINDOWS 0
#define APP_ID_OFFICE2010 1
#define APP_ID_OFFICE2013 2

// Update these numbers in License Manager
#define KMS_ID_VISTA 0
#define KMS_ID_WIN7 1
#define KMS_ID_WIN8_VL 2
#define KMS_ID_WIN_BETA 3
#define KMS_ID_WIN8_RETAIL 4
#define KMS_ID_WIN81_VL 5
#define KMS_ID_WIN81_RETAIL 6
#define KMS_ID_WIN2008A 7
#define KMS_ID_WIN2008B 8
#define KMS_ID_WIN2008C 9
#define KMS_ID_WIN2008R2A 10
#define KMS_ID_WIN2008R2B 11
#define KMS_ID_WIN2008R2C 12
#define KMS_ID_WIN2012 13
#define KMS_ID_WIN2012R2 14
#define KMS_ID_OFFICE2010 15
#define KMS_ID_OFFICE2013 16
#define KMS_ID_WIN_SRV_BETA 17
#define KMS_ID_OFFICE2016 18
#define KMS_ID_WIN10_VL 19
#define KMS_ID_WIN10_RETAIL 20
#define KMS_ID_WIN2016 21
#define KMS_ID_OFFICE2013_BETA 22
#define KMS_ID_WIN10_LTSB2016 23

#define PWINGUID &AppList[APP_ID_WINDOWS].guid
#define POFFICE2010GUID &AppList[APP_ID_OFFICE2010].guid
#define POFFICE2013GUID &AppList[APP_ID_OFFICE2013].guid

typedef BOOL(__stdcall *RequestCallback_t)(const REQUEST *const baseRequest, RESPONSE *const baseResponse, BYTE *const hwId, const char* const ipstr);

size_t CreateResponseV4(REQUEST_V4 *const Request, BYTE *const response_data, const char* const ipstr);
size_t CreateResponseV6(REQUEST_V6 *restrict Request, BYTE *const response_data, const char* const ipstr);
BYTE *CreateRequestV4(size_t *size, const REQUEST* requestBase);
BYTE *CreateRequestV6(size_t *size, const REQUEST* requestBase);
void randomPidInit();
void get16RandomBytes(void* ptr);
RESPONSE_RESULT DecryptResponseV6(RESPONSE_V6* Response_v6, int responseSize, BYTE* const response, const BYTE* const request, BYTE* hwid);
RESPONSE_RESULT DecryptResponseV4(RESPONSE_V4* Response_v4, const int responseSize, BYTE* const response, const BYTE* const request);
void getUnixTimeAsFileTime(FILETIME *const ts);
__pure int64_t fileTimeToUnixTime(const FILETIME *const ts);
const char* getProductNameHE(const GUID *const guid, const KmsIdList *const List, ProdListIndex_t *const i);
const char* getProductNameLE(const GUID *const guid, const KmsIdList *const List, ProdListIndex_t *const i);
__pure ProdListIndex_t getExtendedProductListSize();
__pure ProdListIndex_t getAppListSize(void);

extern const KmsIdList ProductList[];
extern const KmsIdList AppList[];
extern const KmsIdList ExtendedProductList[];

extern RequestCallback_t CreateResponseBase;

#ifdef _PEDANTIC
uint16_t IsValidLcid(const uint16_t Lcid);
#endif // _PEDANTIC

#endif // __kms_h
