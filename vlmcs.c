#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "vlmcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#include <termios.h>
#else // _WIN32
#endif // _WIN32
#include "endian.h"
#include "shared_globals.h"
#include "output.h"
#ifndef USE_MSRPC
#include "network.h"
#include "rpc.h"
#else // USE_MSRPC
#include "msrpc-client.h"
#endif // USE_MSRPC
#include "kms.h"
#include "helpers.h"
#include "dns_srv.h"


#define VLMCS_OPTION_GRAB_INI 1
#define VLMCS_OPTION_NO_GRAB_INI 2

#define kmsVersionMinor 0 // Currently constant. May change in future KMS versions

// Function Prototypes
static void CreateRequestBase(REQUEST *Request);


// KMS Parameters
#ifndef NO_VERBOSE_LOG
static int_fast8_t verbose = FALSE;
#endif

static int_fast8_t VMInfo = FALSE;
static int_fast8_t dnsnames = TRUE;
static int FixedRequests = 0;
static BYTE LicenseStatus = 0x02;
static const char *CMID = NULL;
static const char *CMID_prev = NULL;
static const char *WorkstationName = NULL;
static int BindingExpiration = 43200; //30 days
static const char *RemoteAddr;
static int_fast8_t ReconnectForEachRequest = FALSE;
static int AddressFamily = AF_UNSPEC;
static int_fast8_t incompatibleOptions = 0;
static const char* fn_ini_client = NULL;

#ifndef NO_DNS
static int_fast8_t NoSrvRecordPriority = FALSE;
#endif // NO_DNS


// Structure for handling "License Packs" (e.g. Office2013v5 or WindowsVista)
typedef struct
{
	const char *names;			//This is a list of strings. Terminate with additional Zero!!!
	int N_Policy;
	int kmsVersionMajor;
	const GUID *AppID;
	GUID ActID;
	GUID KMSID;
} LicensePack;


typedef char iniFileEpidLines[3][256];

