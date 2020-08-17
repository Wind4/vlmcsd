#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#if !defined(_WIN32)
#if !__ANDROID__
#include <sys/shm.h>
#endif // !__ANDROID__
#include <sys/socket.h>
#include <sys/ipc.h>
#endif // !defined(_WIN32)

#include "output.h"
#include "crypto.h"
#include "endian.h"
#include "kms.h"
#include "shared_globals.h"
#include "helpers.h"

#define FRIENDLY_NAME_WINDOWS "Windows"
#define FRIENDLY_NAME_OFFICE2010 "Office 2010"
#define FRIENDLY_NAME_OFFICE2013 "Office 2013+"

#ifndef IS_LIBRARY

#ifdef NO_LOG
#define LOGTEXT(x) ""
#else //!NO_LOG
#define LOGTEXT(x) x
#endif // !NO_LOG

int32_t getProductIndex(const GUID* guid, const PVlmcsdData_t list, const int32_t count, char** name, char** ePid)
{
	int i;

	for (i = count - 1; i >= 0; i--)
	{
		if (IsEqualGUID(guid, &list[i].Guid))
		{
			if (name) *name = list[i].Name;
			if (ePid) *ePid = KmsData->CsvlkData[list[i].EPidIndex].EPid;
			return i;
		}
	}

	if (name) *name = (char*)"Unknown";
	if (ePid) *ePid = KmsData->CsvlkData->EPid;
	return i;
}

#if !defined(NO_INI_FILE)||!defined(NO_VERBOSE_LOG)
const char* getNextString(const char* s)
{
	return s + strlen(s) + 1;
}
#endif //!defined(NO_INI_FILE)||!defined(NO_VERBOSE_LOG)

#endif // IS_LIBRARY


#ifndef NO_RANDOM_EPID
//static const uint16_t HostBuilds[] = { 6002, 7601, 9200, 9600, 14393, 17763 };

// Valid language identifiers to be used in the ePID
static const uint16_t LcidList[] = {
	1078, 1052, 1025, 2049, 3073, 4097, 5121, 6145, 7169, 8193, 9217, 10241, 11265, 12289, 13313, 14337, 15361, 16385,
	1067, 1068, 2092, 1069, 1059, 1093, 5146, 1026, 1027, 1028, 2052, 3076, 4100, 5124, 1050, 4122, 1029, 1030, 1125, 1043, 2067,
	1033, 2057, 3081, 4105, 5129, 6153, 7177, 8201, 9225, 10249, 11273, 12297, 13321, 1061, 1080, 1065, 1035, 1036, 2060,
	3084, 4108, 5132, 6156, 1079, 1110, 1031, 2055, 3079, 4103, 5127, 1032, 1095, 1037, 1081, 1038, 1039, 1057, 1040, 2064, 1041, 1099,
	1087, 1111, 1042, 1088, 1062, 1063, 1071, 1086, 2110, 1100, 1082, 1153, 1102, 1104, 1044, 2068, 1045, 1046, 2070,
	1094, 1131, 2155, 3179, 1048, 1049, 9275, 4155, 5179, 3131, 1083, 2107, 8251, 6203, 7227, 1103, 2074, 6170, 3098,
	7194, 1051, 1060, 1034, 2058, 3082, 4106, 5130, 6154, 7178, 8202, 9226, 10250, 11274, 12298, 13322, 14346, 15370, 16394,
	17418, 18442, 19466, 20490, 1089, 1053, 2077, 1114, 1097, 1092, 1098, 1054, 1074, 1058, 1056, 1091, 2115, 1066, 1106, 1076, 1077
};

int32_t getPlatformId(int32_t hostBuild)
{
	int32_t i;

	for (i = 0; i < KmsData->HostBuildCount; i++)
	{
		if (KmsData->HostBuildList[i].BuildNumber <= hostBuild)
		{
			return KmsData->HostBuildList[i].PlatformId;
		}
	}

	return KmsData->HostBuildList[KmsData->HostBuildCount - 1].PlatformId;
}


time_t getReleaseDate(int32_t hostBuild)
{
	int32_t i;

	for (i = KmsData->HostBuildCount - 1; i >= 0; i--)
	{
		if (KmsData->HostBuildList[i].BuildNumber >= hostBuild)
		{
			return (time_t)KmsData->HostBuildList[i].ReleaseDate;
		}
	}

	return (time_t)KmsData->HostBuildList->ReleaseDate;
}


#ifdef _PEDANTIC
uint16_t IsValidLcid(const uint16_t lcid)
{
	uint16_t i;

	for (i = 0; i < vlmcsd_countof(LcidList); i++)
	{
		if (lcid == LcidList[i]) return lcid;
	}

	return 0;
}


uint32_t IsValidHostBuild(const int32_t hostBuild)
{
	PHostBuild_t hostOS;

	for (hostOS = KmsData->HostBuildList; hostOS < KmsData->HostBuildList + KmsData->HostBuildCount; hostOS++)
	{
		if (hostBuild == hostOS->BuildNumber) return hostBuild;
	}

	return 0;
}
#endif // _PEDANTIC
#endif // NO_RANDOM_EPID


// Unix time is seconds from 1970-01-01. Should be 64 bits to avoid year 2038 overflow bug.
// FILETIME is 100 nanoseconds from 1601-01-01. Must be 64 bits.
void getUnixTimeAsFileTime(FILETIME* ts)
{
	const int64_t unixtime = (int64_t)time(NULL);
	int64_t *filetime = (int64_t*)ts;

	PUT_UA64LE(filetime, (unixtime + 11644473600LL) * 10000000LL);
}

