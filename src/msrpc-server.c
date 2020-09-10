#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifdef USE_MSRPC

#if !defined(_WIN32) && !defined(__CYGWIN__)
#error MSRPC is only available with native Windows or Cygwin
#endif

#if _WIN32 && !defined(NO_PRIVATE_IP_DETECT)
#include <winsock2.h>
#endif

#include "msrpc-server.h"
#include "output.h"
#include "kms.h"
#include "helpers.h"
#include "shared_globals.h"
#include "ntservice.h"
#include "endian.h"
#include "types.h"

#if __amd64 || defined(_M_AMD64) // 64-bit

#ifndef _M_AMD64
#define _M_AMD64
#endif // _M_AMD64

#include "KMSServer_s2_x64_mingw_gcc.c"

#else // 32-bit

#include "KMSServer_s2_mingw_gcc.c"

#endif // 32-bit


#if !defined(NO_LIMIT)
#define MAXCALLS MaxTasks
#else // defined(NO_LIMIT)
#define MAXCALLS RPC_C_LISTEN_MAX_CALLS_DEFAULT
#endif




/*
 * This is the main run loop for the KMS server
 * We actually just setup things (must be after Cygwin fork) and then sleep
 */
int runServer()
{
#	if !defined(NO_SOCKETS) && !defined(NO_SIGHUP) && !defined(_WIN32)

	// The RPC setup survives a Cygwin exec without problems but no daemonizing
	// SIGHUP is currently disabled for Cygwin. So this code should never compile

	if (IsRestarted)
	{
#		ifndef NO_LOG
		logger("vlmcsd %s started successfully\n", Version);
#		endif // NO_LOG
	}
	else
#	endif // !defined(NO_SOCKETS) && !defined(NO_SIGHUP) && !defined(_WIN32)
	{
		RPC_STATUS status;

		// Endpoint is actually a TCP port for "ncacn_ip_tcp"
		status = RpcServerUseProtseqEpA
		(
			(RPC_CSTR)"ncacn_ip_tcp",
			RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
			(RPC_CSTR)defaultport,
			NULL
		);

		if (status)
		{
			printerrorf("Fatal: Cannot bind to port %s: %s\n", defaultport, win_strerror(status));
			return status;
		}

#		ifndef NO_LOG
		logger("Listening on port %s\n", defaultport);
#		endif // NO_LOG

		// Registers the KMSServer interface.
		status = RpcServerRegisterIf2
		(
			KMSServer_v1_0_s_ifspec,
			NULL,
			NULL,
			RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH | RPC_IF_AUTOLISTEN,
			MAXCALLS,
			MAX_RESPONSE_SIZE, 	// currently set to sizeof(RESPONSE_V6)
			NULL
		);

		if (status)
		{
			printerrorf("Fatal: Cannot register RPC interface: %s\n", win_strerror(status));
			return status;
		}

#		ifndef NO_LOG
		logger("vlmcsd %s started successfully\n", Version);
#		endif // NO_LOG

		if (IsNTService) ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 200);
	}

	// We could run RpcServerListen here but we need something
	// that can be signaled from Cygwin. So we just sleep 24h (POSIX sleep, no Windows Sleep),
	// wake up for some nanoseconds and sleep again.

	while(TRUE) sleep(86400); // Sleep one day
}



/*
 * Get's the IP address from an RPC_BINDING_HANDLE. Caller must call RpcStringFreeA to
 * release memory allocated in *ipAddress
 */
#ifndef NO_LOG
RPC_STATUS getClientIp(const RPC_BINDING_HANDLE clientBinding, RPC_CSTR *ipAddress)
{
	RPC_STATUS result;
	RPC_CSTR stringBinding;
	RPC_BINDING_HANDLE serverBinding;

	// Fix for wine (disabled by default, because vlmcsd runs natively on all platforms where wine runs)
	// Feel free to #define SUPPORT_WINE if you really want to run the Windows version with MS RPC (Wine RPC in this case)
	#ifdef SUPPORT_WINE
	HMODULE h = GetModuleHandleA("kernel32.dll");

	if (h)
	{
		// Since wine simply terminates the thread when RpcBindingServerFromClient is called, we exit with an error
		if (GetProcAddress(h, "wine_get_unix_file_name")) return RPC_S_CANNOT_SUPPORT;
	}
	#endif // SUPPORT_WINE

	if ((result = RpcBindingServerFromClient(clientBinding, &serverBinding)) != RPC_S_OK) return result;

	if ((result = RpcBindingToStringBindingA(serverBinding, &stringBinding)) != RPC_S_OK)
	{
		RpcBindingFree(&serverBinding);
		return result;
	}

	result = RpcStringBindingParseA(stringBinding, NULL, NULL, ipAddress, NULL, NULL);
	RpcStringFreeA(&stringBinding);
	RpcBindingFree(&serverBinding);

	return result;
}
#endif // NO_LOG