// Well known "license packs"
static const LicensePack LicensePackList[] =
{
	// 			 List of names          min lics version  appID            skuId                                                                                KMSCountedID
	/* 000 */ { "Vista\000W6\000"
				"WindowsVista\000"
				"Windows\000",                25,      4, PWINGUID,        { 0x4f3d1606, 0x3fea, 0x4c01, { 0xbe, 0x3c, 0x8d, 0x67, 0x1c, 0x40, 0x1e, 0x3b, } }, { 0x212a64dc, 0x43b1, 0x4d3d, { 0xa3, 0x0c, 0x2f, 0xc6, 0x9d, 0x20, 0x95, 0xc6 } } },
	/* 001 */ { "W7\000Windows7\000",         25,      4, PWINGUID,        { 0xb92e9980, 0xb9d5, 0x4821, { 0x9c, 0x94, 0x14, 0x0f, 0x63, 0x2f, 0x63, 0x12, } }, { 0x7fde5219, 0xfbfa, 0x484a, { 0x82, 0xc9, 0x34, 0xd1, 0xad, 0x53, 0xe8, 0x56 } } },
	/* 002 */ { "W8\000Windows8\000",         25,      5, PWINGUID,        { 0xa98bcd6d, 0x5343, 0x4603, { 0x8a, 0xfe, 0x59, 0x08, 0xe4, 0x61, 0x11, 0x12, } }, { 0x3c40b358, 0x5948, 0x45af, { 0x92, 0x3b, 0x53, 0xd2, 0x1f, 0xcc, 0x7e, 0x79 } } },
	/* 003 */ { "W8C\000Windows8C\000",       25,      5, PWINGUID,        { 0xc04ed6bf, 0x55c8, 0x4b47, { 0x9f, 0x8e, 0x5a, 0x1f, 0x31, 0xce, 0xee, 0x60, } }, { 0xbbb97b3b, 0x8ca4, 0x4a28, { 0x97, 0x17, 0x89, 0xfa, 0xbd, 0x42, 0xc4, 0xac } } },
	/* 004 */ { "W81\000Windows81\000",       25,      6, PWINGUID,        { 0xc06b6981, 0xd7fd, 0x4a35, { 0xb7, 0xb4, 0x05, 0x47, 0x42, 0xb7, 0xaf, 0x67, } }, { 0xcb8fc780, 0x2c05, 0x495a, { 0x97, 0x10, 0x85, 0xaf, 0xff, 0xc9, 0x04, 0xd7 } } },
	/* 005 */ { "W81C\000Windows81C\000",     25,      6, PWINGUID,        { 0xfe1c3238, 0x432a, 0x43a1, { 0x8e, 0x25, 0x97, 0xe7, 0xd1, 0xef, 0x10, 0xf3, } }, { 0x6d646890, 0x3606, 0x461a, { 0x86, 0xab, 0x59, 0x8b, 0xb8, 0x4a, 0xce, 0x82 } } },
	/* 006 */ { "W10\000Windows10\000",       25,      6, PWINGUID,        { 0x73111121, 0x5638, 0x40f6, { 0xbc, 0x11, 0xf1, 0xd7, 0xb0, 0xd6, 0x43, 0x00, } }, { 0x58e2134f, 0x8e11, 0x4d17, { 0x9c, 0xb2, 0x91, 0x06, 0x9c, 0x15, 0x11, 0x48 } } },
	/* 007 */ { "W10C\000Windows10C\000",     25,      6, PWINGUID,        { 0x58e97c99, 0xf377, 0x4ef1, { 0x81, 0xd5, 0x4a, 0xd5, 0x52, 0x2b, 0x5f, 0xd8, } }, { 0xe1c51358, 0xfe3e, 0x4203, { 0xa4, 0xa2, 0x3b, 0x6b, 0x20, 0xc9, 0x73, 0x4e } } },
	/* 008 */ { "2008" "\0" "2008A\000",       5,      4, PWINGUID,        { 0xddfa9f7c, 0xf09e, 0x40b9, { 0x8c, 0x1a, 0xbe, 0x87, 0x7a, 0x9a, 0x7f, 0x4b, } }, { 0x33e156e4, 0xb76f, 0x4a52, { 0x9f, 0x91, 0xf6, 0x41, 0xdd, 0x95, 0xac, 0x48 } } },
	/* 009 */ { "2008B\000",                   5,      4, PWINGUID,        { 0xc1af4d90, 0xd1bc, 0x44ca, { 0x85, 0xd4, 0x00, 0x3b, 0xa3, 0x3d, 0xb3, 0xb9, } }, { 0x8fe53387, 0x3087, 0x4447, { 0x89, 0x85, 0xf7, 0x51, 0x32, 0x21, 0x5a, 0xc9 } } },
	/* 010 */ { "2008C\000",                   5,      4, PWINGUID,        { 0x68b6e220, 0xcf09, 0x466b, { 0x92, 0xd3, 0x45, 0xcd, 0x96, 0x4b, 0x95, 0x09, } }, { 0x8a21fdf3, 0xcbc5, 0x44eb, { 0x83, 0xf3, 0xfe, 0x28, 0x4e, 0x66, 0x80, 0xa7 } } },
	/* 011 */ { "2008R2" "\0" "2008R2A\000",   5,      4, PWINGUID,        { 0xa78b8bd9, 0x8017, 0x4df5, { 0xb8, 0x6a, 0x09, 0xf7, 0x56, 0xaf, 0xfa, 0x7c, } }, { 0x0fc6ccaf, 0xff0e, 0x4fae, { 0x9d, 0x08, 0x43, 0x70, 0x78, 0x5b, 0xf7, 0xed } } },
	/* 012 */ { "2008R2B\000",                 5,      4, PWINGUID,        { 0x620e2b3d, 0x09e7, 0x42fd, { 0x80, 0x2a, 0x17, 0xa1, 0x36, 0x52, 0xfe, 0x7a, } }, { 0xca87f5b6, 0xcd46, 0x40c0, { 0xb0, 0x6d, 0x8e, 0xcd, 0x57, 0xa4, 0x37, 0x3f } } },
	/* 013 */ { "2008R2C\000",                 5,      4, PWINGUID,        { 0x7482e61b, 0xc589, 0x4b7f, { 0x8e, 0xcc, 0x46, 0xd4, 0x55, 0xac, 0x3b, 0x87, } }, { 0xb2ca2689, 0xa9a8, 0x42d7, { 0x93, 0x8d, 0xcf, 0x8e, 0x9f, 0x20, 0x19, 0x58 } } },
	/* 014 */ { "2012\000",                    5,      5, PWINGUID,        { 0xf0f5ec41, 0x0d55, 0x4732, { 0xaf, 0x02, 0x44, 0x0a, 0x44, 0xa3, 0xcf, 0x0f, } }, { 0x8665cb71, 0x468c, 0x4aa3, { 0xa3, 0x37, 0xcb, 0x9b, 0xc9, 0xd5, 0xea, 0xac } } },
	/* 015 */ { "2012R2\000" "12R2\000",       5,      6, PWINGUID,        { 0x00091344, 0x1ea4, 0x4f37, { 0xb7, 0x89, 0x01, 0x75, 0x0b, 0xa6, 0x98, 0x8c, } }, { 0x8456EFD3, 0x0C04, 0x4089, { 0x87, 0x40, 0x5b, 0x72, 0x38, 0x53, 0x5a, 0x65 } } },
	/* 016 */ { "Office2010\000O14\000",       5,      4, POFFICE2010GUID, { 0x6f327760, 0x8c5c, 0x417c, { 0x9b, 0x61, 0x83, 0x6a, 0x98, 0x28, 0x7e, 0x0c, } }, { 0xe85af946, 0x2e25, 0x47b7, { 0x83, 0xe1, 0xbe, 0xbc, 0xeb, 0xea, 0xc6, 0x11 } } },
	/* 017 */ { "Office2013\000O15\000",       5,      6, POFFICE2013GUID, { 0xb322da9c, 0xa2e2, 0x4058, { 0x9e, 0x4e, 0xf5, 0x9a, 0x69, 0x70, 0xbd, 0x69, } }, { 0xe6a6f1bf, 0x9d40, 0x40c3, { 0xaa, 0x9f, 0xc7, 0x7b, 0xa2, 0x15, 0x78, 0xc0 } } },
	/* 018 */ { "Office2013V5\000",            5,      5, POFFICE2013GUID, { 0xb322da9c, 0xa2e2, 0x4058, { 0x9e, 0x4e, 0xf5, 0x9a, 0x69, 0x70, 0xbd, 0x69, } }, { 0xe6a6f1bf, 0x9d40, 0x40c3, { 0xaa, 0x9f, 0xc7, 0x7b, 0xa2, 0x15, 0x78, 0xc0 } } },
	/* 019 */ { "Office2016\000" "O16\000",    5,      6, POFFICE2013GUID, { 0xd450596f, 0x894d, 0x49e0, { 0x96, 0x6a, 0xfd, 0x39, 0xed, 0x4c, 0x4c, 0x64, } }, { 0x85b5f61b, 0x320b, 0x4be3, { 0x81, 0x4a, 0xb7, 0x6b, 0x2b, 0xfa, 0xfc, 0x82 } } },
	/* 020 */ { NULL, 0, 0, NULL, { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } }, { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } } }
};


typedef struct
{
	const char* first[16];
	const char* second[16];
	const char* tld[22];
} DnsNames;


// Some names for the DNS name random generator
static DnsNames ClientDnsNames =
{
	{ "www", "ftp", "kms", "hack-me", "smtp", "ns1", "mx1", "ns1", "pop3", "imap", "mail", "dns", "headquarter", "we-love", "_vlmcs._tcp", "ceo-laptop" },
	{ ".microsoft", ".apple", ".amazon", ".samsung", ".adobe", ".google", ".yahoo", ".facebook", ".ubuntu", ".oracle", ".borland", ".htc", ".acer", ".windows", ".linux", ".sony" },
	{ ".com", ".net", ".org", ".cn", ".co.uk", ".de", ".com.tw", ".us", ".fr", ".it", ".me", ".info", ".biz", ".co.jp", ".ua", ".at", ".es", ".pro", ".by", ".ru", ".pl", ".kr" }
};


// This is the one, we are actually using. We use Vista, if user selects nothing
LicensePack ActiveLicensePack;


// Request Count Control Variables
static int RequestsToGo = 1;
static BOOL firstRequestSent = FALSE;


static void string2UuidOrExit(const char *const restrict input, GUID *const restrict guid)
{
	if (strlen(input) != GUID_STRING_LENGTH || !string2Uuid(input, guid))
	{
		errorout("Fatal: Command line contains an invalid GUID.\n");
		exit(!0);
	}
}