__pure int64_t fileTimeToUnixTime(FILETIME* ts)
{
	return GET_UA64LE(ts) / 10000000LL - 11644473600LL;
}


#ifndef NO_STRICT_MODES
#ifndef NO_CLIENT_LIST

static PClientList_t ClientLists;
static BYTE ZeroGuid[16] = { 0 };

#if !defined(_WIN32) && !defined(__CYGWIN__)
pthread_mutex_t* mutex;
#define mutex_size (((sizeof(pthread_mutex_t)+7)>>3)<<3)
#else
CRITICAL_SECTION* mutex;
#define mutex_size (((sizeof(CRITICAL_SECTION)+7)>>3)<<3)
#endif // _WIN32

#ifndef USE_THREADS
static int shmid_clients = -1;
#endif // USE_THREADS

#if !defined(_WIN32) && !defined(__CYGWIN__)
#define lock_client_lists() pthread_mutex_lock(mutex)
#define unlock_client_lists() pthread_mutex_unlock(mutex)
#define mutex_t pthread_mutex_t
#else
#define lock_client_lists() EnterCriticalSection(mutex)
#define unlock_client_lists() LeaveCriticalSection(mutex)
#define mutex_t CRITICAL_SECTION
#endif

void CleanUpClientLists()
{
#	ifndef USE_THREADS
	shmctl(shmid_clients, IPC_RMID, NULL);
#	endif // !USE_THREADS
}

void InitializeClientLists()
{
	int_fast8_t i;
	int_fast16_t j;

#	ifndef USE_THREADS
	if (
		(shmid_clients = shmget(IPC_PRIVATE, sizeof(ClientList_t) * KmsData->AppItemCount + mutex_size, IPC_CREAT | 0600)) < 0 ||
		(mutex = (mutex_t*)shmat(shmid_clients, NULL, 0)) == (mutex_t*)-1
		)
	{
		int errno_save = errno;
		printerrorf("Warning: CMID lists disabled. Could not create shared memory: %s\n", vlmcsd_strerror(errno_save));
		if (shmid_clients >= 0) shmctl(shmid_clients, IPC_RMID, NULL);
		MaintainClients = FALSE;
		return;
	}

	ClientLists = (PClientList_t)((BYTE*)mutex + mutex_size);

#	if __CYGWIN__
	InitializeCriticalSection(mutex);
#	else // !__CYGWIN__
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(mutex, &mutex_attr);

#	endif // !__CYGWIN__

#	else // USE_THREADS

	ClientLists = (PClientList_t)vlmcsd_malloc(sizeof(ClientList_t) * KmsData->AppItemCount);
	mutex = (mutex_t*)vlmcsd_malloc(sizeof(mutex_t));

#	if !_WIN32 && !__CYGWIN__
	pthread_mutex_init(mutex, NULL);
#	else //_WIN32 || __CYGWIN__
	InitializeCriticalSection(mutex);
#   endif //_WIN32 || __CYGWIN__

#	endif // USE_THREADS

	memset(ClientLists, 0, sizeof(ClientList_t) * KmsData->AppItemCount);

	if (!StartEmpty)
	{
		for (i = 0; i < KmsData->AppItemCount; i++)
		{
			const uint8_t maxCount = KmsData->AppItemList[i].NCountPolicy;
			ClientLists[i].CurrentCount = (maxCount >> 1) - 1;
			ClientLists[i].MaxCount = maxCount;

			for (j = 0; j < (maxCount >> 1) - 1; j++)
			{
				get16RandomBytes(&ClientLists[i].Guid[j]);
			}
		}
	}
}
#endif // NO_CLIENT_LIST
#endif // !NO_STRICT_MODES

#ifndef NO_RANDOM_EPID
// formats an int with a fixed number of digits with leading zeros (helper for ePID generation)
static char* itoc(char *const c, const int i, uint_fast8_t digits)
{
	char formatString[8];
	if (digits > 9) digits = 0;
	strcpy(formatString, "%");

	if (digits)
	{
		formatString[1] = '0';
		formatString[2] = digits | 0x30;
		formatString[3] = 0;
	}

	strcat(formatString, "u");
	sprintf(c, formatString, i);
	return c;
}

static uint8_t getRandomServerType()
{
#	if defined(USE_MSRPC) || defined(SIMPLE_RPC)

	return rand() % KmsData->HostBuildCount;

#	else // !defined(USE_MSRPC) && !defined(SIMPLE_RPC)
	while (TRUE)
	{
		const uint32_t buildIndex = rand() % KmsData->HostBuildCount;

		if (!(KmsData->HostBuildList[buildIndex].Flags & UseNdr64) == !UseServerRpcNDR64)
		{
			return (uint8_t)buildIndex;
		}
	}
#	endif // !defined(USE_MSRPC) && !defined(SIMPLE_RPC)
}


/*
 * Generates a random ePID
 */
