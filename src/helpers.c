/*
 * Helper functions used by other modules
 */

 //#ifndef _GNU_SOURCE
 //#define _GNU_SOURCE
 //#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifndef _WIN32
#include <errno.h>
#include <libgen.h>
#endif // _WIN32
#ifndef _MSC_VER
#include <getopt.h>
#else
#include "wingetopt.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "helpers.h"
#include "output.h"
#include "endian.h"
#include "shared_globals.h"

#ifndef NO_INTERNAL_DATA
#include "kmsdata.h"
#endif // NO_INTERNAL_DATA

#ifdef _WIN32
#include <shlwapi.h>
#endif // _WIN32

#if __APPLE__
#include <mach-o/dyld.h>
#endif // __APPLE__

#if (__GLIBC__ || __linux__) && defined(USE_AUXV)
#include <sys/auxv.h>
#endif

#if __FreeBSD__ || __FreeBSD_kernel__
#include <sys/sysctl.h>
#endif

 /*
  *  UCS2 <-> UTF-8 functions
  *  All functions use little endian UCS2 since we only need it to communicate with Windows via RPC
  */

  // Convert one character from UTF-8 to UCS2
  // Returns 0xffff, if utf-8 evaluates to > 0xfffe (outside basic multilingual pane)
WCHAR utf8_to_ucs2_char(const unsigned char *input, const unsigned char **end_ptr)
{
	*end_ptr = input;
	if (input[0] == 0)
		return (WCHAR)~0;

	if (input[0] < 0x80) {
		*end_ptr = input + 1;
		return LE16(input[0]);
	}

	if ((input[0] & 0xE0) == 0xE0) {

		if (input[1] == 0 || input[2] == 0)
			return (WCHAR)~0;

		*end_ptr = input + 3;

		return
			LE16((input[0] & 0x0F) << 12 |
			(input[1] & 0x3F) << 6 |
				(input[2] & 0x3F));
	}

	if ((input[0] & 0xC0) == 0xC0) {
		if (input[1] == 0)
			return (WCHAR)~0;

		*end_ptr = input + 2;

		return
			LE16((input[0] & 0x1F) << 6 |
			(input[1] & 0x3F));
	}
	return (WCHAR)~0;
}

// Convert one character from UCS2 to UTF-8
// Returns length of UTF-8 char (1, 2 or 3) or -1 on error (UTF-16 outside UCS2)
// char *utf8 must be large enough to hold 3 bytes
int ucs2_to_utf8_char(const WCHAR ucs2_le, char *utf8)
{
	const WCHAR ucs2 = LE16(ucs2_le);

	if (ucs2 < 0x80) {
		utf8[0] = (char)ucs2;
		utf8[1] = '\0';
		return 1;
	}

	if (ucs2 >= 0x80 && ucs2 < 0x800) {
		utf8[0] = (char)((ucs2 >> 6) | 0xC0);
		utf8[1] = (char)((ucs2 & 0x3F) | 0x80);
		utf8[2] = '\0';
		return 2;
	}

	if (ucs2 >= 0x800 && ucs2 < 0xFFFF) {

		if (ucs2 >= 0xD800 && ucs2 <= 0xDFFF) {
			/* Ill-formed (UTF-16 ouside of BMP) */
			return -1;
		}

		utf8[0] = ((ucs2 >> 12)) | 0xE0;
		utf8[1] = ((ucs2 >> 6) & 0x3F) | 0x80;
		utf8[2] = ((ucs2) & 0x3F) | 0x80;
		utf8[3] = '\0';
		return 3;
	}

	return -1;
}


