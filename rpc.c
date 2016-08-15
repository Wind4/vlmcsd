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
#include <ctype.h>
#include <time.h>
#if !defined(_WIN32)
#include <sys/socket.h>
#include <netdb.h>
#endif
#include "rpc.h"
#include "output.h"
#include "crypto.h"
#include "endian.h"
#include "helpers.h"
#include "network.h"
#include "shared_globals.h"

/* Forwards */

static int checkRpcHeader(const RPC_HEADER *const Header, const BYTE desiredPacketType, const PRINTFUNC p);


/* Data definitions */

// All GUIDs are defined as BYTE[16] here. No big-endian/little-endian byteswapping required.
static const BYTE TransferSyntaxNDR32[] = {
	0x04, 0x5D, 0x88, 0x8A, 0xEB, 0x1C, 0xC9, 0x11, 0x9F, 0xE8, 0x08, 0x00, 0x2B, 0x10, 0x48, 0x60
};

static const BYTE InterfaceUuid[] = {
	0x75, 0x21, 0xc8, 0x51, 0x4e, 0x84, 0x50, 0x47, 0xB0, 0xD8, 0xEC, 0x25, 0x55, 0x55, 0xBC, 0x06
};

static const BYTE TransferSyntaxNDR64[] = {
	0x33, 0x05, 0x71, 0x71, 0xba, 0xbe, 0x37, 0x49, 0x83, 0x19, 0xb5, 0xdb, 0xef, 0x9c, 0xcc, 0x36
};