static void generateRandomPid(const int index, char *const szPid, int16_t lang, int32_t hostBuild)
{
	char numberBuffer[12];

	if (!hostBuild)
	{
		hostBuild = KmsData->HostBuildList[getRandomServerType()].BuildNumber;
	}


	strcpy(szPid, itoc(numberBuffer, getPlatformId(hostBuild), 5));
	strcat(szPid, "-");

	//if (index > 3) index = 0;

	PCsvlkData_t csvlkData = &KmsData->CsvlkData[index];
	strcat(szPid, itoc(numberBuffer, csvlkData->GroupId, 5));
	strcat(szPid, "-");

	const int keyId = (rand32() % (csvlkData->MaxKeyId - csvlkData->MinKeyId)) + csvlkData->MinKeyId;
	strcat(szPid, itoc(numberBuffer, keyId / 1000000, 3));
	strcat(szPid, "-");
	strcat(szPid, itoc(numberBuffer, keyId % 1000000, 6));
	strcat(szPid, "-03-");

	if (lang < 1) lang = LcidList[rand() % vlmcsd_countof(LcidList)];
	strcat(szPid, itoc(numberBuffer, lang, 0));
	strcat(szPid, "-");

	strcat(szPid, itoc(numberBuffer, hostBuild, 0));
	strcat(szPid, ".0000-");

	const time_t hostBuildReleaseDate = getReleaseDate(hostBuild);
	const time_t minTime = csvlkData->ReleaseDate < hostBuildReleaseDate ? hostBuildReleaseDate : csvlkData->ReleaseDate;

	time_t maxTime;
	time(&maxTime);

#	ifndef BUILD_TIME
#	define BUILD_TIME 1538922811
#   endif

	if (maxTime < (time_t)BUILD_TIME) // Just in case the system time is < 10/17/2013 1:00 pm
		maxTime = (time_t)BUILD_TIME;

	time_t kmsTime = (rand32() % (maxTime - minTime)) + minTime;
	struct tm *pidTime = gmtime(&kmsTime);

	strcat(szPid, itoc(numberBuffer, pidTime->tm_yday + 1, 3));
	strcat(szPid, itoc(numberBuffer, pidTime->tm_year + 1900, 4));
}


/*
 * Generates random ePIDs and stores them if not already read from ini file.
 * For use with randomization level 1
 */
void randomPidInit()
{
	uint32_t i;

	const int16_t lang = Lcid ? Lcid : LcidList[rand() % vlmcsd_countof(LcidList)];

	for (i = 0; i < KmsData->CsvlkCount; i++)
	{
		if (KmsResponseParameters[i].Epid) continue;

		char Epid[PID_BUFFER_SIZE];

		if (!HostBuild)
		{
			uint8_t index;

#if defined(USE_MSRPC) || defined(SIMPLE_RPC)
			index = getRandomServerType();
#else // !(defined(USE_MSRPC) || defined(SIMPLE_RPC))
			if (IsNDR64Defined)
			{
				index = getRandomServerType();
			}
			else
			{
				index = (uint8_t)(rand() % KmsData->HostBuildCount);
				UseServerRpcNDR64 = !!(KmsData->HostBuildList[index].Flags & UseNdr64);
			}
#endif // !(defined(USE_MSRPC) || defined(SIMPLE_RPC))

			HostBuild = (uint16_t)KmsData->HostBuildList[index].BuildNumber;
		}

		generateRandomPid(i, Epid, lang, HostBuild);
		KmsResponseParameters[i].Epid = (const char*)vlmcsd_strdup(Epid);

#ifndef NO_LOG
		KmsResponseParameters[i].EpidSource = "randomized at program start";
		KmsResponseParameters[i].IsRandom = TRUE;
#endif // NO_LOG
	}
}

#endif // NO_RANDOM_EPID


#ifndef NO_LOG
static int32_t getProductIndexFromAllLists(const GUID* guid, char** productName)
{
	return getProductIndex(guid, KmsData->AppItemList, KmsData->AppItemCount + KmsData->KmsItemCount + KmsData->SkuItemCount, productName, NULL);
}

/*
 * Logs a Request
 */
static void logRequest(REQUEST* baseRequest)
{
#ifndef NO_VERBOSE_LOG
	if (logverbose)
	{
		logger("<<< Incoming KMS request\n");
		logRequestVerbose(baseRequest, &logger);
		return;
	}
#endif // NO_VERBOSE_LOG

	char *productName;
	char clientName[64];

	int32_t index = getProductIndexFromAllLists(&baseRequest->ActID, &productName);
	if (index < 0) index = getProductIndexFromAllLists(&baseRequest->KMSID, &productName);
	if (index < 0) index = getProductIndexFromAllLists(&baseRequest->AppID, &productName);

	if (index < 0 || !strcasecmp(productName, "Unknown"))
	{
		productName = (char*)alloca(GUID_STRING_LENGTH + 1);
		uuid2StringLE(&baseRequest->ActID, productName);
	}

	ucs2_to_utf8(baseRequest->WorkstationName, clientName, 64, 64);
	logger("KMS v%i.%i request from %s for %s\n", LE16(baseRequest->MajorVer), LE16(baseRequest->MinorVer), clientName, productName);
}
#endif // NO_LOG


/*
 * Converts a utf-8 ePID string to UCS-2 and writes it to a RESPONSE struct
 */
#ifndef IS_LIBRARY
static void getEpidFromString(RESPONSE *const Response, const char *const pid)
{
	const size_t length = utf8_to_ucs2(Response->KmsPID, pid, PID_BUFFER_SIZE, PID_BUFFER_SIZE * 3);
	Response->PIDSize = LE32(((unsigned int)length + 1) << 1);
}


/*
 * get ePID from appropriate source
 */