// Converts UTF8 to UCS2. Returns size in bytes of the converted string or -1 on error
size_t utf8_to_ucs2(WCHAR* const ucs2_le, const char* const utf8, const size_t maxucs2, const size_t maxutf8)
{
	const unsigned char* current_utf8 = (unsigned char*)utf8;
	WCHAR* current_ucs2_le = ucs2_le;

	for (; *current_utf8; current_ucs2_le++)
	{
		size_t size = (char*)current_utf8 - utf8;

		if (size >= maxutf8) return (size_t)-1;
		if (((*current_utf8 & 0xc0) == 0xc0) && (size >= maxutf8 - 1)) return (size_t)-1;
		if (((*current_utf8 & 0xe0) == 0xe0) && (size >= maxutf8 - 2)) return (size_t)-1;
		if (current_ucs2_le - ucs2_le >= (intptr_t)maxucs2 - 1) return (size_t)-1;

		*current_ucs2_le = utf8_to_ucs2_char(current_utf8, &current_utf8);
		current_ucs2_le[1] = 0;

		if (*current_ucs2_le == (WCHAR)-1) return (size_t)-1;
	}
	return current_ucs2_le - ucs2_le;
}

// Converts UCS2 to UTF-8. Returns TRUE or FALSE
BOOL ucs2_to_utf8(const WCHAR* const ucs2_le, char* utf8, size_t maxucs2, size_t maxutf8)
{
	char utf8_char[4];
	const WCHAR* current_ucs2 = ucs2_le;
	unsigned int index_utf8 = 0;

	for (*utf8 = 0; *current_ucs2; current_ucs2++)
	{
		if (current_ucs2 - ucs2_le > (intptr_t)maxucs2) return FALSE;
		int len = ucs2_to_utf8_char(*current_ucs2, utf8_char);
		if (index_utf8 + len > maxutf8) return FALSE;
		strncat(utf8, utf8_char, len);
		index_utf8 += len;
	}

	return TRUE;
}

/* End of UTF-8 <-> UCS2 conversion */


// Checks, whether a string is a valid integer number between min and max. Returns TRUE or FALSE. Puts int value in *value
BOOL stringToInt(const char *const szValue, const unsigned int min, const unsigned int max, unsigned int *const value)
{
	char *nextchar;

	errno = 0;
	long long result = vlmcsd_strtoll(szValue, &nextchar, 10);

	if (errno || result < (long long)min || result >(long long)max || *nextchar)
	{
		return FALSE;
	}

	*value = (unsigned int)result;
	return TRUE;
}


//Converts a String Guid to a host binary guid in host endianess
int_fast8_t string2UuidLE(const char *const restrict input, GUID *const restrict guid)
{
	int i;

	if (strlen(input) < GUID_STRING_LENGTH) return FALSE;
	if (input[8] != '-' || input[13] != '-' || input[18] != '-' || input[23] != '-') return FALSE;

	for (i = 0; i < GUID_STRING_LENGTH; i++)
	{
		if (i == 8 || i == 13 || i == 18 || i == 23) continue;

		const char c = (char)toupper((int)input[i]);

		if (c < '0' || c > 'F' || (c > '9' && c < 'A')) return FALSE;
	}

	char inputCopy[GUID_STRING_LENGTH + 1];
	strncpy(inputCopy, input, GUID_STRING_LENGTH + 1);
	inputCopy[8] = inputCopy[13] = inputCopy[18] = 0;

	hex2bin((BYTE*)&guid->Data1, inputCopy, 8);
	hex2bin((BYTE*)&guid->Data2, inputCopy + 9, 4);
	hex2bin((BYTE*)&guid->Data3, inputCopy + 14, 4);
	hex2bin(guid->Data4, input + 19, 16);

	guid->Data1 = BS32(guid->Data1);
	guid->Data2 = BS16(guid->Data2);
	guid->Data3 = BS16(guid->Data3);
	return TRUE;
}


__pure DWORD timeSpanString2Seconds(const char *const restrict argument)
{
	char *unitId;

	long long val = vlmcsd_strtoll(argument, &unitId, 10);

	switch (toupper((int)*unitId))
	{
	case 'W':
		val *= 7;
	case 'D':
		val *= 24;
	case 'H':
		val *= 60;
	case 0:
	case 'M':
		val *= 60;
	case 'S':
		break;
	default:
		return 0;
	}

	if (*unitId && unitId[1]) return 0;
	if (val < 1) val = 1;
	return (DWORD)(val & UINT_MAX);
}