static const BYTE BindTimeFeatureNegotiation[] = {
	0x2c, 0x1c, 0xb7, 0x6c, 0x12, 0x98, 0x40, 0x45, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


//
// Dispatch RPC payload to kms.c
//
typedef int (*CreateResponse_t)(const void *const, void *const, const char* const);

static const struct {
	unsigned int  RequestSize;
	CreateResponse_t CreateResponse;
} _Versions[] = {
	{ sizeof(REQUEST_V4), (CreateResponse_t) CreateResponseV4 },
	{ sizeof(REQUEST_V6), (CreateResponse_t) CreateResponseV6 },
	{ sizeof(REQUEST_V6), (CreateResponse_t) CreateResponseV6 }
};

RPC_FLAGS RpcFlags;
static int_fast8_t firstPacketSent;

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
		kmsMajorVersion = LE16(((WORD*)Request->Ndr.Data)[1]);
	else
		kmsMajorVersion = LE16(((WORD*)Request->Ndr64.Data)[1]);

	if (kmsMajorVersion > 6)
	{
		logger("Fatal: KMSv%u is not supported.\n", (unsigned int)kmsMajorVersion);
	}
	else
	{
		if (len >_Versions[kmsMajorVersion].RequestSize + requestSize)
			logger("Warning: %u excess bytes in RPC request.\n",
					len - _Versions[kmsMajorVersion].RequestSize
			);
	}

	if (Ctx != *Ndr64Ctx && Ctx != *NdrCtx)
		logger("Warning: Context id should be %u (NDR32) or %u (NDR64) but is %u.\n",
				(unsigned int)*NdrCtx,
				(unsigned int)*Ndr64Ctx,
				Ctx
		);

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

	if (Ctx != *Ndr64Ctx)
	{
		version = LE32(*(DWORD*)Request->Ndr.Data);
	}
	else
	{
		version = LE32(*(DWORD*)Request->Ndr64.Data);
	}

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
	WORD Ctx = LE16(Request->ContextId);
	BYTE* requestData;
	BYTE* responseData;
	BYTE* pRpcReturnCode;
	int len;

	if (Ctx != *Ndr64Ctx)
	{
		requestData = (BYTE*)&Request->Ndr.Data;
		responseData = (BYTE*)&Response->Ndr.Data;
	}
	else
	{
		requestData = (BYTE*)&Request->Ndr64.Data;
		responseData = (BYTE*)&Response->Ndr64.Data;
	}

	ResponseSize = 0x8007000D; // Invalid Data

	if (isValid)
	{
		uint16_t majorIndex = LE16(((WORD*)requestData)[1]) - 4;
		if (!(ResponseSize = _Versions[majorIndex].CreateResponse(requestData, responseData, ipstr))) ResponseSize = 0x8007000D;
	}

	if (Ctx != *Ndr64Ctx)
	{
		if (ResponseSize < 0)
		{
			Response->Ndr.DataSizeMax = Response->Ndr.DataLength = 0;
			len = sizeof(Response->Ndr) - sizeof(Response->Ndr.DataSizeIs);
		}
		else
		{
			Response->Ndr.DataSizeMax = LE32(0x00020000);
			Response->Ndr.DataLength  =	Response->Ndr.DataSizeIs = LE32(ResponseSize);
			len = ResponseSize + sizeof(Response->Ndr);
		}
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

	pRpcReturnCode = ((BYTE*)&Response->Ndr) + len;
	UA32(pRpcReturnCode) = ResponseSize < 0 ? LE32(ResponseSize) : 0;
	len += sizeof(DWORD);

	// Pad zeros to 32-bit align (seems not neccassary but Windows RPC does it this way)
	int pad = ((~len & 3) + 1) & 3;
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

	uint32_t CapCtxItems =	(len - sizeof(*Request) + sizeof(Request->CtxItems)) / sizeof(Request->CtxItems);
	DWORD NumCtxItems = LE32(Request->NumCtxItems);

	if (NumCtxItems < CapCtxItems) // Can't be too small because already handled by RpcBindSize
		logger("Warning: Excess bytes in RPC bind request.\n");

	for (i = 0; i < NumCtxItems; i++)
	{
		if (!IsEqualGUID(&Request->CtxItems[i].InterfaceUUID, InterfaceUuid))
		{
			uuid2StringLE((GUID*)&Request->CtxItems[i].InterfaceUUID, guidBuffer1);
			uuid2StringLE((GUID*)InterfaceUuid, guidBuffer2);
			logger("Warning: Interface UUID is %s but should be %s in Ctx item %u.\n", guidBuffer1, guidBuffer2, (unsigned int)i);
		}

		if (Request->CtxItems[i].NumTransItems != LE16(1))
			logger("Fatal: %u NDR32 transfer items detected in Ctx item %u, but only one is supported.\n",
					(unsigned int)LE16(Request->CtxItems[i].NumTransItems), (unsigned int)i
			);

		if (Request->CtxItems[i].InterfaceVerMajor != LE16(1) || Request->CtxItems[i].InterfaceVerMinor != 0)
			logger("Warning: NDR32 Interface version is %u.%u but should be 1.0.\n",
					(unsigned int)LE16(Request->CtxItems[i].InterfaceVerMajor),
					(unsigned int)LE16(Request->CtxItems[i].InterfaceVerMinor)
			);

		if (Request->CtxItems[i].ContextId != LE16((WORD)i))
			logger("Warning: context id of Ctx item %u is %u.\n", (unsigned int)i, (unsigned int)Request->CtxItems[i].ContextId);

		if ( IsEqualGUID((GUID*)TransferSyntaxNDR32, &Request->CtxItems[i].TransferSyntax) )
		{
			HasTransferSyntaxNDR32 = TRUE;

			if (Request->CtxItems[i].SyntaxVersion != LE32(2))
				logger("NDR32 transfer syntax version is %u but should be 2.\n", LE32(Request->CtxItems[i].SyntaxVersion));
		}
		else if ( IsEqualGUID((GUID*)TransferSyntaxNDR64, &Request->CtxItems[i].TransferSyntax) )
		{
			if (Request->CtxItems[i].SyntaxVersion != LE32(1))
				logger("NDR64 transfer syntax version is %u but should be 1.\n", LE32(Request->CtxItems[i].SyntaxVersion));
		}
		else if (!memcmp(BindTimeFeatureNegotiation, (BYTE*)(&Request->CtxItems[i].TransferSyntax), 8))
		{
			if (Request->CtxItems[i].SyntaxVersion != LE32(1))
				logger("BTFN syntax version is %u but should be 1.\n", LE32(Request->CtxItems[i].SyntaxVersion));
		}
	}

	if (!HasTransferSyntaxNDR32)
		logger("Warning: RPC bind request has no NDR32 CtxItem.\n");
}
#endif // defined(_PEDANTIC) && !defined(NO_LOG)


/*
 * Check, if we receive enough bytes to return a valid RPC bind response
 */
static unsigned int checkRpcBindSize(const RPC_BIND_REQUEST *const Request, const unsigned int RequestSize, WORD* NdrCtx, WORD* Ndr64Ctx)
{
	if ( RequestSize < sizeof(RPC_BIND_REQUEST) ) return FALSE;

	unsigned int _NumCtxItems = LE32(Request->NumCtxItems);

	if ( RequestSize < sizeof(RPC_BIND_REQUEST) - sizeof(Request->CtxItems[0]) + _NumCtxItems * sizeof(Request->CtxItems[0]) ) return FALSE;

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
	unsigned int  i, _st = FALSE;
	DWORD numCtxItems = LE32(Request->NumCtxItems);
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
		portNumberSize = Response->SecondaryAddressLength = 0;
	}
	else
	{
		portNumberSize = strlen((char*)Response->SecondaryAddress) + 1;
		Response->SecondaryAddressLength = LE16(portNumberSize);
	}

	Response->MaxXmitFrag = Request->MaxXmitFrag;
	Response->MaxRecvFrag = Request->MaxRecvFrag;
	Response->AssocGroup  = LE32(RpcAssocGroup);

	// This is really ugly (but efficient) code to support padding after the secondary address field
	if (portNumberSize < 3)
	{
		Response = (RPC_BIND_RESPONSE*)((BYTE*)Response - 4);
	}

	Response->NumResults = Request->NumCtxItems;

	if (UseRpcNDR64)
	{
		for (i = 0; i < numCtxItems; i++)
		{
			if ( IsEqualGUID((GUID*)TransferSyntaxNDR32, &Request->CtxItems[i].TransferSyntax) )
			{
				/*if (packetType == RPC_PT_BIND_REQ)*/
					*NdrCtx = LE16(Request->CtxItems[i].ContextId);
			}

			if ( IsEqualGUID((GUID*)TransferSyntaxNDR64, &Request->CtxItems[i].TransferSyntax) )
			{
				IsNDR64possible = TRUE;

				/*if (packetType == RPC_PT_BIND_REQ)*/
					*Ndr64Ctx = LE16(Request->CtxItems[i].ContextId);
			}
		}
	}

	for (i = 0; i < numCtxItems; i++)
	{
		memset(&Response->Results[i].TransferSyntax, 0, sizeof(GUID));

		if ( !IsNDR64possible && IsEqualGUID((GUID*)TransferSyntaxNDR32, &Request->CtxItems[i].TransferSyntax) )
		{
			Response->Results[i].SyntaxVersion = LE32(2);
			Response->Results[i].AckResult =
			Response->Results[i].AckReason = RPC_BIND_ACCEPT;
			memcpy(&Response->Results[i].TransferSyntax, TransferSyntaxNDR32, sizeof(GUID));

			_st = TRUE;
		}
		else if ( IsNDR64possible && IsEqualGUID((GUID*)TransferSyntaxNDR64, &Request->CtxItems[i].TransferSyntax) )
		{
			Response->Results[i].SyntaxVersion = LE32(1);
			Response->Results[i].AckResult =
			Response->Results[i].AckReason = RPC_BIND_ACCEPT;
			memcpy(&Response->Results[i].TransferSyntax, TransferSyntaxNDR64, sizeof(GUID));

			_st = TRUE;
		}
		else if ( UseRpcBTFN && !memcmp(BindTimeFeatureNegotiation, (BYTE*)(&Request->CtxItems[i].TransferSyntax), 8) )
		{
			Response->Results[i].SyntaxVersion = 0;
			Response->Results[i].AckResult = RPC_BIND_ACK;

			// Features requested are actually encoded in the GUID
			Response->Results[i].AckReason =
					((WORD*)(&Request->CtxItems[i].TransferSyntax))[4] &
					(RPC_BTFN_SEC_CONTEXT_MULTIPLEX | RPC_BTFN_KEEP_ORPHAN);
		}
		else
		{
			Response->Results[i].SyntaxVersion = 0;
			Response->Results[i].AckResult =
			Response->Results[i].AckReason = RPC_BIND_NACK; // Unsupported
		}
	}

	if ( !_st ) return 0;

	return sizeof(RPC_BIND_RESPONSE) + numCtxItems * sizeof(((RPC_BIND_RESPONSE *)0)->Results[0]) - (portNumberSize < 3 ? 4 : 0);
}


