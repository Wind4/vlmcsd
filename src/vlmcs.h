#ifndef VLMCS_H_
#define VLMCS_H_

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#if !defined(USE_MSRPC) && defined(_WIN32)
#include <winsock2.h>
#endif // defined(USE_MSRPC) && defined(_WIN32)
#include "types.h"
#ifndef USE_MSRPC
#include "rpc.h"
#else // USE_MSRPC
#include "msrpc-client.h"
#endif // USE_MSRPC
#include "kms.h"

#if MULTI_CALL_BINARY < 1
#define client_main main
#else
int client_main(int argc, CARGV argv);
#endif

int SendActivationRequest(const RpcCtx sock, RESPONSE *baseResponse, REQUEST *baseRequest, RESPONSE_RESULT *result, BYTE *const hwid);

#endif /* VLMCS_H_ */

