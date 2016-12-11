#ifndef __types_h
#define __types_h

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#if defined(NO_INTERNAL_DATA) && defined(NO_EXTERNAL_DATA)
#error NO_INTERAL_DATA and NO_EXTERNAL_DATA cannot be used together
#endif

#if defined(_WIN32)

//#ifndef USE_MSRPC
#include <winsock2.h>
//#include <ws2tcpip.h>
//#endif // USE_MSRPC
#endif // defined(_WIN32)

#define ANDROID_API_LEVEL ANDROID_HELPER1(__ANDROID_API__)
#define ANDROID_HELPER1(s) ANDROID_HELPER2(s)
#define ANDROID_HELPER2(s) #s

#if !_WIN32 && !__CYGWIN__

#if !__minix__
#include <pthread.h>
#endif // !__minix__

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
//#include <limits.h>
#include <stdint.h>

#ifdef __ANDROID__
#include <android/api-level.h>
#endif // __ANDROID__

#ifndef _WIN32
#include <unistd.h>
#include <netinet/in.h>
#endif // _WIN32


#if __linux__ // Some versions of uclibc do not define IP_FREEBIND in the correct header file
#ifndef IP_FREEBIND
#define IP_FREEBIND 15
#endif // IP_FREEBIND
#endif // __linux__

#ifdef NO_EXTERNAL_DATA
#ifndef UNSAFE_DATA_LOAD
#define UNSAFE_DATA_LOAD
#endif // UNSAFE_DATA_LOAD
#endif // NO_EXTERNAL_DATA

#if (IP_BINDANY || IP_FREEBIND || IPV6_BINDANY || IP_NONLOCALOK) && !defined(NO_FREEBIND) && !defined(USE_MSRPC) && !defined(SIMPLE_SOCKETS)
#define HAVE_FREEBIND 1
#endif

#if !defined(NO_GETIFADDRS) && !defined(USE_MSRPC) && !defined(SIMPLE_SOCKETS) && !defined(NO_SOCKETS) && !defined(NO_PRIVATE_IP_DETECT) 
#define HAVE_GETIFADDR 1
#endif

//#if (__minix__ || defined(NO_SOCKETS)) && !defined(NO_STRICT_MODES)
//#define NO_STRICT_MODES
//#endif // __minix__ && !defined(NO_STRICT_MODES)

#if (defined(NO_STRICT_MODES) || defined(NO_SOCKETS)) && !defined(NO_CLIENT_LIST)
#define NO_CLIENT_LIST
#endif // defined(NO_STRICT_MODES) || defined(NO_SOCKETS) && !defined(NO_CLIENT_LIST)

#if !_WIN32 && !__CYGWIN__

#if !defined(_POSIX_THREADS) || (!defined(_POSIX_THREAD_PROCESS_SHARED) && !defined(USE_THREADS) && !__ANDROID__)
#ifndef NO_CLIENT_LIST
#define NO_CLIENT_LIST
#endif // !NO_CLIENT_LIST
#endif // !defined(_POSIX_THREADS) || (!defined(_POSIX_THREAD_PROCESS_SHARED) && !defined(USE_THREADS))

#if !defined(_POSIX_THREADS) && !defined(NO_LIMIT)
#define NO_LIMIT
#endif // !defined(POSIX_THREADS) && !defined(NO_LIMIT)

#endif // !_WIN32 && !__CYGWIN__

#ifndef alloca
#ifdef __GNUC__
#define alloca(x) __builtin_alloca(x)
#endif // __GNUC__
#endif // alloca


#ifndef alloca
#ifdef __has_builtin // clang feature test
#if __has_builtin(__builtin_alloca)
#define alloca(x) __builtin_alloca(x)
#endif // __has_builtin(__builtin_alloca)
#endif // __has_builtin
#endif // alloca

#ifndef alloca
#if !_MSC_VER
#include <alloca.h>
#else
#include <malloc.h>
//#define alloca _malloca
#endif
//#define alloca _malloca
//#endif // _MSC_VER
#endif // alloca

#ifndef __packed
#if _MSC_VER
#define __packed
#else // !_MSC_VER
#define __packed  __attribute__((packed))
#endif // !_MSC_VER
#endif

