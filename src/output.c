#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif // _DEFAULT_SOURCE

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "output.h"
#include "shared_globals.h"
#include "endian.h"
#include "helpers.h"

// ReSharper disable All

#ifndef NO_LOG
static void vlogger(const char *message, va_list args)
{
	FILE *log;

#	ifdef _NTSERVICE
	if (!IsNTService && logstdout) log = stdout;
#	else
	if (logstdout) log = stdout;
#	endif
	else
	{
		if (fn_log == NULL) return;

#		ifndef _WIN32
		if (!strcmp(fn_log, "syslog"))
		{
			openlog("vlmcsd", LOG_CONS | LOG_PID, LOG_USER);

			////PORTABILITY: vsyslog is not in Posix but virtually all Unixes have it
			vsyslog(LOG_INFO, message, args);

			closelog();
			return;
		}
#		endif // _WIN32

		log = fopen(fn_log, "a");
		if (!log) return;
	}

	time_t now = time(0);

#	ifdef USE_THREADS
	char mbstr[2048];
#	else
	char mbstr[24];
#	endif

	if (LogDateAndTime)
		strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %X: ", localtime(&now));
	else
		*mbstr = 0;

#	ifndef USE_THREADS

	fprintf(log, "%s", mbstr);
	vfprintf(log, message, args);
	fflush(log);

#	else // USE_THREADS

	// We write everything to a string before we really log inside the critical section
	// so formatting the output can be concurrent
	int len = (int)strlen(mbstr);
	//#	if !_MSC_VER

	vlmcsd_vsnprintf(mbstr + len, sizeof(mbstr) - len, message, args);
	//#	else
	//	wvsprintf(mbstr + len, message, args);
	//#	endif

	lock_mutex(&logmutex);
	fprintf(log, "%s", mbstr);
	fflush(log);
	unlock_mutex(&logmutex);

#	endif // USE_THREADS
	if (log != stdout) fclose(log);
}


// Always sends to log output
int logger(const char *const fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogger(fmt, args);
	va_end(args);
	return 0;
}

#endif //NO_LOG


// Output to stderr if it is available or to log otherwise (e.g. if running as daemon/service)
int printerrorf(const char *const fmt, ...)
{
	int error = errno;
	va_list arglist;

	va_start(arglist, fmt);

#	ifdef IS_LIBRARY

	size_t len = strlen(ErrorMessage);
	vlmcsd_vsnprintf(ErrorMessage + len, MESSAGE_BUFFER_SIZE - len - 1, fmt, arglist);

#	else // !IS_LIBRARY

#	ifndef NO_LOG
#	ifdef _NTSERVICE
	if (InetdMode || IsNTService)
#	else // !_NTSERVICE
	if (InetdMode)
#	endif // NTSERVIICE
		vlogger(fmt, arglist);
	else
#	endif //NO_LOG

#	endif // IS_LIBRARY
	{
		vfprintf(stderr, fmt, arglist);
		fflush(stderr);
	}

	va_end(arglist);
	errno = error;
	return 0;
}


// Always output to stderr
int errorout(const char* fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	int i = vfprintf(stderr, fmt, args);
	va_end(args);
	fflush(stderr);

	return i;
}


#if !defined(NO_VERBOSE_LOG) && !defined(NO_LOG)
static const char *LicenseStatusText[] =
{
	"Unlicensed", "Licensed", "OOB grace", "OOT grace", "Non-Genuine", "Notification", "Extended grace"
};
#endif // !defined(NO_VERBOSE_LOG) && !defined(NO_LOG)


void uuid2StringLE(const GUID *const guid, char *const string)
{
	sprintf(string,
#		ifdef _WIN32
		"%08x-%04x-%04x-%04x-%012I64x",
#		else
		"%08x-%04x-%04x-%04x-%012llx",
#		endif
		(unsigned int)LE32(guid->Data1),
		(unsigned int)LE16(guid->Data2),
		(unsigned int)LE16(guid->Data3),
		(unsigned int)BE16(*(uint16_t*)guid->Data4),
		(unsigned long long)BE64(*(uint64_t*)(guid->Data4)) & 0xffffffffffffLL
	);
}