static void getEpid(RESPONSE *const baseResponse, const char** EpidSource, const int32_t index, BYTE *const HwId, const char* defaultEPid)
{
#if !defined(NO_RANDOM_EPID) || !defined(NO_CL_PIDS) || !defined(NO_INI_FILE)
	const char* pid;
	if (KmsResponseParameters[index].Epid == NULL)
	{
#ifndef NO_RANDOM_EPID
		if (RandomizationLevel == 2)
		{
			char ePid[PID_BUFFER_SIZE];
			generateRandomPid(index, ePid, Lcid, HostBuild);
			pid = ePid;

#ifndef NO_LOG
			*EpidSource = "randomized on every request";
#endif // NO_LOG
		}
		else
#endif // NO_RANDOM_EPID
		{
			pid = defaultEPid;
#ifndef NO_LOG
			*EpidSource = "vlmcsd default";
#endif // NO_LOG
		}
	}
	else
	{
		pid = KmsResponseParameters[index].Epid;

		if (HwId && KmsResponseParameters[index].HwId != NULL)
			memcpy(HwId, KmsResponseParameters[index].HwId, sizeof(((RESPONSE_V6 *)0)->HwId));

#ifndef NO_LOG
		*EpidSource = KmsResponseParameters[index].EpidSource;
#endif // NO_LOG
	}

	getEpidFromString(baseResponse, pid);

#else // defined(NO_RANDOM_EPID) && defined(NO_CL_PIDS) && !defined(NO_INI_FILE)

	getEpidFromString(baseResponse, defaultEPid);

#	ifndef NO_LOG
	*EpidSource = "vlmcsd default";
#	endif // NO_LOG

#endif // defined(NO_RANDOM_EPID) && defined(NO_CL_PIDS) && !defined(NO_INI_FILE)
}
#endif // IS_LIBRARY


#if !defined(NO_LOG) && defined(_PEDANTIC)
static BOOL CheckVersion4Uuid(const GUID *const guid, const char *const szGuidName)
{
	if (LE16(guid->Data3) >> 12 != 4 || guid->Data4[0] >> 6 != 2)
	{
		logger("Warning: %s does not conform to version 4 UUID according to RFC 4122\n", szGuidName);
		return FALSE;
	}
	return TRUE;
}


static void CheckRequest(const REQUEST *const Request)
{
	CheckVersion4Uuid(&Request->CMID, "Client machine ID");
	CheckVersion4Uuid(&Request->AppID, "Application ID");
	CheckVersion4Uuid(&Request->KMSID, "Server SKU ID");
	CheckVersion4Uuid(&Request->ActID, "Client SKU ID");

	if (LE32(Request->IsClientVM) > 1)
		logger("Warning: Virtual Machine field in request must be 0 or 1 but is %u\n", LE32(Request->IsClientVM));

	if (LE32(Request->LicenseStatus) > 6)
		logger("Warning: License status must be between 0 and 6 but is %u\n", LE32(Request->LicenseStatus));
}
#endif // !defined(NO_LOG) && defined(_PEDANTIC)


#ifndef NO_LOG
/*
 * Logs the Response
 */
static void logResponse(RESPONSE* baseResponse, const BYTE *const hwId, const char *const EpidSource)
{
	char utf8pid[PID_BUFFER_SIZE * 3];
	ucs2_to_utf8(baseResponse->KmsPID, utf8pid, PID_BUFFER_SIZE, PID_BUFFER_SIZE * 3);

#ifndef NO_VERBOSE_LOG
	if (!logverbose)
	{
#endif // NO_VERBOSE_LOG
		logger("Sending ePID (%s): %s\n", EpidSource, utf8pid);
#ifndef NO_VERBOSE_LOG
	}
	else
	{
		logger(">>> Sending response, ePID source = %s\n", EpidSource);
		logResponseVerbose(utf8pid, hwId, baseResponse, &logger);
	}
#endif // NO_VERBOSE_LOG

}
#endif


#if __UCLIBC__ && !defined(NO_STRICT_MODES)
long long int llabs(long long int j);
#endif


/*
 * Creates the unencrypted base response
 */
