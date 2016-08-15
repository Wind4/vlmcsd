#ifndef INCLUDED_SHARED_GLOBALS_H
#define INCLUDED_SHARED_GLOBALS_H

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include <sys/types.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#if (!defined(NO_LIMIT) || defined(USE_THREADS)) && !__minix__
#include <pthread.h>
#endif // (!defined(NO_LIMIT) || defined(USE_THREADS)) && !__minix__
#include <fcntl.h>
#include <sys/stat.h>
#if !defined(NO_LIMIT) && !__minix__
#include <semaphore.h>
#endif // !defined(NO_LIMIT) && !__minix__
#else
#ifndef USE_MSRPC
#include <winsock2.h>
#include <ws2tcpip.h>
#endif // USE_MSRPC
#include <windows.h>
#endif

#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
//#include <semaphore.h>
#include "types.h"

#define MAX_KMSAPPS 3
typedef struct
{
	const char* Epid;
	const BYTE* HwId;
	#ifndef NO_LOG
	const char* EpidSource;
	#endif // NO_LOG
} KmsResponseParam_t, *PKmsResponseParam_t;

#if !defined(NO_LIMIT) && !__minix__
#ifndef SEM_VALUE_MAX // Android does not define this
#ifdef __ANDROID__
#define SEM_VALUE_MAX 0x3fffffff
#elif !defined(_WIN32)
#define SEM_VALUE_MAX 0x7fffffff
#else
#define SEM_VALUE_MAX 0x7fff // Be cautious if unknown
#endif // __ANDROID__
#endif // !defined(SEM_VALUE_MAX)
#endif // !defined(NO_LIMIT) && !__minix__

extern const char *const Version;

//Fix for stupid eclipse parser
#ifndef UINT_MAX
#define UINT_MAX 4294967295
#endif

#define MESSAGE_BUFFER_SIZE 4096
#ifdef IS_LIBRARY
extern char ErrorMessage[MESSAGE_BUFFER_SIZE];
#endif // IS_LIBRARY

extern int global_argc, multi_argc;
extern CARGV global_argv, multi_argv;
#ifndef _WIN32
extern int_fast8_t nodaemon;
#endif // _WIN32
extern DWORD VLActivationInterval;
extern DWORD VLRenewalInterval;
extern int_fast8_t DisconnectImmediately;
extern KmsResponseParam_t KmsResponseParameters[MAX_KMSAPPS];
extern const char *const cIPv4;
extern const char *const cIPv6;
extern int_fast8_t InetdMode;

#ifndef USE_MSRPC
extern int_fast8_t UseMultiplexedRpc;
extern int_fast8_t UseRpcNDR64;
extern int_fast8_t UseRpcBTFN;
#endif // USE_MSRPC

#ifndef NO_SOCKETS
extern const char *defaultport;
#endif // NO_SOCKETS

#if !defined(NO_PRIVATE_IP_DETECT)
extern uint32_t PublicIPProtectionLevel;
#endif

#if !defined(NO_SOCKETS) && !defined(NO_SIGHUP) && !defined(_WIN32)
extern int_fast8_t IsRestarted;
#endif // !defined(NO_SOCKETS) && !defined(NO_SIGHUP) && !defined(_WIN32)

#if !defined(NO_TIMEOUT) && !__minix__
extern DWORD ServerTimeout;
#endif // !defined(NO_TIMEOUT) && !__minix__

#if !defined(NO_LIMIT) && !defined (NO_SOCKETS) && !__minix__
extern uint32_t MaxTasks;
#endif // !defined(NO_LIMIT) && !defined (NO_SOCKETS) && !__minix__

#ifndef NO_LOG
extern int_fast8_t LogDateAndTime;
extern char *fn_log;
extern int_fast8_t logstdout;
#ifndef NO_VERBOSE_LOG
extern int_fast8_t logverbose;
#endif
#endif

#ifndef NO_RANDOM_EPID
extern int_fast8_t RandomizationLevel;
extern uint16_t Lcid;
#endif

#if !defined(NO_SOCKETS) && !defined(USE_MSRPC)
#if defined(SIMPLE_SOCKETS)
extern SOCKET s_server;
#else // !defined(SIMPLE_SOCKETS)
extern SOCKET *SocketList;
extern int numsockets;
#endif // !defined(SIMPLE_SOCKETS)

#if !defined(NO_LIMIT) && !__minix__

#ifndef _WIN32
extern sem_t *Semaphore;
#else // _WIN32
extern HANDLE Semaphore;
#endif // _WIN32

#endif // !defined(NO_LIMIT) && !__minix__

#endif // !defined(NO_SOCKETS) && !defined(USE_MSRPC)

#ifdef _NTSERVICE
extern int_fast8_t IsNTService;
extern int_fast8_t ServiceShutdown;
#endif

#ifndef NO_LOG
#ifdef USE_THREADS
#if !defined(_WIN32) && !defined(__CYGWIN__)
extern pthread_mutex_t logmutex;
#else
extern CRITICAL_SECTION logmutex;
#endif // _WIN32
#endif // USE_THREADS
#endif // NO_LOG

#if HAVE_FREEBIND
extern int_fast8_t freebind;
#endif // HAVE_FREEBIND


#endif // INCLUDED_SHARED_GLOBALS_H