#ifndef NO_HELP

__noreturn static void clientUsage(const char* const programName)
{
	errorout(
		"vlmcs %s \n\n"
#		ifndef NO_DNS
			"Usage: %s [options] [ <host>[:<port>] | .<domain> | - ] [options]\n\n"
#		else // DNS
			"Usage: %s [options] [<host>[:<port>]] [options]\n\n"
#		endif // DNS

		"Options:\n\n"

#		ifndef NO_VERBOSE_LOG
		"  -v Be verbose\n"
#		endif
		"  -l <app>\n"
		"  -4 Force V4 protocol\n"
		"  -5 Force V5 protocol\n"
		"  -6 Force V6 protocol\n"
#		ifndef USE_MSRPC
		"  -i <IpVersion> Use IP protocol (4 or 6)\n"
#		endif // USE_MSRPC
		"  -e Show some valid examples\n"
		"  -x Show valid Apps\n"
		"  -d no DNS names, use Netbios names (no effect if -w is used)\n"
		"  -V show version information and exit\n\n"

		"Advanced options:\n\n"

		"  -a <AppGUID> Use custom Application GUID\n"
		"  -s <ActGUID> Use custom Activation Configuration GUID\n"
		"  -k <KmsGUID> Use custom KMS GUID\n"
		"  -c <ClientGUID> Use custom Client GUID. Default: Use random\n"
		"  -o <PreviousClientGUID> Use custom Prevoius Client GUID. Default: ZeroGUID\n"
		"  -w <Workstation> Use custom workstation name. Default: Use random\n"
		"  -r <RequiredClientCount> Fake required clients\n"
		"  -n <Requests> Fixed # of requests (Default: Enough to charge)\n"
		"  -m Pretend to be a virtual machine\n"
		"  -G <file> Get ePID/HwId data and write to <file>. Can't be used with -l, -4, -5, -6, -a, -s, -k, -r and -n\n"
#		ifndef USE_MSRPC
		"  -T Use a new TCP connection for each request.\n"
		"  -N <0|1> disable or enable NDR64. Default: 1\n"
		"  -B <0|1> disable or enable RPC bind time feature negotiation. Default: 1\n"
#		endif // USE_MSRPC
		"  -t <LicenseStatus> Use specfic license status (0 <= T <= 6)\n"
		"  -g <BindingExpiration> Use a specfic binding expiration time in minutes. Default 43200\n"
#		ifndef NO_DNS
		"  -P Ignore priority and weight in DNS SRV records\n"
#		endif // NO_DNS
#		ifndef USE_MSRPC
		"  -p Don't use multiplexed RPC bind\n"
#		endif // USE_MSRPC
		"\n"

		"<port>:\t\tTCP port name of the KMS to use. Default 1688.\n"
		"<host>:\t\thost name of the KMS to use. Default 127.0.0.1\n"
#		ifndef NO_DNS
		".<domain>:\tfind KMS server in <domain> via DNS\n"
#		endif // NO_DNS
		"<app>:\t\t(Type %s -x to see a list of valid apps)\n\n",
		Version, programName, programName
	);

	exit(!0);
}

__pure static int getLineWidth(void)
{
	#ifdef TERMINAL_FIXED_WIDTH // For Toolchains that to not have winsize
	return TERMINAL_FIXED_WIDTH;
	#else // Can determine width of terminal
	#ifndef _WIN32

	struct winsize w;

	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w))
	{
		return 80; // Return this if stdout is not a tty
	}

	return w.ws_col;

	#else // _WIN32

	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

	if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo))
	{
		return 80; // Return this if stdout is not a Console
	}

	return csbiInfo.srWindow.Right - csbiInfo.srWindow.Left;

	#endif // WIN32

	#endif // Can determine width of terminal

}

__noreturn static void showProducts(PRINTFUNC p)
{
	int cols = getLineWidth();
	int itemsPerLine;
	uint8_t i;

	p(
		"The following "
		#if !defined(NO_EXTENDED_PRODUCT_LIST) && !defined(NO_BASIC_PRODUCT_LIST)
		"aliases "
		#else
		"names "
		#endif
		"can be used with -l:\n\n"
	);

	const LicensePack* lp;

	itemsPerLine = cols / 20;
	if (!itemsPerLine) itemsPerLine = 1;

	for (i = 1, lp = LicensePackList; lp->names; lp++)
	{
		const char* name;

		for (name = lp->names; *name; name += strlen(name) + 1, i++)
		{
			p("%-20s", name);

			if (!(i % itemsPerLine)) p("\n");
		}
	}

	p("\n\n");

	#if !defined(NO_EXTENDED_PRODUCT_LIST) && !defined(NO_BASIC_PRODUCT_LIST)

	const KmsIdList* currentProduct;
	uint_fast8_t longestString = 0;
	uint8_t k, items = getExtendedProductListSize();

	p("You may also use these product names or numbers:\n\n");

	for (currentProduct = ExtendedProductList; currentProduct->name; currentProduct++)
	{
		uint_fast8_t len = strlen(currentProduct->name);

		if (len > longestString)
			longestString = len;
	}

	itemsPerLine = cols / (longestString + 10);
	if (!itemsPerLine) itemsPerLine = 1;
	uint8_t lines = items / itemsPerLine;
	if (items % itemsPerLine) lines++;

	for (i = 0; i < lines; i++)
	{
		for (k = 0; k < itemsPerLine; k++)
		{
			uint8_t j;
			uint8_t index = k * lines + i;

			if (index >= items) break;

			p("%3u = %s",  index + 1, ExtendedProductList[index].name);

			for (j = 0; j < longestString + 4 - strlen(ExtendedProductList[index].name); j++)
			{
				p(" ");
			}
		}

		p("\n");
	}

	p("\n");

	#endif // !defined(NO_EXTENDED_PRODUCT_LIST) && !defined(NO_BASIC_PRODUCT_LIST)

	exit(0);
}