#ifndef IS_LIBRARY
static HRESULT __stdcall CreateResponseBaseCallback(REQUEST* baseRequest, RESPONSE *const baseResponse, BYTE *const hwId, const char* const ipstr_unused)
{
	const char* EpidSource;
#ifndef NO_LOG
	logRequest(baseRequest);
#ifdef _PEDANTIC
	CheckRequest(baseRequest);
#endif // _PEDANTIC
#endif // NO_LOG

	char* ePid;
	const DWORD minClients = LE32(baseRequest->N_Policy);
	const DWORD required_clients = minClients < 1 ? 1 : minClients << 1;

	const int32_t index = getProductIndex(&baseRequest->KMSID, KmsData->KmsItemList, KmsData->KmsItemCount, NULL, &ePid);

#	ifndef NO_STRICT_MODES

	if (required_clients > 2000)
	{
#		ifndef NO_LOG
		logger("Rejecting request with more than 1000 minimum clients (0x8007000D)\n");
#		endif

		return 0x8007000D;
	}

	if (CheckClientTime)
	{
		time_t requestTime = (time_t)fileTimeToUnixTime(&baseRequest->ClientTime);

		if (llabs(requestTime - time(NULL)) > 60 * 60 * 4)
		{
#			ifndef NO_LOG
			logger("Client time differs more than 4 hours from system time (0xC004F06C)\n");
#			endif // !NO_LOG

			return 0xC004F06C;
		}
	}

	if (WhitelistingLevel & 2)
	{
		if (index >= 0 && (KmsData->KmsItemList[index].IsPreview || KmsData->KmsItemList[index].IsRetail))
		{
#			ifndef NO_LOG
			logger("Refusing retail or beta product (0xC004F042)\n");
#			endif // !NO_LOG

			return 0xC004F042;
		}
	}

	if ((WhitelistingLevel & 1) && index < 0)
	{
#		ifndef NO_LOG
		logger("Refusing unknown product (0xC004F042)\n");
#		endif // !NO_LOG

		return 0xC004F042;
	}

#	ifndef NO_CLIENT_LIST
	const int32_t appIndex = index < 0 ? 0 : KmsData->KmsItemList[index].AppIndex;
#	endif // NO_CLIENT_LIST

#	endif // !NO_STRICT_MODES

	const int32_t ePidIndex = index < 0 ? 0 : KmsData->KmsItemList[index].EPidIndex;

#	if !defined(NO_STRICT_MODES)

	if ((WhitelistingLevel & 1) && index >= 0 && !IsEqualGUID(&KmsData->AppItemList[KmsData->KmsItemList[index].AppIndex].Guid, &baseRequest->AppID))
	{
#		ifndef NO_LOG
		logger("Refusing product with incorrect Application ID (0xC004F042)\n");
#		endif // NO_LOG
		return 0xC004F042;
	}

#	ifndef NO_CLIENT_LIST
	if (MaintainClients)
	{
		lock_client_lists();

		int_fast16_t i;
		int_fast8_t isKnownClient = FALSE;

		if (required_clients > (DWORD)ClientLists[appIndex].MaxCount) ClientLists[appIndex].MaxCount = required_clients;

		for (i = 0; i < ClientLists[appIndex].MaxCount; i++)
		{
			if (IsEqualGUID(&ClientLists[appIndex].Guid[i], &baseRequest->CMID))
			{
				isKnownClient = TRUE;
				break;
			}
		}

		if (isKnownClient)
		{
			baseResponse->Count = LE32(ClientLists[appIndex].CurrentCount);
		}
		else
		{
			for (i = 0; i < ClientLists[appIndex].MaxCount; i++)
			{
				if (IsEqualGUID(ZeroGuid, &ClientLists[appIndex].Guid[i]))
				{
					if (ClientLists[appIndex].CurrentCount >= MAX_CLIENTS)
					{
#						ifndef NO_LOG
						logger("Rejecting more than 671 clients (0xC004D104)\n");
#						endif // !NO_LOG

						unlock_client_lists();
						return 0xC004D104;
					}

					baseResponse->Count = LE32(++ClientLists[appIndex].CurrentCount);
					memcpy(&ClientLists[appIndex].Guid[i], &baseRequest->CMID, sizeof(GUID));
					break;
				}
			}

			if (i >= ClientLists[appIndex].MaxCount)
			{
				memcpy(&ClientLists[appIndex].Guid[ClientLists[appIndex].CurrentPosition], &baseRequest->CMID, sizeof(GUID));
				ClientLists[appIndex].CurrentPosition = (ClientLists[appIndex].CurrentPosition + 1) % (ClientLists[appIndex].MaxCount > MAX_CLIENTS ? MAX_CLIENTS : ClientLists[appIndex].MaxCount);
				baseResponse->Count = LE32(ClientLists[appIndex].CurrentCount);
			}
		}

		unlock_client_lists();
	}
	else
#	endif // !NO_CLIENT_LIST
#	endif // !defined(NO_STRICT_MODES)
	{
		const uint8_t minimum_answer_clients = (uint8_t)KmsData->CsvlkData[ePidIndex].MinActiveClients;
		baseResponse->Count = LE32(required_clients > minimum_answer_clients ? required_clients : minimum_answer_clients);
		//if (LE32(baseRequest->N_Policy) > LE32(baseResponse->Count)) baseResponse->Count = LE32(LE32(baseRequest->N_Policy) << 1);
	}

	getEpid(baseResponse, &EpidSource, ePidIndex, hwId, ePid);

	baseResponse->Version = baseRequest->Version;

	memcpy(&baseResponse->CMID, &baseRequest->CMID, sizeof(GUID));
	memcpy(&baseResponse->ClientTime, &baseRequest->ClientTime, sizeof(FILETIME));

	baseResponse->VLActivationInterval = LE32(VLActivationInterval);
	baseResponse->VLRenewalInterval = LE32(VLRenewalInterval);

#ifndef NO_LOG
	logResponse(baseResponse, hwId, EpidSource);
#endif // NO_LOG

	return S_OK;
}

RequestCallback_t CreateResponseBase = &CreateResponseBaseCallback;

#else // IS_LIBRARY

RequestCallback_t CreateResponseBase = NULL;

#endif // IS_LIBRARY


////TODO: Move to helpers.c
void get16RandomBytes(void* ptr)
{
	int i;
	for (i = 0; i < 4; i++)	((DWORD*)ptr)[i] = rand32();
}

/*
 * Creates v4 response
 */
size_t CreateResponseV4(REQUEST_V4 *const request_v4, BYTE *const responseBuffer, const char* const ipString)
{
	RESPONSE_V4* response = (RESPONSE_V4*)responseBuffer;

	HRESULT hResult;
	if (FAILED(hResult = CreateResponseBase(&request_v4->RequestBase, &response->ResponseBase, NULL, ipString))) return hResult;

	const DWORD pidSize = LE32(response->ResponseBase.PIDSize);
	BYTE* postEpidPtr = responseBuffer + V4_PRE_EPID_SIZE + pidSize;
	memmove(postEpidPtr, &response->ResponseBase.CMID, V4_POST_EPID_SIZE);

	const size_t encryptSize = V4_PRE_EPID_SIZE + V4_POST_EPID_SIZE + pidSize;
	AesCmacV4(responseBuffer, encryptSize, responseBuffer + encryptSize);

	return encryptSize + sizeof(response->MAC);
}

/*
// Workaround for buggy GCC 4.2/4.3
#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 4)
__attribute__((noinline))
#endif
__pure static uint64_t TimestampInterval(void *ts)
{
	return ( GET_UA64LE(ts) / TIME_C1 ) * TIME_C2 + TIME_C3;
}*/


/*
 * Creates the HMAC for v6
 */
