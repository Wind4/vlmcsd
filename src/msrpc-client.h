/*
 * msrpc-client.h
 */

#ifdef USE_MSRPC
#ifndef MSRPC_CLIENT_H_
#define MSRPC_CLIENT_H_

#include "types.h"
#include "shared_globals.h"
#include <setjmp.h>
#include "output.h"

typedef int_fast8_t RpcCtx;
typedef RPC_STATUS RpcStatus;

RpcCtx connectToAddress(char *const addr, const int AddressFamily_unused, int_fast8_t showHostName);
int_fast8_t isDisconnected(const RpcCtx handle);
RpcStatus rpcBindClient(const RpcCtx handle, const int_fast8_t verbose, PRpcDiag_t rpcDiag);
RpcStatus rpcSendRequest(const RpcCtx handle, BYTE* KmsRequest, size_t requestSize, BYTE **KmsResponse, size_t *responseSize);
RpcStatus closeRpc(RpcCtx s);

#define INVALID_RPCCTX ((RpcCtx)~0)
#endif // USE_MSRPC

#endif /* MSRPC_CLIENT_H_ */
