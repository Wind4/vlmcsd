#ifndef __rpc_h
#define __rpc_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "types.h"
#include "shared_globals.h"

#if !defined(_WIN32) && !defined(__CYGWIN__)
#define RPC_S_OK 0
#define RPC_S_INVALID_ARG 87
#define RPC_S_OUT_OF_MEMORY 14
#define RPC_S_OUT_OF_THREADS 164
#define RPC_S_INVALID_LEVEL RPC_S_INVALID_ARG
#define RPC_S_BUFFER_TOO_SMALL 122
#define RPC_S_INVALID_SECURITY_DESC 1338
#define RPC_S_ACCESS_DENIED 5
#define RPC_S_SERVER_OUT_OF_MEMORY 1130
#define RPC_S_ASYNC_CALL_PENDING 997
#define RPC_S_UNKNOWN_PRINCIPAL 1332
#define RPC_S_TIMEOUT 1460
#define RPC_S_INVALID_STRING_BINDING 1700
#define RPC_S_WRONG_KIND_OF_BINDING 1701
#define RPC_S_INVALID_BINDING 1702
#define RPC_S_PROTSEQ_NOT_SUPPORTED 1703
#define RPC_S_INVALID_RPC_PROTSEQ 1704
#define RPC_S_INVALID_STRING_UUID 1705
#define RPC_S_INVALID_ENDPOINT_FORMAT 1706
#define RPC_S_INVALID_NET_ADDR 1707
#define RPC_S_NO_ENDPOINT_FOUND 1708
#define RPC_S_INVALID_TIMEOUT 1709
#define RPC_S_OBJECT_NOT_FOUND 1710
#define RPC_S_ALREADY_REGISTERED 1711
#define RPC_S_TYPE_ALREADY_REGISTERED 1712
#define RPC_S_ALREADY_LISTENING 1713
#define RPC_S_NO_PROTSEQS_REGISTERED 1714
#define RPC_S_NOT_LISTENING 1715
#define RPC_S_UNKNOWN_MGR_TYPE 1716
#define RPC_S_UNKNOWN_IF 1717
#define RPC_S_NO_BINDINGS 1718
#define RPC_S_NO_PROTSEQS 1719
#define RPC_S_CANT_CREATE_ENDPOINT 1720
#define RPC_S_OUT_OF_RESOURCES 1721
#define RPC_S_SERVER_UNAVAILABLE 1722
#define RPC_S_SERVER_TOO_BUSY 1723
#define RPC_S_INVALID_NETWORK_OPTIONS 1724
#define RPC_S_NO_CALL_ACTIVE 1725
#define RPC_S_CALL_FAILED 1726
#define RPC_S_CALL_FAILED_DNE 1727
#define RPC_S_PROTOCOL_ERROR 1728
#define RPC_S_PROXY_ACCESS_DENIED 1729
#define RPC_S_UNSUPPORTED_TRANS_SYN 1730
#define RPC_S_UNSUPPORTED_TYPE 1732
#define RPC_S_INVALID_TAG 1733
#define RPC_S_INVALID_BOUND 1734
#define RPC_S_NO_ENTRY_NAME 1735
#define RPC_S_INVALID_NAME_SYNTAX 1736
#define RPC_S_UNSUPPORTED_NAME_SYNTAX 1737
#define RPC_S_UUID_NO_ADDRESS 1739
#define RPC_S_DUPLICATE_ENDPOINT 1740
#define RPC_S_UNKNOWN_AUTHN_TYPE 1741
#define RPC_S_MAX_CALLS_TOO_SMALL 1742
#define RPC_S_STRING_TOO_LONG 1743
#define RPC_S_PROTSEQ_NOT_FOUND 1744
#define RPC_S_PROCNUM_OUT_OF_RANGE 1745
#define RPC_S_BINDING_HAS_NO_AUTH 1746
#define RPC_S_UNKNOWN_AUTHN_SERVICE 1747
#define RPC_S_UNKNOWN_AUTHN_LEVEL 1748
#define RPC_S_INVALID_AUTH_IDENTITY 1749
#define RPC_S_UNKNOWN_AUTHZ_SERVICE 1750
#define EPT_S_INVALID_ENTRY 1751
#define EPT_S_CANT_PERFORM_OP 1752
#define EPT_S_NOT_REGISTERED 1753
#define RPC_S_NOTHING_TO_EXPORT 1754
#define RPC_S_INCOMPLETE_NAME 1755
#define RPC_S_INVALID_VERS_OPTION 1756
#define RPC_S_NO_MORE_MEMBERS 1757
#define RPC_S_NOT_ALL_OBJS_UNEXPORTED 1758
#define RPC_S_INTERFACE_NOT_FOUND 1759
#define RPC_S_ENTRY_ALREADY_EXISTS 1760
#define RPC_S_ENTRY_NOT_FOUND 1761
#define RPC_S_NAME_SERVICE_UNAVAILABLE 1762
#define RPC_S_INVALID_NAF_ID 1763
#define RPC_S_CANNOT_SUPPORT 1764
#define RPC_S_NO_CONTEXT_AVAILABLE 1765
#define RPC_S_INTERNAL_ERROR 1766
#define RPC_S_ZERO_DIVIDE 1767
#define RPC_S_ADDRESS_ERROR 1768
#define RPC_S_FP_DIV_ZERO 1769
#define RPC_S_FP_UNDERFLOW 1770
#define RPC_S_FP_OVERFLOW 1771
#define RPC_X_NO_MORE_ENTRIES 1772
#define RPC_X_SS_CHAR_TRANS_OPEN_FAIL 1773
#define RPC_X_SS_CHAR_TRANS_SHORT_FILE 1774
#define RPC_X_SS_IN_NULL_CONTEXT 1775
#define RPC_X_SS_CONTEXT_DAMAGED 1777
#define RPC_X_SS_HANDLES_MISMATCH 1778
#define RPC_X_SS_CANNOT_GET_CALL_HANDLE 1779
#define RPC_X_NULL_REF_POINTER 1780
#define RPC_X_ENUM_VALUE_OUT_OF_RANGE 1781
#define RPC_X_BYTE_COUNT_TOO_SMALL 1782
#define RPC_X_BAD_STUB_DATA 1783
#define RPC_S_CALL_IN_PROGRESS 1791
#define RPC_S_NO_MORE_BINDINGS 1806
#define RPC_S_NO_INTERFACES 1817
#define RPC_S_CALL_CANCELLED 1818
#define RPC_S_BINDING_INCOMPLETE 1819
#define RPC_S_COMM_FAILURE 1820
#define RPC_S_UNSUPPORTED_AUTHN_LEVEL 1821
#define RPC_S_NO_PRINC_NAME 1822
#define RPC_S_NOT_RPC_ERROR 1823
#define RPC_S_UUID_LOCAL_ONLY 1824
#define RPC_S_SEC_PKG_ERROR 1825
#define RPC_S_NOT_CANCELLED 1826
#define RPC_X_INVALID_ES_ACTION 1827
#define RPC_X_WRONG_ES_VERSION 1828
#define RPC_X_WRONG_STUB_VERSION 1829
#define RPC_X_INVALID_PIPE_OBJECT 1830
#define RPC_X_WRONG_PIPE_ORDER 1831
#define RPC_X_WRONG_PIPE_VERSION 1832
#define RPC_S_COOKIE_AUTH_FAILED 1833
#define RPC_S_GROUP_MEMBER_NOT_FOUND 1898
#define EPT_S_CANT_CREATE 1899
#define RPC_S_INVALID_OBJECT 1900
#define RPC_S_SEND_INCOMPLETE 1913
#define RPC_S_INVALID_ASYNC_HANDLE 1914
#define RPC_S_INVALID_ASYNC_CALL 1915
#define RPC_X_PIPE_CLOSED 1916
#define RPC_X_PIPE_DISCIPLINE_ERROR 1917
#define RPC_X_PIPE_EMPTY 1918
#define RPC_S_ENTRY_TYPE_MISMATCH 1922
#define RPC_S_NOT_ALL_OBJS_EXPORTED 1923
#define RPC_S_INTERFACE_NOT_EXPORTED 1924
#define RPC_S_PROFILE_NOT_ADDED 1925
#define RPC_S_PRF_ELT_NOT_ADDED 1926
#define RPC_S_PRF_ELT_NOT_REMOVED 1927
#define RPC_S_GRP_ELT_NOT_ADDED 1928
#define RPC_S_GRP_ELT_NOT_REMOVED 1929
#endif // !defined(_WIN32) && !_defined(__CYGWIN__)


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
	struct CtxItem {
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
	struct CtxResults {
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
		struct
		{
			DWORD Code;
			DWORD Padding;
		} Error;

	};
} /*__packed*/ RPC_RESPONSE64;