//
// Main RPC handling routine
//
typedef unsigned int (*GetResponseSize_t)(const void *const request, const unsigned int requestSize, WORD* NdrCtx, WORD* Ndr64Ctx);
typedef int (*GetResponse_t)(const void* const request, void* response, const DWORD rpcAssocGroup, const SOCKET socket, WORD* NdrCtx, WORD* Ndr64Ctx, BYTE packetType, const char* const ipstr);

static const struct {
	BYTE  ResponsePacketType;
	GetResponseSize_t CheckRequestSize;
	GetResponse_t GetResponse;
}
_Actions[] = {
	{ RPC_PT_BIND_ACK,         (GetResponseSize_t)checkRpcBindSize,    (GetResponse_t) rpcBind    },
	{ RPC_PT_RESPONSE,         (GetResponseSize_t)checkRpcRequestSize, (GetResponse_t) rpcRequest },
	{ RPC_PT_ALTERCONTEXT_ACK, (GetResponseSize_t)checkRpcBindSize,    (GetResponse_t) rpcBind    },
};


/*
 * This is the main RPC server loop. Returns after KMS request has been serviced
 * or a timeout has occured.
 */
void rpcServer(const SOCKET sock, const DWORD RpcAssocGroup, const char* const ipstr)
{
	RPC_HEADER  rpcRequestHeader;
	WORD NdrCtx = INVALID_NDR_CTX, Ndr64Ctx = INVALID_NDR_CTX;

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
		RPC_RESPONSE* rpcResponse     = (RPC_RESPONSE*)(responseBuffer + sizeof(rpcRequestHeader));

		// The request is larger than the buffer size
		if (request_len > MAX_REQUEST_SIZE + sizeof(RPC_REQUEST64)) return;

		// Unable to receive the complete request
		if (!_recv(sock, requestBuffer, request_len)) return;

		// Request is invalid
		BYTE isValid = _Actions[_a].CheckRequestSize(requestBuffer, request_len, &NdrCtx, &Ndr64Ctx);
		if (rpcRequestHeader.PacketType != RPC_PT_REQUEST && !isValid) return;

		// Unable to create a valid response from request
		if (!(response_len = _Actions[_a].GetResponse(requestBuffer, rpcResponse, RpcAssocGroup, sock, &NdrCtx, &Ndr64Ctx, rpcRequestHeader.PacketType != RPC_PT_REQUEST ? rpcRequestHeader.PacketType : isValid, ipstr))) return;

		response_len += sizeof(RPC_HEADER);

		memcpy(rpcResponseHeader, &rpcRequestHeader, sizeof(RPC_HEADER));

		rpcResponseHeader->FragLength = LE16(response_len);
		rpcResponseHeader->PacketType = _Actions[_a].ResponsePacketType;

		if (rpcResponseHeader->PacketType == RPC_PT_ALTERCONTEXT_ACK)
			rpcResponseHeader->PacketFlags = RPC_PF_FIRST | RPC_PF_LAST;

		if (!_send(sock, responseBuffer, response_len)) return;

		if (DisconnectImmediately && rpcResponseHeader->PacketType == RPC_PT_RESPONSE)
			shutdown(sock, VLMCSD_SHUT_RDWR);
	}
}