#if !defined(NO_VERBOSE_LOG) && !defined(NO_LOG)
void logRequestVerbose(REQUEST* Request, const PRINTFUNC p)
{
	char guidBuffer[GUID_STRING_LENGTH + 1];
	char WorkstationBuffer[3 * WORKSTATION_NAME_BUFFER];
	char* productName;

	p("Protocol version                : %u.%u\n", LE16(Request->MajorVer), LE16(Request->MinorVer));
	p("Client is a virtual machine     : %s\n", LE32(Request->VMInfo) ? "Yes" : "No");
	p("Licensing status                : %u (%s)\n", (uint32_t)LE32(Request->LicenseStatus), LE32(Request->LicenseStatus) < vlmcsd_countof(LicenseStatusText) ? LicenseStatusText[LE32(Request->LicenseStatus)] : "Unknown");
	p("Remaining time (0 = forever)    : %i minutes\n", (uint32_t)LE32(Request->BindingExpiration));

	uuid2StringLE(&Request->AppID, guidBuffer);
	getProductIndex(&Request->AppID, KmsData->AppItemList, KmsData->AppItemCount, &productName, NULL);
	p("Application ID                  : %s (%s)\n", guidBuffer, productName);

	uuid2StringLE(&Request->ActID, guidBuffer);
	getProductIndex(&Request->ActID, KmsData->SkuItemList, KmsData->SkuItemCount, &productName, NULL);
	p("SKU ID (aka Activation ID)      : %s (%s)\n", guidBuffer, productName);

	uuid2StringLE(&Request->KMSID, guidBuffer);
	getProductIndex(&Request->KMSID, KmsData->KmsItemList, KmsData->KmsItemCount, &productName, NULL);
	p("KMS ID (aka KMS counted ID)     : %s (%s)\n", guidBuffer, productName);

	uuid2StringLE(&Request->CMID, guidBuffer);
	p("Client machine ID               : %s\n", guidBuffer);

	uuid2StringLE(&Request->CMID_prev, guidBuffer);
	p("Previous client machine ID      : %s\n", guidBuffer);


	char mbstr[64];
	time_t st;
	st = fileTimeToUnixTime(&Request->ClientTime);
	strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %X", gmtime(&st));
	p("Client request timestamp (UTC)  : %s\n", mbstr);

	ucs2_to_utf8(Request->WorkstationName, WorkstationBuffer, WORKSTATION_NAME_BUFFER, sizeof(WorkstationBuffer));

	p("Workstation name                : %s\n", WorkstationBuffer);
	p("N count policy (minimum clients): %u\n", (uint32_t)LE32(Request->N_Policy));
}

void logResponseVerbose(const char *const ePID, const BYTE *const hwid, RESPONSE* response, const PRINTFUNC p)
{
	char guidBuffer[GUID_STRING_LENGTH + 1];

	p("Protocol version                : %u.%u\n", (uint32_t)LE16(response->MajorVer), (uint32_t)LE16(response->MinorVer));
	p("KMS host extended PID           : %s\n", ePID);
	if (LE16(response->MajorVer) > 5)
#	ifndef _WIN32
		p("KMS host Hardware ID            : %016llX\n", (unsigned long long)BE64(*(uint64_t*)hwid));
#	else // _WIN32
		p("KMS host Hardware ID            : %016I64X\n", (unsigned long long)BE64(*(uint64_t*)hwid));
#	endif // WIN32

	uuid2StringLE(&response->CMID, guidBuffer);
	p("Client machine ID               : %s\n", guidBuffer);

	char mbstr[64];
	time_t st;

	st = fileTimeToUnixTime(&response->ClientTime);
	strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %X", gmtime(&st));
	p("Client request timestamp (UTC)  : %s\n", mbstr);

	p("KMS host current active clients : %u\n", (uint32_t)LE32(response->Count));
	p("Renewal interval policy         : %u\n", (uint32_t)LE32(response->VLRenewalInterval));
	p("Activation interval policy      : %u\n", (uint32_t)LE32(response->VLActivationInterval));
}
#endif // !defined(NO_VERBOSE_LOG) && !defined(NO_LOG)