//#define RpcCtx SOCKET
typedef SOCKET RpcCtx;
typedef int RpcStatus;

#define RPC_INVALID_CTX ((WORD)~0)

#define RPC_BIND_ACCEPT (0)
#define RPC_BIND_NACK   (LE16(2))
#define RPC_BIND_ACK    (LE16(3))

#define RPC_SYNTAX_UNSUPPORTED         (LE16(2))
#define RPC_ABSTRACTSYNTAX_UNSUPPORTED (LE16(1))
#define RPC_NCA_UNK_IF                 (LE32(0x1c010003))
#define RPC_NCA_PROTO_ERROR            (LE32(0x1c01000b))

#define RPC_BTFN_SEC_CONTEXT_MULTIPLEX (LE16(1))
#define RPC_BTFN_KEEP_ORPHAN           (LE16(2))

#define INVALID_RPCCTX INVALID_SOCKET
#define closeRpc socketclose

#define RPC_PT_REQUEST            0
#define RPC_PT_RESPONSE           2
#define RPC_PT_FAULT              3
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
		BOOL HasNDR32 : 1;
		BOOL HasNDR64 : 1;
		BOOL HasBTFN : 1;
	};
} RPC_FLAGS, *PRPC_FLAGS;

extern RPC_FLAGS RpcFlags;

void rpcServer(const SOCKET sock, const DWORD rpcAssocGroup, const char* const ipstr);
RpcStatus rpcBindClient(const RpcCtx sock, const int_fast8_t verbose, PRpcDiag_t rpcDiag);
RpcStatus rpcSendRequest(const RpcCtx socket, const BYTE *const kmsRequest, const size_t requestSize, BYTE **kmsResponse, size_t *const responseSize);

#endif // __rpc_h