/* RPC client functions */

static DWORD CallId = 2; // M$ starts with CallId 2. So we do the same.


/*
 * Checks RPC header. Returns 0 on success.
 * This is mainly for debugging a non Microsoft KMS server that uses its own RPC code.
 */
static int checkRpcHeader(const RPC_HEADER *const Header, const BYTE desiredPacketType, const PRINTFUNC p)
{
	int status = 0;

	if (Header->PacketType != desiredPacketType)
	{
		p("Fatal: Received wrong RPC packet type. Expected %u but got %u\n",
				(uint32_t)desiredPacketType,
				Header->PacketType
		);
		status = RPC_S_PROTOCOL_ERROR;
	}

	if (Header->DataRepresentation != BE32(0x10000000))
	{
		p("Fatal: RPC response does not conform to Microsoft's limited support of DCE RPC\n");
		status = RPC_S_PROTOCOL_ERROR;
	}

	if (Header->AuthLength != 0)
	{
		p("Fatal: RPC response requests authentication\n");
		status = RPC_S_UNKNOWN_AUTHN_TYPE;
	}

	// vlmcsd does not support fragmented packets (not yet neccassary)
	if ( (Header->PacketFlags & (RPC_PF_FIRST | RPC_PF_LAST)) != (RPC_PF_FIRST | RPC_PF_LAST) )
	{
		p("Fatal: RPC packet flags RPC_PF_FIRST and RPC_PF_LAST are not both set.\n");
		status = RPC_S_CANNOT_SUPPORT;
	}

	if (Header->PacketFlags & RPC_PF_CANCEL_PENDING)	p("Warning: %s should not be set\n", "RPC_PF_CANCEL_PENDING");
	if (Header->PacketFlags & RPC_PF_RESERVED)			p("Warning: %s should not be set\n", "RPC_PF_RESERVED");
	if (Header->PacketFlags & RPC_PF_NOT_EXEC)			p("Warning: %s should not be set\n", "RPC_PF_NOT_EXEC");
	if (Header->PacketFlags & RPC_PF_MAYBE)				p("Warning: %s should not be set\n", "RPC_PF_MAYBE");
	if (Header->PacketFlags & RPC_PF_OBJECT)			p("Warning: %s should not be set\n", "RPC_PF_OBJECT");

	if (Header->VersionMajor != 5 || Header->VersionMinor != 0)
	{
		p("Fatal: Expected RPC version 5.0 and got %u.%u\n", Header->VersionMajor, Header->VersionMinor);
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
 * Initializes an RPC request header as needed for KMS, i.e. packet always fits in one fragment.
 * size cannot be greater than fragment length negotiated during RPC bind.
 */
static void createRpcRequestHeader(RPC_HEADER* RequestHeader, BYTE packetType, WORD size)
{
	RequestHeader->PacketType 			= packetType;
	RequestHeader->PacketFlags 			= RPC_PF_FIRST | RPC_PF_LAST;
	RequestHeader->VersionMajor 		= 5;
	RequestHeader->VersionMinor			= 0;
	RequestHeader->AuthLength			= 0;
	RequestHeader->DataRepresentation	= BE32(0x10000000); // Little endian, ASCII charset, IEEE floating point
	RequestHeader->CallId				= LE32(CallId);
	RequestHeader->FragLength			= LE16(size);
}


/*
 * Sends a KMS request via RPC and receives a response.
 * Parameters are raw (encrypted) reqeuests / responses.
 * Returns 0 on success.
 */
RpcStatus rpcSendRequest(const RpcCtx sock, const BYTE *const KmsRequest, const size_t requestSize, BYTE **KmsResponse, size_t *const responseSize)
{
	#define MAX_EXCESS_BYTES 16
	RPC_HEADER *RequestHeader, ResponseHeader;
	RPC_REQUEST64 *RpcRequest;
	RPC_RESPONSE64 _Response;
	int status = 0;
	int_fast8_t useNdr64 = UseRpcNDR64 && firstPacketSent;
	size_t size = sizeof(RPC_HEADER) + (useNdr64 ? sizeof(RPC_REQUEST64) : sizeof(RPC_REQUEST)) + requestSize;
	size_t responseSize2;

	*KmsResponse = NULL;

	BYTE *_Request = (BYTE*)vlmcsd_malloc(size);

	RequestHeader = (RPC_HEADER*)_Request;
	RpcRequest = (RPC_REQUEST64*)(_Request + sizeof(RPC_HEADER));

	createRpcRequestHeader(RequestHeader, RPC_PT_REQUEST, size);

	// Increment CallId for next Request
	CallId++;

	RpcRequest->Opnum = 0;

	if (useNdr64)
	{
		RpcRequest->ContextId = LE16(1); // We negotiate NDR64 always as context 1
		RpcRequest->AllocHint = LE32(requestSize + sizeof(RpcRequest->Ndr64));
		RpcRequest->Ndr64.DataLength = LE64((uint64_t)requestSize);
		RpcRequest->Ndr64.DataSizeIs = LE64((uint64_t)requestSize);
		memcpy(RpcRequest->Ndr64.Data, KmsRequest, requestSize);
	}
	else
	{
		RpcRequest->ContextId = 0; // We negotiate NDR32 always as context 0
		RpcRequest->AllocHint = LE32(requestSize + sizeof(RpcRequest->Ndr));
		RpcRequest->Ndr.DataLength = LE32(requestSize);
		RpcRequest->Ndr.DataSizeIs = LE32(requestSize);
		memcpy(RpcRequest->Ndr.Data, KmsRequest, requestSize);
	}

	for(;;)
	{
		int bytesread;

		if (!_send(sock, _Request, size))
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

		if (!_recv(sock, &_Response, size))
		{
			printerrorf("\nFatal: RPC response is incomplete\n");
			status = RPC_S_COMM_FAILURE;
			break;
		}

		if (_Response.CancelCount != 0)
		{
			printerrorf("\nFatal: RPC response cancel count is not 0\n");
			status = RPC_S_CALL_CANCELLED;
		}

		if (_Response.ContextId != (useNdr64 ? LE16(1) : 0))
		{
			printerrorf("\nFatal: RPC response context id %u is not bound\n", (unsigned int)LE16(_Response.ContextId));
			status = RPC_X_SS_CONTEXT_DAMAGED;
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
		}

		*KmsResponse = (BYTE*)vlmcsd_malloc(*responseSize + MAX_EXCESS_BYTES);

		// If RPC stub is too short, assume missing bytes are zero (same ill behavior as MS RPC)
		memset(*KmsResponse, 0, *responseSize + MAX_EXCESS_BYTES);

		// Read up to 16 bytes more than bytes expected to detect faulty KMS emulators
		if ((bytesread = recv(sock, (char*)*KmsResponse, *responseSize + MAX_EXCESS_BYTES, 0)) < (int)*responseSize)
		{
			printerrorf("\nFatal: No or incomplete KMS response received. Required %u bytes but only got %i\n",
					(uint32_t)*responseSize,
					(int32_t)(bytesread < 0 ? 0 : bytesread)
			);

			status = RPC_S_PROTOCOL_ERROR;
			break;
		}

		DWORD *pReturnCode;

		size_t len = *responseSize + (useNdr64 ? sizeof(_Response.Ndr64) : sizeof(_Response.Ndr)) + sizeof(*pReturnCode);
		size_t pad = ((~len & 3) + 1) & 3;

		if (len + pad != LE32(_Response.AllocHint))
		{
			printerrorf("\nWarning: RPC stub size is %u, should be %u (probably incorrect padding)\n", (uint32_t)LE32(_Response.AllocHint), (uint32_t)(len + pad));
		}
		else
		{
			size_t i;
			for (i = 0; i < pad; i++)
			{
				if (*(*KmsResponse + *responseSize + sizeof(*pReturnCode) + i))
				{
					printerrorf("\nWarning: RPC stub data not padded to zeros according to Microsoft standard\n");
					break;
				}
			}
		}

		pReturnCode = (DWORD*)(*KmsResponse + *responseSize + pad);
		status = LE32(UA32(pReturnCode));

		break;
	}

	free(_Request);
	firstPacketSent = TRUE;
	return status;
	#undef MAX_EXCESS_BYTES
}


static int_fast8_t IsNullGuid(BYTE* guidPtr)
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
RpcStatus rpcBindOrAlterClientContext(const RpcCtx sock, BYTE packetType, const int_fast8_t verbose)
{
	RPC_HEADER *RequestHeader, ResponseHeader;
	RPC_BIND_REQUEST *bindRequest;
	RPC_BIND_RESPONSE *bindResponse;
	int status;
	WORD ctxItems = 1 + (packetType == RPC_PT_BIND_REQ ? UseRpcNDR64 + UseRpcBTFN : 0);
	size_t rpcBindSize = (sizeof(RPC_HEADER) + sizeof(RPC_BIND_REQUEST) + (ctxItems - 1) * sizeof(bindRequest->CtxItems[0]));
	WORD ctxIndex = 0;
	WORD i;
	WORD CtxBTFN = (WORD)~0, CtxNDR64 = (WORD)~0;
	BYTE _Request[rpcBindSize];

	RequestHeader = (RPC_HEADER*)_Request;
	bindRequest = (RPC_BIND_REQUEST* )(_Request + sizeof(RPC_HEADER));

	createRpcRequestHeader(RequestHeader, packetType, rpcBindSize);
	RequestHeader->PacketFlags |=  UseMultiplexedRpc ? RPC_PF_MULTIPLEX : 0;

	bindRequest->AssocGroup		= 0;
	bindRequest->MaxRecvFrag	= bindRequest->MaxXmitFrag = LE16(5840);
	bindRequest->NumCtxItems	= LE32(ctxItems);

	// data that is identical in all Ctx items
	for (i = 0; i < ctxItems; i++)
	{
		bindRequest->CtxItems[i].ContextId         = LE16(i);
		bindRequest->CtxItems[i].InterfaceVerMajor = LE16(1);
		bindRequest->CtxItems[i].InterfaceVerMinor = 0;
		bindRequest->CtxItems[i].NumTransItems     = LE16(1);
		bindRequest->CtxItems[i].SyntaxVersion     = i ? LE32(1) : LE32(2);

		memcpy(&bindRequest->CtxItems[i].InterfaceUUID, InterfaceUuid, sizeof(GUID));
	}

	memcpy(&bindRequest->CtxItems[0].TransferSyntax, TransferSyntaxNDR32, sizeof(GUID));

	if (UseRpcNDR64 && packetType == RPC_PT_BIND_REQ)
	{
		memcpy(&bindRequest->CtxItems[++ctxIndex].TransferSyntax, TransferSyntaxNDR64, sizeof(GUID));
		CtxNDR64 = ctxIndex;
	}

	if (UseRpcBTFN && packetType == RPC_PT_BIND_REQ)
	{
		memcpy(&bindRequest->CtxItems[++ctxIndex].TransferSyntax, BindTimeFeatureNegotiation, sizeof(GUID));
		CtxBTFN = ctxIndex;
	}

	if (!_send(sock, _Request, rpcBindSize))
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
	else
	{
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
					i == CtxBTFN ? "BTFN" :  i == CtxNDR64 ? "NDR64" : "NDR32";

			if (bindResponse->Results[i].AckResult == RPC_BIND_NACK) // transfer syntax was declined
			{
				if (!IsNullGuid((BYTE*)&bindResponse->Results[i].TransferSyntax))
				{
					printerrorf(
						"\nWarning: Rejected transfer syntax %s did not return NULL Guid\n",
						transferSyntaxName
					);
				}

				if (bindResponse->Results[i].SyntaxVersion)
				{
					printerrorf(
						"\nWarning: Rejected transfer syntax %s did not return syntax version 0 but %u\n",
						transferSyntaxName,
						LE32(bindResponse->Results[i].SyntaxVersion)
					);
				}

				if (bindResponse->Results[i].AckReason == RPC_ABSTRACTSYNTAX_UNSUPPORTED)
				{
					printerrorf(
						"\nWarning: Transfer syntax %s does not support KMS activation\n",
						transferSyntaxName
					);
				}
				else if (bindResponse->Results[i].AckReason != RPC_SYNTAX_UNSUPPORTED)
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
				if (bindResponse->Results[i].AckResult != RPC_BIND_ACK)
				{
					printerrorf("\nWarning: BTFN did not respond with RPC_BIND_ACK or RPC_BIND_NACK\n");
				}

				if (bindResponse->Results[i].AckReason != LE16(3))
				{
					printerrorf("\nWarning: BTFN did not return expected feature mask 0x3 but 0x%X\n", (unsigned int)LE16(bindResponse->Results[i].AckReason));
				}

				if (verbose) printf("... BTFN ");
				RpcFlags.HasBTFN = TRUE;

				continue;
			}

			// NDR32 or NDR64 Ctx
			if (bindResponse->Results[i].AckResult != RPC_BIND_ACCEPT)
			{
				printerrorf(
					"\nFatal: transfer syntax %s returned an invalid status, neither RPC_BIND_ACCEPT nor RPC_BIND_NACK\n",
					transferSyntaxName
				);

				status = RPC_S_PROTOCOL_ERROR;
			}

			if (!IsEqualGUID(&bindResponse->Results[i].TransferSyntax, &bindRequest->CtxItems[i].TransferSyntax))
			{
				printerrorf(
					"\nFatal: Transfer syntax of RPC bind request and response does not match\n"
				);

				status = RPC_S_UNSUPPORTED_TRANS_SYN;
			}

			if (bindResponse->Results[i].SyntaxVersion != bindRequest->CtxItems[i].SyntaxVersion)
			{
				printerrorf("\nFatal: Expected transfer syntax version %u for %s but got %u\n",
						(uint32_t)LE32(bindRequest->CtxItems[0].SyntaxVersion),
						transferSyntaxName,
						(uint32_t)LE32(bindResponse->Results[0].SyntaxVersion)
				);

				status = RPC_S_UNSUPPORTED_TRANS_SYN;
			}

			// The ack reason field is actually undefined here but Microsoft sets this to 0
			if (bindResponse->Results[i].AckReason != 0)
			{
				printerrorf(
					"\nWarning: Ack reason should be 0 but is %u\n",
					LE16(bindResponse->Results[i].AckReason)
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
	}

	free(bindResponseBytePtr);

	if (!RpcFlags.HasNDR64 && !RpcFlags.HasNDR32)
	{
		printerrorf("\nFatal: Could neither negotiate NDR32 nor NDR64 with the RPC server\n");
		status = RPC_S_NO_PROTSEQS;
	}

	return status;
}

RpcStatus rpcBindClient(const RpcCtx sock, const int_fast8_t verbose)
{
	firstPacketSent = FALSE;
	RpcFlags.mask = 0;

	RpcStatus status =
		rpcBindOrAlterClientContext(sock, RPC_PT_BIND_REQ, verbose);

	if (status) return status;

	if (!RpcFlags.HasNDR32)
		status = rpcBindOrAlterClientContext(sock, RPC_PT_ALTERCONTEXT_REQ, verbose);

	return status;
}

#endif // USE_MSRPC