#ifndef NO_VERSION_INFORMATION
void printPlatform()
{
	int testNumber = 0x1234;

#	if _MSC_VER
	printf("Compiler: VC++ %02i.%02i build %i\n", _MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 100000);
#	elif defined(VLMCSD_COMPILER)
	printf
	(
		"Compiler: %s\n", VLMCSD_COMPILER
#		ifdef __VERSION__
		" " __VERSION__
#		endif // __VERSION__
	);
#	endif // VLMCSD_COMPILER

	printf
	(
		"Intended platform:%s %s\n", ""

#		if __i386__ || _M_IX86
		" Intel x86"
#		endif

#		if __x86_64__ || __amd64__ || _M_X64 || _M_AMD64
		" Intel x86_64"
#		endif

#		if _M_ARM || __arm__
		" ARM"
#		endif

#		if __thumb__
		" thumb"
#		endif

#		if __aarch64__
		" ARM64"
#		endif

#		if __hppa__
		" HP/PA RISC"
#		endif

#		if __ia64__
		" Intel Itanium"
#		endif

#		if __mips__
		" MIPS"
#		endif

#		if defined(_MIPS_ARCH)
		" " _MIPS_ARCH
#		endif

#		if __mips16
		" mips16"
#		endif

#		if __mips_micromips
		" micromips"
#		endif

#		if __ppc__ || __powerpc__
		" PowerPC"
#		endif

#		if __powerpc64__ || __ppc64__
		" PowerPC64"
#		endif

#		if __sparc__
		" SPARC"
#		endif

#		if defined(__s390__) && !defined(__zarch__) && !defined(__s390x__)
		" IBM S/390"
#		endif

#		if __zarch__ || __s390x__
		" IBM z/Arch (S/390x)"
#		endif

#		if __m68k__
		" Motorola 68k"
#		endif

#		if __ANDROID__
		" Android"
#		endif

#		if __ANDROID_API__
		" (API level " ANDROID_API_LEVEL ")"
#		endif

#		if __FreeBSD__ || __FreeBSD_kernel__
		" FreeBSD"
#		endif

#		if __NetBSD__
		" NetBSD"
#		endif

#		if __OpenBSD__
		" OpenBSD"
#		endif

#		if __DragonFly__
		" DragonFly BSD"
#		endif

#		if defined(__CYGWIN__) && !defined(_WIN64)
		" Cygwin32"
#		endif

#		if defined(__CYGWIN__) && defined(_WIN64)
		" Cygwin64"
#		endif

#		if __GNU__
		" GNU"
#		endif

#		if __gnu_hurd__
		" Hurd"
#		endif

#		if __MACH__
		" Mach"
#		endif

#		if __linux__
		" Linux"
#		endif

#		if __APPLE__ && __MACH__
		" Darwin"
#		endif

#		if  __minix__
		" Minix"
#		endif

#		if __QNX__
		" QNX"
#		endif

#		if __svr4__ || __SVR4
		" SYSV R4"
#		endif	

#		if (defined(__sun__) || defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
		" Solaris"
#		endif

#		if (defined(__sun__) || defined(sun) || defined(__sun)) && !defined(__SVR4) && !defined(__svr4__)
		" SunOS"
#		endif

#		if defined(_WIN32) && !defined(_WIN64)
		" Windows32"
#		endif

#		if defined(_WIN32) && defined(_WIN64)
		" Windows64"
#		endif

#		if __MVS__ || __TOS_MVS__
		" z/OS"
#		endif

#		if defined(__GLIBC__) && !defined(__UCLIBC__)
		" glibc"
#		endif

#		if __UCLIBC__
		" uclibc"
#		endif

#		if defined(__linux__) && !defined(__GLIBC__) && !defined(__UCLIBC__) && !defined(__ANDROID__) && !defined(__BIONIC__)
		" musl"
#		endif

		//#		if _MIPSEL || __MIPSEL__ || __ARMEL__ || __THUMBEL__
		//		" little-endian"
		//#		endif
		//
		//#		if _MIPSEB || __MIPSEB__ || __ARMEB__ || __THUMBEB__
		//		" big-endian"
		//#		endif

#		if __PIE__ || __pie__
		" PIE"
#		endif
		,
		*((uint8_t*)&testNumber) == 0x34 ? "little-endian" : "big-endian"
	);

}


