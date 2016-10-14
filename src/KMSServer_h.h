

/* this ALWAYS GENERATED file contains the definitions for the interfaces */

/* Modified by Hotbird64 for use with MingW and gcc */


 /* File created by MIDL compiler version 8.00.0595 */
/* at Thu Oct 18 15:24:14 2012
 */
/* Compiler settings for KMSServer.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.00.0595 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if _WIN32
#include "winsock2.h"
#endif

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

//#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __KMSServer_h_h__
#define __KMSServer_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __KMSServer_INTERFACE_DEFINED__
#define __KMSServer_INTERFACE_DEFINED__

/* interface KMSServer */
/* [version][uuid] */ 

int RequestActivation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ int requestSize,
    /* [size_is][in] */ unsigned char *request,
    /* [out] */ int *responseSize,
    /* [size_is][size_is][out] */ unsigned char **response);



extern RPC_IF_HANDLE KMSServer_v1_0_c_ifspec;
extern RPC_IF_HANDLE KMSServer_v1_0_s_ifspec;
#endif /* __KMSServer_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