static int_fast8_t CreateV6Hmac(BYTE *const encrypt_start, const size_t encryptSize, const int_fast8_t tolerance)
{
	BYTE hash[32];
	const uint8_t halfHashSize = sizeof(hash) >> 1;
	BYTE *responseEnd = encrypt_start + encryptSize;

	// This is the time from the response
	FILETIME* ft = (FILETIME*)(responseEnd - V6_POST_EPID_SIZE + sizeof(((RESPONSE*)0)->CMID));

	// Generate a time slot that changes every 4.11 hours.
	// Request and response time must match +/- 1 slot.
	// When generating a response tolerance must be 0.
	// If verifying the hash, try tolerance -1, 0 and +1. One of them must match.

	uint64_t timeSlot = LE64((GET_UA64LE(ft) / TIME_C1 * TIME_C2 + TIME_C3) + (tolerance * TIME_C1));

	// The time slot is hashed with SHA256 so it is not so obvious that it is time
	Sha256((BYTE*)&timeSlot, sizeof(timeSlot), hash);

	// The last 16 bytes of the hashed time slot are the actual HMAC key
	if (!Sha256Hmac
	(
		hash + halfHashSize,									// Use last 16 bytes of SHA256 as HMAC key
		encrypt_start,											// hash only the encrypted part of the v6 response
		(DWORD)(encryptSize - sizeof(((RESPONSE_V6*)0)->HMAC)),	// encryptSize minus the HMAC itself
		hash													// use same buffer for resulting hash where the key came from
	))
	{
		return FALSE;
	}

	memcpy(responseEnd - sizeof(((RESPONSE_V6*)0)->HMAC), hash + halfHashSize, halfHashSize);
	return TRUE;
}


/*
 * Creates v5 or v6 response
 */
size_t CreateResponseV6(REQUEST_V6 *restrict request_v6, BYTE *const responseBuffer, const char* const ipString)
{
	// The response will be created in a fixed sized struct to
	// avoid unaligned access macros and packed structs on RISC systems
	// which largely increase code size.
	//
	// The fixed sized struct with 64 WCHARs for the ePID will be converted
	// to a variable sized struct later and requires unaligned access macros.

	RESPONSE_V6* response = (RESPONSE_V6*)responseBuffer;
	RESPONSE* baseResponse = &response->ResponseBase;

#ifdef _DEBUG
	// ReSharper disable once CppDeclaratorNeverUsed
	RESPONSE_V6_DEBUG* xxx_unused = (RESPONSE_V6_DEBUG*)responseBuffer;
#endif

	static const BYTE DefaultHwId[8] = { HWID };
	const int_fast8_t v6 = LE16(request_v6->MajorVer) > 5;
	AesCtx aesCtx;

	AesInitKey(&aesCtx, v6 ? AesKeyV6 : AesKeyV5, v6, AES_KEY_BYTES);
	AesDecryptCbc(&aesCtx, NULL, request_v6->IV, V6_DECRYPT_SIZE);

	// get random salt and SHA256 it
	get16RandomBytes(response->RandomXoredIVs);
	Sha256(response->RandomXoredIVs, sizeof(response->RandomXoredIVs), response->Hash);

	if (v6) // V6 specific stuff
	{
		// In v6 a random IV is generated
		response->Version = request_v6->Version;
		get16RandomBytes(response->IV);

		// pre-fill with default HwId (not required for v5)
		memcpy(response->HwId, DefaultHwId, sizeof(response->HwId));

		// Just copy decrypted request IV (using Null IV) here. Note this is identical
		// to XORing non-decrypted request and response IVs
		memcpy(response->XoredIVs, request_v6->IV, sizeof(response->XoredIVs));
	}
	else // V5 specific stuff
	{
		// In v5 IVs of request and response must be identical (MS client checks this)
		// The following memcpy copies Version and IVs at once
		memcpy(response, request_v6, V6_UNENCRYPTED_SIZE);
	}

	// Xor Random bytes with decrypted request IV
	XorBlock(request_v6->IV, response->RandomXoredIVs);

	// Get the base response
	HRESULT hResult;
	if (FAILED(hResult = CreateResponseBase(&request_v6->RequestBase, baseResponse, response->HwId, ipString))) return hResult;

	// Convert the fixed sized struct into variable sized
	const DWORD pidSize = LE32(baseResponse->PIDSize);
	BYTE* postEpidPtr = responseBuffer + V6_PRE_EPID_SIZE + pidSize;
	const size_t post_epid_size = v6 ? V6_POST_EPID_SIZE : V5_POST_EPID_SIZE;

	memmove(postEpidPtr, &baseResponse->CMID, post_epid_size);

	// number of bytes to encrypt
	size_t encryptSize =
		V6_PRE_EPID_SIZE
		- sizeof(response->Version)
		+ pidSize
		+ post_epid_size;

	//AesDecryptBlock(&aesCtx, Response->IV);
	if (v6 && !CreateV6Hmac(response->IV, encryptSize, 0)) return 0;

	// Padding auto handled by encryption func
	AesEncryptCbc(&aesCtx, NULL, response->IV, &encryptSize);

	return encryptSize + sizeof(response->Version);
}


// Create Hashed KMS Client Request Data for KMS Protocol Version 4
BYTE *CreateRequestV4(size_t *size, const REQUEST* requestBase)
{
	*size = sizeof(REQUEST_V4);

	// Build a proper KMS client request data
	BYTE *request = (BYTE *)vlmcsd_malloc(sizeof(REQUEST_V4));

	// Temporary Pointer for access to REQUEST_V4 structure
	REQUEST_V4 *request_v4 = (REQUEST_V4 *)request;

	// Set KMS Client Request Base
	memcpy(&request_v4->RequestBase, requestBase, sizeof(REQUEST));

	// Generate Hash Signature
	AesCmacV4(request, sizeof(REQUEST), request_v4->MAC);

	// Return Request Data
	return request;
}