#if !IS_LIBRARY
//Checks a command line argument if it is numeric and between min and max. Returns the numeric value or exits on error
__pure unsigned int getOptionArgumentInt(const char o, const unsigned int min, const unsigned int max)
{
	unsigned int result;

	if (!stringToInt(optarg, min, max, &result))
	{
		printerrorf("Fatal: Option \"-%c\" must be numeric between %u and %u.\n", o, min, max);
		exit(VLMCSD_EINVAL);
	}

	return result;
}

// Resets getopt() to start parsing from the beginning
void optReset(void)
{
#if __minix__ || defined(__BSD__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__OpenBSD__)
	optind = 1;
	optreset = 1; // Makes newer BSD getopt happy
#elif defined(__UCLIBC__) // uClibc headers also define __GLIBC__ so be careful here
	optind = 0; // uClibc seeks compatibility with GLIBC
#elif defined(__GLIBC__)
	optind = 0; // Makes GLIBC getopt happy
#else // Standard for most systems
	optind = 1;
#endif
}
#endif // !IS_LIBRARY

#if _WIN32 || __CYGWIN__

// Returns a static message buffer containing text for a given Win32 error. Not thread safe (same as strerror)
char* win_strerror(const int message)
{
#define STRERROR_BUFFER_SIZE 256
	static char buffer[STRERROR_BUFFER_SIZE];

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, message, 0, buffer, STRERROR_BUFFER_SIZE, NULL);
	return buffer;
}

#endif // _WIN32 || __CYGWIN__


/*
 * parses an address in the form host:[port] in addr
 * returns host and port in seperate strings
 */
void parseAddress(char *const addr, char** szHost, char** szPort)
{
	*szHost = addr;

#	ifndef NO_SOCKETS
	*szPort = (char*)defaultport;
#	else // NO_SOCKETS
	*szPort = "1688";
#	endif // NO_SOCKETS

	char *lastcolon = strrchr(addr, ':');
	char *firstcolon = strchr(addr, ':');
	char *closingbracket = strrchr(addr, ']');

	if (*addr == '[' && closingbracket) //Address in brackets
	{
		*closingbracket = 0;
		(*szHost)++;

		if (closingbracket[1] == ':')
			*szPort = closingbracket + 2;
	}
	else if (firstcolon && firstcolon == lastcolon) //IPv4 address or hostname with port
	{
		*firstcolon = 0;
		*szPort = firstcolon + 1;
	}
}


// Initialize random generator (needs to be done in each thread)
void randomNumberInit()
{
#	if _MSC_VER
	srand(GetTickCount());
#	else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand((unsigned int)(tv.tv_sec ^ tv.tv_usec));
#	endif
}


// We always exit immediately if any OOM condition occurs
__noreturn void OutOfMemory(void)
{
	errorout("Fatal: Out of memory");
	exit(VLMCSD_ENOMEM);
}


void* vlmcsd_malloc(size_t len)
{
	void* buf = malloc(len);
	if (!buf) OutOfMemory();
	return buf;
}

char* vlmcsd_strdup(const char* src)
{
#	if _MSC_VER
	char* dst = _strdup(src);
#	else // !_MSC_VER
	char* dst = strdup(src);
#	endif

	if (!dst) OutOfMemory();
	return dst;
}


/*
 * Converts hex digits to bytes in big-endian order.
 * Ignores any non-hex characters
 */
void hex2bin(BYTE *const bin, const char *hex, const size_t maxbin)
{
	static const char *const hexdigits = "0123456789ABCDEF";
	char* nextchar;
	size_t i;

	for (i = 0; (i < 16) && utf8_to_ucs2_char((const unsigned char*)hex, (const unsigned char**)&nextchar) != (WCHAR)-1; hex = nextchar)
	{
		const char* pos = strchr(hexdigits, toupper((int)*hex));
		if (!pos) continue;

		if (!(i & 1)) bin[i >> 1] = 0;
		bin[i >> 1] |= (char)(pos - hexdigits);
		if (!(i & 1)) bin[i >> 1] <<= 4;
		i++;
		if (i >> 1 > maxbin) break;
	}
}