__noreturn static void examples(const char* const programName)
{
	printf(
		"\nRequest activation for Office2013 using V4 protocol from 192.168.1.5:1688\n"
		"\t%s -l O15 -4 192.168.1.5\n"
		"\t%s -l O15 -4 192.168.1.5:1688\n\n"

		"Request activation for Windows Server 2012 using V4 protocol from localhost:1688\n"
		"\t%s -4 -l Windows -k 8665cb71-468c-4aa3-a337-cb9bc9d5eaac\n"
		"\t%s -4 -l 2012\n"
		"\t%s -4 -l 2012 [::1]:1688\n"
		"\t%s -4 -l 12 127.0.0.2:1688\n\n"

		"Send 100,000 requests to localhost:1688\n"
		"\t%s -n 100000 -l Office2010\n\n"

		"Request Activation for Windows 8 from 10.0.0.1:4711 and pretend to be Steve Ballmer\n"
		"\t%s -l Windows8 -w steveb1.redmond.microsoft.com 10.0.0.1:4711\n\n",
		programName, programName, programName, programName, programName, programName, programName, programName
	);

	exit(0);
}


#else // NO_HELP


__noreturn static void clientUsage(const char* const programName)
{
	errorout("Incorrect parameter specified.\n");
	exit(!0);
}


#endif // NO_HELP


static BOOL findLicensePackByName(const char* const name, LicensePack* const lp)
{
	// Try to find a package in the short list first

	LicensePack *licensePack;
	for (licensePack = (LicensePack*)&LicensePackList; licensePack->names; licensePack ++)
	{
		const char *currentName;
		for (currentName = licensePack->names; *currentName; currentName += strlen(currentName) + 1)
		{
			if (!strcasecmp(name, currentName))
			{
				*lp = *licensePack;
				return TRUE;
			}
		}
	}

	#if defined(NO_BASIC_PRODUCT_LIST) || defined(NO_EXTENDED_PRODUCT_LIST)

	return FALSE;

	#else // Both Lists are available

	// search extended product list

    uint8_t items = getExtendedProductListSize();
    unsigned int index;

    if (stringToInt(name, 1, items, &index))
    {
    	index--;
    }
    else
    {
    	for (index = 0; index < items; index++)
    	{
    		if (!strcasecmp(ExtendedProductList[index].name, name)) break;
    	}

    	if (index >= items) return FALSE;
    }

	lp->AppID			= &AppList[ExtendedProductList[index].AppIndex].guid;
	lp->KMSID			= ProductList[ExtendedProductList[index].KmsIndex].guid;
	lp->ActID			= ExtendedProductList[index].guid;
	lp->N_Policy 		= ProductList[ExtendedProductList[index].KmsIndex].KMS_PARAM_REQUIREDCOUNT;
	lp->kmsVersionMajor	= ProductList[ExtendedProductList[index].KmsIndex].KMS_PARAM_MAJOR;

	return TRUE;

	#endif // Both Lists are available
}

static const char* const client_optstring = "+N:B:i:l:a:s:k:c:w:r:n:t:g:G:o:pPTv456mexdV";


//First pass. We handle only "-l". Since -a -k -s -4 -5 and -6 are exceptions to -l, we process -l first
static void parseCommandLinePass1(const int argc, CARGV argv)
{
	int o;
	optReset();

	for (opterr = 0; ( o = getopt(argc, (char* const*)argv, client_optstring) ) > 0; ) switch (o)
	{
		case 'l': // Set "License Pack" and protocol version (e.g. Windows8, Office2013v5, ...)

			if (!findLicensePackByName(optarg, &ActiveLicensePack))
			{
				errorout("Invalid client application. \"%s\" is not valid for -l.\n\n", optarg);
				#ifndef NO_HELP
				showProducts(&errorout);
				#endif // !NO_HELP
			}

			break;

		default:
			break;
	}
}


// Second Pass. Handle all options except "-l"
static void parseCommandLinePass2(const char *const programName, const int argc, CARGV argv)
{
	int o;
	optReset();

	for (opterr = 0; ( o = getopt(argc, (char* const*)argv, client_optstring) ) > 0; ) switch (o)
	{
			#ifndef NO_HELP

			case 'e': // Show examples

				examples(programName);
				break;

			case 'x': // Show Apps

				showProducts(&printf);
				break;

			#endif // NO_HELP

#			ifndef NO_DNS

			case 'P':

				NoSrvRecordPriority = TRUE;
				break;

#			endif // NO_DNS

			case 'G':

				incompatibleOptions |= VLMCS_OPTION_GRAB_INI;
				fn_ini_client = optarg;
				break;

#			ifndef USE_MSRPC

			case 'N':
				if (!getArgumentBool(&UseRpcNDR64, optarg)) clientUsage(programName);
				break;

			case 'B':
				if (!getArgumentBool(&UseRpcBTFN, optarg)) clientUsage(programName);
				break;

			case 'i':

				switch(getOptionArgumentInt(o, 4, 6))
				{
					case 4:
						AddressFamily = AF_INET;
						break;
					case 6:
						AddressFamily = AF_INET6;
						break;
					default:
						errorout("IPv5 does not exist.\n");
						exit(!0);
						break;
				}

				break;

			case 'p': // Multiplexed RPC

				UseMultiplexedRpc = FALSE;
				break;

#			endif // USE_MSRPC

			case 'n': // Fixed number of Requests (regardless, whether they are required)

				incompatibleOptions |= VLMCS_OPTION_NO_GRAB_INI;
				FixedRequests = getOptionArgumentInt(o, 1, INT_MAX);
				break;

			case 'r': // Fake minimum required client count

				incompatibleOptions |= VLMCS_OPTION_NO_GRAB_INI;
				ActiveLicensePack.N_Policy = getOptionArgumentInt(o, 1, INT_MAX);
				break;

			case 'c': // use a specific client GUID

				// If using a constant Client ID, send only one request unless /N= explicitly specified
				if (!FixedRequests) FixedRequests = 1;

				CMID = optarg;
				break;

			case 'o': // use a specific previous client GUID

				CMID_prev = optarg;
				break;

			case 'a': // Set specific App Id

				incompatibleOptions |= VLMCS_OPTION_NO_GRAB_INI;
				ActiveLicensePack.AppID = (GUID*)vlmcsd_malloc(sizeof(GUID));

				string2UuidOrExit(optarg, (GUID*)ActiveLicensePack.AppID);
				break;

			case 'g': // Set custom "grace" time in minutes (default 30 days)

				BindingExpiration = getOptionArgumentInt(o, 0, INT_MAX);
				break;

			case 's': // Set specfic SKU ID

				incompatibleOptions |= VLMCS_OPTION_NO_GRAB_INI;
				string2UuidOrExit(optarg, &ActiveLicensePack.ActID);
				break;

			case 'k': // Set specific KMS ID

				incompatibleOptions |= VLMCS_OPTION_NO_GRAB_INI;
				string2UuidOrExit(optarg, &ActiveLicensePack.KMSID);
				break;

			case '4': // Force V4 protocol
			case '5': // Force V5 protocol
			case '6': // Force V5 protocol

				incompatibleOptions |= VLMCS_OPTION_NO_GRAB_INI;
				ActiveLicensePack.kmsVersionMajor = o - 0x30;
				break;

			case 'd': // Don't use DNS names

				dnsnames = FALSE;
				break;

#			ifndef NO_VERBOSE_LOG

			case 'v': // Be verbose

				verbose = TRUE;
				break;

#			endif // NO_VERBOSE_LOG

			case 'm': // Pretend to be a virtual machine

				VMInfo = TRUE;
				break;

			case 'w': // WorkstationName (max. 63 chars)

				WorkstationName = optarg;

				if (strlen(WorkstationName) > 63)
				{
					errorout("\007WARNING! Truncating Workstation name to 63 characters (%s).\n", WorkstationName);
				}

				break;

			case 't':

				LicenseStatus = getOptionArgumentInt(o, 0, 6) & 0xff;
				break;

#			ifndef USE_MSRPC

			case 'T':

				ReconnectForEachRequest = TRUE;
				break;

#			endif // USE_MSRPC

			case 'l':
				incompatibleOptions |= VLMCS_OPTION_NO_GRAB_INI;
				break;

#			ifndef NO_VERSION_INFORMATION

			case 'V':
#				if defined(__s390__) && !defined(__zarch__) && !defined(__s390x__)
				printf("vlmcs %s %i-bit\n", Version, sizeof(void*) == 4 ? 31 : (int)sizeof(void*) << 3);
#				else
				printf("vlmcs %s %i-bit\n", Version, (int)sizeof(void*) << 3);
#				endif // defined(__s390__) && !defined(__zarch__) && !defined(__s390x__)
				printPlatform();
				printCommonFlags();
				printClientFlags();
				exit(0);

#			endif // NO_VERSION_INFORMATION

			default:
				clientUsage(programName);
	}
	if ((incompatibleOptions & (VLMCS_OPTION_NO_GRAB_INI | VLMCS_OPTION_GRAB_INI)) == (VLMCS_OPTION_NO_GRAB_INI | VLMCS_OPTION_GRAB_INI))
		clientUsage(programName);
}


