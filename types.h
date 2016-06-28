#ifndef __types_h
#define __types_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#define ANDROID_API_LEVEL ANDROID_HELPER1(__ANDROID_API__)
#define ANDROID_HELPER1(s) ANDROID_HELPER2(s)
#define ANDROID_HELPER2(s) #s

#if !defined(_WIN32) && !__CYGWIN__
#define __declspec(x) __attribute__((__visibility__("default")))
#endif

#if !defined(EXTERNAL)
#define EXTERNAL dllimport
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#ifdef __ANDROID__
#include <android/api-level.h>
#endif // __ANDROID__

#ifndef _WIN32
#include <netinet/in.h>
#endif // _WIN32


#if __linux__ // Some versions of uclibc do not define IP_FREEBIND in the correct header file
#ifndef IP_FREEBIND
#define IP_FREEBIND 15
#endif // IP_FREEBIND
#endif // __linux__

#if (IP_BINDANY || IP_FREEBIND || IPV6_BINDANY || IP_NONLOCALOK) && !defined(NO_FREEBIND) && !defined(USE_MSRPC) && !defined(SIMPLE_SOCKETS)
#define HAVE_FREEBIND 1
#endif

#ifndef alloca
#ifdef __GNUC__
#define alloca(x) __builtin_alloca(x)
#endif // __GNUC__
#endif // alloca

#ifndef alloca
#if _MSC_VER
#define alloca _malloca
#endif // _MSC_VER
#endif // alloca

#ifndef alloca
#ifdef __has_builtin // clang feature test
#if __has_builtin(__builtin_alloca)
#define alloca(x) __builtin_alloca(x)
#endif // __has_builtin(__builtin_alloca)
#endif // __has_builtin
#endif // alloca

#ifndef alloca
#include <alloca.h>
#endif

#ifndef __packed
#if _MSC_VER
#define __packed
#else // !_MSC_VER
#define __packed  __attribute__((packed))
#endif // !_MSC_VER
#endif

#ifndef __pure
#define __pure	  __attribute__((pure))
#endif

#ifndef __noreturn
#define __noreturn	__attribute__((noreturn))
#endif

#define restrict	__restrict

typedef struct __packed
{
	uint16_t val[0];
} PACKED16;

typedef struct __packed
{
	uint32_t val[0];
} PACKED32;

typedef struct __packed
{
	uint64_t val[0];
} PACKED64;

// Extend this type to 16 or 32 bits if more than 254 products appear
typedef uint8_t ProdListIndex_t;

// Deal with Mingw32-w64 C++ header which defines a _countof that is incompatible with vlmcsd
#define vlmcsd_countof(x)	( sizeof(x) / sizeof(x[0]) )

// PATH_MAX is optional in Posix. We use a default of 260 here
#ifndef PATH_MAX
#ifdef _WIN32
#define PATH_MAX MAX_PATH
#else
#define PATH_MAX 260
#endif // _WIN32
#endif // !PATH_MAX

#if PATH_MAX > 260
#define VLMCSD_PATH_MAX 260
#else
#define VLMCSD_PATH_MAX PATH_MAX
#endif

// Synchronization Objects

// Mutexes
#ifdef USE_THREADS
#if !defined(_WIN32) && !defined(__CYGWIN__)
#define lock_mutex(x) pthread_mutex_lock(x)
#define unlock_mutex(x) pthread_mutex_unlock(x)
#else
#define lock_mutex(x) EnterCriticalSection(x)
#define unlock_mutex(x) LeaveCriticalSection(x)
#endif
#else // !USE_THREADS
//defines to nothing
#define lock_mutex(x)
#define unlock_mutex(x)
#endif // !USE_THREADS

// Semaphores
#ifndef _WIN32
#define semaphore_wait(x) sem_wait(x)
#define semaphore_post(x) sem_post(x)
#else // _WIN32
#define semaphore_wait(x) WaitForSingleObject(x, INFINITE)
#define semaphore_post(x) ReleaseSemaphore(x, 1, NULL)
#endif // _WIN32

// Stupid MingW just uses rand() from msvcrt.dll which uses RAND_MAX of 0x7fff
#if RAND_MAX < 0x7fffffff
#define rand32(x) ((uint32_t)((rand(x) << 17) | (rand(x) << 2) | (rand(x) & 3)))
#elif RAND_MAX < 0xffffffff
#define rand32(x) ((uint32_t)((rand(x) << 1) | (rand(x) & 1)))
#else
#define rand32(x) (uint32_t)rand(x)
#endif