#ifndef __pure
#if _MSC_VER
#define __pure
#else
#define __pure	  __attribute__((pure))
#endif
#endif

#ifndef __noreturn
#if _MSC_VER
#define __noreturn __declspec(noreturn)
#else
#define __noreturn	__attribute__((noreturn))
#endif
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
#define rand32() ((uint32_t)((rand() << 17) | (rand() << 2) | (rand() & 3)))
#elif RAND_MAX < 0xffffffff
#define rand32(x) ((uint32_t)((rand(x) << 1) | (rand(x) & 1)))
#else
#define rand32(x) (uint32_t)rand(x)
#endif

#if (defined(_WIN32) || defined(__CYGWIN__)) && !defined(NO_SOCKETS)
#define _NTSERVICE
#else
#ifndef NO_TAP
#define NO_TAP
#endif
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
#include <windows.h>
//#include <VersionHelpers.h>

typedef char* sockopt_t;
/* Unknown Winsock error codes */
#define WSAENODEV -1

// Map VLMCSD error codes to WSAGetLastError() and GetLastError() codes
// Add more if you need them
#define SOCKET_EADDRINUSE WSAEADDRINUSE
#define SOCKET_ENODEV WSAENODEV
#define SOCKET_EADDRNOTAVAIL WSAEADDRNOTAVAIL
#define SOCKET_EACCES WSAEACCES
#define SOCKET_EINVAL WSAEINVAL
#define SOCKET_ENOTSOCK WSAENOTSOCK
#define SOCKET_EINTR WSAEINTR
#define SOCKET_EINPROGRESS WSAEINPROGRESS
#define SOCKET_ECONNABORTED WSAECONNABORTED
#define SOCKET_EALREADY WSAEALREADY

#define VLMCSD_EACCES ERROR_ACCESS_DENIED
#define VLMCSD_EINVAL ERROR_INVALID_PARAMETER
#define VLMCSD_ENOMEM ERROR_OUTOFMEMORY
#define VLMCSD_EPERM ERROR_CAN_NOT_COMPLETE


#define socket_errno WSAGetLastError()
#define socketclose(x) (closesocket(x))
#define vlmcsd_strerror(x) win_strerror(x)
#define VLMCSD_SHUT_RD SD_RECEIVE
#define VLMCSD_SHUT_WR SD_SEND
#define VLMCSD_SHUT_RDWR SD_BOTH

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
typedef int32_t         BOOL;
typedef int32_t			HRESULT;
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define S_OK	((HRESULT)0)


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
#define SOCKET_EADDRINUSE EADDRINUSE
#define SOCKET_ENODEV ENODEV
#define SOCKET_EADDRNOTAVAIL EADDRNOTAVAIL
#define SOCKET_EACCES EACCES
#define SOCKET_EINVAL EINVAL
#define SOCKET_ENOTSOCK ENOTSOCK
#define SOCKET_EINTR EINTR
#define SOCKET_EINPROGRESS EINPROGRESS
#define SOCKET_ECONNABORTED ECONNABORTED
#define SOCKET_EALREADY EALREADY

#define VLMCSD_EACCES EACCES
#define VLMCSD_EINVAL EINVAL
#define VLMCSD_EINTR EINTR
#define VLMCSD_ENOMEM ENOMEM
#define VLMCSD_EPERM EPERM

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

#if !defined(__stdcall) && !_MSC_VER
#define __stdcall
#endif

#if !defined(__cdecl) && !_MSC_VER
#define __cdecl
#endif

typedef const char *const * CARGV;

typedef struct {
	SOCKET socket;
	DWORD RpcAssocGroup;
} CLDATA, *const PCLDATA;


#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define vlmcsd_snprintf _snprintf
#define vlmcsd_vsnprintf _vsnprintf
#define vlmcsd_unlink DeleteFile
#define vlmcsd_strtoll _strtoi64
#else // !_MSC_VER
#define vlmcsd_snprintf snprintf
#define vlmcsd_vsnprintf vsnprintf
#define vlmcsd_strtoll strtoll
#define vlmcsd_unlink unlink
#endif  // !_MSC_VER

#endif // __types_h