/*
 * Compares 2 GUIDs where one is host-endian and the other is little-endian (network byte order)
 */
int_fast8_t IsEqualGuidLEHE(const GUID* const guid1, const GUID* const guid2)
{
	GUID tempGuid;
	LEGUID(&tempGuid, guid2);
	return IsEqualGUID(guid1, &tempGuid);
}


#ifndef USE_MSRPC
static void checkRpcLevel(const REQUEST* request, RESPONSE* response)
{
	if (!RpcFlags.HasNDR32)
		errorout("\nWARNING: Server's RPC protocol does not support NDR32.\n");

	if (UseRpcBTFN && UseRpcNDR64 && RpcFlags.HasNDR64 && !RpcFlags.HasBTFN)
		errorout("\nWARNING: Server's RPC protocol has NDR64 but no BTFN.\n");

#	ifndef NO_BASIC_PRODUCT_LIST
	if (!IsEqualGuidLEHE(&request->KMSID, &ProductList[15].guid) && UseRpcBTFN && !RpcFlags.HasBTFN)
		errorout("\nWARNING: A server with pre-Vista RPC activated a product other than Office 2010.\n");
#	endif // NO_BASIC_PRODUCT_LIST
}
#endif // USE_MSRPC


static void displayResponse(const RESPONSE_RESULT result, const REQUEST* request, RESPONSE* response, BYTE *hwid)
{
	fflush(stdout);

	if (!result.RpcOK)				errorout("\n\007ERROR: Non-Zero RPC result code.\n");
	if (!result.DecryptSuccess)		errorout("\n\007ERROR: Decryption of V5/V6 response failed.\n");
	if (!result.IVsOK)				errorout("\n\007ERROR: AES CBC initialization vectors (IVs) of request and response do not match.\n");
	if (!result.PidLengthOK)		errorout("\n\007ERROR: The length of the PID is not valid.\n");
	if (!result.HashOK)				errorout("\n\007ERROR: Computed hash does not match hash in response.\n");
	if (!result.ClientMachineIDOK)	errorout("\n\007ERROR: Client machine GUIDs of request and response do not match.\n");
	if (!result.TimeStampOK)		errorout("\n\007ERROR: Time stamps of request and response do not match.\n");
	if (!result.VersionOK)			errorout("\n\007ERROR: Protocol versions of request and response do not match.\n");
	if (!result.HmacSha256OK)		errorout("\n\007ERROR: Keyed-Hash Message Authentication Code (HMAC) is incorrect.\n");
	if (!result.IVnotSuspicious)	errorout("\nWARNING: Response uses an IV following KMSv5 rules in KMSv6 protocol.\n");

	if (result.effectiveResponseSize != result.correctResponseSize)
	{
		errorout("\n\007WARNING: Size of RPC payload (KMS Message) should be %u but is %u.", result.correctResponseSize, result.effectiveResponseSize);
	}

#	ifndef USE_MSRPC
	checkRpcLevel(request, response);
#	endif // USE_MSRPC

	if (!result.DecryptSuccess) return; // Makes no sense to display anything

	char ePID[3 * PID_BUFFER_SIZE];
	if (!ucs2_to_utf8(response->KmsPID, ePID, PID_BUFFER_SIZE, 3 * PID_BUFFER_SIZE))
	{
		memset(ePID + 3 * PID_BUFFER_SIZE - 3, 0, 3);
	}

	// Read KMSPID from Response
#	ifndef NO_VERBOSE_LOG
	if (!verbose)
#	endif // NO_VERBOSE_LOG
	{
		printf(" -> %s", ePID);

		if (LE16(response->MajorVer) > 5)
		{
#			ifndef _WIN32
			printf(" (%016llX)", (unsigned long long)BE64(*(uint64_t*)hwid));
#			else // _WIN32
			printf(" (%016I64X)", (unsigned long long)BE64(*(uint64_t*)hwid));
#			endif // _WIN32
		}

		printf("\n");
	}
#	ifndef NO_VERBOSE_LOG
	else
	{
		printf(
				"\n\nResponse from KMS server\n========================\n\n"
				"Size of KMS Response            : %u (0x%x)\n", result.effectiveResponseSize, result.effectiveResponseSize
		);

		logResponseVerbose(ePID, hwid, response, &printf);
		printf("\n");
	}
#	endif // NO_VERBOSE_LOG
}


