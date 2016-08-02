#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "shared_globals.h"

int global_argc, multi_argc = 0;
CARGV global_argv, multi_argv = NULL;
const char *const Version = VERSION;
DWORD VLActivationInterval = 60 * 2;   // 2 hours
DWORD VLRenewalInterval = 60 * 24 * 7; // 7 days
int_fast8_t DisconnectImmediately = FALSE;
const char *const cIPv4 = "IPv4";
const char *const cIPv6 = "IPv6";

#ifndef USE_MSRPC
int_fast8_t UseMultiplexedRpc = TRUE;
int_fast8_t UseRpcNDR64 = TRUE;
int_fast8_t UseRpcBTFN = TRUE;
#endif // USE_MSRPC

#ifndef NO_SOCKETS
const char *defaultport = "1688";
#endif // NO_SOCKETS

#if !defined(NO_PRIVATE_IP_DETECT)
uint32_t PublicIPProtectionLevel = 0;
#endif

KmsResponseParam_t KmsResponseParameters[MAX_KMSAPPS];

#if !defined(NO_SOCKETS) && !defined(NO_SIGHUP) && !defined(_WIN32)
int_fast8_t IsRestarted = FALSE;
#endif // !defined(NO_SOCKETS) && !defined(NO_SIGHUP) && !defined(_WIN32)

#if !defined(NO_TIMEOUT) && !__minix__
DWORD ServerTimeout = 30;
#endif // !defined(NO_TIMEOUT) && !__minix__

#if !defined(NO_LIMIT) && !defined (NO_SOCKETS) && !__minix__
#ifdef USE_MSRPC
uint32_t MaxTasks = RPC_C_LISTEN_MAX_CALLS_DEFAULT;
#else // !USE_MSRPC
uint32_t MaxTasks = SEM_VALUE_MAX;
#endif // !USE_MSRPC
#endif // !defined(NO_LIMIT) && !defined (NO_SOCKETS) && !__minix__

#ifndef NO_LOG
int_fast8_t LogDateAndTime = TRUE;
char *fn_log = NULL;
int_fast8_t logstdout = 0;
#ifndef NO_VERBOSE_LOG
int_fast8_t logverbose = 0;
#endif // NO_VERBOSE_LOG
#endif // NO_LOG

#ifndef NO_SOCKETS
#ifndef _WIN32
int_fast8_t nodaemon = 0;
#endif // _WIN32
int_fast8_t InetdMode = 0;
#else
#ifndef _WIN32
int_fast8_t nodaemon = 1;
#endif // _WIN32
int_fast8_t InetdMode = 1;
#endif

#ifndef NO_RANDOM_EPID
int_fast8_t RandomizationLevel = 1;
uint16_t Lcid = 0;
#endif

#ifndef NO_SOCKETS
#ifdef SIMPLE_SOCKETS
SOCKET s_server;
#else
SOCKET *SocketList;
int numsockets = 0;
#endif

#if !defined(NO_LIMIT) && !__minix__
#ifndef _WIN32 // Posix
sem_t *Semaphore;
#else // _WIN32
HANDLE Semaphore;
#endif // _WIN32

#endif // !defined(NO_LIMIT) && !__minix__
#endif // NO_SOCKETS

#ifdef _NTSERVICE
int_fast8_t IsNTService = TRUE;
int_fast8_t ServiceShutdown = FALSE;
#endif // _NTSERVICE

#ifndef NO_LOG
#ifdef USE_THREADS
#if !defined(_WIN32) && !defined(__CYGWIN__)
pthread_mutex_t logmutex = PTHREAD_MUTEX_INITIALIZER;
#else
CRITICAL_SECTION logmutex;
#endif // !defined(_WIN32) && !defined(__CYGWIN__)
#endif // USE_THREADS
#endif // NO_LOG

#if HAVE_FREEBIND
int_fast8_t freebind = FALSE;
#endif // HAVE_FREEBIND