// Create Encrypted KMS Client Request Data for KMS Protocol Version 6
BYTE* CreateRequestV6(size_t *size, const REQUEST* requestBase)
{
	*size = sizeof(REQUEST_V6);

	// Temporary Pointer for access to REQUEST_V5 structure
	REQUEST_V6 *request = (REQUEST_V6 *)vlmcsd_malloc(sizeof(REQUEST_V6));

	// KMS Protocol Version
	request->Version = requestBase->Version;

	// Initialize the IV
	get16RandomBytes(request->IV);

	// Set KMS Client Request Base
	memcpy(&request->RequestBase, requestBase, sizeof(REQUEST));

	// Encrypt KMS Client Request
	size_t encryptSize = sizeof(request->RequestBase);
	AesCtx ctx;
	const int_fast8_t v6 = LE16(request->MajorVer) > 5;
	AesInitKey(&ctx, v6 ? AesKeyV6 : AesKeyV5, v6, 16);
	AesEncryptCbc(&ctx, request->IV, (BYTE*)(&request->RequestBase), &encryptSize);

	// Return Proper Request Data
	return (BYTE*)request;
}


/*
 * Checks whether Length of ePID is valid
 */
static uint8_t checkPidLength(const RESPONSE *const responseBase)
{
	unsigned int i;

	if (LE32(responseBase->PIDSize) > (PID_BUFFER_SIZE << 1)) return FALSE;
	if (responseBase->KmsPID[(LE32(responseBase->PIDSize) >> 1) - 1]) return FALSE;

	for (i = 0; i < (LE32(responseBase->PIDSize) >> 1) - 2; i++)
	{
		if (!responseBase->KmsPID[i]) return FALSE;
	}

	return TRUE;
}


/*
 * "Decrypts" a KMS v4 response. Actually just copies to a fixed size buffer
 */
RESPONSE_RESULT DecryptResponseV4(RESPONSE_V4* response_v4, const int responseSize, BYTE* const rawResponse, const BYTE* const rawRequest)
{
	const int copySize =
		V4_PRE_EPID_SIZE +
		(LE32(((RESPONSE_V4*)rawResponse)->ResponseBase.PIDSize) <= PID_BUFFER_SIZE << 1 ?
			LE32(((RESPONSE_V4*)rawResponse)->ResponseBase.PIDSize) :
			PID_BUFFER_SIZE << 1);

	const int messageSize = copySize + V4_POST_EPID_SIZE;

	memcpy(response_v4, rawResponse, copySize);
	memcpy(&response_v4->ResponseBase.CMID, rawResponse + copySize, responseSize - copySize);

	// ensure PID is null terminated
	response_v4->ResponseBase.KmsPID[PID_BUFFER_SIZE - 1] = 0;

	uint8_t* mac = rawResponse + messageSize;
	AesCmacV4(rawResponse, messageSize, mac);

	REQUEST_V4* request_v4 = (REQUEST_V4*)rawRequest;
	RESPONSE_RESULT result;

	result.mask = (DWORD)~0;
	result.PidLengthOK = checkPidLength((RESPONSE*)rawResponse);
	result.VersionOK = response_v4->ResponseBase.Version == request_v4->RequestBase.Version;
	result.HashOK = !memcmp(&response_v4->MAC, mac, sizeof(response_v4->MAC));
	result.TimeStampOK = !memcmp(&response_v4->ResponseBase.ClientTime, &request_v4->RequestBase.ClientTime, sizeof(FILETIME));
	result.ClientMachineIDOK = !memcmp(&response_v4->ResponseBase.CMID, &request_v4->RequestBase.CMID, sizeof(GUID));
	result.effectiveResponseSize = responseSize;
	result.correctResponseSize = sizeof(RESPONSE_V4) - sizeof(response_v4->ResponseBase.KmsPID) + LE32(response_v4->ResponseBase.PIDSize);

	return result;
}


static RESPONSE_RESULT VerifyResponseV6(RESPONSE_RESULT result, RESPONSE_V6* response_v6, REQUEST_V6* request_v6, BYTE* const rawResponse)
{
	// Check IVs
	result.IVsOK = !memcmp // In V6 the XoredIV is actually the request IV
	(
		response_v6->XoredIVs,
		request_v6->IV,
		sizeof(response_v6->XoredIVs)
	);

	result.IVnotSuspicious = !!memcmp // If IVs are identical, it is obviously an emulator
	(
		request_v6->IV,
		response_v6->IV,
		sizeof(request_v6->IV)
	);

	// Check Hmac
	int_fast8_t tolerance;
	BYTE OldHmac[sizeof(response_v6->HMAC)];

	result.HmacSha256OK = FALSE;

	memcpy	// Save received HMAC to compare with calculated HMAC later
	(
		OldHmac,
		response_v6->HMAC,
		sizeof(response_v6->HMAC)
	);

	//AesEncryptBlock(Ctx, Response_v6->IV); // CreateV6Hmac needs original IV as received over the network

	for (tolerance = -1; tolerance < 2; tolerance++)
	{
		CreateV6Hmac
		(
			rawResponse + sizeof(response_v6->Version),                          // Pointer to start of the encrypted part of the response
			(size_t)result.correctResponseSize - sizeof(response_v6->Version),   // size of the encrypted part
			tolerance                                                            // tolerance -1, 0, or +1
		);

		result.HmacSha256OK = !memcmp // Compare both HMACs
		(
			OldHmac,
			rawResponse + (size_t)result.correctResponseSize - sizeof(response_v6->HMAC),
			sizeof(OldHmac)
		);

		if (result.HmacSha256OK) break;
	}

	return result;
}