static void connectRpc(RpcCtx *s)
{
#	ifdef NO_DNS

	*s = connectToAddress(RemoteAddr, AddressFamily, FALSE);
	if (*s == INVALID_RPCCTX)
	{
		errorout("Fatal: Could not connect to %s\n", RemoteAddr);
		exit(!0);
	}

	if (verbose)
		printf("\nPerforming RPC bind ...\n");

	if (rpcBindClient(*s, verbose))
	{
		errorout("Fatal: Could not bind RPC\n");
		exit(!0);
	}

	if (verbose) printf("... successful\n");

#	else // DNS

	static kms_server_dns_ptr* serverlist = NULL;
	static int numServers = 0;
	//static int_fast8_t ServerListAlreadyPrinted = FALSE;
	int i;

	if (!strcmp(RemoteAddr, "-") || *RemoteAddr == '.') // Get KMS server via DNS SRV record
	{
		if (!serverlist)
			numServers = getKmsServerList(&serverlist, RemoteAddr);

		if (numServers < 1)
		{
			errorout("Fatal: No KMS servers found\n");
			exit(!0);
		}

		if (!NoSrvRecordPriority) sortSrvRecords(serverlist, numServers);

#		ifndef NO_VERBOSE_LOG
		if (verbose /*&& !ServerListAlreadyPrinted*/)
		{
			for (i = 0; i < numServers; i++)
			{
				printf(
						"Found %-40s (priority: %hu, weight: %hu, randomized weight: %i)\n",
						serverlist[i]->serverName,
						serverlist[i]->priority, serverlist[i]->weight,
						NoSrvRecordPriority ? 0 : serverlist[i]->random_weight
				);
			}

			printf("\n");
			//ServerListAlreadyPrinted = TRUE;
		}
#		endif // NO_VERBOSE_LOG
	}
	else // Just use the server supplied on the command line
	{
		if (!serverlist)
		{
			serverlist = (kms_server_dns_ptr*)vlmcsd_malloc(sizeof(kms_server_dns_ptr));
			*serverlist = (kms_server_dns_ptr)vlmcsd_malloc(sizeof(kms_server_dns_t));

			numServers = 1;
			strncpy((*serverlist)->serverName, RemoteAddr, sizeof((*serverlist)->serverName));
		}
	}

	for (i = 0; i < numServers; i++)
	{
		*s = connectToAddress(serverlist[i]->serverName, AddressFamily, (*RemoteAddr == '.' || *RemoteAddr == '-'));

		if (*s == INVALID_RPCCTX) continue;

#		ifndef NO_VERBOSE_LOG
		if (verbose) printf("\nPerforming RPC bind ...\n");

		if (rpcBindClient(*s, verbose))
#		else
		if (rpcBindClient(*s, FALSE))
#		endif
		{
			errorout("Warning: Could not bind RPC\n");
			continue;
		}

#		ifndef NO_VERBOSE_LOG
		if (verbose) printf("... successful\n");
#		endif

		return;
	}

	errorout("Fatal: Could not connect to any KMS server\n");
	exit(!0);

#	endif // DNS
}


static int SendActivationRequest(const RpcCtx sock, RESPONSE *baseResponse, REQUEST *baseRequest, RESPONSE_RESULT *result, BYTE *const hwid)
{
	size_t requestSize, responseSize;
	BYTE *request, *response;
	int status;

	result->mask = 0;

	if (LE16(baseRequest->MajorVer) == 4)
		request = CreateRequestV4(&requestSize, baseRequest);
	else
		request = CreateRequestV6(&requestSize, baseRequest);

	if (!(status = rpcSendRequest(sock, request, requestSize, &response, &responseSize)))
	{
		if (LE16(((RESPONSE*)(response))->MajorVer) == 4)
		{
			RESPONSE_V4 response_v4;
			*result = DecryptResponseV4(&response_v4, responseSize, response, request);
			memcpy(baseResponse, &response_v4.ResponseBase, sizeof(RESPONSE));
		}
		else
		{
			RESPONSE_V6 response_v6;
			*result = DecryptResponseV6(&response_v6, responseSize, response, request, hwid);
			memcpy(baseResponse, &response_v6.ResponseBase, sizeof(RESPONSE));
		}

		result->RpcOK = TRUE;
	}

	if (response) free(response);
	free(request);
	return status;
}


static int sendRequest(RpcCtx *const s, REQUEST *const request, RESPONSE *const response, hwid_t hwid, RESPONSE_RESULT *const result)
{
	CreateRequestBase(request);

	if (*s == INVALID_RPCCTX )
		connectRpc(s);
	else
	{
		// Check for lame KMS emulators that close the socket after each request
		int_fast8_t disconnected = isDisconnected(*s);

		if (disconnected)
			errorout("\nWarning: Server closed RPC connection (probably non-multitasked KMS emulator)\n");

		if (ReconnectForEachRequest || disconnected)
		{
			closeRpc(*s);
			connectRpc(s);
		}
	}

	printf("Sending activation request (KMS V%u) ", ActiveLicensePack.kmsVersionMajor);
	fflush(stdout);

	return SendActivationRequest(*s, response, request, result, hwid);
}


static void displayRequestError(RpcCtx *const s, const int status, const int currentRequest, const int totalRequests)
{
	errorout("\nError 0x%08X while sending request %u of %u\n", status, currentRequest, RequestsToGo + totalRequests);

	switch(status)
	{
	case 0xC004F042: // not licensed
		errorout("The server refused to activate the requested product\n");
		break;

	case 0x8007000D:  // e.g. v6 protocol on a v5 server
		errorout("The server didn't understand the request\n");
		break;

	case 1:
		errorout("An RPC protocol error has occured\n");
		closeRpc(*s);
		connectRpc(s);
		break;

	default:
		break;
	}
}


