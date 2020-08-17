#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifdef USE_MSRPC

#if !defined(_WIN32) && !defined(__CYGWIN__)
#error MSRPC is only available with native Windows or Cygwin
#endif

#include "msrpc-client.h"
#include <stdio.h>
#include "output.h"
#include "helpers.h"

#if __amd64 || defined(_M_AMD64) // 64-bit

#ifndef _M_AMD64
#define _M_AMD64
#endif // _M_AMD64

#include "KMSServer_c_x64_mingw_gcc.c"

#else // 32-bit

#include "KMSServer_c_mingw_gcc.c"

#endif // 32-bit

static RPC_CSTR stringBinding;
jmp_buf jmp;
RPC_STATUS PreviousRpcCallFailed = RPC_S_OK;


/*
 * Creates an RPC string binding that is used to connect to the server.
 * Input is host:port, e.g. "[::1]:1688" or "127.0.0.1:1688"
 * Output is for example "ncacn_ip_tcp:127.0.0.1[endpoint=1688]"
 */
#if !__amd64
#pragma GCC optimize("O0") ////TODO: Find out why gcc needs -O0 for RPC handling
#endif

static RPC_STATUS createStringBinding(char *const addr, RPC_CSTR* stringBinding)
{
	char *szHost, *szPort;

	parseAddress(addr, &szHost, &szPort);

	return RpcStringBindingComposeA
	(
		NULL,						/* UUID */
		(RPC_CSTR)"ncacn_ip_tcp",	/* use TCP */
		(RPC_CSTR)szHost,			/* host name or IP address */
		(RPC_CSTR)szPort,			/* endpoint (TCP port here) */
		NULL,						/* options */
		stringBinding				/* resulting string binding */
	);
}


/*
 * This does not actually connect to a TCP port because MS RPC doesn't connect
 * before the actual RPC call is made. So this a stub
 */
RpcCtx connectToAddress(char *const addr, const int AddressFamily_unused, int_fast8_t showHostName_unused)
{
	RPC_STATUS status;

	printf("Connecting to %s ... ", addr);

	if ((status = createStringBinding(addr, &stringBinding)) != RPC_S_OK)
	{
		printerrorf("%s\n", win_strerror(status));
		return !0;
	}

	if (PreviousRpcCallFailed)
	{
		printerrorf("%s\n", win_strerror(PreviousRpcCallFailed));
		return !0;
	}

	printf("successful\n");
	return 0;
}


/*
 * Does not do RPC binding on the wire. Just initializes the interface
 */
RpcStatus rpcBindClient(const RpcCtx handle, const int_fast8_t verbose, PRpcDiag_t rpcDiag)
{
	RPC_STATUS status;

	if ((status = RpcBindingFromStringBindingA(stringBinding, &KMSServer_v1_0_c_ifspec)) != RPC_S_OK)
	{
		errorout("\n%s\n", win_strerror(status));
	}

	rpcDiag->HasRpcDiag = FALSE;
	return status;
}


/*
 * You never know if you have a TCP connection or not
 * This returns true if the previous RPC call failed
 */
int_fast8_t isDisconnected(const RpcCtx handle)
{
	return PreviousRpcCallFailed;
}


/*
 * This is the exception handler because the RPC call may
 * throw an SEH exception and gcc does not support
 * __try / __except as MSVC does.
 */
static LONG WINAPI rpcException (LPEXCEPTION_POINTERS exception_pointers)
{
	DWORD exception = exception_pointers->ExceptionRecord->ExceptionCode;
	if (!exception) exception = (DWORD)~0;
	longjmp(jmp, exception_pointers->ExceptionRecord->ExceptionCode);
	return EXCEPTION_EXECUTE_HANDLER;
}

/*
 * This actually calls the RPC server
 */
#define try SetUnhandledExceptionFilter(rpcException); RPC_STATUS exception = setjmp(jmp); if (!exception)
#define catch else
RpcStatus rpcSendRequest(const RpcCtx handle, BYTE* KmsRequest, const size_t requestSize, BYTE **KmsResponse, size_t* responseSize)
{
	*KmsResponse = NULL; // Let midl_user_allocate do the job

	try
	{
		exception = RequestActivation(KMSServer_v1_0_c_ifspec, (int)requestSize, KmsRequest, (int*)responseSize, KmsResponse);
	}
	catch
	{
		errorout("\n%s", win_strerror(exception));
	}

	PreviousRpcCallFailed = exception;
	SetUnhandledExceptionFilter(NULL);

	return exception;

}
#undef catch
#undef try


/*
 * Only frees local handles. Cannot close the TCP connection
 */
RpcStatus closeRpc(const RpcCtx handle)
{
	RPC_STATUS status;

	if ((status = RpcBindingFree(&KMSServer_v1_0_c_ifspec)) != RPC_S_OK) return status;
	status = RpcStringFreeA(&stringBinding);
	//Ctx = INVALID_RPCCTX;
	return status;
}


#if !MULTI_CALL_BINARY
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

#endif // !MULTI_CALL_BINARY


#endif // USE_MSRPC


