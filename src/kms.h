#ifndef __kms_h
#define __kms_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#if _MSC_VER
//#include <time.h>
#else
#include <sys/time.h>
#endif // _MSC_VER
//#include <stdlib.h>
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

#define MAX_CLIENTS 671

typedef struct
{
	GUID Guid[MAX_CLIENTS];
	int_fast16_t CurrentCount;
	int_fast16_t MaxCount;
	int_fast16_t CurrentPosition;
} ClientList_t, *PClientList_t;

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

typedef enum 
{
	None = 0,
	UseNdr64 = 1 << 0,
	UseForEpid = 1 << 1,
	MayBeServer = 1 << 2,
} HostBuildFlag;

typedef struct CsvlkData
{
	union
	{
		uint64_t EPidOffset;
		char* EPid;
	};

	int64_t ReleaseDate;
	uint32_t GroupId;
	uint32_t MinKeyId;
	uint32_t MaxKeyId;
	uint8_t MinActiveClients;
	uint8_t Reserved[3];

} CsvlkData_t, *PCsvlkData_t;

typedef struct VlmcsdData
{
	union
	{
		GUID Guid;
		uint8_t GuidBytes[16];
	};

	union
	{
		uint64_t NameOffset;
		char* Name;
	};

	uint8_t AppIndex;
	uint8_t KmsIndex;
	uint8_t ProtocolVersion;
	uint8_t NCountPolicy;
	uint8_t IsRetail;
	uint8_t IsPreview;
	uint8_t EPidIndex;
	uint8_t reserved;

} VlmcsdData_t, *PVlmcsdData_t;

typedef struct
{
	union
	{
		uint64_t Offset;
		void* Pointer;
	};
} DataPointer_t;

#define KMS_OPTIONS_USENDR64 1 << 0

typedef struct HostBuild
{
	union
	{
		uint64_t DisplayNameOffset;
		char* DisplayName;
	};

	int64_t ReleaseDate;
	int32_t BuildNumber;
	int32_t PlatformId;
	HostBuildFlag Flags;
	uint8_t reserved[4];

} HostBuild_t, *PHostBuild_t;

typedef struct VlmcsdHeader
{
	BYTE Magic[4];
	VERSION_INFO;
	uint8_t CsvlkCount;
	uint8_t Flags;
	uint8_t Reserved[2];

	union
	{
		int32_t Counts[5];

		struct
		{
			int32_t AppItemCount;
			int32_t KmsItemCount;
			int32_t SkuItemCount;
			int32_t HostBuildCount;
			int32_t reserved2Counts;
		};
	};

	union
	{
		DataPointer_t Datapointers[5];

		struct
		{
			union
			{
				uint64_t AppItemOffset;
				PVlmcsdData_t AppItemList;
			};

			union
			{
				uint64_t KmsItemOffset;
				PVlmcsdData_t KmsItemList;
			};

			union
			{
				uint64_t SkuItemOffset;
				PVlmcsdData_t SkuItemList;
			};

			union
			{
				uint64_t HostBuildOffset;
				PHostBuild_t HostBuildList;
			};

			union
			{
				uint64_t Reserved2Offset;
				void* Reserved2List;
			};

			CsvlkData_t CsvlkData[1];
		};
	};

} VlmcsdHeader_t, *PVlmcsdHeader_t;

//#define EPID_INDEX_WINDOWS 0
//#define EPID_INDEX_OFFICE2010 1
//#define EPID_INDEX_OFFICE2013 2
//#define EPID_INDEX_OFFICE2016 3
//#define EPID_INDEX_WINCHINAGOV 4

typedef HRESULT(__stdcall *RequestCallback_t)(REQUEST* baseRequest, RESPONSE *const baseResponse, BYTE *const hwId, const char* const ipstr);

size_t CreateResponseV4(REQUEST_V4 *const Request, BYTE *const response_data, const char* const ipstr);
size_t CreateResponseV6(REQUEST_V6 *restrict Request, BYTE *const response_data, const char* const ipstr);
BYTE *CreateRequestV4(size_t *size, const REQUEST* requestBase);
BYTE *CreateRequestV6(size_t *size, const REQUEST* requestBase);
void randomPidInit();
void get16RandomBytes(void* ptr);
RESPONSE_RESULT DecryptResponseV6(RESPONSE_V6* response_v6, int responseSize, BYTE* const response, const BYTE* const rawRequest, BYTE* hwid);
RESPONSE_RESULT DecryptResponseV4(RESPONSE_V4* response_v4, const int responseSize, BYTE* const rawResponse, const BYTE* const rawRequest);
void getUnixTimeAsFileTime(FILETIME* ts);
__pure int64_t fileTimeToUnixTime(FILETIME* ts);

#ifndef IS_LIBRARY
int32_t getProductIndex(const GUID* guid, const PVlmcsdData_t list, const int32_t count, char** name, char** ePid);
#if !defined(NO_INI_FILE)||!defined(NO_VERBOSE_LOG)
const char* getNextString(const char* s);
#endif  // !defined(NO_INI_FILE)||!defined(NO_VERBOSE_LOG)
#endif // IS_LIBRARY

#ifndef NO_STRICT_MODES
void InitializeClientLists();
void CleanUpClientLists();
#endif // !NO_STRICT_MODES

extern RequestCallback_t CreateResponseBase;

#ifdef _PEDANTIC
uint16_t IsValidLcid(const uint16_t lcid);
uint32_t IsValidHostBuild(const int32_t hostBuild);
#endif // _PEDANTIC

#endif // __kms_h
