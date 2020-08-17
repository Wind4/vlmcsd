#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif // _DEFAULT_SOURCE

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifndef USE_MSRPC

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
//#include <ctype.h>
//#include <time.h>
#if !defined(_WIN32)
#include <sys/socket.h>
#include <netdb.h>
#endif
#include "rpc.h"
#include "output.h"
//#include "crypto.h"
#include "endian.h"
#include "helpers.h"
#include "network.h"
#include "shared_globals.h"

/* Forwards */

static int checkRpcHeader(const RPC_HEADER *const header, const BYTE desiredPacketType, const PRINTFUNC p);


/* Data definitions */

// All GUIDs are defined as BYTE[16] here. No big-endian/little-endian byteswapping required.
static const BYTE TransferSyntaxNDR32[] = {
	0x04, 0x5D, 0x88, 0x8A, 0xEB, 0x1C, 0xC9, 0x11, 0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60
};

static const BYTE InterfaceUuid[] = {
	0x75, 0x21, 0xc8, 0x51, 0x4e, 0x84, 0x50, 0x47, 0xB0, 0xD8, 0xEC, 0x25, 0x55, 0x55, 0xBC, 0x06
};

//#ifndef SIMPLE_RPC
static const BYTE TransferSyntaxNDR64[] = {
	0x33, 0x05, 0x71, 0x71, 0xba, 0xbe, 0x37, 0x49, 0x83, 0x19, 0xb5, 0xdb, 0xef, 0x9c, 0xcc, 0x36
};