__pure BOOL getArgumentBool(int_fast8_t *result, const char *const argument)
{
	if (
		!strncasecmp(argument, "true", 4) ||
		!strncasecmp(argument, "on", 2) ||
		!strncasecmp(argument, "yes", 3) ||
		!strncasecmp(argument, "1", 1)
		)
	{
		*result = TRUE;
		return TRUE;
	}
	else if (
		!strncasecmp(argument, "false", 5) ||
		!strncasecmp(argument, "off", 3) ||
		!strncasecmp(argument, "no", 2) ||
		!strncasecmp(argument, "0", 1)
		)
	{
		*result = FALSE;
		return TRUE;
	}

	return FALSE;
}

#ifndef IS_LIBRARY
#ifndef NO_EXTERNAL_DATA
__noreturn static void dataFileReadError()
{
	const int error = errno;
	errorout("Fatal: Could not read %s: %s\n", fn_data, strerror(error));
	exit(error);
}

__noreturn static void dataFileFormatError()
{
	errorout("Fatal: %s is not a KMS data file version 2.x\n", fn_data);
	exit(VLMCSD_EINVAL);
}
#endif // NO_EXTERNAL_DATA

#if !defined(DATA_FILE) || !defined(NO_SIGHUP)
void getExeName()
{
	if (fn_exe != NULL) return;

#	if (__GLIBC__ || __linux__) && defined(USE_AUXV)

	fn_exe = (char*)getauxval(AT_EXECFN);

#	elif (__ANDROID__ && __ANDROID_API__ < 16) || (__UCLIBC__ && __UCLIBC_MAJOR__ < 1 && !defined(NO_PROCFS)) // Workaround for older uclibc

	char temp[PATH_MAX + 1];

	if (realpath("/proc/self/exe", temp) == temp)
	{
		fn_exe = vlmcsd_strdup(temp);
	}

#	elif (__linux__ || __CYGWIN__) && !defined(NO_PROCFS)

	fn_exe = realpath("/proc/self/exe", NULL);

#	elif (__FreeBSD__ || __FreeBSD_kernel__)

	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	char path[PATH_MAX + 1];
	size_t cb = sizeof(path);

	if (!sysctl(mib, 4, path, &cb, NULL, 0))
	{
		fn_exe = vlmcsd_strdup(path);
	}

#	elif (__DragonFly__) && !defined(NO_PROCFS)

	fn_exe = realpath("/proc/curproc/file", NULL);

#	elif __NetBSD__ && !defined(NO_PROCFS)

	fn_exe = realpath("/proc/curproc/exe", NULL);

#	elif __sun__

	fn_exe = getexecname();

#	elif __APPLE__

	char path[PATH_MAX + 1];
	uint32_t size = sizeof(path);

	if (_NSGetExecutablePath(path, &size) == 0)
	{
		fn_exe = vlmcsd_strdup(path);
	}

#	elif _WIN32

	char path[512];
	GetModuleFileName(GetModuleHandle(NULL), path, 512);
	path[511] = 0;
	fn_exe = vlmcsd_strdup(path);

#	else
	// Sorry no exe detection
#	endif
}
#endif // defined(DATA_FILE) && defined(NO_SIGHUP)