static void newIniBackupFile(const char* const restrict fname)
{
	FILE *restrict f = fopen(fname, "wb");

	if (!f)
	{
		errorout("Fatal: Cannot create %s: %s\n", fname, strerror(errno));
		exit(!0);
	}

	if (fclose(f))
	{
		errorout("Fatal: Cannot write to %s: %s\n", fname, strerror(errno));
		unlink(fname);
		exit(!0);
	}
}


static void updateIniFile(iniFileEpidLines* const restrict lines)
{
	int_fast8_t lineWritten[_countof(*lines)];
	struct stat statbuf;
	uint_fast8_t i;
	int_fast8_t iniFileExistedBefore = TRUE;
	unsigned int lineNumber;

	memset(lineWritten, FALSE, sizeof(lineWritten));

	char* restrict fn_bak = (char*)vlmcsd_malloc(strlen(fn_ini_client) + 2);

	strcpy(fn_bak, fn_ini_client);
	strcat(fn_bak, "~");

	if (stat(fn_ini_client, &statbuf))
	{
		if (errno != ENOENT)
		{
			errorout("Fatal: %s: %s\n", fn_ini_client, strerror(errno));
			exit(!0);
		}
		else
		{
			iniFileExistedBefore = FALSE;
			newIniBackupFile(fn_bak);
		}
	}
	else
	{
		unlink(fn_bak); // Required for Windows. Most Unix systems don't need it.
		if (rename(fn_ini_client, fn_bak))
		{
			errorout("Fatal: Cannot create %s: %s\n", fn_bak, strerror(errno));
			exit(!0);
		}
	}

	printf("\n%s file %s\n", iniFileExistedBefore ? "Updating" : "Creating", fn_ini_client);

	FILE *restrict in, *restrict out;

	in = fopen(fn_bak, "rb");

	if (!in)
	{
		errorout("Fatal: Cannot open %s: %s\n", fn_bak, strerror(errno));
		exit(!0);
	}

	out = fopen(fn_ini_client, "wb");

	if (!out)
	{
		errorout("Fatal: Cannot create %s: %s\n", fn_ini_client, strerror(errno));
		exit(!0);
	}

	char sourceLine[256];

	for (lineNumber = 1; fgets(sourceLine, sizeof(sourceLine), in); lineNumber++)
	{
		for (i = 0; i < _countof(*lines); i++)
		{
			if (*(*lines)[i] && !strncasecmp(sourceLine, (*lines)[i], GUID_STRING_LENGTH))
			{
				if (lineWritten[i]) break;

				fprintf(out, "%s", (*lines)[i]);
				printf("line %2i: %s", lineNumber, (*lines)[i]);
				lineWritten[i] = TRUE;
				break;
			}
		}

		if (i >= _countof(*lines))
		{
			fprintf(out, "%s", sourceLine);
		}
	}

	if (ferror(in))
	{
		errorout("Fatal: Cannot read from %s: %s\n", fn_bak, strerror(errno));
		exit(!0);
	}

	fclose(in);

	for (i = 0; i < _countof(*lines); i++)
	{
		if (!lineWritten[i] && *(*lines)[i])
		{
			fprintf(out, "%s", (*lines)[i]);
			printf("line %2i: %s", lineNumber + i, (*lines)[i]);
		}
	}

	if (fclose(out))
	{
		errorout("Fatal: Cannot write to %s: %s\n", fn_ini_client, strerror(errno));
		exit(!0);
	}

	if (!iniFileExistedBefore) unlink(fn_bak);

	free(fn_bak);
}

static void grabServerData()
{
	RpcCtx s = INVALID_RPCCTX;
    WORD MajorVer = 6;
	iniFileEpidLines lines;
    int_fast8_t Licenses[_countof(lines)] = { 0, 15, 14 };
    uint_fast8_t i;
	RESPONSE response;
	RESPONSE_RESULT result;
	REQUEST request;
	hwid_t hwid;
	int status;
	size_t len;

	for (i = 0; i < _countof(lines); i++) *lines[i] = 0;

    for (i = 0; i < _countof(Licenses) && MajorVer > 3; i++)
    {
    	ActiveLicensePack = LicensePackList[Licenses[i]];
    	ActiveLicensePack.kmsVersionMajor = MajorVer;
    	status = sendRequest(&s, &request, &response, hwid, &result);
    	printf("%-11s", ActiveLicensePack.names);

    	if (status)
    	{
    		displayRequestError(&s, status, i + 7 - MajorVer, 9 - MajorVer);

    		if (status == 1) break;

    		if ((status & 0xF0000000) == 0x80000000)
    		{
    			MajorVer--;
    			i--;
    		}

    		continue;
    	}

    	printf("%i of %i", (int)(i + 7 - MajorVer), (int)(9 - MajorVer));
    	displayResponse(result, &request, &response, hwid);

    	char guidBuffer[GUID_STRING_LENGTH + 1];
    	char ePID[3 * PID_BUFFER_SIZE];

    	uuid2StringLE(&request.AppID, guidBuffer);

    	if (!ucs2_to_utf8(response.KmsPID, ePID, PID_BUFFER_SIZE, 3 * PID_BUFFER_SIZE))
    	{
    		memset(ePID + 3 * PID_BUFFER_SIZE - 3, 0, 3);
    	}

    	snprintf(lines[i], sizeof(lines[0]), "%s = %s", guidBuffer, ePID);

    	if (response.MajorVer > 5)
    	{
    		len = strlen(lines[i]);
    		snprintf (lines[i] + len, sizeof(lines[0]) - len, "/ %02X %02X %02X %02X %02X %02X %02X %02X", hwid[0], hwid[1], hwid[2], hwid[3], hwid[4], hwid[5], hwid[6], hwid[7]);
    	}

		len = strlen(lines[i]);
    	snprintf(lines[i] + len, sizeof(lines[0]) - len, "\n");

    }

	if (strcmp(fn_ini_client, "-"))
	{
		updateIniFile(&lines);
	}
	else
	{
		printf("\n");
		for (i = 0; i < _countof(lines); i++) printf("%s", lines[i]);
	}
}


