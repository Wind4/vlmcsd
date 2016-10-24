/*
 * libkms.c
 */

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifdef EXTERNAL
#undef EXTERNAL
#endif

#define EXTERNAL dllexport

#define DLLVERSION 0x40000

#include "libkms.h"
#include "shared_globals.h"
#include "network.h"
#include "helpers.h"

#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#endif // WIN32

static int_fast8_t IsServerStarted = FALSE;

#ifdef _WIN32
#ifndef USE_MSRPC

static int_fast8_t SocketsInitialized = FALSE;
WSADATA wsadata;

static int initializeWinSockets()
{
	if (SocketsInitialized) return 0;
	SocketsInitialized = TRUE;
	return WSAStartup(0x0202, &wsadata);
}

#endif // USE_MSRPC
#endif // _WIN32

EXTERNC __declspec(EXTERNAL) char* __cdecl GetErrorMessage()
{
	return ErrorMessage;
}

EXTERNC __declspec(EXTERNAL)SOCKET __cdecl ConnectToServer(const char* host, const char* port, const int addressFamily)
{
	SOCKET sock;
	*ErrorMessage = 0;

#	if defined(_WIN32) && !defined(USE_MSRPC)
	initializeWinSockets();
#	endif // defined(_WIN32) && !defined(USE_MSRPC)

	size_t adrlen = strlen(host) + 16;
	char* RemoteAddr = (char*)alloca(adrlen);
	vlmcsd_snprintf(RemoteAddr, adrlen, "[%s]:%s", host, port);
	sock = connectToAddress(RemoteAddr, addressFamily, FALSE);

	if (sock == INVALID_RPCCTX)
	{
		printerrorf("Fatal: Could not connect to %s\n", RemoteAddr);
		return sock;
	}

	return sock;
}

EXTERNC __declspec(EXTERNAL)RpcStatus __cdecl BindRpc(const SOCKET sock, const int_fast8_t useMultiplexedRpc, const int_fast8_t useRpcNDR64, const int_fast8_t useRpcBTFN, PRpcDiag_t rpcDiag)
{
	*ErrorMessage = 0;
	UseMultiplexedRpc = useMultiplexedRpc;
	UseClientRpcNDR64 = useRpcNDR64;
	UseClientRpcBTFN = useRpcBTFN;
	return rpcBindClient(sock, FALSE, rpcDiag);
}

EXTERNC __declspec(EXTERNAL) void __cdecl CloseConnection(const SOCKET sock)
{
	socketclose(sock);
}


EXTERNC __declspec(EXTERNAL)DWORD __cdecl SendKMSRequest(const SOCKET sock, RESPONSE* baseResponse, REQUEST* baseRequest, RESPONSE_RESULT* result, BYTE *hwid)
{
	*ErrorMessage = 0;
	return SendActivationRequest(sock, baseResponse, baseRequest, result, hwid);
}

EXTERNC __declspec(EXTERNAL)int_fast8_t __cdecl IsDisconnected(const SOCKET sock)
{
	return isDisconnected(sock);
}


EXTERNC __declspec(EXTERNAL)DWORD __cdecl StartKmsServer(const int port, RequestCallback_t requestCallback)
{
#ifndef SIMPLE_SOCKETS
	char listenAddress[64];

	if (IsServerStarted) return SOCKET_EALREADY;

#	ifdef _WIN32
	int error = initializeWinSockets();
	if (error) return error;
#	endif // _WIN32

	CreateResponseBase = requestCallback;

	int maxsockets = 0;
	int_fast8_t haveIPv4 = FALSE;
	int_fast8_t haveIPv6 = FALSE;

	if (checkProtocolStack(AF_INET)) { haveIPv4 = TRUE; maxsockets++; }
	if (checkProtocolStack(AF_INET6)) { haveIPv6 = TRUE; maxsockets++; }

	if (!maxsockets) return SOCKET_EAFNOSUPPORT;

	SocketList = (SOCKET*)vlmcsd_malloc(sizeof(SOCKET) * (size_t)maxsockets);
	numsockets = 0;

	if (haveIPv4)
	{
		snprintf(listenAddress, 64, "0.0.0.0:%u", (unsigned int)port);
		addListeningSocket(listenAddress);
	}

	if (haveIPv6)
	{
		snprintf(listenAddress, 64, "[::]:%u", (unsigned int)port);
		addListeningSocket(listenAddress);
	}

	if (!numsockets)
	{
		free(SocketList);
		return SOCKET_EADDRNOTAVAIL;
	}

	IsServerStarted = TRUE;

	runServer();

	IsServerStarted = FALSE;
	return 0;

#	else // SIMPLE_SOCKETS

	if (IsServerStarted) return SOCKET_EALREADY;
	int error;

#	ifdef _WIN32
	error = initializeWinSockets();
	if (error) return error;
#	endif // _WIN32

	defaultport = vlmcsd_malloc(16);
	vlmcsd_snprintf((char*)defaultport, (size_t)16, "%i", port);

	CreateResponseBase = requestCallback;
	error = listenOnAllAddresses();
	free(defaultport);
	if (error) return error;

	IsServerStarted = TRUE;
	runServer();
	IsServerStarted = FALSE;

	return 0;


#	endif // SIMPLE_SOCKETS
}


EXTERNC __declspec(EXTERNAL)DWORD __cdecl StopKmsServer()
{
	if (!IsServerStarted) return VLMCSD_EPERM;

	closeAllListeningSockets();

#	ifndef SIMPLE_SOCKETS
	if (SocketList) free(SocketList);
#	endif

	return 0;
}


EXTERNC __declspec(EXTERNAL) int __cdecl GetLibKmsVersion()
{
	return DLLVERSION;
}


EXTERNC __declspec(EXTERNAL) const char* const __cdecl GetEmulatorVersion()
{
	return VERSION;
}