#if !defined(DATA_FILE) && !defined(NO_EXTERNAL_DATA)
#ifdef _WIN32
static void getDefaultDataFile()
{
	char fileName[MAX_PATH];
	getExeName();
	strncpy(fileName, fn_exe, MAX_PATH);
	PathRemoveFileSpec(fileName);
	strncat(fileName, "\\vlmcsd.kmd", MAX_PATH - 11);
	fn_data = vlmcsd_strdup(fileName);
}
#else // !_WIN32
static void getDefaultDataFile()
{
	char fileName[512];
	getExeName();

	if (!fn_exe)
	{
		fn_data = (char*)"/etc/vlmcsd.kmd";
		return;
	}

	char* fn_exe_copy = vlmcsd_strdup(fn_exe);
	strncpy(fileName, dirname(fn_exe_copy), 512);
	free(fn_exe_copy);
	strncat(fileName, "/vlmcsd.kmd", 500);
	fn_data = vlmcsd_strdup(fileName);
}
#endif // !_WIN32
#endif // !defined(DATA_FILE) && !defined(NO_EXTERNAL_DATA)

void loadKmsData()
{
#	ifndef NO_INTERNAL_DATA
	KmsData = (PVlmcsdHeader_t)DefaultKmsData;
#	endif // NO_INTERNAL_DATA

#	ifndef NO_EXTERNAL_DATA
	long size;
#	ifndef NO_INTERNAL_DATA
	size = (long)getDefaultKmsDataSize();
#	endif // NO_INTERNAL_DATA

#	ifndef DATA_FILE
	if (!fn_data) getDefaultDataFile();
#	endif // DATA_FILE

	if (strcmp(fn_data, "-"))
	{
		FILE *file = fopen(fn_data, "rb");

		if (!file)
		{
#			ifndef NO_INTERNAL_DATA
			if (ExplicitDataLoad)
#			endif // NO_INTERNAL_DATA
			{
				dataFileReadError();
			}
		}
		else
		{
			if (fseek(file, 0, SEEK_END)) dataFileReadError();
			size = ftell(file);
			if (size == -1L) dataFileReadError();

			KmsData = (PVlmcsdHeader_t)vlmcsd_malloc(size);
			if (fseek(file, 0, SEEK_SET)) dataFileReadError();

			const size_t bytesRead = fread(KmsData, 1, size, file);
			if ((long)bytesRead != size) dataFileReadError();
			fclose(file);

#			if !defined(NO_LOG) && !defined(NO_SOCKETS)
			if (!InetdMode) logger("Read KMS data file version %u.%u %s\n", (unsigned int)LE16(KmsData->MajorVer), (unsigned int)LE16(KmsData->MinorVer), fn_data);
#			endif // NO_LOG
		}
	}


#	endif // NO_EXTERNAL_DATA

#	ifndef UNSAFE_DATA_LOAD
	if (((BYTE*)KmsData)[size - 1] != 0) dataFileFormatError();
#	endif // UNSAFE_DATA_LOAD

	KmsData->MajorVer = LE16(KmsData->MajorVer);
	KmsData->MinorVer = LE16(KmsData->MinorVer);
	KmsData->AppItemCount = LE32(KmsData->AppItemCount);
	KmsData->KmsItemCount = LE32(KmsData->KmsItemCount);
	KmsData->SkuItemCount = LE32(KmsData->SkuItemCount);
	KmsData->HostBuildCount = LE32(KmsData->HostBuildCount);

	uint32_t i;

	for (i = 0; i < vlmcsd_countof(KmsData->Datapointers); i++)
	{
		KmsData->Datapointers[i].Pointer = (BYTE*)KmsData + LE64(KmsData->Datapointers[i].Offset);
#		ifndef UNSAFE_DATA_LOAD
		if ((BYTE*)KmsData->Datapointers[i].Pointer > (BYTE*)KmsData + size) dataFileFormatError();
#		endif // UNSAFE_DATA_LOAD
	}

	for (i = 0; i < KmsData->CsvlkCount; i++)
	{
		PCsvlkData_t csvlkData = &KmsData->CsvlkData[i];
		csvlkData->EPid = (char*)KmsData + LE64(csvlkData->EPidOffset);
		csvlkData->ReleaseDate = LE64(csvlkData->ReleaseDate);
#		ifndef UNSAFE_DATA_LOAD
		if (csvlkData->EPid > (char*)KmsData + size) dataFileFormatError();
#		endif // UNSAFE_DATA_LOAD

#		ifndef NO_RANDOM_EPID
		csvlkData->GroupId = LE32(csvlkData->GroupId);
		csvlkData->MinKeyId = LE32(csvlkData->MinKeyId);
		csvlkData->MaxKeyId = LE32(csvlkData->MaxKeyId);
#		endif // NO_RANDOM_EPID
	}

	for (i = 0; i < (uint32_t)KmsData->HostBuildCount; i++)
	{
		PHostBuild_t hostBuild = &KmsData->HostBuildList[i];
		hostBuild->BuildNumber = LE32(hostBuild->BuildNumber);
		hostBuild->Flags = LE32(hostBuild->Flags);
		hostBuild->PlatformId = LE32(hostBuild->PlatformId);
		hostBuild->ReleaseDate = LE64(hostBuild->ReleaseDate);
		hostBuild->DisplayName = (char*)KmsData + LE64(hostBuild->DisplayNameOffset);
#		ifndef UNSAFE_DATA_LOAD
		if (hostBuild->DisplayName > (char*)KmsData + size) dataFileFormatError();
#		endif // UNSAFE_DATA_LOAD
	}

	const uint32_t totalItemCount = KmsData->AppItemCount + KmsData->KmsItemCount + KmsData->SkuItemCount;

#	ifndef NO_EXTERNAL_DATA
	if (
		memcmp(KmsData->Magic, "KMD", sizeof(KmsData->Magic)) ||
		KmsData->MajorVer != 2
#		ifndef UNSAFE_DATA_LOAD
		||
		sizeof(VlmcsdHeader_t) + totalItemCount * sizeof(VlmcsdData_t) >= ((uint64_t)size)
#		endif //UNSAFE_DATA_LOAD
		)
	{
		dataFileFormatError();
	}
#	endif // NO_EXTERNAL_DATA

	for (i = 0; i < totalItemCount; i++)
	{
		PVlmcsdData_t item = &KmsData->AppItemList[i];
		item->Name = (char*)KmsData + LE64(item->NameOffset);

#		ifndef UNSAFE_DATA_LOAD
		if (
			item->Name >= (char*)KmsData + (uint64_t)size ||
			(KmsData->AppItemCount && item->AppIndex >= KmsData->AppItemCount) ||
			item->KmsIndex >= KmsData->KmsItemCount
			)
		{
			dataFileFormatError();
		}
#		endif // UNSAFE_DATA_LOAD
	}
}