#ifndef NO_PRIVATE_IP_DETECT
static int_fast8_t IsPrivateIPAddress(char* ipAddress)
{
	int family = strchr(ipAddress,'.') ? AF_INET : AF_INET6;

	switch(family)
	{
		case AF_INET:
		{
			int i;
			char* current;
			char* next;
			uint32_t ip;

			for (ip = 0, i = 0, current = ipAddress; i < 4; i++, current = next + 1)
			{
				ip = (ip << 8) | strtoul(current, &next, 10);
				if (*next != '.') break;
			}

			if
			(
				(ip & 0xff000000) == 0x7f000000 || // 127.x.x.x localhost
				(ip & 0xffff0000) == 0xc0a80000 || // 192.168.x.x private routeable
				(ip & 0xffff0000) == 0xa9fe0000 || // 169.254.x.x link local
				(ip & 0xff000000) == 0x0a000000 || // 10.x.x.x private routeable
				(ip & 0xfff00000) == 0xac100000    // 172.16-31.x.x private routeable
			)
			{
				return TRUE;
			}

			break;
		}

		case AF_INET6:
		{
			if (!strcmp(ipAddress, "::1")) return TRUE;
			if (strchr(ipAddress, ':') - ipAddress != 4) break;

			int16_t firstWord;
			hex2bin((BYTE*)&firstWord, ipAddress, 2);
			if ((BE16(firstWord) & 0xe000) != 0x2000) return TRUE;
		}

	}

	return FALSE;
}
#endif // NO_PRIVATE_IP_DETECT

/*
 * This is the callback function for the RPC request as defined in KMSServer.idl
 * Output from the MIDL compiler has been modified manually to support gcc (and compatible compilers)
 * as well as Cygwin and MingW-w64 toolchains.
 */
int ProcessActivationRequest(handle_t IDL_handle, int requestSize, unsigned char *request, int *responseSize, unsigned char **response)
{
	RPC_CSTR clientIpAddress;
	RPC_STATUS result;
	int status = 0;

	result = getClientIp(IDL_handle, &clientIpAddress);

#	ifndef NO_LOG

	logger("RPC connection accepted: %s\n", !result ? (const char*)clientIpAddress : "Unknown IP");

#	endif // NO_LOG

#	ifndef NO_PRIVATE_IP_DETECT
	if (result && (PublicIPProtectionLevel & 2))
	{
#		ifndef NO_LOG
		logger ("Cannot verify that client has a private IP address\n");
#		endif

		return 0x80070000 | RPC_S_ACCESS_DENIED;
	}

	if (!result && (PublicIPProtectionLevel & 2) && !IsPrivateIPAddress((char*)clientIpAddress))
	{
#		ifndef NO_LOG
		logger("Client with public IP address rejected\n");
#		endif

		RpcStringFreeA(&clientIpAddress);
		return 0x80070000 | RPC_S_ACCESS_DENIED;
	}
#	endif // NO_PRIVATE_IP_DETECT

	// Discard any packet smaller than a v4 request
	if (requestSize < (int)sizeof(REQUEST_V4))
	{
		if (!result) RpcStringFreeA(&clientIpAddress);
		return 0x8007000D;
	}

	*response = (uint8_t *)midl_user_allocate(MAX_RESPONSE_SIZE);
	int kmsStatus = 0x8007000D;
	int version = LE32(((REQUEST*)(request))->Version);

	switch(version)
	{
		case 0x40000:
			kmsStatus = CreateResponseV4((REQUEST_V4 *)request, *response, (char*)clientIpAddress);
			break;
		case 0x50000:
		case 0x60000:
			kmsStatus = CreateResponseV6((REQUEST_V6 *) request, *response, (char*)clientIpAddress);
			break;
		default:
#			ifndef NO_LOG
			logger("Fatal: KMSv%u.%u unsupported\n", version >> 16, version & 0xffff);
#			endif // NO_LOG
			break;
	}

	if (kmsStatus < 0)
	{
		status = kmsStatus;
	}
	else
	{
		*responseSize = kmsStatus;
	}

	if (!result) RpcStringFreeA(&clientIpAddress);
	return status;
}


// Memory allocation function for RPC.
void *__RPC_USER midl_user_allocate(size_t len)
{
	return vlmcsd_malloc(len);
}


// Memory deallocation function for RPC.
void __RPC_USER midl_user_free(void __RPC_FAR *ptr)
{
	if (ptr) free(ptr);
	ptr = NULL;
}


#endif // USE_MSRPC
