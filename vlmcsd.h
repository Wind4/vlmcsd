#ifndef __main_h
#define __main_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#define __T(x)    #x
#define  _T(x) __T(x)

extern char *fn_log;

#include "types.h"

//int main(int argc, CARGV);
extern void cleanup();

#ifdef _NTSERVICE
int newmain();
#endif

#if MULTI_CALL_BINARY < 1
#define server_main main
#else
int server_main(int argc, CARGV argv);
#endif

#ifndef SA_NOCLDWAIT    // required for Cygwin
#define SA_NOCLDWAIT 0
#endif

#ifndef NO_INI_FILE
#define INI_PARAM_RANDOMIZATION_LEVEL 1
#define INI_PARAM_LCID 2
#define INI_PARAM_LISTEN 3
#define INI_PARAM_MAX_WORKERS 4
#define INI_PARAM_CONNECTION_TIMEOUT 5
#define INI_PARAM_PID_FILE 6
#define INI_PARAM_LOG_FILE 7
#define INI_PARAM_LOG_VERBOSE 8
#define INI_PARAM_ACTIVATION_INTERVAL 9
#define INI_PARAM_RENEWAL_INTERVAL 10
#define INI_PARAM_DISCONNECT_IMMEDIATELY 11
#define INI_PARAM_UID 12
#define INI_PARAM_GID 13
#define INI_PARAM_PORT 14
#define INI_PARAM_RPC_NDR64 15
#define INI_PARAM_RPC_BTFN 16
#define INI_PARAM_FREEBIND 17
#define INI_PARAM_PUBLIC_IP_PROTECTION_LEVEL 18
#define INI_PARAM_LOG_DATE_AND_TIME 19

#define INI_FILE_PASS_1 1
#define INI_FILE_PASS_2 2

typedef struct
{
	const char* const Name;
	uint_fast8_t Id;
} IniFileParameter_t, *PIniFileParameter_t;
#endif // NO_INI_FILE

#endif // __main_h