static RESPONSE_RESULT VerifyResponseV5(RESPONSE_RESULT result, REQUEST_V5* request_v5, RESPONSE_V5* response_v5)
{
	// Check IVs: in V5 (and only v5) request and response IVs must match
	result.IVsOK = !memcmp(request_v5->IV, response_v5->IV, sizeof(request_v5->IV));

	// V5 has no Hmac, always set to TRUE
	result.HmacSha256OK = TRUE;

	return result;
}


/*
 * Decrypts a KMS v5 or v6 response received from a server.
 * hwid must supply a valid 16 byte buffer for v6. hwid is ignored in v5
 */
RESPONSE_RESULT DecryptResponseV6(RESPONSE_V6* response_v6, int responseSize, BYTE* const response, const BYTE* const rawRequest, BYTE* hwid)
{
	RESPONSE_RESULT result;
	result.mask = (DWORD)~0; // Set all bits in the results mask to 1. Assume success first.
	result.effectiveResponseSize = responseSize;

	int copySize1 =
		sizeof(response_v6->Version);

	// Decrypt KMS Server Response (encrypted part starts after RequestIV)
	responseSize -= copySize1;

	AesCtx ctx;
	const int_fast8_t v6 = LE16(((RESPONSE_V6*)response)->MajorVer) > 5;

	AesInitKey(&ctx, v6 ? AesKeyV6 : AesKeyV5, v6, AES_KEY_BYTES);
	AesDecryptCbc(&ctx, NULL, response + copySize1, responseSize);

	// Check padding
	BYTE* lastPadByte = response + (size_t)result.effectiveResponseSize - 1;

	// Must be from 1 to 16
	if (!*lastPadByte || *lastPadByte > AES_BLOCK_BYTES)
	{
		result.DecryptSuccess = FALSE;
		return result;
	}

	// Check if pad bytes are all the same
	BYTE* padByte;
	for (padByte = lastPadByte - *lastPadByte + 1; padByte < lastPadByte; padByte++)
	{
		if (*padByte != *lastPadByte)
		{
			result.DecryptSuccess = FALSE;
			return result;
		}
	}

	// Add size of Version, KmsPIDLen and variable size PID
	const DWORD pidSize = LE32(((RESPONSE_V6*)response)->ResponseBase.PIDSize);

	copySize1 +=
		V6_UNENCRYPTED_SIZE +
		sizeof(response_v6->ResponseBase.PIDSize) +
		(pidSize <= PID_BUFFER_SIZE << 1 ? pidSize : PID_BUFFER_SIZE << 1);

	// Copy part 1 of response up to variable sized PID
	memcpy(response_v6, response, copySize1);

	// ensure PID is null terminated
	response_v6->ResponseBase.KmsPID[PID_BUFFER_SIZE - 1] = 0;

	// Copy part 2
	const size_t copySize2 = v6 ? V6_POST_EPID_SIZE : V5_POST_EPID_SIZE;
	memcpy(&response_v6->ResponseBase.CMID, response + copySize1, copySize2);

	// Decrypting the response is finished here. Now we check the results for validity
	// A basic client doesn't need the stuff below this comment but we want to use vlmcs
	// as a debug tool for KMS emulators.

	REQUEST_V6* request_v6 = (REQUEST_V6*)rawRequest;
	const DWORD decryptSize = sizeof(request_v6->IV) + sizeof(request_v6->RequestBase) + sizeof(request_v6->Pad);

	AesDecryptCbc(&ctx, NULL, request_v6->IV, decryptSize);

	// Check that all version information is the same
	result.VersionOK =
		request_v6->Version == response_v6->ResponseBase.Version &&
		request_v6->Version == response_v6->Version &&
		request_v6->Version == request_v6->RequestBase.Version;

	// Check Base Request
	result.PidLengthOK = checkPidLength(&((RESPONSE_V6*)response)->ResponseBase);
	result.TimeStampOK = !memcmp(&response_v6->ResponseBase.ClientTime, &request_v6->RequestBase.ClientTime, sizeof(FILETIME));
	result.ClientMachineIDOK = IsEqualGUID(&response_v6->ResponseBase.CMID, &request_v6->RequestBase.CMID);

	// Rebuild Random Key and Sha256 Hash
	BYTE hashVerify[sizeof(response_v6->Hash)];
	BYTE randomKey[sizeof(response_v6->RandomXoredIVs)];

	memcpy(randomKey, request_v6->IV, sizeof(randomKey));
	XorBlock(response_v6->RandomXoredIVs, randomKey);
	Sha256(randomKey, sizeof(randomKey), hashVerify);

	result.HashOK = !memcmp(response_v6->Hash, hashVerify, sizeof(hashVerify));

	// size before encryption (padding not included)
	result.correctResponseSize =
		(v6 ? sizeof(RESPONSE_V6) : sizeof(RESPONSE_V5))
		- sizeof(response_v6->ResponseBase.KmsPID)
		+ LE32(response_v6->ResponseBase.PIDSize);

	// Version specific stuff
	if (v6)
	{
		// Copy the HwId
		memcpy(hwid, response_v6->HwId, sizeof(response_v6->HwId));

		// Verify the V6 specific part of the response
		result = VerifyResponseV6(result, response_v6, request_v6, response);
	}
	else // V5
	{
		// Verify the V5 specific part of the response
		result = VerifyResponseV5(result, request_v6, (RESPONSE_V5*)response_v6);
	}

	// padded size after encryption
	result.correctResponseSize += (~(result.correctResponseSize - sizeof(response_v6->ResponseBase.Version)) & 0xf) + 1;

	return result;
}

