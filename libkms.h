/*
 * libkms.h
 */

#ifndef LIBKMS_H_
#define LIBKMS_H_

#include "types.h"
#include "kms.h"
#include "rpc.h"

#ifndef EXTERNC
#ifdef __cplusplus
#define EXTERNC EXTERN "C"
#else
#define EXTERNC
#endif
#endif

EXTERNC __declspec(EXTERNAL) DWORD __cdecl SendActivationRequest(const char* const hostname, const int port, RESPONSE* baseResponse, const REQUEST* const baseRequest, RESPONSE_RESULT* result, BYTE *hwid);
EXTERNC __declspec(EXTERNAL) DWORD __cdecl StartKmsServer(const int port, RequestCallback_t requestCallback);
EXTERNC __declspec(EXTERNAL) DWORD __cdecl StopKmsServer();
EXTERNC __declspec(EXTERNAL) int __cdecl GetLibKmsVersion();
EXTERNC __declspec(EXTERNAL) const char* const __cdecl GetEmulatorVersion();
//EXTERN_C __declspec(EXTERNAL) unsigned int __cdecl GetRandom32();


#endif /* LIBKMS_H_ */