#if (defined(_WIN32) || defined(__CYGWIN__)) && !defined(NO_SOCKETS)
#define _NTSERVICE
#endif

#if (defined(__CYGWIN__) || defined(_WIN32) || defined(NO_SOCKETS)) && !defined(NO_SIGHUP)
#define NO_SIGHUP
#endif // (defined(__CYGWIN__) || defined(_WIN32) || defined(NO_SOCKETS)) && !defined(NO_SIGHUP)

#ifdef _WIN32
#ifndef USE_THREADS
#define USE_THREADS
#endif
#endif

#if defined(USE_THREADS)
#define _TLS __thread
#else
#define _TLS
#endif

#define GUID_STRING_LENGTH 36

#if defined(_WIN32)

#ifndef USE_MSRPC
#include <winsock2.h>
#include <ws2tcpip.h>
#endif // USE_MSRPC

#include <windows.h>
//#include <VersionHelpers.h>


typedef char* sockopt_t;
// Map VLMCSD error codes to WSAGetLastError() codes
// Add more if you need them
#define VLMCSD_EADDRINUSE WSAEADDRINUSE
#define VLMCSD_ENODEV WSAENODEV
#define VLMCSD_EADDRNOTAVAIL WSAEADDRNOTAVAIL
#define VLMCSD_EACCES WSAEACCES
#define VLMCSD_EINVAL WSAEINVAL
#define VLMCSD_ENOTSOCK WSAENOTSOCK
#define VLMCSD_EINTR WSAEINTR
#define VLMCSD_EINPROGRESS WSAEINPROGRESS
#define VLMCSD_ECONNABORTED WSAECONNABORTED

#define socket_errno WSAGetLastError()
#define socketclose(x) (closesocket(x))
#define vlmcsd_strerror(x) win_strerror(x)
#define VLMCSD_SHUT_RD SD_RECEIVE
#define VLMCSD_SHUT_WR SD_SEND
#define VLMCSD_SHUT_RDWR SD_BOTH

/* Unknown Winsock error codes */
#define WSAENODEV -1

#elif defined(__CYGWIN__)
#include <windows.h>


// Resolve conflicts between OpenSSL and MS Crypto API
#ifdef _CRYPTO_OPENSSL
#undef OCSP_RESPONSE
#undef X509_NAME
#endif

#else
typedef uint32_t		DWORD;
typedef uint16_t		WORD;
typedef uint8_t			BYTE;
typedef uint16_t		WCHAR;
typedef int             BOOL;

#define FALSE  0
#define TRUE   !0

typedef struct {
	DWORD  Data1;
	WORD   Data2;
	WORD   Data3;
	BYTE   Data4[8];
} /*__packed*/ GUID;

typedef struct {
	DWORD  dwLowDateTime;
	DWORD  dwHighDateTime;
} /*__packed*/ FILETIME;

#endif // defined(__CYGWIN__)

#ifndef _WIN32
// Map VLMCSD error codes to POSIX codes
// Add more if you need them
#define VLMCSD_EADDRINUSE EADDRINUSE
#define VLMCSD_ENODEV ENODEV
#define VLMCSD_EADDRNOTAVAIL EADDRNOTAVAIL
#define VLMCSD_EACCES EACCES
#define VLMCSD_EINVAL EINVAL
#define VLMCSD_ENOTSOCK ENOTSOCK
#define VLMCSD_EINTR EINTR
#define VLMCSD_EINPROGRESS EINPROGRESS
#define VLMCSD_ECONNABORTED ECONNABORTED

typedef void* sockopt_t;
#define _countof(x)        ( sizeof(x) / sizeof(x[0]) )
#define SOCKET int
#define INVALID_SOCKET -1
#define socket_errno errno
#define socketclose(x) (close(x))
#define vlmcsd_strerror strerror
#define VLMCSD_SHUT_RD SHUT_RD
#define VLMCSD_SHUT_WR SHUT_WR
#define VLMCSD_SHUT_RDWR SHUT_RDWR

#endif // __MINGW__
#define INVALID_UID ((uid_t)~0)
#define INVALID_GID ((gid_t)~0)

#undef IsEqualGUID
#define IsEqualGUID(a, b)  ( !memcmp(a, b, sizeof(GUID)) )

#ifndef __stdcall
#define __stdcall
#endif

#ifndef __cdecl
#define __cdecl
#endif

typedef const char *const * CARGV;

typedef struct {
	SOCKET socket;
	DWORD RpcAssocGroup;
} CLDATA, *const PCLDATA;




#endif // __types_h