int client_main(const int argc, CARGV argv)
{
	#if defined(_WIN32) && !defined(USE_MSRPC)

	// Windows Sockets must be initialized

	WSADATA wsadata;
	int error;

	if ((error = WSAStartup(0x0202, &wsadata)))
	{
		printerrorf("Fatal: Could not initialize Windows sockets (Error: %d).\n", error);
		return error;
	}

	#endif // _WIN32

	#ifdef _NTSERVICE

	// We are not a service
	IsNTService = FALSE;

	// Set console output page to UTF-8
	// SetConsoleOutputCP(65001);

	#endif // _NTSERVICE

	randomNumberInit();
	ActiveLicensePack = *LicensePackList; //first license is Windows Vista

	parseCommandLinePass1(argc, argv);

	int_fast8_t useDefaultHost = FALSE;

	if (optind < argc)
		RemoteAddr = argv[optind];
	else
		useDefaultHost = TRUE;

	int hostportarg = optind;

	if (optind < argc - 1)
	{
		parseCommandLinePass1(argc - hostportarg, argv + hostportarg);

		if (optind < argc - hostportarg)
			clientUsage(argv[0]);
	}

	parseCommandLinePass2(argv[0], argc, argv);

	if (optind < argc - 1)
		parseCommandLinePass2(argv[0], argc - hostportarg, argv + hostportarg);

	if (useDefaultHost)
		RemoteAddr = AddressFamily == AF_INET6 ? "::1" : "127.0.0.1";

	if (fn_ini_client != NULL)
		grabServerData();
	else
	{
		int requests;
		RpcCtx s = INVALID_RPCCTX;

		for (requests = 0, RequestsToGo = ActiveLicensePack.N_Policy - 1; RequestsToGo; requests++)
		{
			RESPONSE response;
			REQUEST request;
			RESPONSE_RESULT result;
			hwid_t hwid;

			int status = sendRequest(&s, &request, &response, hwid, &result);

			if (FixedRequests) RequestsToGo = FixedRequests - requests - 1;

			if (status)
			{
				displayRequestError(&s, status, requests + 1, RequestsToGo + requests + 1);
				if (!FixedRequests)	RequestsToGo = 0;
			}
			else
			{
				if (!FixedRequests)
				{
					if (firstRequestSent && ActiveLicensePack.N_Policy - (int)response.Count >= RequestsToGo)
					{
						errorout("\nThe KMS server does not increment it's active clients. Aborting...\n");
						RequestsToGo = 0;
					}
					else
					{
						RequestsToGo = ActiveLicensePack.N_Policy - response.Count;
						if (RequestsToGo < 0) RequestsToGo = 0;
					}
				}

				fflush(stderr);
				printf("%i of %i ", requests + 1, RequestsToGo + requests + 1);
				displayResponse(result, &request, &response, hwid);
				firstRequestSent = TRUE;
			}
		}
	}

	return 0;
}


// Create Base KMS Client Request
static void CreateRequestBase(REQUEST *Request)
{

	Request->MinorVer = LE16((WORD)kmsVersionMinor);
	Request->MajorVer = LE16((WORD)ActiveLicensePack.kmsVersionMajor);
	Request->VMInfo = LE32(VMInfo);
	Request->LicenseStatus = LE32(LicenseStatus);
	Request->BindingExpiration = LE32(BindingExpiration);
	LEGUID(&Request->AppID, ActiveLicensePack.AppID);
	LEGUID(&Request->ActID, &ActiveLicensePack.ActID);
	LEGUID(&Request->KMSID, &ActiveLicensePack.KMSID);

	getUnixTimeAsFileTime(&Request->ClientTime);
	Request->N_Policy = LE32(ActiveLicensePack.N_Policy);

	{
		GUID tempGUID;

		if (CMID)
		{
			string2UuidOrExit(CMID, &tempGUID);
			LEGUID(&Request->CMID, &tempGUID);
		}
		else
		{
			get16RandomBytes(&Request->CMID);

			// Set reserved UUID bits
			Request->CMID.Data4[0] &= 0x3F;
			Request->CMID.Data4[0] |= 0x80;

			// Set UUID type 4 (random UUID)
			Request->CMID.Data3 &= LE16(0xfff);
			Request->CMID.Data3 |= LE16(0x4000);
		}

		if (CMID_prev)
		{
			string2UuidOrExit(CMID_prev, &tempGUID);
			LEGUID(&Request->CMID_prev, &tempGUID);
		}
		else
		{
			memset(&Request->CMID_prev, 0, sizeof(Request->CMID_prev));
		}
	}

	static const char alphanum[] = "0123456789" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" /*"abcdefghijklmnopqrstuvwxyz" */;

	if (WorkstationName)
	{
		utf8_to_ucs2(Request->WorkstationName, WorkstationName, WORKSTATION_NAME_BUFFER, WORKSTATION_NAME_BUFFER * 3);
	}
	else if (dnsnames)
	{
		int len, len2;
		unsigned int index = rand() % _countof(ClientDnsNames.first);
		len = utf8_to_ucs2(Request->WorkstationName, ClientDnsNames.first[index], WORKSTATION_NAME_BUFFER, WORKSTATION_NAME_BUFFER * 3);

		index = rand() % _countof(ClientDnsNames.second);
		len2 = utf8_to_ucs2(Request->WorkstationName + len, ClientDnsNames.second[index], WORKSTATION_NAME_BUFFER, WORKSTATION_NAME_BUFFER * 3);

		index = rand() % _countof(ClientDnsNames.tld);
		utf8_to_ucs2(Request->WorkstationName + len + len2, ClientDnsNames.tld[index], WORKSTATION_NAME_BUFFER, WORKSTATION_NAME_BUFFER * 3);
	}
	else
	{
		unsigned int size = (rand() % 14) + 1;
		const unsigned char *dummy;
		unsigned int i;

		for (i = 0; i < size; i++)
		{
			Request->WorkstationName[i] = utf8_to_ucs2_char((unsigned char*)alphanum + (rand() % (sizeof(alphanum) - 1)), &dummy);
		}

		Request->WorkstationName[size] = 0;
	}

#	ifndef NO_VERBOSE_LOG
	if (verbose)
	{
		printf("\nRequest Parameters\n==================\n\n");
		logRequestVerbose(Request, &printf);
		printf("\n");
	}
#	endif // NO_VERBOSE_LOG
}