static const BYTE BindTimeFeatureNegotiation[] = {
	0x2c, 0x1c, 0xb7, 0x6c, 0x12, 0x98, 0x40, 0x45, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
//#endif // SIMPLE_RPC

//
// Dispatch RPC payload to kms.c
//
typedef int(*CreateResponse_t)(const void *const, void *const, const char* const);

// ReSharper disable CppIncompatiblePointerConversion
static const struct {
	unsigned int  RequestSize;
	CreateResponse_t CreateResponse;
} _Versions[] = {
	{ sizeof(REQUEST_V4), (CreateResponse_t)CreateResponseV4 },
	{ sizeof(REQUEST_V6), (CreateResponse_t)CreateResponseV6 },
	{ sizeof(REQUEST_V6), (CreateResponse_t)CreateResponseV6 }
};
// ReSharper restore CppIncompatiblePointerConversion

RPC_FLAGS RpcFlags;
static int_fast8_t firstPacketSent;
static DWORD CallId = 2; // M$ starts with CallId 2. So we do the same.

//
// RPC request (server)
//
#if defined(_PEDANTIC) && !defined(NO_LOG)
static void CheckRpcRequest(const RPC_REQUEST64 *const Request, const unsigned int len, WORD* NdrCtx, WORD* Ndr64Ctx, WORD Ctx)
{
	uint_fast8_t kmsMajorVersion;
	uint32_t requestSize = Ctx != *Ndr64Ctx ? sizeof(RPC_REQUEST) : sizeof(RPC_REQUEST64);

	if (len < requestSize)
	{
		logger("Fatal: RPC request (including header) must be at least %i bytes but is only %i bytes.\n",
			(int)(sizeof(RPC_HEADER) + requestSize),
			(int)(len + sizeof(RPC_HEADER))
		);

		return;
	}

	if (len < requestSize + sizeof(DWORD))
	{
		logger("Fatal: KMS Request too small to contain version info (less than 4 bytes).\n");
		return;
	}

	if (Ctx != *Ndr64Ctx)
		kmsMajorVersion = (uint_fast8_t)LE16(((WORD*)Request->Ndr.Data)[1]);
	else
		kmsMajorVersion = (uint_fast8_t)LE16(((WORD*)Request->Ndr64.Data)[1]);

	if (kmsMajorVersion > 6)
	{
		logger("Fatal: KMSv%u is not supported.\n", (unsigned int)kmsMajorVersion);
	}
	else
	{
		if (len > _Versions[kmsMajorVersion - 4].RequestSize + requestSize)
			logger("Warning: %u excess bytes in RPC request.\n",
				len - (_Versions[kmsMajorVersion - 4].RequestSize + requestSize)
			);
	}

	if (Ctx != *Ndr64Ctx && Ctx != *NdrCtx)
	{
		if (*Ndr64Ctx == RPC_INVALID_CTX)
		{
			logger("Warning: Context id should be %u but is %u.\n", (unsigned int)*NdrCtx, Ctx);
		}
		else
		{
			logger("Warning: Context id should be %u (NDR32) or %u (NDR64) but is %u.\n",
				(unsigned int)*NdrCtx,
				(unsigned int)*Ndr64Ctx,
				Ctx
			);
		}
	}

	if (Request->Opnum)
		logger("Warning: OpNum should be 0 but is %u.\n",
		(unsigned int)LE16(Request->Opnum)
		);

	if (LE32(Request->AllocHint) != len - sizeof(RPC_REQUEST) + sizeof(Request->Ndr))
		logger("Warning: Allocation hint should be %u but is %u.\n",
			len + sizeof(Request->Ndr),
			LE32(Request->AllocHint)
		);

	if (Ctx != *Ndr64Ctx)
	{
		if (LE32(Request->Ndr.DataLength) != len - sizeof(RPC_REQUEST))
			logger("Warning: NDR32 data length field should be %u but is %u.\n",
				len - sizeof(RPC_REQUEST),
				LE32(Request->Ndr.DataLength)
			);

		if (LE32(Request->Ndr.DataSizeIs) != len - sizeof(RPC_REQUEST))
			logger("Warning: NDR32 data size field should be %u but is %u.\n",
				len - sizeof(RPC_REQUEST),
				LE32(Request->Ndr.DataSizeIs)
			);
	}
	else
	{
		if (LE64(Request->Ndr64.DataLength) != len - sizeof(RPC_REQUEST64))
			logger("Warning: NDR32 data length field should be %u but is %u.\n",
				len - sizeof(RPC_REQUEST) + sizeof(Request->Ndr),
				LE64(Request->Ndr64.DataLength)
			);

		if (LE64(Request->Ndr64.DataSizeIs) != len - sizeof(RPC_REQUEST64))
			logger("Warning: NDR32 data size field should be %u but is %u.\n",
				len - sizeof(RPC_REQUEST64),
				LE64(Request->Ndr64.DataSizeIs)
			);
	}
}
#endif // defined(_PEDANTIC) && !defined(NO_LOG)

/*
 * check RPC request for (somewhat) correct size
 * allow any size that does not cause CreateResponse to fail badly
 */
static unsigned int checkRpcRequestSize(const RPC_REQUEST64 *const Request, const unsigned int requestSize, WORD* NdrCtx, WORD* Ndr64Ctx)
{
	WORD Ctx = LE16(Request->ContextId);

#	if defined(_PEDANTIC) && !defined(NO_LOG)
	CheckRpcRequest(Request, requestSize, NdrCtx, Ndr64Ctx, Ctx);
#	endif // defined(_PEDANTIC) && !defined(NO_LOG)

	// Anything that is smaller than a v4 request is illegal
	if (requestSize < sizeof(REQUEST_V4) + (Ctx != *Ndr64Ctx ? sizeof(RPC_REQUEST) : sizeof(RPC_REQUEST64))) return 0;

	// Get KMS major version
	uint16_t majorIndex, minor;
	DWORD version;

#	ifndef SIMPLE_RPC

	if (Ctx != *Ndr64Ctx)
	{
		version = LE32(*(DWORD*)Request->Ndr.Data);
	}
	else
	{
		version = LE32(*(DWORD*)Request->Ndr64.Data);
	}

#	else // SIMPLE_RPC

	version = LE32(*(DWORD*)Request->Ndr.Data);

#	endif // SIMPLE_RPC

	majorIndex = (uint16_t)(version >> 16) - 4;
	minor = (uint16_t)(version & 0xffff);

	// Only KMS v4, v5 and v6 are supported
	if (majorIndex >= vlmcsd_countof(_Versions) || minor)
	{
#		ifndef NO_LOG
		logger("Fatal: KMSv%hu.%hu unsupported\n", (unsigned short)majorIndex + 4, (unsigned short)minor);
#		endif // NO_LOG
		return 0;
	}

	// Could check for equality but allow bigger requests to support buggy RPC clients (e.g. wine)
	// Buffer overrun is check by caller.
	return (requestSize >= _Versions[majorIndex].RequestSize);
}

#ifndef SIMPLE_RPC
static int SendError(RPC_RESPONSE64 *const Response, DWORD nca_error)
{
	Response->Error.Code = nca_error;
	Response->Error.Padding = 0;
	Response->AllocHint = LE32(32);
	Response->ContextId = 0;
	return 32;
}
#endif // SIMPLE_RPC

/*
 * Handles the actual KMS request from the client.
 * Calls KMS functions (CreateResponseV4 or CreateResponseV6) in kms.c
 * Returns size of the KMS response packet or 0 on failure.
 *
 * The RPC packet size (excluding header) is actually in Response->AllocHint
 */
static int rpcRequest(const RPC_REQUEST64 *const Request, RPC_RESPONSE64 *const Response, const DWORD RpcAssocGroup_unused, const SOCKET sock_unused, WORD* NdrCtx, WORD* Ndr64Ctx, BYTE isValid, const char* const ipstr)
{
	int ResponseSize; // <0 = Errorcode (HRESULT)
	BYTE* requestData;
	BYTE* responseData;
	BYTE* pRpcReturnCode;
	int len;

#	ifndef SIMPLE_RPC

	const WORD Ctx = LE16(Request->ContextId);

	if (Ctx == *NdrCtx)
	{
		requestData = (BYTE*)&Request->Ndr.Data;
		responseData = (BYTE*)&Response->Ndr.Data;
	}
	else if (Ctx == *Ndr64Ctx)
	{
		requestData = (BYTE*)&Request->Ndr64.Data;
		responseData = (BYTE*)&Response->Ndr64.Data;
	}
	else
	{
		return SendError(Response, RPC_NCA_UNK_IF);
	}

#	else // SIMPLE_RPC

	requestData = (BYTE*)&Request->Ndr.Data;
	responseData = (BYTE*)&Response->Ndr.Data;

#	endif // SIMPLE_RPC

	ResponseSize = 0x8007000D; // Invalid Data

	if (isValid)
	{
		const uint16_t majorIndex = LE16(((WORD*)requestData)[1]) - 4;
		if (!((ResponseSize = _Versions[majorIndex].CreateResponse(requestData, responseData, ipstr)))) ResponseSize = 0x8007000D;
	}

#	ifndef SIMPLE_RPC

	if (Ctx != *Ndr64Ctx)
	{

#	endif // !SIMPLE_RPC
		if (ResponseSize < 0)
		{
			Response->Ndr.DataSizeMax = Response->Ndr.DataLength = 0;
			len = sizeof(Response->Ndr) - sizeof(Response->Ndr.DataSizeIs);
		}
		else
		{
			Response->Ndr.DataSizeMax = LE32(0x00020000);
			Response->Ndr.DataLength = Response->Ndr.DataSizeIs = LE32(ResponseSize);
			len = ResponseSize + sizeof(Response->Ndr);
		}

#	ifndef SIMPLE_RPC

	}
	else
	{
		if (ResponseSize < 0)
		{
			Response->Ndr64.DataSizeMax = Response->Ndr64.DataLength = 0;
			len = sizeof(Response->Ndr64) - sizeof(Response->Ndr64.DataSizeIs);
		}
		else
		{
			Response->Ndr64.DataSizeMax = LE64(0x00020000ULL);
			Response->Ndr64.DataLength = Response->Ndr64.DataSizeIs = LE64((uint64_t)ResponseSize);
			len = ResponseSize + sizeof(Response->Ndr64);
		}
	}

#	endif // !SIMPLE_RPC

	pRpcReturnCode = ((BYTE*)&Response->Ndr) + len;
	PUT_UA32LE(pRpcReturnCode, ResponseSize < 0 ? ResponseSize : 0);
	len += sizeof(DWORD);

	// Pad zeros to 32-bit align (seems not neccassary but Windows RPC does it this way)
	const int pad = ((~len & 3) + 1) & 3;
	memset(pRpcReturnCode + sizeof(DWORD), 0, pad);
	len += pad;

	Response->AllocHint = LE32(len);
	Response->ContextId = Request->ContextId;

	*((WORD*)&Response->CancelCount) = 0; // CancelCount + Pad1

	return len + 8;
}


#if defined(_PEDANTIC) && !defined(NO_LOG)
static void CheckRpcBindRequest(const RPC_BIND_REQUEST *const Request, const unsigned int len)
{
	uint_fast8_t i, HasTransferSyntaxNDR32 = FALSE;
	char guidBuffer1[GUID_STRING_LENGTH + 1], guidBuffer2[GUID_STRING_LENGTH + 1];

	const uint32_t CapCtxItems = (len - sizeof(*Request) + sizeof(Request->CtxItems)) / sizeof(Request->CtxItems);
	const DWORD NumCtxItems = LE32(Request->NumCtxItems);

	if (NumCtxItems < CapCtxItems) // Can't be too small because already handled by RpcBindSize
		logger("Warning: Excess bytes in RPC bind request.\n");

	for (i = 0; i < NumCtxItems; i++)
	{
		struct CtxItem const* ctxItem = Request->CtxItems + i;
		if (!IsEqualGUID(&ctxItem->InterfaceUUID, InterfaceUuid))
		{
			uuid2StringLE(&ctxItem->InterfaceUUID, guidBuffer1);
			uuid2StringLE((GUID*)InterfaceUuid, guidBuffer2);
			logger("Fatal: Interface UUID is %s but should be %s in Ctx item %u.\n", guidBuffer1, guidBuffer2, (unsigned int)i);
		}

		if (ctxItem->NumTransItems != LE16(1))
			logger("Fatal: %u NDR32 transfer items detected in Ctx item %u, but only one is supported.\n",
			(unsigned int)LE16(ctxItem->NumTransItems), (unsigned int)i
			);

		if (ctxItem->InterfaceVerMajor != LE16(1) || ctxItem->InterfaceVerMinor != 0)
			logger("Warning: Interface version is %u.%u but should be 1.0.\n",
			(unsigned int)LE16(ctxItem->InterfaceVerMajor),
				(unsigned int)LE16(ctxItem->InterfaceVerMinor)
			);

		if (ctxItem->ContextId != LE16((WORD)i))
			logger("Warning: context id of Ctx item %u is %u.\n", (unsigned int)i, (unsigned int)ctxItem->ContextId);

		if (IsEqualGUID((GUID*)TransferSyntaxNDR32, &ctxItem->TransferSyntax))
		{
			HasTransferSyntaxNDR32 = TRUE;

			if (ctxItem->SyntaxVersion != LE32(2))
				logger("NDR32 transfer syntax version is %u but should be 2.\n", LE32(ctxItem->SyntaxVersion));
		}
		else if (IsEqualGUID((GUID*)TransferSyntaxNDR64, &ctxItem->TransferSyntax))
		{
			if (ctxItem->SyntaxVersion != LE32(1))
				logger("NDR64 transfer syntax version is %u but should be 1.\n", LE32(ctxItem->SyntaxVersion));
		}
		else if (!memcmp(BindTimeFeatureNegotiation, (BYTE*)(&ctxItem->TransferSyntax), 8))
		{
			if (ctxItem->SyntaxVersion != LE32(1))
				logger("BTFN syntax version is %u but should be 1.\n", LE32(ctxItem->SyntaxVersion));
		}
	}

	if (!HasTransferSyntaxNDR32)
		logger("Warning: RPC bind request has no NDR32 CtxItem.\n");
}
#endif // defined(_PEDANTIC) && !defined(NO_LOG)


/*
 * Check, if we receive enough bytes to return a valid RPC bind response
 */
static unsigned int checkRpcBindSize(const RPC_BIND_REQUEST *const Request, const unsigned int RequestSize, WORD* NdrCtx_unused, WORD* Ndr64Ctx_unused)
{
	if (RequestSize < sizeof(RPC_BIND_REQUEST)) return FALSE;

	const unsigned int numCtxItems = LE32(Request->NumCtxItems);

	if (RequestSize < sizeof(RPC_BIND_REQUEST) - sizeof(Request->CtxItems[0]) + numCtxItems * sizeof(Request->CtxItems[0])) return FALSE;

#if defined(_PEDANTIC) && !defined(NO_LOG)
	CheckRpcBindRequest(Request, RequestSize);
#endif // defined(_PEDANTIC) && !defined(NO_LOG)

	return TRUE;
}


/*
 * Accepts a bind or alter context request from the client and composes the bind response.
 * Needs the socket because the tcp port number is part of the response.
 * len is not used here.
 *
 * Returns TRUE on success.
 */
static int rpcBind(const RPC_BIND_REQUEST *const Request, RPC_BIND_RESPONSE* Response, const DWORD RpcAssocGroup, const SOCKET sock, WORD* NdrCtx, WORD* Ndr64Ctx, BYTE packetType, const char* const ipstr_unused)
{
	unsigned int i;
	const DWORD numCtxItems = LE32(Request->NumCtxItems);
	int_fast8_t IsNDR64possible = FALSE;
	uint_fast8_t portNumberSize;

	socklen_t socklen;
	struct sockaddr_storage addr;

	// M$ RPC does not do this. Pad bytes contain apparently random data
	// memset(Response->SecondaryAddress, 0, sizeof(Response->SecondaryAddress));

	socklen = sizeof addr;

	if (
		packetType == RPC_PT_ALTERCONTEXT_REQ ||
		getsockname(sock, (struct sockaddr*)&addr, &socklen) ||
		getnameinfo((struct sockaddr*)&addr, socklen, NULL, 0, (char*)Response->SecondaryAddress, sizeof(Response->SecondaryAddress), NI_NUMERICSERV))
	{
		portNumberSize = 0;
		Response->SecondaryAddressLength = 0;
	}
	else
	{
		portNumberSize = (uint_fast8_t)strlen((char*)Response->SecondaryAddress) + 1;
		Response->SecondaryAddressLength = LE16(portNumberSize);
	}

	Response->MaxXmitFrag = Request->MaxXmitFrag;
	Response->MaxRecvFrag = Request->MaxRecvFrag;
	Response->AssocGroup = LE32(RpcAssocGroup);

	// This is really ugly (but efficient) code to support padding after the secondary address field
	if (portNumberSize < 3)
	{
		Response = (RPC_BIND_RESPONSE*)((BYTE*)Response - 4);
	}

	Response->NumResults = Request->NumCtxItems;

#	ifndef SIMPLE_RPC

	for (i = 0; i < numCtxItems; i++)
	{
		const struct CtxItem* ctxItem = &Request->CtxItems[i];
		if (IsEqualGUID((GUID*)TransferSyntaxNDR32, &ctxItem->TransferSyntax))
		{
			/*if (packetType == RPC_PT_BIND_REQ)*/
			*NdrCtx = LE16(ctxItem->ContextId);
		}

		if (UseServerRpcNDR64 && IsEqualGUID((GUID*)TransferSyntaxNDR64, &ctxItem->TransferSyntax))
		{
			IsNDR64possible = TRUE;

			/*if (packetType == RPC_PT_BIND_REQ)*/
			*Ndr64Ctx = LE16(ctxItem->ContextId);
		}
	}

#	endif // !SIMPLE_RPC

	for (i = 0; i < numCtxItems; i++)
	{
		struct CtxResults* result = Response->Results + i;
		const GUID* ctxTransferSyntax = &Request->CtxItems[i].TransferSyntax;

#		ifndef SIMPLE_RPC
		WORD nackReason = RPC_ABSTRACTSYNTAX_UNSUPPORTED;
#		endif // !SIMPLE_RPC

		memset(&result->TransferSyntax, 0, sizeof(GUID));

#		ifndef SIMPLE_RPC
		const int isInterfaceUUID = IsEqualGUID(&Request->CtxItems[i].InterfaceUUID, (GUID*)InterfaceUuid);
		if (isInterfaceUUID) nackReason = RPC_SYNTAX_UNSUPPORTED;
#		else // SIMPLE_RPC
#		define isInterfaceUUID TRUE
#		endif // SIMPLE_RPC

		if (isInterfaceUUID && !IsNDR64possible && IsEqualGUID((GUID*)TransferSyntaxNDR32, ctxTransferSyntax))
		{
			result->SyntaxVersion = LE32(2);
			result->AckResult = result->AckReason = RPC_BIND_ACCEPT;
			memcpy(&result->TransferSyntax, TransferSyntaxNDR32, sizeof(GUID));
			continue;
		}

#		ifndef SIMPLE_RPC

		if (IsEqualGUID((GUID*)TransferSyntaxNDR64, ctxTransferSyntax))
		{
			if (!UseServerRpcNDR64) nackReason = RPC_SYNTAX_UNSUPPORTED;

			if (isInterfaceUUID && IsNDR64possible)
			{
				result->SyntaxVersion = LE32(1);
				result->AckResult = result->AckReason = RPC_BIND_ACCEPT;
				memcpy(&result->TransferSyntax, TransferSyntaxNDR64, sizeof(GUID));
				continue;
			}
		}

		if (!memcmp(BindTimeFeatureNegotiation, ctxTransferSyntax, 8))
		{
			nackReason = RPC_SYNTAX_UNSUPPORTED;

			if (UseServerRpcBTFN)
			{
				result->SyntaxVersion = 0;
				result->AckResult = RPC_BIND_ACK;

				// Features requested are actually encoded in the GUID
				result->AckReason =
					((WORD*)(ctxTransferSyntax))[4] &
					(RPC_BTFN_SEC_CONTEXT_MULTIPLEX | RPC_BTFN_KEEP_ORPHAN);

				continue;
			}
		}

#		endif // !SIMPLE_RPC

		result->SyntaxVersion = 0;
		result->AckResult = RPC_BIND_NACK;
#		ifndef SIMPLE_RPC
		result->AckReason = nackReason;
#		else // SIMPLE_RPC
#		undef isInterfaceUUID
		result->AckReason = RPC_SYNTAX_UNSUPPORTED;
#		endif // SIMPLE_RPC
	}

	//if (!_st) return 0;

	return sizeof(RPC_BIND_RESPONSE) + numCtxItems * sizeof(struct CtxResults) - (portNumberSize < 3 ? 4 : 0);
}


//
// Main RPC handling routine
//
typedef unsigned int(*GetResponseSize_t)(const void *const request, const unsigned int requestSize, WORD* NdrCtx, WORD* Ndr64Ctx);
typedef int(*GetResponse_t)(const void* const request, void* response, const DWORD rpcAssocGroup, const SOCKET socket, WORD* NdrCtx, WORD* Ndr64Ctx, BYTE packetType, const char* const ipstr);

// ReSharper disable CppIncompatiblePointerConversion
static const struct {
	BYTE  ResponsePacketType;
	GetResponseSize_t CheckRequest;
	GetResponse_t GetResponse;
}
_Actions[] = {
	{ RPC_PT_BIND_ACK,         (GetResponseSize_t)checkRpcBindSize,    (GetResponse_t)rpcBind    },
	{ RPC_PT_RESPONSE,         (GetResponseSize_t)checkRpcRequestSize, (GetResponse_t)rpcRequest },
	{ RPC_PT_ALTERCONTEXT_ACK, (GetResponseSize_t)checkRpcBindSize,    (GetResponse_t)rpcBind    },
};
// ReSharper restore CppIncompatiblePointerConversion


/*
* Initializes an RPC request header as needed for KMS, i.e. packet always fits in one fragment.
* size cannot be greater than fragment length negotiated during RPC bind.
*/
static void createRpcHeader(RPC_HEADER* header, BYTE packetType, WORD size)
{
	header->PacketType = packetType;
	header->PacketFlags = RPC_PF_FIRST | RPC_PF_LAST;
	header->VersionMajor = 5;
	header->VersionMinor = 0;
	header->AuthLength = 0;
	header->DataRepresentation = BE32(0x10000000); // Little endian, ASCII charset, IEEE floating point
	header->CallId = LE32(CallId);
	header->FragLength = LE16(size);
}


/*
 * This is the main RPC server loop. Returns after KMS request has been serviced
 * or a timeout has occured.
 */
void rpcServer(const SOCKET sock, const DWORD rpcAssocGroup, const char* const ipstr)
{
	RPC_HEADER  rpcRequestHeader;
	WORD NdrCtx = RPC_INVALID_CTX, Ndr64Ctx = RPC_INVALID_CTX;

	randomNumberInit();

	while (_recv(sock, &rpcRequestHeader, sizeof(rpcRequestHeader)))
	{
		//int_fast8_t  _st;
		unsigned int request_len, response_len;
		uint_fast8_t _a;

#if defined(_PEDANTIC) && !defined(NO_LOG)
		checkRpcHeader(&rpcRequestHeader, rpcRequestHeader.PacketType, &logger);
#endif // defined(_PEDANTIC) && !defined(NO_LOG)

		switch (rpcRequestHeader.PacketType)
		{
		case RPC_PT_BIND_REQ:         _a = 0; break;
		case RPC_PT_REQUEST:          _a = 1; break;
		case RPC_PT_ALTERCONTEXT_REQ: _a = 2; break;
		default: return;
		}

		request_len = LE16(rpcRequestHeader.FragLength) - sizeof(rpcRequestHeader);

		BYTE requestBuffer[MAX_REQUEST_SIZE + sizeof(RPC_RESPONSE64)];
		BYTE responseBuffer[MAX_RESPONSE_SIZE + sizeof(RPC_HEADER) + sizeof(RPC_RESPONSE64)];

		RPC_HEADER *rpcResponseHeader = (RPC_HEADER *)responseBuffer;
		RPC_RESPONSE* rpcResponse = (RPC_RESPONSE*)(responseBuffer + sizeof(rpcRequestHeader));

		// The request is larger than the buffer size
		if (request_len > MAX_REQUEST_SIZE + sizeof(RPC_REQUEST64)) return;

		// Unable to receive the complete request
		if (!_recv(sock, requestBuffer, request_len)) return;

#       if !defined(SIMPLE_RPC) && defined(_PEDANTIC)
		if (rpcRequestHeader.PacketType == RPC_PT_REQUEST && (rpcRequestHeader.VersionMajor != 5 || rpcRequestHeader.VersionMinor != 0))
		{
			response_len = SendError((RPC_RESPONSE64*)rpcResponse, RPC_NCA_PROTO_ERROR);
		}
		else
#		endif // !defined(SIMPLE_RPC) && defined(_PEDANTIC)
		{
			BYTE isValid = (BYTE)_Actions[_a].CheckRequest(requestBuffer, request_len, &NdrCtx, &Ndr64Ctx);
			if (rpcRequestHeader.PacketType != RPC_PT_REQUEST && !isValid) return;

			// Unable to create a valid response from request
			if (!((response_len = _Actions[_a].GetResponse(requestBuffer, rpcResponse, rpcAssocGroup, sock, &NdrCtx, &Ndr64Ctx, rpcRequestHeader.PacketType != RPC_PT_REQUEST ? rpcRequestHeader.PacketType : isValid, ipstr)))) return;
		}

		memcpy(rpcResponseHeader, &rpcRequestHeader, sizeof(RPC_HEADER));

#       ifndef SIMPLE_RPC
		if (response_len == 32)
		{
			createRpcHeader(rpcResponseHeader, RPC_PT_FAULT, 0);
			rpcResponseHeader->PacketFlags = RPC_PF_FIRST | RPC_PF_LAST | RPC_PF_NOT_EXEC;
		}
		else
#		endif // SIMPLE_RPC
		{
			response_len += sizeof(RPC_HEADER);
			rpcResponseHeader->PacketType = _Actions[_a].ResponsePacketType;

			if (rpcResponseHeader->PacketType == RPC_PT_ALTERCONTEXT_ACK)
			{
				rpcResponseHeader->PacketFlags = RPC_PF_FIRST | RPC_PF_LAST;
			}
		}

		rpcResponseHeader->FragLength = LE16((WORD)response_len);

		if (!_send(sock, responseBuffer, response_len)) return;

		if (DisconnectImmediately && (rpcResponseHeader->PacketType == RPC_PT_RESPONSE || rpcResponseHeader->PacketType == RPC_PT_FAULT))
			return;
	}
}


/* RPC client functions */


/*
 * Checks RPC header. Returns 0 on success.
 * This is mainly for debugging a non Microsoft KMS server that uses its own RPC code.
 */
static int checkRpcHeader(const RPC_HEADER *const header, const BYTE desiredPacketType, const PRINTFUNC p)
{
	int status = 0;

	if (header->PacketType != desiredPacketType)
	{
		p("Fatal: Received wrong RPC packet type. Expected %u but got %u\n",
			(uint32_t)desiredPacketType,
			header->PacketType
		);
		status = RPC_S_PROTOCOL_ERROR;
	}

	if (header->DataRepresentation != BE32(0x10000000))
	{
		p("Fatal: RPC response does not conform to Microsoft's limited support of DCE RPC\n");
		status = RPC_S_PROTOCOL_ERROR;
	}

	if (header->AuthLength != 0)
	{
		p("Fatal: RPC response requests authentication\n");
		status = RPC_S_UNKNOWN_AUTHN_TYPE;
	}

	// vlmcsd does not support fragmented packets (not yet neccassary)
	if ((header->PacketFlags & (RPC_PF_FIRST | RPC_PF_LAST)) != (RPC_PF_FIRST | RPC_PF_LAST))
	{
		p("Fatal: RPC packet flags RPC_PF_FIRST and RPC_PF_LAST are not both set.\n");
		status = RPC_S_CANNOT_SUPPORT;
	}

	if (header->PacketFlags & RPC_PF_CANCEL_PENDING)	p("Warning: %s should not be set\n", "RPC_PF_CANCEL_PENDING");
	if (header->PacketFlags & RPC_PF_RESERVED)			p("Warning: %s should not be set\n", "RPC_PF_RESERVED");
	if (header->PacketFlags & RPC_PF_NOT_EXEC)			p("Warning: %s should not be set\n", "RPC_PF_NOT_EXEC");
	if (header->PacketFlags & RPC_PF_MAYBE)				p("Warning: %s should not be set\n", "RPC_PF_MAYBE");
	if (header->PacketFlags & RPC_PF_OBJECT)			p("Warning: %s should not be set\n", "RPC_PF_OBJECT");

	if (header->VersionMajor != 5 || header->VersionMinor != 0)
	{
		p("Fatal: Expected RPC version 5.0 and got %u.%u\n", header->VersionMajor, header->VersionMinor);
		status = RPC_S_INVALID_VERS_OPTION;
	}

	return status;
}


/*
 * Checks an RPC response header. Does basic header checks by calling checkRpcHeader()
 * and then does additional checks if response header complies with the respective request header.
 * PRINTFUNC p can be anything that has the same prototype as printf.
 * Returns 0 on success.
 */
 // ReSharper disable once CppIncompatiblePointerConversion
static int checkRpcResponseHeader(const RPC_HEADER *const ResponseHeader, const RPC_HEADER *const RequestHeader, const BYTE desiredPacketType, const PRINTFUNC p)
{
	static int_fast8_t WineBugDetected = FALSE;
	int status = checkRpcHeader(ResponseHeader, desiredPacketType, p);

	if (desiredPacketType == RPC_PT_BIND_ACK)
	{
		if ((ResponseHeader->PacketFlags & RPC_PF_MULTIPLEX) != (RequestHeader->PacketFlags & RPC_PF_MULTIPLEX))
		{
			p("Warning: RPC_PF_MULTIPLEX of RPC request and response should match\n");
		}
	}
	else
	{
		if (ResponseHeader->PacketFlags & RPC_PF_MULTIPLEX)
		{
			p("Warning: %s should not be set\n", "RPC_PF_MULTIPLEX");
		}
	}

	if (!status && ResponseHeader->CallId == LE32(1))
	{
		if (!WineBugDetected)
		{
			p("Warning: Buggy RPC of Wine detected. Call Id of Response is always 1\n");
			WineBugDetected = TRUE;
		}
	}
	else if (ResponseHeader->CallId != RequestHeader->CallId)
	{
		p("Fatal: Sent Call Id %u but received answer for Call Id %u\n",
			(uint32_t)LE32(RequestHeader->CallId),
			(uint32_t)LE32(ResponseHeader->CallId)
		);

		status = RPC_S_PROTOCOL_ERROR;
	}

	return status;
}

/*
 * Sends a KMS request via RPC and receives a response.
 * Parameters are raw (encrypted) reqeuests / responses.
 * Returns 0 on success.
 */
RpcStatus rpcSendRequest(const RpcCtx sock, const BYTE *const kmsRequest, const size_t requestSize, BYTE **kmsResponse, size_t *const responseSize)
{
#define MAX_EXCESS_BYTES 16
	RPC_HEADER *RequestHeader, ResponseHeader;
	RPC_REQUEST64 *RpcRequest;
	RPC_RESPONSE64 _Response;
	int status;
	const int_fast8_t useNdr64 = RpcFlags.HasNDR64 && UseClientRpcNDR64 && firstPacketSent;
	size_t size = sizeof(RPC_HEADER) + (useNdr64 ? sizeof(RPC_REQUEST64) : sizeof(RPC_REQUEST)) + requestSize;
	size_t responseSize2;

	*kmsResponse = NULL;

	BYTE *_Request = (BYTE*)vlmcsd_malloc(size);

	RequestHeader = (RPC_HEADER*)_Request;
	RpcRequest = (RPC_REQUEST64*)(_Request + sizeof(RPC_HEADER));

	createRpcHeader(RequestHeader, RPC_PT_REQUEST, (WORD)size);

	// Increment CallId for next Request
	CallId++;

	RpcRequest->Opnum = 0;

	if (useNdr64)
	{
		RpcRequest->ContextId = LE16(1); // We negotiate NDR64 always as context 1
		RpcRequest->AllocHint = LE32((DWORD)(requestSize + sizeof(RpcRequest->Ndr64)));
		RpcRequest->Ndr64.DataLength = LE64((uint64_t)requestSize);
		RpcRequest->Ndr64.DataSizeIs = LE64((uint64_t)requestSize);
		memcpy(RpcRequest->Ndr64.Data, kmsRequest, requestSize);
	}
	else
	{
		RpcRequest->ContextId = 0; // We negotiate NDR32 always as context 0
		RpcRequest->AllocHint = LE32((DWORD)(requestSize + sizeof(RpcRequest->Ndr)));
		RpcRequest->Ndr.DataLength = LE32((DWORD)requestSize);
		RpcRequest->Ndr.DataSizeIs = LE32((DWORD)requestSize);
		memcpy(RpcRequest->Ndr.Data, kmsRequest, requestSize);
	}

	for (;;)
	{
		int bytesread;

		if (!_send(sock, _Request, (int)size))
		{
			printerrorf("\nFatal: Could not send RPC request\n");
			status = RPC_S_COMM_FAILURE;
			break;
		}

		if (!_recv(sock, &ResponseHeader, sizeof(RPC_HEADER)))
		{
			printerrorf("\nFatal: No RPC response received from server\n");
			status = RPC_S_COMM_FAILURE;
			break;
		}

		if ((status = checkRpcResponseHeader(&ResponseHeader, RequestHeader, RPC_PT_RESPONSE, &printerrorf))) break;

		size = useNdr64 ? sizeof(RPC_RESPONSE64) : sizeof(RPC_RESPONSE);

		if (size > LE16(ResponseHeader.FragLength) - sizeof(ResponseHeader))
			size = LE16(ResponseHeader.FragLength) - sizeof(ResponseHeader);

		if (!_recv(sock, &_Response, (int)size))
		{
			printerrorf("\nFatal: RPC response is incomplete\n");
			status = RPC_S_COMM_FAILURE;
			break;
		}

		if (_Response.CancelCount != 0)
		{
			printerrorf("\nFatal: RPC response cancel count is not 0\n");
			status = RPC_S_CALL_CANCELLED;
			break;
		}

		if (_Response.ContextId != (useNdr64 ? LE16(1) : 0))
		{
			printerrorf("\nFatal: RPC response context id %u is not bound\n", (unsigned int)LE16(_Response.ContextId));
			status = RPC_X_SS_CONTEXT_DAMAGED;
			break;
		}

		int_fast8_t sizesMatch;

		if (useNdr64)
		{
			*responseSize = (size_t)LE64(_Response.Ndr64.DataLength);
			responseSize2 = (size_t)LE64(_Response.Ndr64.DataSizeIs);

			if (/*!*responseSize ||*/ !_Response.Ndr64.DataSizeMax)
			{
				status = (int)LE32(_Response.Ndr64.status);
				break;
			}

			sizesMatch = (size_t)LE64(_Response.Ndr64.DataLength) == responseSize2;
		}
		else
		{
			*responseSize = (size_t)LE32(_Response.Ndr.DataLength);
			responseSize2 = (size_t)LE32(_Response.Ndr.DataSizeIs);

			if (/*!*responseSize ||*/ !_Response.Ndr.DataSizeMax)
			{
				status = (int)LE32(_Response.Ndr.status);
				break;
			}

			sizesMatch = (size_t)LE32(_Response.Ndr.DataLength) == responseSize2;
		}

		if (!sizesMatch)
		{
			printerrorf("\nFatal: NDR data length (%u) does not match NDR data size (%u)\n",
				(uint32_t)*responseSize,
				(uint32_t)LE32(_Response.Ndr.DataSizeIs)
			);

			status = RPC_S_PROTOCOL_ERROR;
			break;
		}

		*kmsResponse = (BYTE*)vlmcsd_malloc(*responseSize + MAX_EXCESS_BYTES);

		// If RPC stub is too short, assume missing bytes are zero (same ill behavior as MS RPC)
		memset(*kmsResponse, 0, *responseSize + MAX_EXCESS_BYTES);

		// Read up to 16 bytes more than bytes expected to detect faulty KMS emulators
		if ((bytesread = recv(sock, (char*)*kmsResponse, (int)(*responseSize) + MAX_EXCESS_BYTES, 0)) < (int)*responseSize)
		{
			printerrorf("\nFatal: No or incomplete KMS response received. Required %u bytes but only got %i\n",
				(uint32_t)*responseSize,
				(int32_t)(bytesread < 0 ? 0 : bytesread)
			);

			status = RPC_S_PROTOCOL_ERROR;
			break;
		}

		DWORD *pReturnCode;

		const size_t len = *responseSize + (useNdr64 ? sizeof(_Response.Ndr64) : sizeof(_Response.Ndr)) + sizeof(*pReturnCode);
		const size_t pad = ((~len & 3) + 1) & 3;

		if (len + pad != LE32(_Response.AllocHint))
		{
			printerrorf("\nWarning: RPC stub size is %u, should be %u (probably incorrect padding)\n", (uint32_t)LE32(_Response.AllocHint), (uint32_t)(len + pad));
		}
		else
		{
			size_t i;
			for (i = 0; i < pad; i++)
			{
				if (*(*kmsResponse + *responseSize + sizeof(*pReturnCode) + i))
				{
					printerrorf("\nWarning: RPC stub data not padded to zeros according to Microsoft standard\n");
					break;
				}
			}
		}

		pReturnCode = (DWORD*)(*kmsResponse + *responseSize + pad);
		status = GET_UA32LE(pReturnCode);
		//status = LE32(UA32(pReturnCode));

		break;
	}

	free(_Request);
	firstPacketSent = TRUE;
	return status;
#undef MAX_EXCESS_BYTES
}


static int_fast8_t IsNullGuid(const BYTE* guidPtr)
{
	int_fast8_t i;

	for (i = 0; i < 16; i++)
	{
		if (guidPtr[i]) return FALSE;
	}

	return TRUE;
}

/*
 * Perform RPC client bind. Accepts a connected client socket.
 * Returns 0 on success. RPC binding is required before any payload can be
 * exchanged. It negotiates about protocol details.
 */
static RpcStatus rpcBindOrAlterClientContext(const RpcCtx sock, const BYTE packetType, const int_fast8_t verbose)
{
	RPC_HEADER *RequestHeader, ResponseHeader;
	RPC_BIND_REQUEST *bindRequest;
	RPC_BIND_RESPONSE *bindResponse;
	int status;
	const WORD ctxItems = 1 + (packetType == RPC_PT_BIND_REQ ? UseClientRpcNDR64 + UseClientRpcBTFN : 0);
	const size_t rpcBindSize = (sizeof(RPC_HEADER) + sizeof(RPC_BIND_REQUEST) + (ctxItems - 1) * sizeof(bindRequest->CtxItems[0]));
	WORD ctxIndex = 0;
	WORD i;
	WORD CtxBTFN = RPC_INVALID_CTX, CtxNDR64 = RPC_INVALID_CTX;
	BYTE* request = (BYTE*)alloca(rpcBindSize);

	RequestHeader = (RPC_HEADER*)request;
	bindRequest = (RPC_BIND_REQUEST*)(request + sizeof(RPC_HEADER));

	createRpcHeader(RequestHeader, packetType, (WORD)rpcBindSize);
	RequestHeader->PacketFlags |= UseMultiplexedRpc ? RPC_PF_MULTIPLEX : 0;

	bindRequest->AssocGroup = 0;
	bindRequest->MaxRecvFrag = bindRequest->MaxXmitFrag = LE16(5840);
	bindRequest->NumCtxItems = LE32(ctxItems);

	// data that is identical in all Ctx items
	for (i = 0; i < ctxItems; i++)
	{
		struct CtxItem* ctxItem = bindRequest->CtxItems + i;
		ctxItem->ContextId = LE16(i);
		ctxItem->InterfaceVerMajor = LE16(1);
		ctxItem->InterfaceVerMinor = 0;
		ctxItem->NumTransItems = LE16(1);
		ctxItem->SyntaxVersion = i ? LE32(1) : LE32(2);

		memcpy(&ctxItem->InterfaceUUID, InterfaceUuid, sizeof(GUID));
	}

	memcpy(&bindRequest->CtxItems[0].TransferSyntax, TransferSyntaxNDR32, sizeof(GUID));

	if (UseClientRpcNDR64 && packetType == RPC_PT_BIND_REQ)
	{
		memcpy(&bindRequest->CtxItems[++ctxIndex].TransferSyntax, TransferSyntaxNDR64, sizeof(GUID));
		CtxNDR64 = ctxIndex;
	}

	if (UseClientRpcBTFN && packetType == RPC_PT_BIND_REQ)
	{
		memcpy(&bindRequest->CtxItems[++ctxIndex].TransferSyntax, BindTimeFeatureNegotiation, sizeof(GUID));
		CtxBTFN = ctxIndex;
	}

	if (!_send(sock, request, (int)rpcBindSize))
	{
		printerrorf("\nFatal: Sending RPC bind request failed\n");
		return RPC_S_COMM_FAILURE;
	}

	if (!_recv(sock, &ResponseHeader, sizeof(RPC_HEADER)))
	{
		printerrorf("\nFatal: Did not receive a response from server\n");
		return RPC_S_COMM_FAILURE;
	}

	if ((status = checkRpcResponseHeader
	(
		&ResponseHeader,
		RequestHeader,
		packetType == RPC_PT_BIND_REQ ? RPC_PT_BIND_ACK : RPC_PT_ALTERCONTEXT_ACK,
		&printerrorf
	)))
	{
		return status;
	}

	bindResponse = (RPC_BIND_RESPONSE*)vlmcsd_malloc(LE16(ResponseHeader.FragLength) - sizeof(RPC_HEADER));
	BYTE* bindResponseBytePtr = (BYTE*)bindResponse;

	if (!_recv(sock, bindResponse, LE16(ResponseHeader.FragLength) - sizeof(RPC_HEADER)))
	{
		printerrorf("\nFatal: Incomplete RPC bind acknowledgement received\n");
		free(bindResponseBytePtr);
		return RPC_S_COMM_FAILURE;
	}

	/*
	 * checking, whether a bind or alter context response is as expected.
	 * This check is very strict and checks whether a KMS emulator behaves exactly the same way
	 * as Microsoft's RPC does.
	 */
	status = 0;

	if (bindResponse->SecondaryAddressLength < LE16(3))
		bindResponse = (RPC_BIND_RESPONSE*)(bindResponseBytePtr - 4);

	if (bindResponse->NumResults != bindRequest->NumCtxItems)
	{
		printerrorf("\nFatal: Expected %u CTX items but got %u\n",
			(uint32_t)LE32(bindRequest->NumCtxItems),
			(uint32_t)LE32(bindResponse->NumResults)
		);

		status = RPC_S_PROTOCOL_ERROR;
	}

	for (i = 0; i < ctxItems; i++)
	{
		const char* transferSyntaxName =
			i == CtxBTFN ? "BTFN" : i == CtxNDR64 ? "NDR64" : "NDR32";

		struct CtxResults* ctxResult = bindResponse->Results + i;
		struct CtxItem* ctxItem = bindRequest->CtxItems + i;
		if (ctxResult->AckResult == RPC_BIND_NACK) // transfer syntax was declined
		{
			if (!IsNullGuid((BYTE*)&ctxResult->TransferSyntax))
			{
				printerrorf(
					"\nWarning: Rejected transfer syntax %s did not return NULL Guid\n",
					transferSyntaxName
				);
			}

			if (ctxResult->SyntaxVersion)
			{
				printerrorf(
					"\nWarning: Rejected transfer syntax %s did not return syntax version 0 but %u\n",
					transferSyntaxName,
					LE32(ctxResult->SyntaxVersion)
				);
			}

			if (ctxResult->AckReason == RPC_ABSTRACTSYNTAX_UNSUPPORTED)
			{
				printerrorf(
					"\nWarning: Transfer syntax %s does not support KMS activation\n",
					transferSyntaxName
				);
			}
			else if (ctxResult->AckReason != RPC_SYNTAX_UNSUPPORTED)
			{
				printerrorf(
					"\nWarning: Rejected transfer syntax %s did not return ack reason RPC_SYNTAX_UNSUPPORTED\n",
					transferSyntaxName
				);
			}

			continue;
		}

		if (i == CtxBTFN) // BTFN
		{
			if (ctxResult->AckResult != RPC_BIND_ACK)
			{
				printerrorf("\nWarning: BTFN did not respond with RPC_BIND_ACK or RPC_BIND_NACK\n");
			}

			if (ctxResult->AckReason != LE16(3))
			{
				printerrorf("\nWarning: BTFN did not return expected feature mask 0x3 but 0x%X\n", (unsigned int)LE16(ctxResult->AckReason));
			}

			if (verbose) printf("... BTFN ");
			RpcFlags.HasBTFN = TRUE;

			continue;
		}

		// NDR32 or NDR64 Ctx
		if (ctxResult->AckResult != RPC_BIND_ACCEPT)
		{
			printerrorf(
				"\nFatal: transfer syntax %s returned an invalid status, neither RPC_BIND_ACCEPT nor RPC_BIND_NACK\n",
				transferSyntaxName
			);

			status = RPC_S_PROTOCOL_ERROR;
		}

		if (!IsEqualGUID(&ctxResult->TransferSyntax, &ctxItem->TransferSyntax))
		{
			printerrorf(
				"\nFatal: Transfer syntax of RPC bind request and response does not match\n"
			);

			status = RPC_S_UNSUPPORTED_TRANS_SYN;
		}

		if (ctxResult->SyntaxVersion != ctxItem->SyntaxVersion)
		{
			printerrorf("\nFatal: Expected transfer syntax version %u for %s but got %u\n",
				(uint32_t)LE32(ctxItem->SyntaxVersion),
				transferSyntaxName,
				(uint32_t)LE32(ctxResult->SyntaxVersion)
			);

			status = RPC_S_UNSUPPORTED_TRANS_SYN;
		}

		// The ack reason field is actually undefined here but Microsoft sets this to 0
		if (ctxResult->AckReason != 0)
		{
			printerrorf(
				"\nWarning: Ack reason should be 0 but is %u\n",
				LE16(ctxResult->AckReason)
			);
		}

		if (!status)
		{
			if (i == CtxNDR64)
			{
				RpcFlags.HasNDR64 = TRUE;
				if (verbose) printf("... NDR64 ");
			}
			if (!i)
			{
				RpcFlags.HasNDR32 = TRUE;
				if (verbose) printf("... NDR32 ");
			}

		}
	}

	free(bindResponseBytePtr);

	if (!RpcFlags.HasNDR64 && !RpcFlags.HasNDR32)
	{
		printerrorf("\nFatal: Could neither negotiate NDR32 nor NDR64 with the RPC server\n");
		status = RPC_S_NO_PROTSEQS;
	}

	return status;
}

RpcStatus rpcBindClient(const RpcCtx sock, const int_fast8_t verbose, PRpcDiag_t rpcDiag)
{
	firstPacketSent = FALSE;
	RpcFlags.mask = 0;

	RpcStatus status =
		rpcBindOrAlterClientContext(sock, RPC_PT_BIND_REQ, verbose);

	if (status) goto end;

	if (!RpcFlags.HasNDR32)
		status = rpcBindOrAlterClientContext(sock, RPC_PT_ALTERCONTEXT_REQ, verbose);

end:
	rpcDiag->HasRpcDiag = TRUE;
	rpcDiag->HasNDR64 = !!RpcFlags.HasNDR64;
	rpcDiag->HasBTFN = !!RpcFlags.HasBTFN;
	return status;
}

#endif // USE_MSRPC
