#ifndef __rpc_h
#define __rpc_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "types.h"

typedef struct {
	BYTE   VersionMajor;
	BYTE   VersionMinor;
	BYTE   PacketType;
	BYTE   PacketFlags;
	DWORD  DataRepresentation;
	WORD   FragLength;
	WORD   AuthLength;
	DWORD  CallId;
} /*__packed*/ RPC_HEADER;


typedef struct {
	WORD   MaxXmitFrag;
	WORD   MaxRecvFrag;
	DWORD  AssocGroup;
	DWORD  NumCtxItems;
	struct {
		WORD   ContextId;
		WORD   NumTransItems;
		GUID   InterfaceUUID;
		WORD   InterfaceVerMajor;
		WORD   InterfaceVerMinor;
		GUID   TransferSyntax;
		DWORD  SyntaxVersion;
	} CtxItems[1];
} /*__packed*/ RPC_BIND_REQUEST;

typedef struct {
	WORD   MaxXmitFrag;
	WORD   MaxRecvFrag;
	DWORD  AssocGroup;
	WORD   SecondaryAddressLength;
	BYTE   SecondaryAddress[6];
	DWORD  NumResults;
	struct {
		WORD   AckResult;
		WORD   AckReason;
		GUID   TransferSyntax;
		DWORD  SyntaxVersion;
	} Results[0];
} /*__packed*/ RPC_BIND_RESPONSE;


typedef struct {
	DWORD  AllocHint;
	WORD   ContextId;
	WORD   Opnum;
	struct {
		DWORD  DataLength;
		DWORD  DataSizeIs;
	} Ndr;
	BYTE   Data[0];
} /*__packed*/ RPC_REQUEST;

typedef struct {
	DWORD  AllocHint;
	WORD   ContextId;
	BYTE   CancelCount;
	BYTE   Pad1;
	struct {
		DWORD  DataLength;
		DWORD  DataSizeIs1;
		DWORD  DataSizeIs2;
	} Ndr;
	BYTE   Data[0];
} /*__packed*/ RPC_RESPONSE;

typedef struct {
	DWORD  AllocHint;
	WORD   ContextId;
	WORD   Opnum;
	union {
		struct {
			DWORD  DataLength;
			DWORD  DataSizeIs;
			BYTE   Data[0];
		} Ndr;
		struct {
			uint64_t DataLength;
			uint64_t DataSizeIs;
			BYTE     Data[0];
		} Ndr64;
	};
} /*__packed*/ RPC_REQUEST64;

typedef struct {
	DWORD  AllocHint;
	WORD   ContextId;
	BYTE   CancelCount;
	BYTE   Pad1;
	union {
		struct {
			DWORD  DataLength;
			DWORD  DataSizeMax;
			union
			{
				DWORD DataSizeIs;
				DWORD status;
			};
			BYTE   Data[0];
		} Ndr;
		struct {
			uint64_t DataLength;
			uint64_t DataSizeMax;
			union
			{
				uint64_t DataSizeIs;
				DWORD    status;
			};
			BYTE     Data[0];
		} Ndr64;
	};
} /*__packed*/ RPC_RESPONSE64;


typedef SOCKET RpcCtx;
typedef int RpcStatus;

#define INVALID_NDR_CTX ((WORD)~0)

#define RPC_BIND_ACCEPT (0)
#define RPC_BIND_NACK   (LE16(2))
#define RPC_BIND_ACK    (LE16(3))

#define RPC_SYNTAX_UNSUPPORTED         (LE16(2))
#define RPC_ABSTRACTSYNTAX_UNSUPPORTED (LE16(1))

#define RPC_BTFN_SEC_CONTEXT_MULTIPLEX (LE16(1))
#define RPC_BTFN_KEEP_ORPHAN           (LE16(2))

#define INVALID_RPCCTX INVALID_SOCKET
#define closeRpc socketclose

#define RPC_PT_REQUEST            0
#define RPC_PT_RESPONSE           2
#define RPC_PT_BIND_REQ          11
#define RPC_PT_BIND_ACK          12
#define RPC_PT_ALTERCONTEXT_REQ  14
#define RPC_PT_ALTERCONTEXT_ACK  15

#define RPC_PF_FIRST			  1
#define RPC_PF_LAST				  2
#define RPC_PF_CANCEL_PENDING	  4
#define RPC_PF_RESERVED			  8
#define RPC_PF_MULTIPLEX		 16
#define RPC_PF_NOT_EXEC			 32
#define RPC_PF_MAYBE			 64
#define RPC_PF_OBJECT			128

typedef union _RPC_FLAGS
{
	DWORD mask;
	struct {
		uint32_t FlagsBTFN : 16;
		BOOL HasNDR32      :  1;
		BOOL HasNDR64      :  1;
		BOOL HasBTFN       :  1;
	};
} RPC_FLAGS, *PRPC_FLAGS;

extern RPC_FLAGS RpcFlags;

void rpcServer(const RpcCtx socket, const DWORD RpcAssocGroup, const char* const ipstr);
RpcStatus rpcBindClient(const RpcCtx sock, const int_fast8_t verbose);
RpcStatus rpcSendRequest(const RpcCtx socket, const BYTE *const KmsRequest, const size_t requestSize, BYTE **KmsResponse, size_t *const responseSize);

#endif // __rpc_h