void printCommonFlags()
{
	printf
	(
		"Common flags:%s\n", ""

#		ifdef NO_EXTERNAL_DATA
		" NO_EXTERNAL_DATA"
#		endif // NO_EXTERNAL_DATA

#		ifdef NO_INTERNAL_DATA
		" NO_INTERNAL_DATA"
#		endif // NO_INTERNAL_DATA

#		if !defined(NO_EXTERNAL_DATA)

#		ifdef DATA_FILE
		" DATA=" DATA_FILE
#		endif // DATA_FILE

#		ifdef UNSAFE_DATA_LOAD
		" UNSAFE_DATA_LOAD"
#		endif // UNSAFE_DATA_LOAD

#		endif // !defined(NO_EXTERNAL_DATA)

#		ifdef USE_MSRPC
		" USE_MSRPC"
#		endif // USE_MSRPC

#		ifdef _CRYPTO_OPENSSL
		" _CRYPTO_OPENSSL"
#		endif // _CRYPTO_OPENSSL

#		ifdef _CRYPTO_POLARSSL
		" _CRYPTO_POLARSSL"
#		endif // _CRYPTO_POLARSSL

#		ifdef _CRYPTO_WINDOWS
		" _CRYPTO_WINDOWS"
#		endif // _CRYPTO_WINDOWS

#		if defined(_OPENSSL_SOFTWARE) && defined(_CRYPTO_OPENSSL)
		" _OPENSSL_SOFTWARE"
#		endif // _OPENSSL_SOFTWARE

#		if defined(_USE_AES_FROM_OPENSSL) && defined(_CRYPTO_OPENSSL)
		" _USE_AES_FROM_OPENSSL"
#		endif // _USE_AES_FROM_OPENSSL

#		if defined(_OPENSSL_NO_HMAC) && defined(_CRYPTO_OPENSSL)
		" OPENSSL_HMAC=0"
#		endif // _OPENSSL_NO_HMAC

#		ifdef _PEDANTIC
		" _PEDANTIC"
#		endif // _PEDANTIC

#		ifdef INCLUDE_BETAS
		" INCLUDE_BETAS"
#		endif // INCLUDE_BETAS

#		if __minix__ || defined(NO_TIMEOUT)
		" NO_TIMEOUT=1"
#		endif // __minix__ || defined(NO_TIMEOUT)
	);
}


void printClientFlags()
{
	printf
	(
		"vlmcs flags:%s\n", ""

#		ifdef NO_DNS
		" NO_DNS=1"
#		endif

#		if !defined(NO_DNS)
#		if defined(DNS_PARSER_INTERNAL) && !defined(_WIN32)
		" DNS_PARSER=internal"
#		else // !defined(DNS_PARSER_INTERNAL) || defined(_WIN32)
		" DNS_PARSER=OS"
#		endif // !defined(DNS_PARSER_INTERNAL) || defined(_WIN32)
#		endif // !defined(NO_DNS)

#		if defined(DISPLAY_WIDTH)
		" TERMINAL_WIDTH=" DISPLAY_WIDTH
#		endif
	);
}


