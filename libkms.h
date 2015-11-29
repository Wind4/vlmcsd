/*
 * libkms.h
 */

#ifndef LIBKMS_H_
#define LIBKMS_H_

#include "types.h"
#include "kms.h"
#include "rpc.h"

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#if !defined(_WIN32) && !__CYGWIN__
#define __declspec(x) __attribute__((__visibility__("default")))
#endif

#if !defined(EXTERNAL)
#define EXTERNAL dllimport
#endif

EXTERNC __declspec(EXTERNAL) DWORD __cdecl SendActivationRequest(const char* const hostname, const int port, RESPONSE* baseResponse, const REQUEST* const baseRequest, RESPONSE_RESULT* result, BYTE *hwid);
EXTERNC __declspec(EXTERNAL) DWORD __cdecl StartKmsServer(const int port, RequestCallback_t requestCallback);
EXTERNC __declspec(EXTERNAL) DWORD __cdecl StopKmsServer();
EXTERNC __declspec(EXTERNAL) int __cdecl GetLibKmsVersion();
//EXTERN_C __declspec(EXTERNAL) unsigned int __cdecl GetRandom32();


#endif /* LIBKMS_H_ */