#ifndef NO_SOCKETS
void exitOnWarningLevel(const int_fast8_t level)
{
	if (ExitLevel >= level)
	{
		printerrorf("Fatal: Exiting on warning level %i or greater\n", (int)ExitLevel);
		exit(-1);
	}
}
#endif // !NO_SOCKETS

#endif // IS_LIBRARY


#if __ANDROID__ && !defined(USE_THREADS) // Bionic does not wrap these syscalls (intentionally because Google fears, developers don't know how to use it)

#ifdef __NR_shmget
int shmget(key_t key, size_t size, int shmflg)
{
	return syscall(__NR_shmget, key, size, shmflg);
}
#endif // __NR_shmget

#ifdef __NR_shmat
void *shmat(int shmid, const void *shmaddr, int shmflg)
{
	return (void *)syscall(__NR_shmat, shmid, shmaddr, shmflg);
}
#endif // __NR_shmat

#ifdef __NR_shmdt
int shmdt(const void *shmaddr)
{
	return syscall(__NR_shmdt, shmaddr);
}
#endif // __NR_shmdt

#ifdef __NR_shmctl
int shmctl(int shmid, int cmd, /*struct shmid_ds*/void *buf)
{
	return syscall(__NR_shmctl, shmid, cmd, buf);
}
#endif // __NR_shmctl

#endif // __ANDROID__ && !defined(USE_THREADS)