void printServerFlags()
{
	printf
	(
		"vlmcsd flags:%s\n", ""

#		ifdef NO_LOG
		" NO_LOG"
#		endif // NO_LOG

#		ifdef NO_RANDOM_EPID
		" NO_RANDOM_EPID"
#		endif // NO_RANDOM_EPID

#		ifdef NO_INI_FILE
		" NO_INI_FILE"
#		endif // NO_INI_FILE

#		if !defined(NO_INI_FILE) && defined(INI_FILE)
		" INI=" INI_FILE
#		endif // !defined(NO_INI_FILE)

#		ifdef NO_PID_FILE
		" NO_PID_FILE"
#		endif // NO_PID_FILE

#		ifdef NO_USER_SWITCH
		" NO_USER_SWITCH"
#		endif // NO_USER_SWITCH

#		ifdef NO_HELP
		" NO_HELP"
#		endif // NO_HELP

#		ifdef NO_STRICT_MODES
		" NO_STRICT_MODES"
#		endif // NO_STRICT_MODES

#		ifdef NO_CUSTOM_INTERVALS
		" NO_CUSTOM_INTERVALS"
#		endif // NO_CUSTOM_INTERVALS

#		ifdef NO_SOCKETS
		" NO_SOCKETS"
#		endif // NO_SOCKETS

#		ifdef NO_CL_PIDS
		" NO_CL_PIDS"
#		endif // NO_CL_PIDS

#		ifdef NO_LIMIT
		" NO_LIMIT"
#		endif // NO_LIMIT

#		ifdef NO_SIGHUP
		" NO_SIGHUP"
#		endif // NO_SIGHUP

#		ifdef NO_PROCFS
		" NOPROCFS=1"
#		endif // NO_PROCFS

#		ifdef USE_THREADS
		" THREADS=1"
#		endif // USE_THREADS

#		ifdef USE_AUXV
		" AUXV=1"
#		endif // USE_AUXV

#		if defined(CHILD_HANDLER) || __minix__
		" CHILD_HANDLER=1"
#		endif // defined(CHILD_HANDLER) || __minix__

#		if !defined(NO_SOCKETS) && defined(SIMPLE_SOCKETS)
		" SIMPLE_SOCKETS"
#		endif // !defined(NO_SOCKETS) && defined(SIMPLE_SOCKETS)

#		ifdef SIMPLE_RPC
		" SIMPLE_RPC"
#		endif // SIMPLE_RPC

#		ifdef NO_STRICT_MODES
		" NO_STRICT_MODES"
#		endif // NO_STRICT_MODES

#		ifdef NO_CLIENT_LIST
		" NO_CLIENT_LIST"
#		endif // NO_CLIENT_LIST

#		if (_WIN32 || __CYGWIN__) && (!defined(USE_MSRPC) || defined(SUPPORT_WINE))
		" SUPPORT_WINE"
#		endif // (_WIN32 || __CYGWIN__) && (!defined(USE_MSRPC) || defined(SUPPORT_WINE))

#		if (_WIN32 || __CYGWIN__) && defined(NO_TAP)
		" NO_TAP"
#		endif // (_WIN32 || __CYGWIN__) && defined(NO_TAP)

#		if !HAVE_FREEBIND
		" NO_FREEBIND"
#		endif //!HAVE_FREEBIND

#		if !HAVE_GETIFADDR
		" !HAVE_GETIFADDR"
#		endif // !HAVE_GETIFADDR

#		if HAVE_GETIFADDR && defined(GETIFADDRS_MUSL)
		" GETIFADDRS=musl"
#		endif // HAVE_GETIFADDR && defined(GETIFADDRS_MUSL)

#		if defined(NO_PRIVATE_IP_DETECT)
		" NO_PRIVATE_IP_DETECT"
#		endif // defined(NO_PRIVATE_IP_DETECT)
	);
}
#endif // NO_VERSION_INFORMATION
