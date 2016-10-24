/*
 * libkms.h
 */

#ifndef LIBKMS_H_
#define LIBKMS_H_

#include "types.h"
#include "kms.h"
#include "rpc.h"
#include "vlmcs.h"

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC EXTERN "C"
#else
#define EXTERNC
#endif
#endif

EXTERNC __declspec(EXTERNAL) DWORD __cdecl SendKMSRequest(const SOCKET sock, RESPONSE* baseResponse, REQUEST* baseRequest, RESPONSE_RESULT* result, BYTE *hwid);
EXTERNC __declspec(EXTERNAL) DWORD __cdecl StartKmsServer(const int port, RequestCallback_t requestCallback);
EXTERNC __declspec(EXTERNAL) DWORD __cdecl StopKmsServer();
EXTERNC __declspec(EXTERNAL) int __cdecl GetLibKmsVersion();
EXTERNC __declspec(EXTERNAL) const char* const __cdecl GetEmulatorVersion();
EXTERNC __declspec(EXTERNAL) SOCKET __cdecl ConnectToServer(const char* host, const char* port, const int addressFamily);
EXTERNC __declspec(EXTERNAL) char* __cdecl GetErrorMessage();
EXTERNC __declspec(EXTERNAL) void __cdecl CloseConnection(const SOCKET sock);
EXTERNC __declspec(EXTERNAL) RpcStatus __cdecl BindRpc(const SOCKET sock, const int_fast8_t useMultiplexedRpc, const int_fast8_t useRpcNDR64, const int_fast8_t useRpcBTFN, PRpcDiag_t rpcDiag);
EXTERNC __declspec(EXTERNAL) int_fast8_t __cdecl IsDisconnected(const SOCKET sock);
//EXTERN_C __declspec(EXTERNAL) unsigned int __cdecl GetRandom32();


#endif /* LIBKMS_H_ */
