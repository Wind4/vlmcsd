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
static int_fast8_t verbose = FALSE;
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

		"  -v Be verbose\n"
		"  -l <app>\n"
		"  -4 Force V4 protocol\n"
		"  -5 Force V5 protocol\n"
		"  -6 Force V6 protocol\n"
#		ifndef USE_MSRPC
		"  -i <IpVersion> Use IP protocol (4 or 6)\n"
#		endif // USE_MSRPC
		"  -e Show some valid examples\n"
		"  -x Show valid Apps\n"
		"  -d no DNS names, use Netbios names (no effect if -w is used)\n\n"

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

static const char* const client_optstring = "+N:B:i:l:a:s:k:c:w:r:n:t:g:G:o:pPTv456mexd";


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

			case 'v': // Be verbose

				verbose = TRUE;
				break;

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

	if (!IsEqualGuidLEHE(&request->KMSID, &ProductList[15].guid) && UseRpcBTFN && !RpcFlags.HasBTFN)
		errorout("\nWARNING: A server with pre-Vista RPC activated a product other than Office 2010.\n");
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
	if (!verbose)
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
	else
	{
		printf(
				"\n\nResponse from KMS server\n========================\n\n"
				"Size of KMS Response            : %u (0x%x)\n", result.effectiveResponseSize, result.effectiveResponseSize
		);

		logResponseVerbose(ePID, hwid, response, &printf);
		printf("\n");
	}
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

		if (verbose)
			printf("\nPerforming RPC bind ...\n");

		if (rpcBindClient(*s, verbose))
		{
			errorout("Warning: Could not bind RPC\n");
			continue;
		}

		if (verbose) printf("... successful\n");

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

	//Show Details
	if (verbose)
	{
		printf("\nRequest Parameters\n==================\n\n");
		logRequestVerbose(Request, &printf);
		printf("\n");
	}
}


#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "crypto.h"
#include "endian.h"
#include <stdint.h>

const BYTE AesKeyV4[] = {
	0x05, 0x3D, 0x83, 0x07, 0xF9, 0xE5, 0xF0, 0x88, 0xEB, 0x5E, 0xA6, 0x68, 0x6C, 0xF0, 0x37, 0xC7, 0xE4, 0xEF, 0xD2, 0xD6};

const BYTE AesKeyV5[] = {
	0xCD, 0x7E, 0x79, 0x6F, 0x2A, 0xB2, 0x5D, 0xCB, 0x55, 0xFF, 0xC8, 0xEF, 0x83, 0x64, 0xC4, 0x70 };

const BYTE AesKeyV6[] = {
	0xA9, 0x4A, 0x41, 0x95, 0xE2, 0x01, 0x43, 0x2D, 0x9B, 0xCB, 0x46, 0x04, 0x05, 0xD8, 0x4A, 0x21 };

static const BYTE SBox[] = {
	0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B,
	0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
	0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26,
	0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
	0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2,
	0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
	0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED,
	0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
	0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F,
	0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
	0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC,
	0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
	0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14,
	0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
	0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D,
	0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
	0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F,
	0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
	0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11,
	0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
	0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F,
	0xB0, 0x54, 0xBB, 0x16
};


void XorBlock(const BYTE *const in, const BYTE *out) // Ensure that this is always 32 bit aligned
{
	/*UAA64( out, 0 ) ^= UAA64( in, 0 );
	UAA64( out, 1 ) ^= UAA64( in, 1 );*/

	uint_fast8_t i;

	for (i = 0; i < AES_BLOCK_WORDS; i++)
	{
		((DWORD*)out)[i] ^= ((DWORD*)in)[i];
	}
}

#define AddRoundKey(d, rk) XorBlock((const BYTE *)rk, (const BYTE *)d)

#define Mul2(word) (((word & 0x7f7f7f7f) << 1) ^ (((word & 0x80808080) >> 7) * 0x1b))
#define Mul3(word) (Mul2(word) ^ word)
#define Mul4(word) (Mul2(Mul2(word)))
#define Mul8(word) (Mul2(Mul2(Mul2(word))))
#define Mul9(word) (Mul8(word) ^ word)
#define MulB(word) (Mul8(word) ^ Mul3(word))
#define MulD(word) (Mul8(word) ^ Mul4(word) ^ word)
#define MulE(word) (Mul8(word) ^ Mul4(word) ^ Mul2(word))

//32 bit Galois Multiplication (generates bigger code than Macros)
/*static DWORD Mul(DWORD x, DWORD y)
{
	DWORD result = x, yTemp = y, log2;

	if (!y) return 0;

	for (log2 = 0; yTemp >>= 1; log2++ )
	{
		result = Mul2(result);
	}

	return result ^ Mul(x, y - (1 << log2));
}*/


void MixColumnsR(BYTE *restrict state)
{
	uint_fast8_t i = 0;
	for (; i < AES_BLOCK_WORDS; i++)
	{
		#if defined(_CRYPTO_OPENSSL) && defined(_OPENSSL_SOFTWARE) && defined(_USE_AES_FROM_OPENSSL) //Always byte swap regardless of endianess
			DWORD word = BS32(((DWORD *) state)[i]);
			((DWORD *) state)[i] = BS32(MulE(word) ^ ROR32(MulB(word), 8) ^ ROR32(MulD(word), 16) ^ ROR32(Mul9(word), 24));
		#else
			DWORD word = LE32(((DWORD *) state)[i]);
			((DWORD *) state)[i] = LE32(MulE(word) ^ ROR32(MulB(word), 8) ^ ROR32(MulD(word), 16) ^ ROR32(Mul9(word), 24));
		#endif
	}
}


static DWORD SubDword(DWORD v)
{
	BYTE *b = (BYTE *)&v;
	uint_fast8_t i = 0;

	for (; i < sizeof(DWORD); i++) b[i] = SBox[b[i]];

	return v;
}


void AesInitKey(AesCtx *Ctx, const BYTE *Key, int_fast8_t IsV6, int RijndaelKeyBytes)
{
	int RijndaelKeyDwords = RijndaelKeyBytes / sizeof(DWORD);
	Ctx->rounds = (uint_fast8_t)(RijndaelKeyDwords + 6);

	static const DWORD RCon[] = {
		0x00000000, 0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000,
		0x20000000, 0x40000000, 0x80000000, 0x1B000000, 0x36000000 };

	uint_fast8_t  i;
	DWORD  temp;

	memcpy(Ctx->Key, Key, RijndaelKeyBytes);

	for ( i = RijndaelKeyDwords; i < ( Ctx->rounds + 1 ) << 2; i++ )
	{
		temp = Ctx->Key[ i - 1 ];

		if ( ( i % RijndaelKeyDwords ) == 0 )
			temp = BE32( SubDword( ROR32( BE32(temp), 24)  ) ^ RCon[ i / RijndaelKeyDwords ] );

		Ctx->Key[ i ] = Ctx->Key[ i - RijndaelKeyDwords ] ^ temp;
	}

	if ( IsV6 )
	{
		BYTE *_p = (BYTE *)Ctx->Key;

		_p[ 4 * 16 ] ^= 0x73;
		_p[ 6 * 16 ] ^= 0x09;
		_p[ 8 * 16 ] ^= 0xE4;
	}
}


#if !defined(_CRYPTO_OPENSSL) || !defined(_USE_AES_FROM_OPENSSL) || defined(_OPENSSL_SOFTWARE)
static void SubBytes(BYTE *block)
{
	uint_fast8_t i;

	for (i = 0; i < AES_BLOCK_BYTES; i++)
		block[i] = SBox[ block[i] ];
}


static void ShiftRows(BYTE *state)
{
	BYTE bIn[AES_BLOCK_BYTES];
	uint_fast8_t i;

	memcpy(bIn, state, AES_BLOCK_BYTES);
	for (i = 0; i < AES_BLOCK_BYTES; i++)
	{
		state[i] = bIn[(i + ((i & 3) << 2)) & 0xf];
	}
};


static void MixColumns(BYTE *state)
{
	uint_fast8_t i = 0;
	for (; i < AES_BLOCK_WORDS; i++)
	{
		DWORD word = LE32(((DWORD *) state)[i]);
		((DWORD *) state)[i] = LE32(Mul2(word) ^ ROR32(Mul3(word), 8) ^ ROR32(word, 16) ^ ROR32(word, 24));
	}
}


void AesEncryptBlock(const AesCtx *const Ctx, BYTE *block)
{
	uint_fast8_t  i;

	for ( i = 0 ;; i += 4 )
	{
		AddRoundKey(block, &Ctx->Key[ i ]);
		SubBytes(block);
		ShiftRows(block);

		if ( i >= ( Ctx->rounds - 1 ) << 2 ) break;

		MixColumns(block);
	}

	AddRoundKey(block, &Ctx->Key[ Ctx->rounds << 2 ]);
}


void AesCmacV4(BYTE *Message, size_t MessageSize, BYTE *MacOut)
{
    size_t i;
    BYTE mac[AES_BLOCK_BYTES];
    AesCtx Ctx;

    AesInitKey(&Ctx, AesKeyV4, FALSE, V4_KEY_BYTES);

    memset(mac, 0, sizeof(mac));
    memset(Message + MessageSize, 0, AES_BLOCK_BYTES);
    Message[MessageSize] = 0x80;

    for (i = 0; i <= MessageSize; i += AES_BLOCK_BYTES)
    {
        XorBlock(Message + i, mac);
        AesEncryptBlock(&Ctx, mac);
    }

    memcpy(MacOut, mac, AES_BLOCK_BYTES);
}
#endif

#if !defined(_CRYPTO_OPENSSL) || !defined(_USE_AES_FROM_OPENSSL)

static const BYTE SBoxR[] = {
	0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E,
	0x81, 0xF3, 0xD7, 0xFB, 0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87,
	0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB, 0x54, 0x7B, 0x94, 0x32,
	0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
	0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49,
	0x6D, 0x8B, 0xD1, 0x25, 0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16,
	0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92, 0x6C, 0x70, 0x48, 0x50,
	0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
	0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05,
	0xB8, 0xB3, 0x45, 0x06, 0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02,
	0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B, 0x3A, 0x91, 0x11, 0x41,
	0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
	0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8,
	0x1C, 0x75, 0xDF, 0x6E, 0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89,
	0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B, 0xFC, 0x56, 0x3E, 0x4B,
	0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
	0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59,
	0x27, 0x80, 0xEC, 0x5F, 0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D,
	0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF, 0xA0, 0xE0, 0x3B, 0x4D,
	0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
	0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63,
	0x55, 0x21, 0x0C, 0x7D
};


static void ShiftRowsR(BYTE *state)
{
	BYTE b[AES_BLOCK_BYTES];
	uint_fast8_t i;

	memcpy(b, state, AES_BLOCK_BYTES);

	for (i = 0; i < AES_BLOCK_BYTES; i++)
		state[i] = b[(i - ((i & 0x3) << 2)) & 0xf];
}


static void SubBytesR(BYTE *block)
{
	uint_fast8_t i;

	for (i = 0; i < AES_BLOCK_BYTES; i++)
		block[i] = SBoxR[ block[i] ];
}


void AesEncryptCbc(const AesCtx *const Ctx, BYTE *restrict iv, BYTE *restrict data, size_t *restrict len)
{
	// Pad up to blocksize inclusive
	size_t i;
	uint_fast8_t pad = (~*len & (AES_BLOCK_BYTES - 1)) + 1;

	#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ == 8) // gcc 4.8 memset bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56977
		for (i = 0; i < pad; i++) data[*len + i] = pad;
	#else
		memset(data + *len, pad, pad);
	#endif
	*len += pad;

	if ( iv ) XorBlock(iv, data);
	AesEncryptBlock(Ctx, data);

	for (i = *len - AES_BLOCK_BYTES; i; i -= AES_BLOCK_BYTES)
	{
		XorBlock(data, data + AES_BLOCK_BYTES);
		data += AES_BLOCK_BYTES;
		AesEncryptBlock(Ctx, data);
	}
}


void AesDecryptBlock(const AesCtx *const Ctx, BYTE *block)
{
	uint_fast8_t  i;

	AddRoundKey(block, &Ctx->Key[ Ctx->rounds << 2 ]);

	for ( i = ( Ctx->rounds - 1 ) << 2 ;; i -= 4 )
	{
		ShiftRowsR(block);
		SubBytesR(block);
		AddRoundKey(block, &Ctx->Key[ i ]);

		if ( i == 0 ) break;

		MixColumnsR(block);
	}
}


void AesDecryptCbc(const AesCtx *const Ctx, BYTE *iv, BYTE *data, size_t len)
{
	BYTE  *cc;

	for (cc = data + len - AES_BLOCK_BYTES; cc > data; cc -= AES_BLOCK_BYTES)
	{
		AesDecryptBlock(Ctx, cc);
		XorBlock(cc - AES_BLOCK_BYTES, cc);
	}

	AesDecryptBlock(Ctx, cc);
	if ( iv ) XorBlock(iv, cc);
}
#endif // _CRYPTO_OPENSSL || OPENSSL_VERSION_NUMBER < 0x10000000L
#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#if !defined(_WIN32)
#include <sys/socket.h>
#endif

#include "output.h"
#include "crypto.h"
#include "endian.h"
#include "kms.h"
#include "shared_globals.h"
#include "helpers.h"

#define FRIENDLY_NAME_WINDOWS "Windows"
#define FRIENDLY_NAME_OFFICE2010 "Office 2010"
#define FRIENDLY_NAME_OFFICE2013 "Office"

#ifndef NO_BASIC_PRODUCT_LIST
// Do not change the order of this list. Append items as necessary
const KmsIdList ProductList[] = {
	/* 000 */ { { 0x212a64dc, 0x43b1, 0x4d3d, { 0xa3, 0x0c, 0x2f, 0xc6, 0x9d, 0x20, 0x95, 0xc6 } } /*"212a64dc-43b1-4d3d-a30c-2fc69d2095c6"*/, "Vista",                    EPID_WINDOWS,    4, 25 },
	/* 001 */ { { 0x7fde5219, 0xfbfa, 0x484a, { 0x82, 0xc9, 0x34, 0xd1, 0xad, 0x53, 0xe8, 0x56 } } /*"7fde5219-fbfa-484a-82c9-34d1ad53e856"*/, "Windows 7",                EPID_WINDOWS,    4, 25 },
	/* 002 */ { { 0x3c40b358, 0x5948, 0x45af, { 0x92, 0x3b, 0x53, 0xd2, 0x1f, 0xcc, 0x7e, 0x79 } } /*"3c40b358-5948-45af-923b-53d21fcc7e79"*/, "Windows 8 VL",             EPID_WINDOWS,    5, 25 },
	/* 003 */ { { 0x5f94a0bb, 0xd5a0, 0x4081, { 0xa6, 0x85, 0x58, 0x19, 0x41, 0x8b, 0x2f, 0xe0 } } /*"5f94a0bb-d5a0-4081-a685-5819418b2fe0"*/, "Windows Preview",          EPID_WINDOWS,    6, 25 },
	/* 004 */ { { 0xbbb97b3b, 0x8ca4, 0x4a28, { 0x97, 0x17, 0x89, 0xfa, 0xbd, 0x42, 0xc4, 0xac } } /*"bbb97b3b-8ca4-4a28-9717-89fabd42c4ac"*/, "Windows 8 Retail",         EPID_WINDOWS,    5, 25 },
	/* 005 */ { { 0xcb8fc780, 0x2c05, 0x495a, { 0x97, 0x10, 0x85, 0xaf, 0xff, 0xc9, 0x04, 0xd7 } } /*"cb8fc780-2c05-495a-9710-85afffc904d7"*/, "Windows 8.1 VL",           EPID_WINDOWS,    6, 25 },
	/* 006 */ { { 0x6d646890, 0x3606, 0x461a, { 0x86, 0xab, 0x59, 0x8b, 0xb8, 0x4a, 0xce, 0x82 } } /*"6d646890-3606-461a-86ab-598bb84ace82"*/, "Windows 8.1 Retail",       EPID_WINDOWS,    6, 25 },
	/* 007 */ { { 0x33e156e4, 0xb76f, 0x4a52, { 0x9f, 0x91, 0xf6, 0x41, 0xdd, 0x95, 0xac, 0x48 } } /*"33e156e4-b76f-4a52-9f91-f641dd95ac48"*/, "Windows 2008 A",           EPID_WINDOWS,    4,  5 },
	/* 008 */ { { 0x8fe53387, 0x3087, 0x4447, { 0x89, 0x85, 0xf7, 0x51, 0x32, 0x21, 0x5a, 0xc9 } } /*"8fe53387-3087-4447-8985-f75132215ac9"*/, "Windows 2008 B",           EPID_WINDOWS,    4,  5 },
	/* 009 */ { { 0x8a21fdf3, 0xcbc5, 0x44eb, { 0x83, 0xf3, 0xfe, 0x28, 0x4e, 0x66, 0x80, 0xa7 } } /*"8a21fdf3-cbc5-44eb-83f3-fe284e6680a7"*/, "Windows 2008 C",           EPID_WINDOWS,    4,  5 },
	/* 010 */ { { 0x0fc6ccaf, 0xff0e, 0x4fae, { 0x9d, 0x08, 0x43, 0x70, 0x78, 0x5b, 0xf7, 0xed } } /*"0fc6ccaf-ff0e-4fae-9d08-4370785bf7ed"*/, "Windows 2008 R2 A",        EPID_WINDOWS,    4,  5 },
	/* 011 */ { { 0xca87f5b6, 0xcd46, 0x40c0, { 0xb0, 0x6d, 0x8e, 0xcd, 0x57, 0xa4, 0x37, 0x3f } } /*"ca87f5b6-cd46-40c0-b06d-8ecd57a4373f"*/, "Windows 2008 R2 B",        EPID_WINDOWS,    4,  5 },
	/* 012 */ { { 0xb2ca2689, 0xa9a8, 0x42d7, { 0x93, 0x8d, 0xcf, 0x8e, 0x9f, 0x20, 0x19, 0x58 } } /*"b2ca2689-a9a8-42d7-938d-cf8e9f201958"*/, "Windows 2008 R2 C",        EPID_WINDOWS,    4,  5 },
	/* 013 */ { { 0x8665cb71, 0x468c, 0x4aa3, { 0xa3, 0x37, 0xcb, 0x9b, 0xc9, 0xd5, 0xea, 0xac } } /*"8665cb71-468c-4aa3-a337-cb9bc9d5eaac"*/, "Windows 2012",             EPID_WINDOWS,    5,  5 },
	/* 014 */ { { 0x8456EFD3, 0x0C04, 0x4089, { 0x87, 0x40, 0x5b, 0x72, 0x38, 0x53, 0x5a, 0x65 } } /*"8456EFD3-0C04-4089-8740-5B7238535A65"*/, "Windows 2012 R2",          EPID_WINDOWS,    6,  5 },
	/* 015 */ { { 0xe85af946, 0x2e25, 0x47b7, { 0x83, 0xe1, 0xbe, 0xbc, 0xeb, 0xea, 0xc6, 0x11 } } /*"e85af946-2e25-47b7-83e1-bebcebeac611"*/, "Office 2010",              EPID_OFFICE2010, 4,  5 },
	/* 016 */ { { 0xe6a6f1bf, 0x9d40, 0x40c3, { 0xaa, 0x9f, 0xc7, 0x7b, 0xa2, 0x15, 0x78, 0xc0 } } /*"e6a6f1bf-9d40-40c3-aa9f-c77ba21578c0"*/, "Office 2013",              EPID_OFFICE2013, 6,  5 },
	/* 017 */ { { 0x6d5f5270, 0x31ac, 0x433e, { 0xb9, 0x0a, 0x39, 0x89, 0x29, 0x23, 0xc6, 0x57 } } /*"6d5f5270-31ac-433e-b90a-39892923c657"*/, "Windows Server Preview",   EPID_WINDOWS,    6,  5 },
	/* 018 */ { { 0x85b5f61b, 0x320b, 0x4be3, { 0x81, 0x4a, 0xb7, 0x6b, 0x2b, 0xfa, 0xfc, 0x82 } } /*"85b5f61b-320b-4be3-814a-b76b2bfafc82"*/, "Office 2016",              EPID_OFFICE2013, 6,  5 },
	/* 019 */ { { 0x58e2134f, 0x8e11, 0x4d17, { 0x9c, 0xb2, 0x91, 0x06, 0x9c, 0x15, 0x11, 0x48 } } /*"58e2134f-8e11-4d17-9cb2-91069c151148"*/, "Windows 10 VL",            EPID_WINDOWS,    6, 25 },
	/* 020 */ { { 0xe1c51358, 0xfe3e, 0x4203, { 0xa4, 0xa2, 0x3b, 0x6b, 0x20, 0xc9, 0x73, 0x4e } } /*"e1c51358-fe3e-4203-a4a2-3b6b20c9734e"*/, "Windows 10 Retail",        EPID_WINDOWS,    6, 25 },
	/* 021 */ { { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, NULL, NULL, 0, 0 }
};
#endif

// Application ID is used by KMS server to count KeyManagementServiceCurrentCount
// Do not change the order of this list. Append items as necessary
const KmsIdList AppList[] = {
	/* 000 */ { { 0x55c92734, 0xd682, 0x4d71, { 0x98, 0x3e, 0xd6, 0xec, 0x3f, 0x16, 0x05, 0x9f } } /*"55C92734-D682-4D71-983E-D6EC3F16059F"*/, FRIENDLY_NAME_WINDOWS,    EPID_WINDOWS, 		0,	0},
	/* 001 */ { { 0x59A52881, 0xa989, 0x479d, { 0xaf, 0x46, 0xf2, 0x75, 0xc6, 0x37, 0x06, 0x63 } } /*"59A52881-A989-479D-AF46-F275C6370663"*/, FRIENDLY_NAME_OFFICE2010, EPID_OFFICE2010,	0,	0},
	/* 002 */ { { 0x0FF1CE15, 0xA989, 0x479D, { 0xaf, 0x46, 0xf2, 0x75, 0xc6, 0x37, 0x06, 0x63 } } /*"0FF1CE15-A989-479D-AF46-F275C6370663"*/, FRIENDLY_NAME_OFFICE2013, EPID_OFFICE2013,	0,	0},
	/* 003 */ { { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, NULL, NULL, 0, 0 }
};

#ifndef NO_EXTENDED_PRODUCT_LIST
const KmsIdList ExtendedProductList [] = {

	// Windows Server

	{ { 0xad2542d4, 0x9154, 0x4c6d, { 0x8a, 0x44, 0x30, 0xf1, 0x1e, 0xe9, 0x69, 0x89, } } /*ad2542d4-9154-4c6d-8a44-30f11ee96989*/, "Windows Server 2008 Standard",                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008A },
	{ { 0x2401e3d0, 0xc50a, 0x4b58, { 0x87, 0xb2, 0x7e, 0x79, 0x4b, 0x7d, 0x26, 0x07, } } /*2401e3d0-c50a-4b58-87b2-7e794b7d2607*/, "Windows Server 2008 Standard V",              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008A },
	{ { 0x68b6e220, 0xcf09, 0x466b, { 0x92, 0xd3, 0x45, 0xcd, 0x96, 0x4b, 0x95, 0x09, } } /*68b6e220-cf09-466b-92d3-45cd964b9509*/, "Windows Server 2008 Datacenter",              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008C },
	{ { 0xfd09ef77, 0x5647, 0x4eff, { 0x80, 0x9c, 0xaf, 0x2b, 0x64, 0x65, 0x9a, 0x45, } } /*fd09ef77-5647-4eff-809c-af2b64659a45*/, "Windows Server 2008 Datacenter V",            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008C },
	{ { 0xc1af4d90, 0xd1bc, 0x44ca, { 0x85, 0xd4, 0x00, 0x3b, 0xa3, 0x3d, 0xb3, 0xb9, } } /*c1af4d90-d1bc-44ca-85d4-003ba33db3b9*/, "Windows Server 2008 Enterprise",              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008B },
	{ { 0x8198490a, 0xadd0, 0x47b2, { 0xb3, 0xba, 0x31, 0x6b, 0x12, 0xd6, 0x47, 0xb4, } } /*8198490a-add0-47b2-b3ba-316b12d647b4*/, "Windows Server 2008 Enterprise V",            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008B  },
	{ { 0xddfa9f7c, 0xf09e, 0x40b9, { 0x8c, 0x1a, 0xbe, 0x87, 0x7a, 0x9a, 0x7f, 0x4b, } } /*ddfa9f7c-f09e-40b9-8c1a-be877a9a7f4b*/, "Windows Server 2008 Web",                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008A },
	{ { 0x7afb1156, 0x2c1d, 0x40fc, { 0xb2, 0x60, 0xaa, 0xb7, 0x44, 0x2b, 0x62, 0xfe, } } /*7afb1156-2c1d-40fc-b260-aab7442b62fe*/, "Windows Server 2008 Compute Cluster",         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008C },
	{ { 0x68531fb9, 0x5511, 0x4989, { 0x97, 0xbe, 0xd1, 0x1a, 0x0f, 0x55, 0x63, 0x3f, } } /*68531fb9-5511-4989-97be-d11a0f55633f*/, "Windows Server 2008 R2 Standard",             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2A },
	{ { 0x7482e61b, 0xc589, 0x4b7f, { 0x8e, 0xcc, 0x46, 0xd4, 0x55, 0xac, 0x3b, 0x87, } } /*7482e61b-c589-4b7f-8ecc-46d455ac3b87*/, "Windows Server 2008 R2 Datacenter",           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2C },
	{ { 0x620e2b3d, 0x09e7, 0x42fd, { 0x80, 0x2a, 0x17, 0xa1, 0x36, 0x52, 0xfe, 0x7a, } } /*620e2b3d-09e7-42fd-802a-17a13652fe7a*/, "Windows Server 2008 R2 Enterprise",           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2B },
	{ { 0xa78b8bd9, 0x8017, 0x4df5, { 0xb8, 0x6a, 0x09, 0xf7, 0x56, 0xaf, 0xfa, 0x7c, } } /*a78b8bd9-8017-4df5-b86a-09f756affa7c*/, "Windows Server 2008 R2 Web",                  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2A },
	{ { 0xcda18cf3, 0xc196, 0x46ad, { 0xb2, 0x89, 0x60, 0xc0, 0x72, 0x86, 0x99, 0x94, } } /*cda18cf3-c196-46ad-b289-60c072869994*/, "Windows Server 2008 R2 Compute Cluster",      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2008R2C },
	{ { 0xd3643d60, 0x0c42, 0x412d, { 0xa7, 0xd6, 0x52, 0xe6, 0x63, 0x53, 0x27, 0xf6, } } /*d3643d60-0c42-412d-a7d6-52e6635327f6*/, "Windows Server 2012 Datacenter",              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012 },
	{ { 0xf0f5ec41, 0x0d55, 0x4732, { 0xaf, 0x02, 0x44, 0x0a, 0x44, 0xa3, 0xcf, 0x0f, } } /*f0f5ec41-0d55-4732-af02-440a44a3cf0f*/, "Windows Server 2012 Standard",                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012 },
	{ { 0x95fd1c83, 0x7df5, 0x494a, { 0xbe, 0x8b, 0x13, 0x00, 0xe1, 0xc9, 0xd1, 0xcd, } } /*95fd1c83-7df5-494a-be8b-1300e1c9d1cd*/, "Windows Server 2012 MultiPoint Premium",      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012 },
	{ { 0x7d5486c7, 0xe120, 0x4771, { 0xb7, 0xf1, 0x7b, 0x56, 0xc6, 0xd3, 0x17, 0x0c, } } /*7d5486c7-e120-4771-b7f1-7b56c6d3170c*/, "Windows Server 2012 MultiPoint Standard",     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012 },
	{ { 0x00091344, 0x1ea4, 0x4f37, { 0xb7, 0x89, 0x01, 0x75, 0x0b, 0xa6, 0x98, 0x8c, } } /*00091344-1ea4-4f37-b789-01750ba6988c*/, "Windows Server 2012 R2 Datacenter",           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012R2 },
	{ { 0xb3ca044e, 0xa358, 0x4d68, { 0x98, 0x83, 0xaa, 0xa2, 0x94, 0x1a, 0xca, 0x99, } } /*b3ca044e-a358-4d68-9883-aaa2941aca99*/, "Windows Server 2012 R2 Standard",             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012R2 },
	{ { 0xb743a2be, 0x68d4, 0x4dd3, { 0xaf, 0x32, 0x92, 0x42, 0x5b, 0x7b, 0xb6, 0x23, } } /*b743a2be-68d4-4dd3-af32-92425b7bb623*/, "Windows Server 2012 R2 Cloud Storage",        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012R2 },
	{ { 0x21db6ba4, 0x9a7b, 0x4a14, { 0x9e, 0x29, 0x64, 0xa6, 0x0c, 0x59, 0x30, 0x1d, } } /*21db6ba4-9a7b-4a14-9e29-64a60c59301d*/, "Windows Server 2012 R2 Essentials",           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN2012R2 },
	{ { 0xba947c44, 0xd19d, 0x4786, { 0xb6, 0xae, 0x22, 0x77, 0x0b, 0xc9, 0x4c, 0x54, } } /*ba947c44-d19d-4786-b6ae-22770bc94c54*/, "Windows Server 2016 Datacenter Preview",      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_SRV_BETA },


	// Windows 10 Preview
#	ifdef INCLUDE_BETAS
	{ { 0x6496e59d, 0x89dc, 0x49eb, { 0xa3, 0x53, 0x09, 0xce, 0xb9, 0x40, 0x48, 0x45, } } /*6496e59d-89dc-49eb-a353-09ceb9404845*/, "Windows 10 Core Preview",                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
	{ { 0xa4383e6b, 0xdada, 0x423d, { 0xa4, 0x3d, 0xf2, 0x56, 0x78, 0x42, 0x96, 0x76, } } /*a4383e6b-dada-423d-a43d-f25678429676*/, "Windows 10 Professional Preview",             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
	{ { 0xcf59a07b, 0x1a2a, 0x4be0, { 0xbf, 0xe0, 0x42, 0x3b, 0x58, 0x23, 0xe6, 0x63, } } /*cf59a07b-1a2a-4be0-bfe0-423b5823e663*/, "Windows 10 Professional WMC Preview",         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
	{ { 0xcde952c7, 0x2f96, 0x4d9d, { 0x8f, 0x2b, 0x2d, 0x34, 0x9f, 0x64, 0xfc, 0x51, } } /*cde952c7-2f96-4d9d-8f2b-2d349f64fc51*/, "Windows 10 Enterprise Preview",               EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
#	endif


	// Windows 10

	{ { 0x73111121, 0x5638, 0x40f6, { 0xbc, 0x11, 0xf1, 0xd7, 0xb0, 0xd6, 0x43, 0x00, } } /*73111121-5638-40f6-bc11-f1d7b0d64300*/, "Windows 10 Enterprise",                       EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
	{ { 0xe272e3e2, 0x732f, 0x4c65, { 0xa8, 0xf0, 0x48, 0x47, 0x47, 0xd0, 0xd9, 0x47, } } /*e272e3e2-732f-4c65-a8f0-484747d0d947*/, "Windows 10 Enterprise N",                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL},
	{ { 0x7b51a46c, 0x0c04, 0x4e8f, { 0x9a, 0xf4, 0x84, 0x96, 0xcc, 0xa9, 0x0d, 0x5e, } } /*7b51a46c-0c04-4e8f-9af4-8496cca90d5e*/, "Windows 10 Enterprise LTSB",                  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL},
	{ { 0x87b838b7, 0x41b6, 0x4590, { 0x83, 0x18, 0x57, 0x97, 0x95, 0x1d, 0x85, 0x29, } } /*87b838b7-41b6-4590-8318-5797951d8529*/, "Windows 10 Enterprise LTSB N",                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL},
	{ { 0xe0c42288, 0x980c, 0x4788, { 0xa0, 0x14, 0xc0, 0x80, 0xd2, 0xe1, 0x92, 0x6e, } } /*e0c42288-980c-4788-a014-c080d2e1926e*/, "Windows 10 Education",                        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL},
	{ { 0x3c102355, 0xd027, 0x42c6, { 0xad, 0x23, 0x2e, 0x7e, 0xf8, 0xa0, 0x25, 0x85, } } /*3c102355-d027-42c6-ad23-2e7ef8a02585*/, "Windows 10 Education N",                      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL},
	{ { 0x2de67392, 0xb7a7, 0x462a, { 0xb1, 0xca, 0x10, 0x8d, 0xd1, 0x89, 0xf5, 0x88, } } /*2de67392-b7a7-462a-b1ca-108dd189f588*/, "Windows 10 Professional",                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL },
	{ { 0xa80b5abf, 0x75ad, 0x428b, { 0xb0, 0x5d, 0xa4, 0x7d, 0x2d, 0xff, 0xee, 0xbf, } } /*a80b5abf-76ad-428b-b05d-a47d2dffeebf*/, "Windows 10 Professional N",                   EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_VL},
	{ { 0x58e97c99, 0xf377, 0x4ef1, { 0x81, 0xd5, 0x4a, 0xd5, 0x52, 0x2b, 0x5f, 0xd8, } } /*58e97c99-f377-4ef1-81d5-4ad5522b5fd8*/, "Windows 10 Home",                             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_RETAIL},
	{ { 0x7b9e1751, 0xa8da, 0x4f75, { 0x95, 0x60, 0x5f, 0xad, 0xfe, 0x3d, 0x8e, 0x38, } } /*7b9e1751-a8da-4f75-9560-5fadfe3d8e38*/, "Windows 10 Home N",			               EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_RETAIL},
	{ { 0xcd918a57, 0xa41b, 0x4c82, { 0x8d, 0xce, 0x1a, 0x53, 0x8e, 0x22, 0x1a, 0x83, } } /*cd918a57-a41b-4c82-8dce-1a538e221a83*/, "Windows 10 Home Single Language",             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_RETAIL},
	{ { 0xa9107544, 0xf4a0, 0x4053, { 0xa9, 0x6a, 0x14, 0x79, 0xab, 0xde, 0xf9, 0x12, } } /*a9107544-f4a0-4053-a96a-1479abdef912*/, "Windows 10 Home Country Specific",            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN10_RETAIL},


	// Windows 8.x

#	ifdef INCLUDE_BETAS
	{ { 0x2B9C337F, 0x7A1D, 0x4271, { 0x90, 0xA3, 0xC6, 0x85, 0x5A, 0x2B, 0x8A, 0x1C, } } /*2B9C337F-7A1D-4271-90A3-C6855A2B8A1C*/, "Windows 8.x Preview",                         EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
	{ { 0x631EAD72, 0xA8AB, 0x4DF8, { 0xBB, 0xDF, 0x37, 0x20, 0x29, 0x98, 0x9B, 0xDD, } } /*631EAD72-A8AB-4DF8-BBDF-372029989BDD*/, "Windows 8.x Preview ARM",                     EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN_BETA },
#	endif
	{ { 0x81671aaf, 0x79d1, 0x4eb1, { 0xb0, 0x04, 0x8c, 0xbb, 0xe1, 0x73, 0xaf, 0xea, } } /*81671aaf-79d1-4eb1-b004-8cbbe173afea*/, "Windows 8.1 Enterprise",                      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0x113e705c, 0xfa49, 0x48a4, { 0xbe, 0xea, 0x7d, 0xd8, 0x79, 0xb4, 0x6b, 0x14, } } /*113e705c-fa49-48a4-beea-7dd879b46b14*/, "Windows 8.1 Enterprise N",                    EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0x096ce63d, 0x4fac, 0x48a9, { 0x82, 0xa9, 0x61, 0xae, 0x9e, 0x80, 0x0e, 0x5f, } } /*096ce63d-4fac-48a9-82a9-61ae9e800e5f*/, "Windows 8.1 Professional WMC",                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
	{ { 0xc06b6981, 0xd7fd, 0x4a35, { 0xb7, 0xb4, 0x05, 0x47, 0x42, 0xb7, 0xaf, 0x67, } } /*c06b6981-d7fd-4a35-b7b4-054742b7af67*/, "Windows 8.1 Professional",                    EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0x7476d79f, 0x8e48, 0x49b4, { 0xab, 0x63, 0x4d, 0x0b, 0x81, 0x3a, 0x16, 0xe4, } } /*7476d79f-8e48-49b4-ab63-4d0b813a16e4*/, "Windows 8.1 Professional N",                  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0xfe1c3238, 0x432a, 0x43a1, { 0x8e, 0x25, 0x97, 0xe7, 0xd1, 0xef, 0x10, 0xf3, } } /*fe1c3238-432a-43a1-8e25-97e7d1ef10f3*/, "Windows 8.1 Core",                            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
	{ { 0x78558a64, 0xdc19, 0x43fe, { 0xa0, 0xd0, 0x80, 0x75, 0xb2, 0xa3, 0x70, 0xa3, } } /*78558a64-dc19-43fe-a0d0-8075b2a370a3*/, "Windows 8.1 Core N",                          EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
	{ { 0xffee456a, 0xcd87, 0x4390, { 0x8e, 0x07, 0x16, 0x14, 0x6c, 0x67, 0x2f, 0xd0, } } /*ffee456a-cd87-4390-8e07-16146c672fd0*/, "Windows 8.1 Core ARM",                        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
	{ { 0xc72c6a1d, 0xf252, 0x4e7e, { 0xbd, 0xd1, 0x3f, 0xca, 0x34, 0x2a, 0xcb, 0x35, } } /*c72c6a1d-f252-4e7e-bdd1-3fca342acb35*/, "Windows 8.1 Core Single Language",            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
	{ { 0xdb78b74f, 0xef1c, 0x4892, { 0xab, 0xfe, 0x1e, 0x66, 0xb8, 0x23, 0x1d, 0xf6, } } /*db78b74f-ef1c-4892-abfe-1e66b8231df6*/, "Windows 8.1 Core Country Specific",           EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
	{ { 0xe9942b32, 0x2e55, 0x4197, { 0xb0, 0xbd, 0x5f, 0xf5, 0x8c, 0xba, 0x88, 0x60, } } /*e9942b32-2e55-4197-b0bd-5ff58cba8860*/, "Windows 8.1 Core Connected",                  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0xc6ddecd6, 0x2354, 0x4c19, { 0x90, 0x9b, 0x30, 0x6a, 0x30, 0x58, 0x48, 0x4e, } } /*c6ddecd6-2354-4c19-909b-306a3058484e*/, "Windows 8.1 Core Connected N",                EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0xb8f5e3a3, 0xed33, 0x4608, { 0x81, 0xe1, 0x37, 0xd6, 0xc9, 0xdc, 0xfd, 0x9c, } } /*b8f5e3a3-ed33-4608-81e1-37d6c9dcfd9c*/, "Windows 8.1 Core Connected Single Language",  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0xba998212, 0x460a, 0x44db, { 0xbf, 0xb5, 0x71, 0xbf, 0x09, 0xd1, 0xc6, 0x8b, } } /*ba998212-460a-44db-bfb5-71bf09d1c68b*/, "Windows 8.1 Core Connected Country Specific", EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0xe58d87b5, 0x8126, 0x4580, { 0x80, 0xfb, 0x86, 0x1b, 0x22, 0xf7, 0x92, 0x96, } } /*e58d87b5-8126-4580-80fb-861b22f79296*/, "Windows 8.1 Professional Student",            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
	{ { 0xcab491c7, 0xa918, 0x4f60, { 0xb5, 0x02, 0xda, 0xb7, 0x5e, 0x33, 0x4f, 0x40, } } /*cab491c7-a918-4f60-b502-dab75e334f40*/, "Windows 8.1 Professional Student N",          EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_RETAIL },
	{ { 0xa00018a3, 0xf20f, 0x4632, { 0xbf, 0x7c, 0x8d, 0xaa, 0x53, 0x51, 0xc9, 0x14, } } /*a00018a3-f20f-4632-bf7c-8daa5351c914*/, "Windows 8 Professional WMC",                  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },
	{ { 0xa98bcd6d, 0x5343, 0x4603, { 0x8a, 0xfe, 0x59, 0x08, 0xe4, 0x61, 0x11, 0x12, } } /*a98bcd6d-5343-4603-8afe-5908e4611112*/, "Windows 8 Professional",                      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },
	{ { 0xebf245c1, 0x29a8, 0x4daf, { 0x9c, 0xb1, 0x38, 0xdf, 0xc6, 0x08, 0xa8, 0xc8, } } /*ebf245c1-29a8-4daf-9cb1-38dfc608a8c8*/, "Windows 8 Professional N",                    EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },
	{ { 0x458e1bec, 0x837a, 0x45f6, { 0xb9, 0xd5, 0x92, 0x5e, 0xd5, 0xd2, 0x99, 0xde, } } /*458e1bec-837a-45f6-b9d5-925ed5d299de*/, "Windows 8 Enterprise",                        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },
	{ { 0xe14997e7, 0x800a, 0x4cf7, { 0xad, 0x10, 0xde, 0x4b, 0x45, 0xb5, 0x78, 0xdb, } } /*e14997e7-800a-4cf7-ad10-de4b45b578db*/, "Windows 8 Enterprise N",                      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_VL },
	{ { 0xc04ed6bf, 0x55c8, 0x4b47, { 0x9f, 0x8e, 0x5a, 0x1f, 0x31, 0xce, 0xee, 0x60, } } /*c04ed6bf-55c8-4b47-9f8e-5a1f31ceee60*/, "Windows 8 Core",                              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },
	{ { 0x197390a0, 0x65f6, 0x4a95, { 0xbd, 0xc4, 0x55, 0xd5, 0x8a, 0x3b, 0x02, 0x53, } } /*197390a0-65f6-4a95-bdc4-55d58a3b0253*/, "Windows 8 Core N",                            EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },
	{ { 0x9d5584a2, 0x2d85, 0x419a, { 0x98, 0x2c, 0xa0, 0x08, 0x88, 0xbb, 0x9d, 0xdf, } } /*9d5584a2-2d85-419a-982c-a00888bb9ddf*/, "Windows 8 Core Country Specific",             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },
	{ { 0x8860fcd4, 0xa77b, 0x4a20, { 0x90, 0x45, 0xa1, 0x50, 0xff, 0x11, 0xd6, 0x09, } } /*8860fcd4-a77b-4a20-9045-a150ff11d609*/, "Windows 8 Core Single Language",              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN8_RETAIL },


	// Windows 7

	{ { 0xae2ee509, 0x1b34, 0x41c0, { 0xac, 0xb7, 0x6d, 0x46, 0x50, 0x16, 0x89, 0x15, } } /*ae2ee509-1b34-41c0-acb7-6d4650168915*/, "Windows 7 Enterprise",                        EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
	{ { 0x1cb6d605, 0x11b3, 0x4e14, { 0xbb, 0x30, 0xda, 0x91, 0xc8, 0xe3, 0x98, 0x3a, } } /*1cb6d605-11b3-4e14-bb30-da91c8e3983a*/, "Windows 7 Enterprise N",                      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
	{ { 0xb92e9980, 0xb9d5, 0x4821, { 0x9c, 0x94, 0x14, 0x0f, 0x63, 0x2f, 0x63, 0x12, } } /*b92e9980-b9d5-4821-9c94-140f632f6312*/, "Windows 7 Professional",                      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
	{ { 0x54a09a0d, 0xd57b, 0x4c10, { 0x8b, 0x69, 0xa8, 0x42, 0xd6, 0x59, 0x0a, 0xd5, } } /*54a09a0d-d57b-4c10-8b69-a842d6590ad5*/, "Windows 7 Professional N",                    EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },


	// Windows Vista

	{ { 0xcfd8ff08, 0xc0d7, 0x452b, { 0x9f, 0x60, 0xef, 0x5c, 0x70, 0xc3, 0x20, 0x94, } } /*cfd8ff08-c0d7-452b-9f60-ef5c70c32094*/, "Windows Vista Enterprise",                    EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_VISTA },
	{ { 0xd4f54950, 0x26f2, 0x4fb4, { 0xba, 0x21, 0xff, 0xab, 0x16, 0xaf, 0xca, 0xde, } } /*d4f54950-26f2-4fb4-ba21-ffab16afcade*/, "Windows Vista Enterprise N",                  EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_VISTA },
	{ { 0x4f3d1606, 0x3fea, 0x4c01, { 0xbe, 0x3c, 0x8d, 0x67, 0x1c, 0x40, 0x1e, 0x3b, } } /*4f3d1606-3fea-4c01-be3c-8d671c401e3b*/, "Windows Vista Business",                      EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_VISTA },
	{ { 0x2c682dc2, 0x8b68, 0x4f63, { 0xa1, 0x65, 0xae, 0x29, 0x1d, 0x4c, 0xf1, 0x38, } } /*2c682dc2-8b68-4f63-a165-ae291d4cf138*/, "Windows Vista Business N",                    EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_VISTA },


	// Windows Embedded

	{ { 0xaa6dd3aa, 0xc2b4, 0x40e2, { 0xa5, 0x44, 0xa6, 0xbb, 0xb3, 0xf5, 0xc3, 0x95, } } /*aa6dd3aa-c2b4-40e2-a544-a6bbb3f5c395*/, "Windows ThinPC",                              EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
	{ { 0xdb537896, 0x376f, 0x48ae, { 0xa4, 0x92, 0x53, 0xd0, 0x54, 0x77, 0x73, 0xd0, } } /*db537896-376f-48ae-a492-53d0547773d0*/, "Windows Embedded POSReady 7",                 EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN7 },
	{ { 0x0ab82d54, 0x47f4, 0x4acb, { 0x81, 0x8c, 0xcc, 0x5b, 0xf0, 0xec, 0xb6, 0x49, } } /*0ab82d54-47f4-4acb-818c-cc5bf0ecb649*/, "Windows Embedded Industry 8.1",               EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0xcd4e2d9f, 0x5059, 0x4a50, { 0xa9, 0x2d, 0x05, 0xd5, 0xbb, 0x12, 0x67, 0xc7, } } /*cd4e2d9f-5059-4a50-a92d-05d5bb1267c7*/, "Windows Embedded Industry E 8.1",             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },
	{ { 0xf7e88590, 0xdfc7, 0x4c78, { 0xbc, 0xcb, 0x6f, 0x38, 0x65, 0xb9, 0x9d, 0x1a, } } /*f7e88590-dfc7-4c78-bccb-6f3865b99d1a*/, "Windows Embedded Industry A 8.1",             EPID_WINDOWS,    APP_ID_WINDOWS,    KMS_ID_WIN81_VL },

	// Office 2010

	{ { 0x8ce7e872, 0x188c, 0x4b98, { 0x9d, 0x90, 0xf8, 0xf9, 0x0b, 0x7a, 0xad, 0x02, } } /*8ce7e872-188c-4b98-9d90-f8f90b7aad02*/, "Office Access 2010",                          EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0xcee5d470, 0x6e3b, 0x4fcc, { 0x8c, 0x2b, 0xd1, 0x74, 0x28, 0x56, 0x8a, 0x9f, } } /*cee5d470-6e3b-4fcc-8c2b-d17428568a9f*/, "Office Excel 2010",                           EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0x8947d0b8, 0xc33b, 0x43e1, { 0x8c, 0x56, 0x9b, 0x67, 0x4c, 0x05, 0x28, 0x32, } } /*8947d0b8-c33b-43e1-8c56-9b674c052832*/, "Office Groove 2010",                          EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0xca6b6639, 0x4ad6, 0x40ae, { 0xa5, 0x75, 0x14, 0xde, 0xe0, 0x7f, 0x64, 0x30, } } /*ca6b6639-4ad6-40ae-a575-14dee07f6430*/, "Office InfoPath 2010",                        EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0x09ed9640, 0xf020, 0x400a, { 0xac, 0xd8, 0xd7, 0xd8, 0x67, 0xdf, 0xd9, 0xc2, } } /*09ed9640-f020-400a-acd8-d7d867dfd9c2*/, "Office Mondo 2010",                           EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0xef3d4e49, 0xa53d, 0x4d81, { 0xa2, 0xb1, 0x2c, 0xa6, 0xc2, 0x55, 0x6b, 0x2c, } } /*ef3d4e49-a53d-4d81-a2b1-2ca6c2556b2c*/, "Office Mondo 2010",                           EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0xab586f5c, 0x5256, 0x4632, { 0x96, 0x2f, 0xfe, 0xfd, 0x8b, 0x49, 0xe6, 0xf4, } } /*ab586f5c-5256-4632-962f-fefd8b49e6f4*/, "Office OneNote 2010",                         EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0xecb7c192, 0x73ab, 0x4ded, { 0xac, 0xf4, 0x23, 0x99, 0xb0, 0x95, 0xd0, 0xcc, } } /*ecb7c192-73ab-4ded-acf4-2399b095d0cc*/, "Office OutLook 2010",                         EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0x45593b1d, 0xdfb1, 0x4e91, { 0xbb, 0xfb, 0x2d, 0x5d, 0x0c, 0xe2, 0x22, 0x7a, } } /*45593b1d-dfb1-4e91-bbfb-2d5d0ce2227a*/, "Office PowerPoint 2010",                      EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0xdf133ff7, 0xbf14, 0x4f95, { 0xaf, 0xe3, 0x7b, 0x48, 0xe7, 0xe3, 0x31, 0xef, } } /*df133ff7-bf14-4f95-afe3-7b48e7e331ef*/, "Office Project Pro 2010",                     EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0x5dc7bf61, 0x5ec9, 0x4996, { 0x9c, 0xcb, 0xdf, 0x80, 0x6a, 0x2d, 0x0e, 0xfe, } } /*5dc7bf61-5ec9-4996-9ccb-df806a2d0efe*/, "Office Project Standard 2010",                EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0xb50c4f75, 0x599b, 0x43e8, { 0x8d, 0xcd, 0x10, 0x81, 0xa7, 0x96, 0x72, 0x41, } } /*b50c4f75-599b-43e8-8dcd-1081a7967241*/, "Office Publisher 2010",                       EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0x92236105, 0xbb67, 0x494f, { 0x94, 0xc7, 0x7f, 0x7a, 0x60, 0x79, 0x29, 0xbd, } } /*92236105-bb67-494f-94c7-7f7a607929bd*/, "Office Visio Premium 2010",                   EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0xe558389c, 0x83c3, 0x4b29, { 0xad, 0xfe, 0x5e, 0x4d, 0x7f, 0x46, 0xc3, 0x58, } } /*e558389c-83c3-4b29-adfe-5e4d7f46c358*/, "Office Visio Pro 2010",                       EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0x9ed833ff, 0x4f92, 0x4f36, { 0xb3, 0x70, 0x86, 0x83, 0xa4, 0xf1, 0x32, 0x75, } } /*9ed833ff-4f92-4f36-b370-8683a4f13275*/, "Office Visio Standard 2010",                  EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0x2d0882e7, 0xa4e7, 0x423b, { 0x8c, 0xcc, 0x70, 0xd9, 0x1e, 0x01, 0x58, 0xb1, } } /*2d0882e7-a4e7-423b-8ccc-70d91e0158b1*/, "Office Word 2010",                            EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0x6f327760, 0x8c5c, 0x417c, { 0x9b, 0x61, 0x83, 0x6a, 0x98, 0x28, 0x7e, 0x0c, } } /*6f327760-8c5c-417c-9b61-836a98287e0c*/, "Office Professional Plus 2010",               EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0x9da2a678, 0xfb6b, 0x4e67, { 0xab, 0x84, 0x60, 0xdd, 0x6a, 0x9c, 0x81, 0x9a, } } /*9da2a678-fb6b-4e67-ab84-60dd6a9c819a*/, "Office Standard 2010",                        EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },
	{ { 0xea509e87, 0x07a1, 0x4a45, { 0x9e, 0xdc, 0xeb, 0xa5, 0xa3, 0x9f, 0x36, 0xaf, } } /*ea509e87-07a1-4a45-9edc-eba5a39f36af*/, "Office Small Business Basics 2010",           EPID_OFFICE2010, APP_ID_OFFICE2010, KMS_ID_OFFICE2010 },

	// Office 2013

	{ { 0x6ee7622c, 0x18d8, 0x4005, { 0x9f, 0xb7, 0x92, 0xdb, 0x64, 0x4a, 0x27, 0x9b, } } /*6ee7622c-18d8-4005-9fb7-92db644a279b*/, "Office Access 2013",                          EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0xf7461d52, 0x7c2b, 0x43b2, { 0x87, 0x44, 0xea, 0x95, 0x8e, 0x0b, 0xd0, 0x9a, } } /*f7461d52-7c2b-43b2-8744-ea958e0bd09a*/, "Office Excel 2013",                           EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0xa30b8040, 0xd68a, 0x423f, { 0xb0, 0xb5, 0x9c, 0xe2, 0x92, 0xea, 0x5a, 0x8f, } } /*a30b8040-d68a-423f-b0b5-9ce292ea5a8f*/, "Office InfoPath 2013",                        EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0x1b9f11e3, 0xc85c, 0x4e1b, { 0xbb, 0x29, 0x87, 0x9a, 0xd2, 0xc9, 0x09, 0xe3, } } /*1b9f11e3-c85c-4e1b-bb29-879ad2c909e3*/, "Office Lync 2013",                            EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0xdc981c6b, 0xfc8e, 0x420f, { 0xaa, 0x43, 0xf8, 0xf3, 0x3e, 0x5c, 0x09, 0x23, } } /*dc981c6b-fc8e-420f-aa43-f8f33e5c0923*/, "Office Mondo 2013",                           EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0xefe1f3e6, 0xaea2, 0x4144, { 0xa2, 0x08, 0x32, 0xaa, 0x87, 0x2b, 0x65, 0x45, } } /*efe1f3e6-aea2-4144-a208-32aa872b6545*/, "Office OneNote 2013",                         EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0x771c3afa, 0x50c5, 0x443f, { 0xb1, 0x51, 0xff, 0x25, 0x46, 0xd8, 0x63, 0xa0, } } /*771c3afa-50c5-443f-b151-ff2546d863a0*/, "Office OutLook 2013",                         EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0x8c762649, 0x97d1, 0x4953, { 0xad, 0x27, 0xb7, 0xe2, 0xc2, 0x5b, 0x97, 0x2e, } } /*8c762649-97d1-4953-ad27-b7e2c25b972e*/, "Office PowerPoint 2013",                      EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0x4a5d124a, 0xe620, 0x44ba, { 0xb6, 0xff, 0x65, 0x89, 0x61, 0xb3, 0x3b, 0x9a, } } /*4a5d124a-e620-44ba-b6ff-658961b33b9a*/, "Office Project Pro 2013",                     EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0x427a28d1, 0xd17c, 0x4abf, { 0xb7, 0x17, 0x32, 0xc7, 0x80, 0xba, 0x6f, 0x07, } } /*427a28d1-d17c-4abf-b717-32c780ba6f07*/, "Office Project Standard 2013",                EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0x00c79ff1, 0x6850, 0x443d, { 0xbf, 0x61, 0x71, 0xcd, 0xe0, 0xde, 0x30, 0x5f, } } /*00c79ff1-6850-443d-bf61-71cde0de305f*/, "Office Publisher 2013",                       EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0xac4efaf0, 0xf81f, 0x4f61, { 0xbd, 0xf7, 0xea, 0x32, 0xb0, 0x2a, 0xb1, 0x17, } } /*ac4efaf0-f81f-4f61-bdf7-ea32b02ab117*/, "Office Visio Standard 2013",                  EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0xe13ac10e, 0x75d0, 0x4aff, { 0xa0, 0xcd, 0x76, 0x49, 0x82, 0xcf, 0x54, 0x1c, } } /*e13ac10e-75d0-4aff-a0cd-764982cf541c*/, "Office Visio Pro 2013",                       EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0xd9f5b1c6, 0x5386, 0x495a, { 0x88, 0xf9, 0x9a, 0xd6, 0xb4, 0x1a, 0xc9, 0xb3, } } /*d9f5b1c6-5386-495a-88f9-9ad6b41ac9b3*/, "Office Word 2013",                            EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0xb322da9c, 0xa2e2, 0x4058, { 0x9e, 0x4e, 0xf5, 0x9a, 0x69, 0x70, 0xbd, 0x69, } } /*b322da9c-a2e2-4058-9e4e-f59a6970bd69*/, "Office Professional Plus 2013",               EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },
	{ { 0xb13afb38, 0xcd79, 0x4ae5, { 0x9f, 0x7f, 0xee, 0xd0, 0x58, 0xd7, 0x50, 0xca, } } /*b13afb38-cd79-4ae5-9f7f-eed058d750ca*/, "Office Standard 2013",                        EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2013 },

	// Office 2016

	{ { 0xd450596f, 0x894d, 0x49e0, { 0x96, 0x6a, 0xfd, 0x39, 0xed, 0x4c, 0x4c, 0x64, } } /*d450596f-894d-49e0-966a-fd39ed4c4c64*/, "Office Professional Plus 2016",               EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0x4f414197, 0x0fc2, 0x4c01, { 0xb6, 0x8a, 0x86, 0xcb, 0xb9, 0xac, 0x25, 0x4c, } } /*4f414197-0fc2-4c01-b68a-86cbb9ac254c*/, "Office Project Pro 2016",                     EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0x6bf301c1, 0xb94a, 0x43e9, { 0xba, 0x31, 0xd4, 0x94, 0x59, 0x8c, 0x47, 0xfb, } } /*6bf301c1-b94a-43e9-ba31-d494598c47fb*/, "Office Visio Pro 2016",                       EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0x041a06cb, 0xc5b8, 0x4772, { 0x80, 0x9f, 0x41, 0x6d, 0x03, 0xd1, 0x66, 0x54, } } /*041a06cb-c5b8-4772-809f-416d03d16654*/, "Office Publisher 2016",                       EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0x67c0fc0c, 0xdeba, 0x401b, { 0xbf, 0x8b, 0x9c, 0x8a, 0xd8, 0x39, 0x58, 0x04, } } /*67c0fc0c-deba-401b-bf8b-9c8ad8395804*/, "Office Access 2016",                          EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0x83e04ee1, 0xfa8d, 0x436d, { 0x89, 0x94, 0xd3, 0x1a, 0x86, 0x2c, 0xab, 0x77, } } /*83e04ee1-fa8d-436d-8994-d31a862cab77*/, "Office Skype for Business 2016",              EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0x9caabccb, 0x61b1, 0x4b4b, { 0x8b, 0xec, 0xd1, 0x0a, 0x3c, 0x3a, 0xc2, 0xce, } } /*9caabccb-61b1-4b4b-8bec-d10a3c3ac2ce*/, "Office Mondo 2016",                           EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0xaa2a7821, 0x1827, 0x4c2c, { 0x8f, 0x1d, 0x45, 0x13, 0xa3, 0x4d, 0xda, 0x97, } } /*aa2a7821-1827-4c2c-8f1d-4513a34dda97*/, "Office Visio Standard 2016",                  EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0xbb11badf, 0xd8aa, 0x470e, { 0x93, 0x11, 0x20, 0xea, 0xf8, 0x0f, 0xe5, 0xcc, } } /*bb11badf-d8aa-470e-9311-20eaf80fe5cc*/, "Office Word 2016",                            EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0xc3e65d36, 0x141f, 0x4d2f, { 0xa3, 0x03, 0xa8, 0x42, 0xee, 0x75, 0x6a, 0x29, } } /*c3e65d36-141f-4d2f-a303-a842ee756a29*/, "Office Excel 2016",                           EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0xd70b1bba, 0xb893, 0x4544, { 0x96, 0xe2, 0xb7, 0xa3, 0x18, 0x09, 0x1c, 0x33, } } /*d70b1bba-b893-4544-96e2-b7a318091c33*/, "Office Powerpoint 2016",                      EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0xd8cace59, 0x33d2, 0x4ac7, { 0x9b, 0x1b, 0x9b, 0x72, 0x33, 0x9c, 0x51, 0xc8, } } /*d8cace59-33d2-4ac7-9b1b-9b72339c51c8*/, "Office OneNote 2016",                         EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0xda7ddabc, 0x3fbe, 0x4447, { 0x9e, 0x01, 0x6a, 0xb7, 0x44, 0x0b, 0x4c, 0xd4, } } /*da7ddabc-3fbe-4447-9e01-6ab7440b4cd4*/, "Office Project Standard 2016",                EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0xdedfa23d, 0x6ed1, 0x45a6, { 0x85, 0xdc, 0x63, 0xca, 0xe0, 0x54, 0x6d, 0xe6, } } /*dedfa23d-6ed1-45a6-85dc-63cae0546de6*/, "Office Standard 2016",                        EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0xe914ea6e, 0xa5fa, 0x4439, { 0xa3, 0x94, 0xa9, 0xbb, 0x32, 0x93, 0xca, 0x09, } } /*e914ea6e-a5fa-4439-a394-a9bb3293ca09*/, "Office Mondo R 2016",                         EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },
	{ { 0xec9d9265, 0x9d1e, 0x4ed0, { 0x83, 0x8a, 0xcd, 0xc2, 0x0f, 0x25, 0x51, 0xa1, } } /*ec9d9265-9d1e-4ed0-838a-cdc20f2551a1*/, "Office Outlook 2016",                         EPID_OFFICE2013, APP_ID_OFFICE2013, KMS_ID_OFFICE2016 },

	// End marker (necessity should be removed when time permits)

	{ { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, NULL, NULL, 0, 0 }
};


// necessary because other .c files cannot access _countof()
__pure ProdListIndex_t getExtendedProductListSize(void)
{
	return _countof(ExtendedProductList) - 1;
}

__pure ProdListIndex_t getAppListSize(void)
{
	return _countof(AppList);
}

#endif

#ifndef NO_RANDOM_EPID
// HostType and OSBuild
static const struct KMSHostOS { uint16_t Type; uint16_t Build; } HostOS[] =
{
	{ 55041, 6002 }, // Windows Server 2008 SP2
    { 55041, 7601 }, // Windows Server 2008 R2 SP1
    {  5426, 9200 }, // Windows Server 2012
    {  6401, 9600 }, // Windows Server 2012 R2
	{  3612, 10240}, // Windows Server 2016
};

// GroupID and PIDRange
static const struct PKEYCONFIG { uint16_t GroupID; uint32_t RangeMin; uint32_t RangeMax; } pkeyconfig[] = {
    { 206, 152000000, 191999999 }, // Windows Server 2012 KMS Host pkeyconfig
    { 206, 271000000, 310999999 }, // Windows Server 2012 R2 KMS Host pkeyconfig
    {  96, 199000000, 217999999 }, // Office2010 KMS Host pkeyconfig
    { 206, 234000000, 255999999 }, // Office2013 KMS Host pkeyconfig
};

// Valid language identifiers to be used in the ePID
static const uint16_t LcidList[] = {
	1078, 1052, 1025, 2049, 3073, 4097, 5121, 6145, 7169, 8193, 9217, 10241, 11265, 12289, 13313, 14337, 15361, 16385,
	1067, 1068, 2092, 1069, 1059, 1093, 5146, 1026, 1027, 1028, 2052, 3076, 4100, 5124, 1050, 4122, 1029, 1030, 1125, 1043, 2067,
	1033, 2057, 3081, 4105, 5129, 6153, 7177, 8201, 9225, 10249, 11273, 12297, 13321, 1061, 1080, 1065, 1035, 1036, 2060,
	3084, 4108, 5132, 6156, 1079, 1110, 1031, 2055, 3079, 4103, 5127, 1032, 1095, 1037, 1081, 1038, 1039, 1057, 1040, 2064, 1041, 1099,
	1087, 1111, 1042, 1088, 1062, 1063, 1071, 1086, 2110, 1100, 1082, 1153, 1102, 1104, 1044, 2068, 1045, 1046, 2070,
	1094, 1131, 2155, 3179, 1048, 1049, 9275, 4155, 5179, 3131, 1083, 2107, 8251, 6203, 7227, 1103, 2074, 6170, 3098,
	7194, 1051, 1060, 1034, 2058, 3082, 4106, 5130, 6154, 7178, 8202, 9226, 10250, 11274, 12298, 13322, 14346, 15370, 16394,
	17418, 18442, 19466, 20490, 1089, 1053, 2077, 1114, 1097, 1092, 1098, 1054, 1074, 1058, 1056, 1091, 2115, 1066, 1106, 1076, 1077
};


#ifdef _PEDANTIC
uint16_t IsValidLcid(const uint16_t Lcid)
{
	uint16_t i;

	for (i = 0; i < _countof(LcidList); i++)
	{
		if (Lcid == LcidList[i]) return Lcid;
	}

	return 0;
}
#endif // _PEDANTIC
#endif // NO_RANDOM_EPID


// Unix time is seconds from 1970-01-01. Should be 64 bits to avoid Year 2035 overflow bug.
// FILETIME is 100 nanoseconds from 1601-01-01. Must be 64 bits.
void getUnixTimeAsFileTime(FILETIME *const ts)
{
	int64_t unixtime = (int64_t)time(NULL);
	int64_t *filetime = (int64_t*)ts;

	*filetime = LE64( (unixtime + 11644473600LL) * 10000000LL );
}

__pure int64_t fileTimeToUnixTime(const FILETIME *const ts)
{
	return LE64( *((const int64_t *const)ts) ) / 10000000LL - 11644473600LL;
}


/*
 * Get's a product name with a GUID in host-endian order.
 * List can be any list defined above.
 */
const char* getProductNameHE(const GUID *const guid, const KmsIdList *const List, ProdListIndex_t *const i)
{
	for (*i = 0; List[*i].name != NULL; (*i)++)
	{
		if (IsEqualGUID(guid, &List[*i].guid))
			return List[*i].name;
	}

	return "Unknown";
}


/*
 * same as getProductnameHE except GUID is in little-endian (network) order
 */
const char* getProductNameLE(const GUID *const guid, const KmsIdList *const List, ProdListIndex_t *const i)
{
	#if __BYTE_ORDER != __LITTLE_ENDIAN
	GUID HeGUID;
	LEGUID(&HeGUID, guid);
	return getProductNameHE(&HeGUID, List, i);
	#else
	return getProductNameHE(guid, List, i);
	#endif
}


#ifndef NO_RANDOM_EPID
// formats an int with a fixed number of digits with leading zeros (helper for ePID generation)
static char* itoc(char *const c, const int i, uint_fast8_t digits)
{
	char formatString[8];
	if (digits > 9) digits = 0;
	strcpy(formatString,"%");

	if (digits)
	{
		formatString[1] = '0';
		formatString[2] = digits | 0x30;
		formatString[3] = 0;
	}

	strcat(formatString, "u");
	sprintf(c, formatString, i);
	return c;
}

static int getRandomServerType()
{
#	ifndef USE_MSRPC
	if (!UseRpcBTFN)
#	endif // USE_MSRPC
	{
		// This isn't possible at all, e.g. KMS host on XP
		return rand() % (int)_countof(HostOS);
	}
#	ifndef USE_MSRPC
	else
	{
		// return 9200/9600/10240 if NDR64 is in use, otherwise 6002/7601
		if (UseRpcNDR64) return (rand() % 3) + 2;
		return (rand() % 2);
	}
#	endif // USE_MSRPC
}


/*
 * Generates a random ePID
 */
static void generateRandomPid(const int index, char *const szPid, int serverType, int16_t lang)
{
	int clientApp;
	char numberBuffer[12];

	if (serverType < 0 || serverType >= (int)_countof(HostOS))
	{
		serverType = getRandomServerType();
	}

	strcpy(szPid, itoc(numberBuffer, HostOS[serverType].Type, 5));
	strcat(szPid, "-");

	if (index == 2)
		clientApp = 3;
	else if (index == 1)
		clientApp = 2;
	else
		clientApp = serverType == 3 /*change if HostOS changes*/ ? 1 : 0;

	strcat(szPid, itoc(numberBuffer, pkeyconfig[clientApp].GroupID, 5));
	strcat(szPid, "-");

	int keyId = (rand32() % (pkeyconfig[clientApp].RangeMax - pkeyconfig[clientApp].RangeMin)) + pkeyconfig[clientApp].RangeMin;
	strcat(szPid, itoc(numberBuffer, keyId / 1000000, 3));
	strcat(szPid, "-");
	strcat(szPid, itoc(numberBuffer, keyId % 1000000, 6));
	strcat(szPid, "-03-");

	if (lang < 0) lang = LcidList[rand() % _countof(LcidList)];
	strcat(szPid, itoc(numberBuffer, lang, 0));
	strcat(szPid, "-");

	strcat(szPid, itoc(numberBuffer, HostOS[serverType].Build, 0));
	strcat(szPid, ".0000-");

#	define minTime ((time_t)1436958000) // Release Date Windows 10 RTM Escrow

	time_t maxTime, kmsTime;
	time(&maxTime);

	if (maxTime < minTime) // Just in case the system time is < 07/15/2015 1:00 pm
		maxTime = (time_t)BUILD_TIME;

	kmsTime = (rand32() % (maxTime - minTime)) + minTime;
#	undef minTime

	struct tm *pidTime;
	pidTime = gmtime(&kmsTime);

	strcat(szPid, itoc(numberBuffer, pidTime->tm_yday, 3));
	strcat(szPid, itoc(numberBuffer, pidTime->tm_year + 1900, 4));
}


/*
 * Generates random ePIDs and stores them if not already read from ini file.
 * For use with randomization level 1
 */
void randomPidInit()
{
	ProdListIndex_t i;

	int serverType = getRandomServerType();
	int16_t lang   = Lcid ? Lcid : LcidList[rand() % _countof(LcidList)];

	for (i = 0; i < _countof(AppList) - 1; i++)
	{
		if (KmsResponseParameters[i].Epid) continue;

		char Epid[PID_BUFFER_SIZE];

		generateRandomPid(i, Epid, serverType, lang);
		KmsResponseParameters[i].Epid = (const char*)vlmcsd_malloc(strlen(Epid) + 1);

		strcpy((char*)KmsResponseParameters[i].Epid, Epid);

		#ifndef NO_LOG
		KmsResponseParameters[i].EpidSource = "randomized at program start";
		#endif // NO_LOG
	}
}

#endif // NO_RANDOM_EPID


#ifndef NO_LOG
/*
 * Logs a Request
 */
static void logRequest(const REQUEST *const baseRequest)
{
	const char *productName;
	char clientname[64];
	ProdListIndex_t index;

	#ifndef NO_EXTENDED_PRODUCT_LIST
	productName = getProductNameLE(&baseRequest->ActID, ExtendedProductList, &index);
	if (++index >= (int)_countof(ExtendedProductList))
	#endif // NO_EXTENDED_PRODUCT_LIST
	{
		#ifndef NO_BASIC_PRODUCT_LIST
		productName = getProductNameLE(&baseRequest->KMSID, ProductList, &index);
		if (++index >= (int)_countof(ProductList))
		#endif // NO_BASIC_PRODUCT_LIST
		{
			productName = getProductNameLE(&baseRequest->AppID, AppList, &index);
		}
	}

	#ifndef NO_VERBOSE_LOG
	if (logverbose)
	{
		logger("<<< Incoming KMS request\n");
		logRequestVerbose(baseRequest, &logger);
	}
	else
	{
	#endif // NO_VERBOSE_LOG
		ucs2_to_utf8(baseRequest->WorkstationName, clientname, 64, 64);
		logger("KMS v%i.%i request from %s for %s\n", LE16(baseRequest->MajorVer), LE16(baseRequest->MinorVer), clientname, productName);
	#ifndef NO_VERBOSE_LOG
	}
	#endif // NO_VERBOSE_LOG
}
#endif // NO_LOG


/*
 * Converts a utf-8 ePID string to UCS-2 and writes it to a RESPONSE struct
 */
static void getEpidFromString(RESPONSE *const Response, const char *const pid)
{
	size_t length = utf8_to_ucs2(Response->KmsPID, pid, PID_BUFFER_SIZE, PID_BUFFER_SIZE * 3);
	Response->PIDSize = LE32(((unsigned int )length + 1) << 1);
}


/*
 * get ePID from appropriate source
 */
static void getEpid(RESPONSE *const baseResponse, const char** EpidSource, const ProdListIndex_t index, BYTE *const HwId)
{
	const char* pid;
	if (KmsResponseParameters[index].Epid == NULL)
	{
		#ifndef NO_RANDOM_EPID
		if (RandomizationLevel == 2)
		{
			char szPid[PID_BUFFER_SIZE];
			generateRandomPid(index, szPid, -1, Lcid ? Lcid : -1);
			pid = szPid;

			#ifndef NO_LOG
			*EpidSource = "randomized on every request";
			#endif // NO_LOG
		}
		else
		#endif // NO_RANDOM_EPID
		{
			pid = AppList[index].pid;
			#ifndef NO_LOG
			*EpidSource = "vlmcsd default";
			#endif // NO_LOG
		}
	}
	else
	{
		pid = KmsResponseParameters[index].Epid;

		if (HwId && KmsResponseParameters[index].HwId != NULL)
			memcpy(HwId, KmsResponseParameters[index].HwId, sizeof(((RESPONSE_V6 *)0)->HwId));

		#ifndef NO_LOG
		*EpidSource = KmsResponseParameters[index].EpidSource;
		#endif // NO_LOG
	}
	getEpidFromString(baseResponse, pid);
}


#if !defined(NO_LOG) && defined(_PEDANTIC)
static BOOL CheckVersion4Uuid(const GUID *const guid, const char *const szGuidName)
{
	if (LE16(guid->Data3) >> 12 != 4 || guid->Data4[0] >> 6 != 2)
	{
		logger("Warning: %s does not conform to version 4 UUID according to RFC 4122\n", szGuidName);
		return FALSE;
	}
	return TRUE;
}


static void CheckRequest(const REQUEST *const Request)
{
	CheckVersion4Uuid(&Request->CMID, "Client machine ID");
	CheckVersion4Uuid(&Request->AppID, "Application ID");
	CheckVersion4Uuid(&Request->KMSID, "Server SKU ID");
	CheckVersion4Uuid(&Request->ActID, "Client SKU ID");

	if (LE32(Request->IsClientVM) > 1)
		logger("Warning: Virtual Machine field in request must be 0 or 1 but is %u\n", LE32(Request->IsClientVM));

	if (LE32(Request->LicenseStatus) > 6 )
		logger("Warning: License status must be between 0 and 6 but is %u\n", LE32(Request->LicenseStatus));
}
#endif // !defined(NO_LOG) && defined(_PEDANTIC)


#ifndef NO_LOG
/*
 * Logs the Response
 */
static void logResponse(const RESPONSE *const baseResponse, const BYTE *const hwId, const char *const EpidSource)
{
	char utf8pid[PID_BUFFER_SIZE * 3];
	ucs2_to_utf8(baseResponse->KmsPID, utf8pid, PID_BUFFER_SIZE, PID_BUFFER_SIZE * 3);

	#ifndef NO_VERBOSE_LOG
	if (!logverbose)
	{
	#endif // NO_VERBOSE_LOG
		logger("Sending ePID (%s): %s\n", EpidSource, utf8pid);
	#ifndef NO_VERBOSE_LOG
	}
	else
	{
		logger(">>> Sending response, ePID source = %s\n", EpidSource);
		logResponseVerbose(utf8pid, hwId, baseResponse, &logger);
	}
	#endif // NO_VERBOSE_LOG

}
#endif


/*
 * Creates the unencrypted base response
 */
static BOOL __stdcall CreateResponseBaseCallback(const REQUEST *const baseRequest, RESPONSE *const baseResponse, BYTE *const hwId, const char* const ipstr)
{
	const char* EpidSource;
	#ifndef NO_LOG
	logRequest(baseRequest);
	#ifdef _PEDANTIC
	CheckRequest(baseRequest);
	#endif // _PEDANTIC
	#endif // NO_LOG

	ProdListIndex_t index;

	getProductNameLE(&baseRequest->AppID, AppList, &index);

	if (index >= _countof(AppList) - 1) index = 0; //default to Windows

	getEpid(baseResponse, &EpidSource, index, hwId);

	baseResponse->Version = baseRequest->Version;

	memcpy(&baseResponse->CMID, &baseRequest->CMID, sizeof(GUID));
	memcpy(&baseResponse->ClientTime, &baseRequest->ClientTime, sizeof(FILETIME));

	baseResponse->Count  				= LE32(LE32(baseRequest->N_Policy) << 1);
	baseResponse->VLActivationInterval	= LE32(VLActivationInterval);
	baseResponse->VLRenewalInterval   	= LE32(VLRenewalInterval);

	#ifndef NO_LOG
	logResponse(baseResponse, hwId, EpidSource);
	#endif // NO_LOG

	return !0;
}

RequestCallback_t CreateResponseBase = &CreateResponseBaseCallback;

////TODO: Move to helpers.c
void get16RandomBytes(void* ptr)
{
	int i;
	for (i = 0; i < 4; i++)	((DWORD*)ptr)[i] = rand32();
}


/*
 * Creates v4 response
 */
size_t CreateResponseV4(REQUEST_V4 *const request_v4, BYTE *const responseBuffer, const char* const ipstr)
{
	RESPONSE_V4* Response = (RESPONSE_V4*)responseBuffer;

	if ( !CreateResponseBase(&request_v4->RequestBase, &Response->ResponseBase, NULL, ipstr) ) return 0;

	DWORD pidSize = LE32(Response->ResponseBase.PIDSize);
	BYTE* postEpidPtr =	responseBuffer + V4_PRE_EPID_SIZE + pidSize;
	memmove(postEpidPtr, &Response->ResponseBase.CMID, V4_POST_EPID_SIZE);

	size_t encryptSize = V4_PRE_EPID_SIZE + V4_POST_EPID_SIZE + pidSize;
	AesCmacV4(responseBuffer, encryptSize, responseBuffer + encryptSize);

	return encryptSize + sizeof(Response->MAC);
}

/*
// Workaround for buggy GCC 4.2/4.3
#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 4)
__attribute__((noinline))
#endif
__pure static uint64_t TimestampInterval(void *ts)
{
	return ( GET_UA64LE(ts) / TIME_C1 ) * TIME_C2 + TIME_C3;
}*/


/*
 * Creates the HMAC for v6
 */
static int_fast8_t CreateV6Hmac(BYTE *const encrypt_start, const size_t encryptSize, int_fast8_t tolerance)
{
	BYTE hash[32];
#	define halfHashSize (sizeof(hash) >> 1)
	uint64_t timeSlot;
	BYTE *responseEnd = encrypt_start + encryptSize;

	// This is the time from the response
	FILETIME* ft = (FILETIME*)(responseEnd - V6_POST_EPID_SIZE + sizeof(((RESPONSE*)0)->CMID));

	// Generate a time slot that changes every 4.11 hours.
	// Request and repsonse time must match +/- 1 slot.
	// When generating a response tolerance must be 0.
	// If verifying the hash, try tolerance -1, 0 and +1. One of them must match.

	timeSlot = LE64( (GET_UA64LE(ft) / TIME_C1 * TIME_C2 + TIME_C3) + (tolerance * TIME_C1) );

	// The time slot is hashed with SHA256 so it is not so obvious that it is time
	Sha256((BYTE*) &timeSlot, sizeof(timeSlot), hash);

	// The last 16 bytes of the hashed time slot are the actual HMAC key
	if (!Sha256Hmac
	(
		hash + halfHashSize,								// Use last 16 bytes of SHA256 as HMAC key
		encrypt_start,										// hash only the encrypted part of the v6 response
		encryptSize - sizeof(((RESPONSE_V6*)0)->HMAC),		// encryptSize minus the HMAC itself
		hash												// use same buffer for resulting hash where the key came from
	))
	{
		return FALSE;
	}

	memcpy(responseEnd - sizeof(((RESPONSE_V6*)0)->HMAC), hash + halfHashSize, halfHashSize);
	return TRUE;
#	undef halfHashSize
}


/*
 * Creates v5 or v6 response
 */
size_t CreateResponseV6(REQUEST_V6 *restrict request_v6, BYTE *const responseBuffer, const char* const ipstr)
{
	// The response will be created in a fixed sized struct to
	// avoid unaligned access macros and packed structs on RISC systems
	// which largely increase code size.
	//
	// The fixed sized struct with 64 WCHARs for the ePID will be converted
	// to a variable sized struct later and requires unaligned access macros.

	RESPONSE_V6* Response = (RESPONSE_V6*)responseBuffer;
	RESPONSE* baseResponse = &Response->ResponseBase;

	#ifdef _DEBUG
		RESPONSE_V6_DEBUG* xxx = (RESPONSE_V6_DEBUG*)responseBuffer;
	#endif

	static const BYTE DefaultHwid[8] = { HWID };
	int_fast8_t v6 = LE16(request_v6->MajorVer) > 5;
	AesCtx aesCtx;

	AesInitKey(&aesCtx, v6 ? AesKeyV6 : AesKeyV5, v6, AES_KEY_BYTES);
	AesDecryptCbc(&aesCtx, NULL, request_v6->IV, V6_DECRYPT_SIZE);

	// get random salt and SHA256 it
	get16RandomBytes(Response->RandomXoredIVs);
	Sha256(Response->RandomXoredIVs, sizeof(Response->RandomXoredIVs), Response->Hash);

	if (v6) // V6 specific stuff
	{
		// In v6 a random IV is generated
		Response->Version = request_v6->Version;
		get16RandomBytes(Response->IV);

		// pre-fill with default HwId (not required for v5)
		memcpy(Response->HwId, DefaultHwid, sizeof(Response->HwId));

        // Just copy decrypted request IV (using Null IV) here. Note this is identical
        // to XORing non-decrypted request and reponse IVs
		memcpy(Response->XoredIVs, request_v6->IV, sizeof(Response->XoredIVs));
	}
	else // V5 specific stuff
	{
		// In v5 IVs of request and response must be identical (MS client checks this)
		// The following memcpy copies Version and IVs at once
		memcpy(Response, request_v6, V6_UNENCRYPTED_SIZE);
	}

	// Xor Random bytes with decrypted request IV
	XorBlock(request_v6->IV, Response->RandomXoredIVs);

	// Get the base response
	if ( !CreateResponseBase(&request_v6->RequestBase, baseResponse, Response->HwId, ipstr) ) return 0;

	// Convert the fixed sized struct into variable sized
	DWORD pidSize = LE32(baseResponse->PIDSize);
	BYTE* postEpidPtr =	responseBuffer + V6_PRE_EPID_SIZE + pidSize;
	size_t post_epid_size = v6 ? V6_POST_EPID_SIZE : V5_POST_EPID_SIZE;

	memmove(postEpidPtr, &baseResponse->CMID, post_epid_size);

	// number of bytes to encrypt
	size_t encryptSize =
		V6_PRE_EPID_SIZE
		- sizeof(Response->Version)
		+ pidSize
		+ post_epid_size;

	//AesDecryptBlock(&aesCtx, Response->IV);
	if (v6 && !CreateV6Hmac(Response->IV, encryptSize, 0)) return 0;

	// Padding auto handled by encryption func
	AesEncryptCbc(&aesCtx, NULL, Response->IV, &encryptSize);

	return encryptSize + sizeof(Response->Version);
}


// Create Hashed KMS Client Request Data for KMS Protocol Version 4
BYTE *CreateRequestV4(size_t *size, const REQUEST* requestBase)
{
	*size = sizeof(REQUEST_V4);

	// Build a proper KMS client request data
	BYTE *request = (BYTE *)vlmcsd_malloc(sizeof(REQUEST_V4));

	// Temporary Pointer for access to REQUEST_V4 structure
	REQUEST_V4 *request_v4 = (REQUEST_V4 *)request;

	// Set KMS Client Request Base
	memcpy(&request_v4->RequestBase, requestBase, sizeof(REQUEST));

	// Generate Hash Signature
	AesCmacV4(request, sizeof(REQUEST), request_v4->MAC);

	// Return Request Data
	return request;
}


// Create Encrypted KMS Client Request Data for KMS Protocol Version 6
BYTE* CreateRequestV6(size_t *size, const REQUEST* requestBase)
{
	*size = sizeof(REQUEST_V6);

	// Temporary Pointer for access to REQUEST_V5 structure
	REQUEST_V6 *request = (REQUEST_V6 *)vlmcsd_malloc(sizeof(REQUEST_V6));

	// KMS Protocol Version
	request->Version = requestBase->Version;

	// Initialize the IV
	get16RandomBytes(request->IV);

	// Set KMS Client Request Base
	memcpy(&request->RequestBase, requestBase, sizeof(REQUEST));

	// Encrypt KMS Client Request
	size_t encryptSize = sizeof(request->RequestBase);
	AesCtx Ctx;
	int_fast8_t v6 = LE16(request->MajorVer) > 5;
	AesInitKey(&Ctx, v6 ? AesKeyV6 : AesKeyV5, v6, 16);
	AesEncryptCbc(&Ctx, request->IV, (BYTE*)(&request->RequestBase), &encryptSize);

	// Return Proper Request Data
	return (BYTE*)request;
}


/*
 * Checks whether Length of ePID is valid
 */
static uint8_t checkPidLength(const RESPONSE *const responseBase)
{
	unsigned int i;

	if (LE32(responseBase->PIDSize) > (PID_BUFFER_SIZE << 1)) return FALSE;
	if (responseBase->KmsPID[(LE32(responseBase->PIDSize) >> 1) - 1]) return FALSE;

	for (i = 0; i < (LE32(responseBase->PIDSize) >> 1) - 2; i++)
	{
		if (!responseBase->KmsPID[i]) return FALSE;
	}

	return TRUE;
}


/*
 * "Decrypts" a KMS v4 response. Actually just copies to a fixed size buffer
 */
RESPONSE_RESULT DecryptResponseV4(RESPONSE_V4* response_v4, const int responseSize, BYTE* const rawResponse, const BYTE* const rawRequest)
{
	int copySize =
		V4_PRE_EPID_SIZE +
		(LE32(((RESPONSE_V4*)rawResponse)->ResponseBase.PIDSize) <= PID_BUFFER_SIZE << 1 ?
		LE32(((RESPONSE_V4*)rawResponse)->ResponseBase.PIDSize) :
		PID_BUFFER_SIZE << 1);

	int messageSize = copySize + V4_POST_EPID_SIZE;

	memcpy(response_v4, rawResponse, copySize);
	memcpy(&response_v4->ResponseBase.CMID, rawResponse + copySize, responseSize - copySize);

	// ensure PID is null terminated
	response_v4->ResponseBase.KmsPID[PID_BUFFER_SIZE-1] = 0;

	uint8_t* mac = rawResponse + messageSize;
	AesCmacV4(rawResponse, messageSize, mac);

	REQUEST_V4* request_v4 = (REQUEST_V4*)rawRequest;
	RESPONSE_RESULT result;

	result.mask					 = (DWORD)~0;
	result.PidLengthOK			 = checkPidLength((RESPONSE*)rawResponse);
	result.VersionOK			 = response_v4->ResponseBase.Version == request_v4->RequestBase.Version;
	result.HashOK				 = !memcmp(&response_v4->MAC, mac, sizeof(response_v4->MAC));
	result.TimeStampOK			 = !memcmp(&response_v4->ResponseBase.ClientTime, &request_v4->RequestBase.ClientTime, sizeof(FILETIME));
	result.ClientMachineIDOK	 = !memcmp(&response_v4->ResponseBase.CMID, &request_v4->RequestBase.CMID, sizeof(GUID));
	result.effectiveResponseSize = responseSize;
	result.correctResponseSize	 = sizeof(RESPONSE_V4) - sizeof(response_v4->ResponseBase.KmsPID) + LE32(response_v4->ResponseBase.PIDSize);

	return result;
}


static RESPONSE_RESULT VerifyResponseV6(RESPONSE_RESULT result, const AesCtx* Ctx,	RESPONSE_V6* response_v6, REQUEST_V6* request_v6, BYTE* const rawResponse)
{
	// Check IVs
	result.IVsOK = !memcmp // In V6 the XoredIV is actually the request IV
	(
		response_v6->XoredIVs,
		request_v6->IV,
		sizeof(response_v6->XoredIVs)
	);

	result.IVnotSuspicious = !!memcmp // If IVs are identical, it is obviously an emulator
	(
		request_v6->IV,
		response_v6->IV,
		sizeof(request_v6->IV)
	);

	// Check Hmac
	int_fast8_t tolerance;
	BYTE OldHmac[sizeof(response_v6->HMAC)];

	result.HmacSha256OK = FALSE;

	memcpy	// Save received HMAC to compare with calculated HMAC later
	(
		OldHmac,
		response_v6->HMAC,
		sizeof(response_v6->HMAC)
	);

	//AesEncryptBlock(Ctx, Response_v6->IV); // CreateV6Hmac needs original IV as received over the network

	for (tolerance = -1; tolerance < 2; tolerance++)
	{
		CreateV6Hmac
		(
			rawResponse + sizeof(response_v6->Version),					// Pointer to start of the encrypted part of the response
			(size_t)result.correctResponseSize - V6_UNENCRYPTED_SIZE,   // size of the encrypted part
			tolerance													// tolerance -1, 0, or +1
		);

		if
		((
			result.HmacSha256OK = !memcmp // Compare both HMACs
			(
				OldHmac,
				rawResponse + (size_t)result.correctResponseSize - sizeof(response_v6->HMAC),
				sizeof(OldHmac)
			)
		))
		{
			break;
		}
	}
	return result;
}


static RESPONSE_RESULT VerifyResponseV5(RESPONSE_RESULT result, REQUEST_V5* request_v5, RESPONSE_V5* response_v5)
{
	// Check IVs: in V5 (and only v5) request and response IVs must match
	result.IVsOK = !memcmp(request_v5->IV, response_v5->IV,	sizeof(request_v5->IV));

	// V5 has no Hmac, always set to TRUE
	result.HmacSha256OK = TRUE;

	return result;
}


/*
 * Decrypts a KMS v5 or v6 response received from a server.
 * hwid must supply a valid 16 byte buffer for v6. hwid is ignored in v5
 */
RESPONSE_RESULT DecryptResponseV6(RESPONSE_V6* response_v6, int responseSize, BYTE* const response, const BYTE* const rawRequest, BYTE* hwid)
{
	RESPONSE_RESULT result;
	result.mask = ~0; // Set all bits in the results mask to 1. Assume success first.
	result.effectiveResponseSize = responseSize;

	int copySize1 =
		sizeof(response_v6->Version);

	// Decrypt KMS Server Response (encrypted part starts after RequestIV)
	responseSize -= copySize1;

	AesCtx Ctx;
	int_fast8_t v6 = LE16(((RESPONSE_V6*)response)->MajorVer) > 5;

	AesInitKey(&Ctx, v6 ? AesKeyV6 : AesKeyV5, v6, AES_KEY_BYTES);
	AesDecryptCbc(&Ctx, NULL, response + copySize1, responseSize);

	// Check padding
	BYTE* lastPadByte = response + (size_t)result.effectiveResponseSize - 1;

	// Must be from 1 to 16
	if (!*lastPadByte || *lastPadByte > AES_BLOCK_BYTES)
	{
		result.DecryptSuccess = FALSE;
		return result;
	}

	// Check if pad bytes are all the same
	BYTE* padByte;
	for (padByte = lastPadByte - *lastPadByte + 1; padByte < lastPadByte; padByte++)
	if (*padByte != *lastPadByte)
	{
		result.DecryptSuccess = FALSE;
		return result;
	}

	// Add size of Version, KmsPIDLen and variable size PID
	DWORD pidSize = LE32(((RESPONSE_V6*) response)->ResponseBase.PIDSize);

	copySize1 +=
		V6_UNENCRYPTED_SIZE	 +
		sizeof(response_v6->ResponseBase.PIDSize) +
		(pidSize <= PID_BUFFER_SIZE << 1 ?	pidSize : PID_BUFFER_SIZE << 1);

	// Copy part 1 of response up to variable sized PID
	memcpy(response_v6, response, copySize1);

	// ensure PID is null terminated
	response_v6->ResponseBase.KmsPID[PID_BUFFER_SIZE - 1] = 0;

	// Copy part 2
	size_t copySize2 = v6 ? V6_POST_EPID_SIZE : V5_POST_EPID_SIZE;
	memcpy(&response_v6->ResponseBase.CMID, response + copySize1, copySize2);

	// Decrypting the response is finished here. Now we check the results for validity
	// A basic client doesn't need the stuff below this comment but we want to use vlmcs
	// as a debug tool for KMS emulators.

	REQUEST_V6* request_v6 = (REQUEST_V6*) rawRequest;
	DWORD decryptSize = sizeof(request_v6->IV) + sizeof(request_v6->RequestBase) + sizeof(request_v6->Pad);

	AesDecryptCbc(&Ctx, NULL, request_v6->IV, decryptSize);

	// Check that all version informations are the same
	result.VersionOK =
		request_v6->Version == response_v6->ResponseBase.Version &&
		request_v6->Version == response_v6->Version &&
		request_v6->Version == request_v6->RequestBase.Version;

	// Check Base Request
	result.PidLengthOK			= checkPidLength(&((RESPONSE_V6*) response)->ResponseBase);
	result.TimeStampOK			= !memcmp(&response_v6->ResponseBase.ClientTime, &request_v6->RequestBase.ClientTime, sizeof(FILETIME));
	result.ClientMachineIDOK	= IsEqualGUID(&response_v6->ResponseBase.CMID, &request_v6->RequestBase.CMID);

	// Rebuild Random Key and Sha256 Hash
	BYTE HashVerify[sizeof(response_v6->Hash)];
	BYTE RandomKey[sizeof(response_v6->RandomXoredIVs)];

	memcpy(RandomKey, request_v6->IV, sizeof(RandomKey));
	XorBlock(response_v6->RandomXoredIVs, RandomKey);
	Sha256(RandomKey, sizeof(RandomKey), HashVerify);

	result.HashOK = !memcmp(response_v6->Hash, HashVerify, sizeof(HashVerify));

	// size before encryption (padding not included)
	result.correctResponseSize =
		(v6 ? sizeof(RESPONSE_V6) : sizeof(RESPONSE_V5))
		- sizeof(response_v6->ResponseBase.KmsPID)
		+ LE32(response_v6->ResponseBase.PIDSize);

	// Version specific stuff
	if (v6)
	{
		// Copy the HwId
		memcpy(hwid, response_v6->HwId, sizeof(response_v6->HwId));

		// Verify the V6 specific part of the response
		result = VerifyResponseV6(result, &Ctx, response_v6, request_v6, response);
	}
	else // V5
	{
		// Verify the V5 specific part of the response
		result = VerifyResponseV5(result, request_v6, (RESPONSE_V5*)response_v6);
	}

	// padded size after encryption
	result.correctResponseSize += (~(result.correctResponseSize - sizeof(response_v6->ResponseBase.Version)) & 0xf) + 1;

	return result;
}

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "endian.h"

#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && defined(__LITTLE_ENDIAN)  \
	&& defined(BS16) && defined(BS32) && defined(BS64)

#else // ! defined(__BYTE_ORDER)

void PUT_UAA64BE(void *p, unsigned long long v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned long long *)p)[i];
	_p[ 0 ] = v >> 56;
	_p[ 1 ] = v >> 48;
	_p[ 2 ] = v >> 40;
	_p[ 3 ] = v >> 32;
	_p[ 4 ] = v >> 24;
	_p[ 5 ] = v >> 16;
	_p[ 6 ] = v >> 8;
	_p[ 7 ] = v;
}

void PUT_UAA32BE(void *p, unsigned int v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned int *)p)[i];
	_p[ 0 ] = v >> 24;
	_p[ 1 ] = v >> 16;
	_p[ 2 ] = v >> 8;
	_p[ 3 ] = v;
}

void PUT_UAA16BE(void *p, unsigned short v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned short *)p)[i];
	_p[ 0 ] = v >> 8;
	_p[ 1 ] = v;
}


void PUT_UAA64LE(void *p, unsigned long long v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned long long *)p)[i];
	_p[ 0 ] = v;
	_p[ 1 ] = v >> 8;
	_p[ 2 ] = v >> 16;
	_p[ 3 ] = v >> 24;
	_p[ 4 ] = v >> 32;
	_p[ 5 ] = v >> 40;
	_p[ 6 ] = v >> 48;
	_p[ 7 ] = v >> 56;
}

void PUT_UAA32LE(void *p, unsigned int v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned int *)p)[i];
	_p[ 0 ] = v;
	_p[ 1 ] = v >> 8;
	_p[ 2 ] = v >> 16;
	_p[ 3 ] = v >> 24;
}

void PUT_UAA16LE(void *p, unsigned short v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned short *)p)[i];
	_p[ 0 ] = v;
	_p[ 1 ] = v >> 8;
}


unsigned long long GET_UAA64BE(void *p, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned long long *)p)[i];
	return
		(unsigned long long)_p[ 0 ] << 56 |
		(unsigned long long)_p[ 1 ] << 48 |
		(unsigned long long)_p[ 2 ] << 40 |
		(unsigned long long)_p[ 3 ] << 32 |
		(unsigned long long)_p[ 4 ] << 24 |
		(unsigned long long)_p[ 5 ] << 16 |
		(unsigned long long)_p[ 6 ] << 8  |
		(unsigned long long)_p[ 7 ];

}

unsigned int GET_UAA32BE(void *p, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned int *)p)[i];
	return
		(unsigned int)_p[ 0 ] << 24 |
		(unsigned int)_p[ 1 ] << 16 |
		(unsigned int)_p[ 2 ] << 8  |
		(unsigned int)_p[ 3 ];
}

unsigned short GET_UAA16BE(void *p, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned short *)p)[i];
	return
		(unsigned short)_p[ 0 ] << 8 |
		(unsigned short)_p[ 1 ];
}


unsigned long long GET_UAA64LE(void *p, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned long long *)p)[i];
	return
		(unsigned long long)_p[ 0 ] |
		(unsigned long long)_p[ 1 ] << 8  |
		(unsigned long long)_p[ 2 ] << 16 |
		(unsigned long long)_p[ 3 ] << 24 |
		(unsigned long long)_p[ 4 ] << 32 |
		(unsigned long long)_p[ 5 ] << 40 |
		(unsigned long long)_p[ 6 ] << 48 |
		(unsigned long long)_p[ 7 ] << 56;

}

unsigned int GET_UAA32LE(void *p, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned int *)p)[i];
	return
		(unsigned int)_p[ 0 ] |
		(unsigned int)_p[ 1 ] << 8  |
		(unsigned int)_p[ 2 ] << 16 |
		(unsigned int)_p[ 3 ] << 24;
}

unsigned short GET_UAA16LE(void *p, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned short *)p)[i];
	return
		(unsigned short)_p[ 0 ] |
		(unsigned short)_p[ 1 ] << 8;
}


unsigned short BE16(unsigned short x)
{
	return GET_UAA16BE(&x, 0);
}

unsigned short LE16(unsigned short x)
{
	return GET_UAA16LE(&x, 0);
}

unsigned int BE32(unsigned int x)
{
	return GET_UAA32BE(&x, 0);
}

unsigned int LE32(unsigned int x)
{
	return GET_UAA32LE(&x, 0);
}

unsigned long long BE64(unsigned long long x)
{
	return GET_UAA64BE(&x, 0);
}

inline unsigned long long LE64(unsigned long long x)
{
	return GET_UAA64LE(&x, 0);
}

#endif // defined(__BYTE_ORDER)
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif // _DEFAULT_SOURCE

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "output.h"
#include "shared_globals.h"
#include "endian.h"
#include "helpers.h"

#ifndef NO_LOG
static void vlogger(const char *message, va_list args)
{
	FILE *log;

	#ifdef _NTSERVICE
	if (!IsNTService && logstdout) log = stdout;
	#else
	if (logstdout) log = stdout;
	#endif
	else
	{
		if (fn_log == NULL) return;

		#ifndef _WIN32
		if (!strcmp(fn_log, "syslog"))
		{
			openlog("vlmcsd", LOG_CONS | LOG_PID, LOG_USER);

			////PORTABILITY: vsyslog is not in Posix but virtually all Unixes have it
			vsyslog(LOG_INFO, message, args);

			closelog();
			return;
		}
		#endif // _WIN32

		log = fopen(fn_log, "a");
		if ( !log ) return;
	}

	time_t now = time(0);

	#ifdef USE_THREADS
	char mbstr[2048];
	#else
	char mbstr[24];
	#endif

	strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %X", localtime(&now));

	#ifndef USE_THREADS

	fprintf(log, "%s: ", mbstr);
	vfprintf(log, message, args);
	fflush(log);

	#else // USE_THREADS

	// We write everything to a string before we really log inside the critical section
	// so formatting the output can be concurrent
	strcat(mbstr, ": ");
	int len = strlen(mbstr);
	vsnprintf(mbstr + len, sizeof(mbstr) - len, message, args);

	lock_mutex(&logmutex);
	fputs(mbstr, log);
	fflush(log);
	unlock_mutex(&logmutex);

	#endif // USE_THREADS
	if (log != stdout) fclose(log);
}


// Always sends to log output
int logger(const char *const fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogger(fmt, args);
	va_end(args);
	return 0;
}

#endif //NO_LOG


// Output to stderr if it is available or to log otherwise (e.g. if running as daemon/service)
void printerrorf(const char *const fmt, ...)
{
	va_list arglist;

	va_start(arglist, fmt);

	#ifndef NO_LOG
	#ifdef _NTSERVICE
	if (InetdMode || IsNTService)
	#else // !_NTSERVICE
	if (InetdMode)
	#endif // NTSERVIICE
		vlogger(fmt, arglist);
	else
	#endif //NO_LOG
	{
		vfprintf(stderr, fmt, arglist);
		fflush(stderr);
	}

	va_end(arglist);
}


// Always output to stderr
int errorout(const char* fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	int i = vfprintf(stderr, fmt, args);
	va_end(args);
	fflush(stderr);

	return i;
}


static const char *LicenseStatusText[] =
{
	"Unlicensed", "Licensed", "OOB grace", "OOT grace", "Non-Genuine", "Notification", "Extended grace"
};


void uuid2StringLE(const GUID *const guid, char *const string)
{
	sprintf(string,
				#ifdef _WIN32
				"%08x-%04x-%04x-%04x-%012I64x",
				#else
				"%08x-%04x-%04x-%04x-%012llx",
				#endif
				(unsigned int)LE32( guid->Data1 ),
				(unsigned int)LE16( guid->Data2 ),
				(unsigned int)LE16( guid->Data3 ),
				(unsigned int)BE16( *(uint16_t*)guid->Data4 ),
				(unsigned long long)BE64(*(uint64_t*)(guid->Data4)) & 0xffffffffffffLL
				);
}


void logRequestVerbose(const REQUEST *const Request, const PRINTFUNC p)
{
	char guidBuffer[GUID_STRING_LENGTH + 1];
	char WorkstationBuffer[3 * WORKSTATION_NAME_BUFFER];
	const char *productName;
	ProdListIndex_t index;

	p("Protocol version                : %u.%u\n", LE16(Request->MajorVer), LE16(Request->MinorVer));
	p("Client is a virtual machine     : %s\n", LE32(Request->VMInfo) ? "Yes" : "No");
	p("Licensing status                : %u (%s)\n", (uint32_t)LE32(Request->LicenseStatus), LE32(Request->LicenseStatus) < _countof(LicenseStatusText) ? LicenseStatusText[LE32(Request->LicenseStatus)] : "Unknown");
	p("Remaining time (0 = forever)    : %i minutes\n", (uint32_t)LE32(Request->BindingExpiration));

	uuid2StringLE(&Request->AppID, guidBuffer);
	productName = getProductNameLE(&Request->AppID, AppList, &index);
	p("Application ID                  : %s (%s)\n", guidBuffer, productName);

	uuid2StringLE(&Request->ActID, guidBuffer);

	#ifndef NO_EXTENDED_PRODUCT_LIST
	productName = getProductNameLE(&Request->ActID, ExtendedProductList, &index);
	#else
	productName = "Unknown";
	#endif

	p("Activation ID (Product)         : %s (%s)\n", guidBuffer, productName);

	uuid2StringLE(&Request->KMSID, guidBuffer);

	#ifndef NO_BASIC_PRODUCT_LIST
	productName = getProductNameLE(&Request->KMSID, ProductList, &index);
	#else
	productName = "Unknown";
	#endif

	p("Key Management Service ID       : %s (%s)\n", guidBuffer, productName);

	uuid2StringLE(&Request->CMID, guidBuffer);
	p("Client machine ID               : %s\n", guidBuffer);

	uuid2StringLE(&Request->CMID_prev, guidBuffer);
	p("Previous client machine ID      : %s\n", guidBuffer);


	char mbstr[64];
	time_t st;
	st = fileTimeToUnixTime(&Request->ClientTime);
	strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %X", gmtime(&st));
	p("Client request timestamp (UTC)  : %s\n", mbstr);

	ucs2_to_utf8(Request->WorkstationName, WorkstationBuffer, WORKSTATION_NAME_BUFFER, sizeof(WorkstationBuffer));

	p("Workstation name                : %s\n", WorkstationBuffer);
	p("N count policy (minimum clients): %u\n", (uint32_t)LE32(Request->N_Policy));
}


void logResponseVerbose(const char *const ePID, const BYTE *const hwid, const RESPONSE *const response, const PRINTFUNC p)
{
	char guidBuffer[GUID_STRING_LENGTH + 1];
	//SYSTEMTIME st;

	p("Protocol version                : %u.%u\n", (uint32_t)LE16(response->MajorVer), (uint32_t)LE16(response->MinorVer));
	p("KMS host extended PID           : %s\n", ePID);
	if (LE16(response->MajorVer) > 5)
#	ifndef _WIN32
	p("KMS host Hardware ID            : %016llX\n", (unsigned long long)BE64(*(uint64_t*)hwid));
#	else // _WIN32
	p("KMS host Hardware ID            : %016I64X\n", (unsigned long long)BE64(*(uint64_t*)hwid));
#	endif // WIN32

	uuid2StringLE(&response->CMID, guidBuffer);
	p("Client machine ID               : %s\n", guidBuffer);

	char mbstr[64];
	time_t st;

	st = fileTimeToUnixTime(&response->ClientTime);
	strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %X", gmtime(&st));
	p("Client request timestamp (UTC)  : %s\n", mbstr);

	p("KMS host current active clients : %u\n", (uint32_t)LE32(response->Count));
	p("Renewal interval policy         : %u\n", (uint32_t)LE32(response->VLRenewalInterval));
	p("Activation interval policy      : %u\n", (uint32_t)LE32(response->VLActivationInterval));
}

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
char *fn_log = NULL;
int_fast8_t logstdout = 0;
#ifndef NO_VERBOSE_LOG
int_fast8_t logverbose = 0;
#endif // NO_VERBOSE_LOG
#endif // NO_LOG

#ifndef NO_SOCKETS
int_fast8_t nodaemon = 0;
int_fast8_t InetdMode = 0;
#else
int_fast8_t nodaemon = 1;
int_fast8_t InetdMode = 1;
#endif

#ifndef NO_RANDOM_EPID
int_fast8_t RandomizationLevel = 1;
uint16_t Lcid = 0;
#endif

#ifndef NO_SOCKETS
SOCKET *SocketList;
int numsockets = 0;

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





/*
 * Helper functions used by other modules
 */

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifndef _WIN32
#include <errno.h>
#endif // _WIN32
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "helpers.h"
#include "output.h"
#include "endian.h"
#include "shared_globals.h"


/*
 *  UCS2 <-> UTF-8 functions
 *  All functions use little endian UCS2 since we only need it to communicate with Windows via RPC
 */

// Convert one character from UTF-8 to UCS2
// Returns 0xffff, if utf-8 evaluates to > 0xfffe (outside basic multilingual pane)
WCHAR utf8_to_ucs2_char (const unsigned char *input, const unsigned char **end_ptr)
{
    *end_ptr = input;
    if (input[0] == 0)
        return ~0;

    if (input[0] < 0x80) {
        *end_ptr = input + 1;
        return LE16(input[0]);
    }

    if ((input[0] & 0xE0) == 0xE0) {

    	if (input[1] == 0 || input[2] == 0)
            return ~0;

    	*end_ptr = input + 3;

    	return
            LE16((input[0] & 0x0F)<<12 |
            (input[1] & 0x3F)<<6  |
            (input[2] & 0x3F));
    }

    if ((input[0] & 0xC0) == 0xC0) {
        if (input[1] == 0)
            return ~0;

        *end_ptr = input + 2;

        return
            LE16((input[0] & 0x1F)<<6  |
            (input[1] & 0x3F));
    }
    return ~0;
}

// Convert one character from UCS2 to UTF-8
// Returns length of UTF-8 char (1, 2 or 3) or -1 on error (UTF-16 outside UCS2)
// char *utf8 must be large enough to hold 3 bytes
int ucs2_to_utf8_char (const WCHAR ucs2_le, char *utf8)
{
    const WCHAR ucs2 = LE16(ucs2_le);

    if (ucs2 < 0x80) {
        utf8[0] = ucs2;
        utf8[1] = '\0';
        return 1;
    }

    if (ucs2 >= 0x80  && ucs2 < 0x800) {
        utf8[0] = (ucs2 >> 6)   | 0xC0;
        utf8[1] = (ucs2 & 0x3F) | 0x80;
        utf8[2] = '\0';
        return 2;
    }

    if (ucs2 >= 0x800 && ucs2 < 0xFFFF) {

    	if (ucs2 >= 0xD800 && ucs2 <= 0xDFFF) {
    		/* Ill-formed (UTF-16 ouside of BMP) */
    		return -1;
    	}

    	utf8[0] = ((ucs2 >> 12)       ) | 0xE0;
        utf8[1] = ((ucs2 >> 6 ) & 0x3F) | 0x80;
        utf8[2] = ((ucs2      ) & 0x3F) | 0x80;
        utf8[3] = '\0';
        return 3;
    }

    return -1;
}


// Converts UTF8 to UCS2. Returns size in bytes of the converted string or -1 on error
size_t utf8_to_ucs2(WCHAR* const ucs2_le, const char* const utf8, const size_t maxucs2, const size_t maxutf8)
{
	const unsigned char* current_utf8 = (unsigned char*)utf8;
	WCHAR* current_ucs2_le = ucs2_le;

	for (; *current_utf8; current_ucs2_le++)
	{
		size_t size = (char*)current_utf8 - utf8;

		if (size >= maxutf8) return (size_t)-1;
		if (((*current_utf8 & 0xc0) == 0xc0) && (size >= maxutf8 - 1)) return (size_t)-1;
		if (((*current_utf8 & 0xe0) == 0xe0) && (size >= maxutf8 - 2)) return (size_t)-1;
		if (current_ucs2_le - ucs2_le >= (intptr_t)maxucs2 - 1) return (size_t)-1;

		*current_ucs2_le = utf8_to_ucs2_char(current_utf8, &current_utf8);
		current_ucs2_le[1] = 0;

		if (*current_ucs2_le == (WCHAR)-1) return (size_t)-1;
	}
	return current_ucs2_le - ucs2_le;
}

// Converts UCS2 to UTF-8. Return TRUE or FALSE
BOOL ucs2_to_utf8(const WCHAR* const ucs2_le, char* utf8, size_t maxucs2, size_t maxutf8)
{
	char utf8_char[4];
	const WCHAR* current_ucs2 = ucs2_le;
	unsigned int index_utf8 = 0;

	for(*utf8 = 0; *current_ucs2; current_ucs2++)
	{
		if (current_ucs2 - ucs2_le > (intptr_t)maxucs2) return FALSE;
		int len = ucs2_to_utf8_char(*current_ucs2, utf8_char);
		if (index_utf8 + len > maxutf8) return FALSE;
		strncat(utf8, utf8_char, len);
		index_utf8+=len;
	}

	return TRUE;
}

/* End of UTF-8 <-> UCS2 conversion */


// Checks, whether a string is a valid integer number between min and max. Returns TRUE or FALSE. Puts int value in *value
BOOL stringToInt(const char *const szValue, const unsigned int min, const unsigned int max, unsigned int *const value)
{
	char *nextchar;

	errno = 0;
	long long result = strtoll(szValue, &nextchar, 10);

	if (errno || result < (long long)min || result > (long long)max || *nextchar)
	{
		return FALSE;
	}

	*value = (unsigned int)result;
	return TRUE;
}


//Converts a String Guid to a host binary guid in host endianess
int_fast8_t string2Uuid(const char *const restrict input, GUID *const restrict guid)
{
	int i;

	if (strlen(input) < GUID_STRING_LENGTH) return FALSE;
	if (input[8] != '-' || input[13] != '-' || input[18] != '-' || input[23] != '-') return FALSE;

	for (i = 0; i < GUID_STRING_LENGTH; i++)
	{
		if (i == 8 || i == 13 || i == 18 || i == 23) continue;

		const char c = toupper((int)input[i]);

		if (c < '0' || c > 'F' || (c > '9' && c < 'A')) return FALSE;
	}

	char inputCopy[GUID_STRING_LENGTH + 1];
	strncpy(inputCopy, input, GUID_STRING_LENGTH + 1);
	inputCopy[8] = inputCopy[13] = inputCopy[18] = 0;

	hex2bin((BYTE*)&guid->Data1, inputCopy, 8);
	hex2bin((BYTE*)&guid->Data2, inputCopy + 9, 4);
	hex2bin((BYTE*)&guid->Data3, inputCopy + 14, 4);
	hex2bin(guid->Data4, input + 19, 16);

	guid->Data1 =  BE32(guid->Data1);
	guid->Data2 =  BE16(guid->Data2);
	guid->Data3 =  BE16(guid->Data3);
	return TRUE;
}


// convert GUID to little-endian
void LEGUID(GUID *const restrict out, const GUID* const restrict in)
{
	#if __BYTE_ORDER != __LITTLE_ENDIAN
	out->Data1 = LE32(in->Data1);
	out->Data2 = LE16(in->Data2);
	out->Data3 = LE16(in->Data3);
	memcpy(out->Data4, in->Data4, sizeof(out->Data4));
	#else
	memcpy(out, in, sizeof(GUID));
	#endif
}


//Checks a command line argument if it is numeric and between min and max. Returns the numeric value or exits on error
__pure unsigned int getOptionArgumentInt(const char o, const unsigned int min, const unsigned int max)
{
	unsigned int result;

	if (!stringToInt(optarg, min, max, &result))
	{
		printerrorf("Fatal: Option \"-%c\" must be numeric between %u and %u.\n", o, min, max);
		exit(!0);
	}

	return result;
}


// Resets getopt() to start parsing from the beginning
void optReset(void)
{
	#if __minix__ || defined(__BSD__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__OpenBSD__)
	optind = 1;
	optreset = 1; // Makes newer BSD getopt happy
	#elif defined(__UCLIBC__) // uClibc headers also define __GLIBC__ so be careful here
	optind = 0; // uClibc seeks compatibility with GLIBC
	#elif defined(__GLIBC__)
	optind = 0; // Makes GLIBC getopt happy
	#else // Standard for most systems
	optind = 1;
	#endif
}


#if defined(_WIN32) || defined(USE_MSRPC)

// Returns a static message buffer containing text for a given Win32 error. Not thread safe (same as strerror)
char* win_strerror(const int message)
{
	#define STRERROR_BUFFER_SIZE 256
	static char buffer[STRERROR_BUFFER_SIZE];

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, message, 0, buffer, STRERROR_BUFFER_SIZE, NULL);
	return buffer;
}

#endif // defined(_WIN32) || defined(USE_MSRPC)


/*
 * parses an address in the form host:[port] in addr
 * returns host and port in seperate strings
 */
void parseAddress(char *const addr, char** szHost, char** szPort)
{
	*szHost = addr;

#	ifndef NO_SOCKETS
	*szPort = (char*)defaultport;
#	else // NO_SOCKETS
	*szPort = "1688";
#	endif // NO_SOCKETS

	char *lastcolon = strrchr(addr, ':');
	char *firstcolon = strchr(addr, ':');
	char *closingbracket = strrchr(addr, ']');

	if (*addr == '[' && closingbracket) //Address in brackets
	{
		*closingbracket = 0;
		(*szHost)++;

		if (closingbracket[1] == ':')
			*szPort = closingbracket + 2;
	}
	else if (firstcolon && firstcolon == lastcolon) //IPv4 address or hostname with port
	{
		*firstcolon = 0;
		*szPort = firstcolon + 1;
	}
}


// Initialize random generator (needs to be done in each thread)
void randomNumberInit()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	srand((unsigned int)(tv.tv_sec ^ tv.tv_usec));
}


// We always exit immediately if any OOM condition occurs
__noreturn void OutOfMemory(void)
{
	errorout("Fatal: Out of memory");
	exit(!0);
}


void* vlmcsd_malloc(size_t len)
{
	void* buf = malloc(len);
	if (!buf) OutOfMemory();
	return buf;
}


/*
 * Converts hex digits to bytes in big-endian order.
 * Ignores any non-hex characters
 */
void hex2bin(BYTE *const bin, const char *hex, const size_t maxbin)
{
	static const char *const hexdigits = "0123456789ABCDEF";
	char* nextchar;
	size_t i;

	for (i = 0; (i < 16) && utf8_to_ucs2_char((const unsigned char*)hex, (const unsigned char**)&nextchar) != (WCHAR)-1; hex = nextchar)
	{
		const char* pos = strchr(hexdigits, toupper((int)*hex));
		if (!pos) continue;

		if (!(i & 1)) bin[i >> 1] = 0;
		bin[i >> 1] |= (char)(pos - hexdigits);
		if (!(i & 1)) bin[i >> 1] <<= 4;
		i++;
		if (i >> 1 > maxbin) break;
	}
}


__pure BOOL getArgumentBool(int_fast8_t *result, const char *const argument)
{
	if (
		!strncasecmp(argument, "true", 4) ||
		!strncasecmp(argument, "on", 2) ||
		!strncasecmp(argument, "yes", 3) ||
		!strncasecmp(argument, "1", 1)
	)
	{
		*result = TRUE;
		return TRUE;
	}
	else if (
			!strncasecmp(argument, "false", 5) ||
			!strncasecmp(argument, "off", 3) ||
			!strncasecmp(argument, "no", 2) ||
			!strncasecmp(argument, "0", 1)
	)
	{
		*result = FALSE;
		return TRUE;
	}

	return FALSE;
}

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifndef USE_MSRPC

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#endif // WIN32

#include "network.h"
#include "endian.h"
#include "output.h"
#include "helpers.h"
#include "shared_globals.h"
#include "rpc.h"


#ifndef _WIN32
typedef ssize_t (*sendrecv_t)(int, void*, size_t, int);
#else
typedef int (WINAPI *sendrecv_t)(SOCKET, void*, int, int);
#endif


// Send or receive a fixed number of bytes regardless if received in one or more chunks
int_fast8_t sendrecv(SOCKET sock, BYTE *data, int len, int_fast8_t do_send)
{
	int n;
	sendrecv_t  f = do_send
			? (sendrecv_t) send
			: (sendrecv_t) recv;

	do
	{
			n = f(sock, data, len, 0);
	}
	while (
			( n < 0 && socket_errno == VLMCSD_EINTR ) || ( n > 0 && ( data += n, (len -= n) > 0 ) ));

	return ! len;
}


static int_fast8_t ip2str(char *restrict result, const size_t resultLength, const struct sockaddr *const restrict socketAddress, const socklen_t socketLength)
{
	static const char *const fIPv4 = "%s:%s";
	static const char *const fIPv6 = "[%s]:%s";
	char ipAddress[64], portNumber[8];

	if (getnameinfo
		(
			socketAddress,
			socketLength,
			ipAddress,
			sizeof(ipAddress),
			portNumber,
			sizeof(portNumber),
			NI_NUMERICHOST | NI_NUMERICSERV
		))
	{
		return FALSE;
	}

	if ((unsigned int)snprintf(result, resultLength, socketAddress->sa_family == AF_INET6 ? fIPv6 : fIPv4, ipAddress, portNumber) > resultLength) return FALSE;
	return TRUE;
}


static int_fast8_t getSocketList(struct addrinfo **saList, const char *const addr, const int flags, const int AddressFamily)
{
	int status;
	char *szHost, *szPort;
	size_t len = strlen(addr) + 1;

	// Don't alloca too much
	if (len > 264) return FALSE;

	char *addrcopy = (char*)alloca(len);
	memcpy(addrcopy, addr, len);

	parseAddress(addrcopy, &szHost, &szPort);

	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = AddressFamily;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = flags;

	if ((status = getaddrinfo(szHost, szPort, &hints, saList)))
	{
		printerrorf("Warning: %s: %s\n", addr, gai_strerror(status));
		return FALSE;
	}

	return TRUE;
}


static int_fast8_t setBlockingEnabled(SOCKET fd, int_fast8_t blocking)
{
	if (fd == INVALID_SOCKET) return FALSE;

	#ifdef _WIN32

	unsigned long mode = blocking ? 0 : 1;
	return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? TRUE : FALSE;

	#else // POSIX

	int flags = fcntl(fd, F_GETFL, 0);

	if (flags < 0) return FALSE;

	flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
	return (fcntl(fd, F_SETFL, flags) == 0) ? TRUE : FALSE;

	#endif // POSIX
}


int_fast8_t isDisconnected(const SOCKET s)
{
	char buffer[1];

	if (!setBlockingEnabled(s, FALSE)) return TRUE;

	int n = recv(s, buffer, 1, MSG_PEEK);

	if (!setBlockingEnabled(s, TRUE)) return TRUE;
	if (n == 0) return TRUE;

	return FALSE;
}


// Connect to TCP address addr (e.g. "kms.example.com:1688") and return an
// open socket for the connection if successful or INVALID_SOCKET otherwise
SOCKET connectToAddress(const char *const addr, const int AddressFamily, int_fast8_t showHostName)
{
	struct addrinfo *saList, *sa;
	SOCKET s = INVALID_SOCKET;
	char szAddr[128];

	if (!getSocketList(&saList, addr, 0, AddressFamily)) return INVALID_SOCKET;

	for (sa = saList; sa; sa = sa->ai_next)
	{
		// struct sockaddr_in* addr4 = (struct sockaddr_in*)sa->ai_addr;
		// struct sockaddr_in6* addr6 = (struct sockaddr_in6*)sa->ai_addr;

		if (ip2str(szAddr, sizeof(szAddr), sa->ai_addr, sa->ai_addrlen))
		{
			if (showHostName)
				printf("Connecting to %s (%s) ... ", addr, szAddr);
			else
				printf("Connecting to %s ... ", szAddr);

			fflush(stdout);
		}

		s = socket(sa->ai_family, SOCK_STREAM, IPPROTO_TCP);

#		if !defined(NO_TIMEOUT) && !__minix__
#		ifndef _WIN32 // Standard Posix timeout structure

		struct timeval to;
		to.tv_sec = 10;
		to.tv_usec = 0;

#		else // Windows requires a DWORD with milliseconds

		DWORD to = 10000;

#		endif // _WIN32

		setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (sockopt_t)&to, sizeof(to));
		setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (sockopt_t)&to, sizeof(to));
#		endif // !defined(NO_TIMEOUT) && !__minix__

		if (!connect(s, sa->ai_addr, sa->ai_addrlen))
		{
			printf("successful\n");
			break;
		}

		errorout("%s\n", socket_errno == VLMCSD_EINPROGRESS ? "Timed out" : vlmcsd_strerror(socket_errno));

		socketclose(s);
		s = INVALID_SOCKET;
	}

	freeaddrinfo(saList);
	return s;
}


#ifndef NO_SOCKETS

// Create a Listening socket for addrinfo sa and return socket s
// szHost and szPort are for logging only
static int listenOnAddress(const struct addrinfo *const ai, SOCKET *s)
{
	int error;
	char ipstr[64];

	ip2str(ipstr, sizeof(ipstr), ai->ai_addr, ai->ai_addrlen);

	//*s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	*s = socket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP);

	if (*s == INVALID_SOCKET)
	{
		error = socket_errno;
		printerrorf("Warning: %s error. %s\n", ai->ai_family == AF_INET6 ? cIPv6 : cIPv4, vlmcsd_strerror(error));
		return error;
	}

#	if !defined(_WIN32) && !defined(NO_SIGHUP)

	int flags = fcntl(*s, F_GETFD, 0);

	if (flags != -1)
	{
		flags |= FD_CLOEXEC;
		fcntl(*s, F_SETFD, flags);
	}
#	ifdef _PEDANTIC
	else
	{
		printerrorf("Warning: Could not set FD_CLOEXEC flag on %s: %s\n", ipstr, vlmcsd_strerror(errno));
	}
#	endif // _PEDANTIC

#	endif // !defined(_WIN32) && !defined(NO_SIGHUP)

	BOOL socketOption = TRUE;

	// fix for lame tomato toolchain
#	ifndef IPV6_V6ONLY
#	ifdef __linux__
#	define IPV6_V6ONLY (26)
#	endif // __linux__
#	endif // IPV6_V6ONLY

#	ifdef IPV6_V6ONLY
	if (ai->ai_family == AF_INET6) setsockopt(*s, IPPROTO_IPV6, IPV6_V6ONLY, (sockopt_t)&socketOption, sizeof(socketOption));
#	endif

#	ifndef _WIN32
	setsockopt(*s, SOL_SOCKET, SO_REUSEADDR, (sockopt_t)&socketOption, sizeof(socketOption));
#	endif

	if (bind(*s, ai->ai_addr, ai->ai_addrlen) || listen(*s, SOMAXCONN))
	{
		error = socket_errno;
		printerrorf("Warning: %s: %s\n", ipstr, vlmcsd_strerror(error));
		socketclose(*s);
		return error;
	}

#	ifndef NO_LOG
	logger("Listening on %s\n", ipstr);
#	endif

	return 0;
}


// Adds a listening socket for an address string,
// e.g. 127.0.0.1:1688 or [2001:db8:dead:beef::1]:1688
BOOL addListeningSocket(const char *const addr)
{
	struct addrinfo *aiList, *ai;
	int result = FALSE;
	SOCKET *s = SocketList + numsockets;

	if (getSocketList(&aiList, addr, AI_PASSIVE | AI_NUMERICHOST, AF_UNSPEC))
	{
		for (ai = aiList; ai; ai = ai->ai_next)
		{
			// struct sockaddr_in* addr4 = (struct sockaddr_in*)sa->ai_addr;
			// struct sockaddr_in6* addr6 = (struct sockaddr_in6*)sa->ai_addr;

			if (numsockets >= FD_SETSIZE)
			{
				#ifdef _PEDANTIC // Do not report this error in normal builds to keep file size low
				printerrorf("Warning: Cannot listen on %s. Your OS only supports %u listening sockets in an FD_SET.\n", addr, FD_SETSIZE);
				#endif
				break;
			}

			if (!listenOnAddress(ai, s))
			{
				numsockets++;
				result = TRUE;
			}
		}

		freeaddrinfo(aiList);
	}
	return result;
}


// Just create some dummy sockets to see if we have a specific protocol (IPv4 or IPv6)
__pure int_fast8_t checkProtocolStack(const int addressfamily)
{
	SOCKET s; // = INVALID_SOCKET;

	s = socket(addressfamily, SOCK_STREAM, 0);
	int_fast8_t success = (s != INVALID_SOCKET);

	socketclose(s);
	return success;
}


// Build an fd_set of all listening socket then use select to wait for an incoming connection
static SOCKET network_accept_any()
{
    fd_set ListeningSocketsList;
    SOCKET maxSocket, sock;
    int i;
    int status;

    FD_ZERO(&ListeningSocketsList);
    maxSocket = 0;

    for (i = 0; i < numsockets; i++)
    {
        FD_SET(SocketList[i], &ListeningSocketsList);
        if (SocketList[i] > maxSocket) maxSocket = SocketList[i];
    }

    status = select(maxSocket + 1, &ListeningSocketsList, NULL, NULL, NULL);

    if (status < 0) return INVALID_SOCKET;

    sock = INVALID_SOCKET;

    for (i = 0; i < numsockets; i++)
    {
        if (FD_ISSET(SocketList[i], &ListeningSocketsList))
        {
            sock = SocketList[i];
            break;
        }
    }

    if (sock == INVALID_SOCKET)
        return INVALID_SOCKET;
    else
        return accept(sock, NULL, NULL);
}


void closeAllListeningSockets()
{
	int i;

	for (i = 0; i < numsockets; i++)
	{
		shutdown(SocketList[i], VLMCSD_SHUT_RDWR);
		socketclose(SocketList[i]);
	}
}
#endif // NO_SOCKETS


static void serveClient(const SOCKET s_client, const DWORD RpcAssocGroup)
{
#	if !defined(NO_TIMEOUT) && !__minix__

#	ifndef _WIN32 // Standard Posix timeout structure

	struct timeval to;
	to.tv_sec = ServerTimeout;
	to.tv_usec = 0;

	#else // Windows requires a DWORD with milliseconds

	DWORD to = ServerTimeout * 1000;

#	endif // _WIN32

#	if !defined(NO_LOG) && defined(_PEDANTIC)

	int result =
		setsockopt(s_client, SOL_SOCKET, SO_RCVTIMEO, (sockopt_t)&to, sizeof(to)) ||
		setsockopt(s_client, SOL_SOCKET, SO_SNDTIMEO, (sockopt_t)&to, sizeof(to));

    if (result) logger("Warning: Set timeout failed: %s\n", vlmcsd_strerror(socket_errno));

#	else // !(!defined(NO_LOG) && defined(_PEDANTIC))

    setsockopt(s_client, SOL_SOCKET, SO_RCVTIMEO, (sockopt_t)&to, sizeof(to));
	setsockopt(s_client, SOL_SOCKET, SO_SNDTIMEO, (sockopt_t)&to, sizeof(to));

#   endif // !(!defined(NO_LOG) && defined(_PEDANTIC))

#	endif // !defined(NO_TIMEOUT) && !__minix__

	char ipstr[64];
	socklen_t len;
	struct sockaddr_storage addr;

	len = sizeof addr;

	if (getpeername(s_client, (struct sockaddr*)&addr, &len) ||
		!ip2str(ipstr, sizeof(ipstr), (struct sockaddr*)&addr, len))
	{
#		if !defined(NO_LOG) && defined(_PEDANTIC)
		logger("Fatal: Cannot determine client's IP address: %s\n", vlmcsd_strerror(errno));
#		endif // !defined(NO_LOG) && defined(_PEDANTIC)
		socketclose(s_client);
		return;
	}


#	ifndef NO_LOG
	const char *const connection_type = addr.ss_family == AF_INET6 ? cIPv6 : cIPv4;
	static const char *const cAccepted = "accepted";
	static const char *const cClosed = "closed";
	static const char *const fIP = "%s connection %s: %s.\n";

	logger(fIP, connection_type, cAccepted, ipstr);
	#endif // NO_LOG

	rpcServer(s_client, RpcAssocGroup, ipstr);

#	ifndef NO_LOG
	logger(fIP, connection_type, cClosed, ipstr);
#	endif // NO_LOG

	socketclose(s_client);
}


#ifndef NO_SOCKETS
static void post_sem(void)
{
	#if !defined(NO_LIMIT) && !__minix__
	if (!InetdMode && MaxTasks != SEM_VALUE_MAX)
	{
		semaphore_post(Semaphore);
	}
	#endif // !defined(NO_LIMIT) && !__minix__
}


static void wait_sem(void)
{
	#if !defined(NO_LIMIT) && !__minix__
	if (!InetdMode && MaxTasks != SEM_VALUE_MAX)
	{
		semaphore_wait(Semaphore);
	}
	#endif // !defined(NO_LIMIT) && !__minix__
}
#endif // NO_SOCKETS

#if defined(USE_THREADS) && !defined(NO_SOCKETS)

#if defined(_WIN32) || defined(__CYGWIN__) // Win32 Threads
static DWORD WINAPI serveClientThreadProc(PCLDATA clData)
#else // Posix threads
static void *serveClientThreadProc (PCLDATA clData)
#endif // Thread proc is identical in WIN32 and Posix threads
{
	serveClient(clData->socket, clData->RpcAssocGroup);
	free(clData);
	post_sem();

	return 0;
}

#endif // USE_THREADS


#ifndef NO_SOCKETS

#if defined(USE_THREADS) && (defined(_WIN32) || defined(__CYGWIN__)) // Windows Threads
static int serveClientAsyncWinThreads(const PCLDATA thr_CLData)
{
	wait_sem();

	HANDLE h = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)serveClientThreadProc, thr_CLData, 0, NULL);

	if (h)
		CloseHandle(h);
	else
	{
		socketclose(thr_CLData->socket);
		free(thr_CLData);
		post_sem();
		return GetLastError();
	}

	return NO_ERROR;
}
#endif // defined(USE_THREADS) && defined(_WIN32) // Windows Threads


#if defined(USE_THREADS) && !defined(_WIN32) && !defined(__CYGWIN__) // Posix Threads
static int ServeClientAsyncPosixThreads(const PCLDATA thr_CLData)
{
	pthread_t p_thr;
	pthread_attr_t attr;

	wait_sem();

	// Must set detached state to avoid memory leak
	if (pthread_attr_init(&attr) ||
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ||
		pthread_create(&p_thr, &attr, (void * (*)(void *))serveClientThreadProc, thr_CLData))
	{
		socketclose(thr_CLData->socket);
		free(thr_CLData);
		post_sem();
		return !0;
	}

	return 0;
}
#endif //  defined(USE_THREADS) && !defined(_WIN32) // Posix Threads

#ifndef USE_THREADS // fork() implementation
static void ChildSignalHandler(const int signal)
{
	if (signal == SIGHUP) return;

	post_sem();

	#ifndef NO_LOG
	logger("Warning: Child killed/crashed by %s\n", strsignal(signal));
	#endif // NO_LOG

	exit(!0);
}

static int ServeClientAsyncFork(const SOCKET s_client, const DWORD RpcAssocGroup)
{
	int pid;
	wait_sem();

	if ((pid = fork()) < 0)
	{
		return errno;
	}
	else if ( pid )
	{
		// Parent process
		socketclose(s_client);
		return 0;
	}
	else
	{
		// Child process

		// Setup a Child Handler for most common termination signals
		struct sigaction sa;

		sa.sa_flags   = 0;
		sa.sa_handler = ChildSignalHandler;

		static int signallist[] = { SIGHUP, SIGINT, SIGTERM, SIGSEGV, SIGILL, SIGFPE, SIGBUS };

		if (!sigemptyset(&sa.sa_mask))
		{
			uint_fast8_t i;

			for (i = 0; i < _countof(signallist); i++)
			{
				sigaction(signallist[i], &sa, NULL);
			}
		}

		serveClient(s_client, RpcAssocGroup);
		post_sem();
		exit(0);
	}
}
#endif


int serveClientAsync(const SOCKET s_client, const DWORD RpcAssocGroup)
{
	#ifndef USE_THREADS // fork() implementation

	return ServeClientAsyncFork(s_client, RpcAssocGroup);

	#else // threads implementation

	PCLDATA thr_CLData = (PCLDATA)vlmcsd_malloc(sizeof(CLDATA));
	thr_CLData->socket = s_client;
	thr_CLData->RpcAssocGroup = RpcAssocGroup;

	#if defined(_WIN32) || defined (__CYGWIN__) // Windows threads

	return serveClientAsyncWinThreads(thr_CLData);

	#else // Posix Threads

	return ServeClientAsyncPosixThreads(thr_CLData);

	#endif // Posix Threads

	#endif // USE_THREADS
}

#endif // NO_SOCKETS


int runServer()
{
	DWORD RpcAssocGroup = rand32();

	// If compiled for inetd-only mode just serve the stdin socket
	#ifdef NO_SOCKETS
	serveClient(STDIN_FILENO, RpcAssocGroup);
	return 0;
	#else
	// In inetd mode just handle the stdin socket
	if (InetdMode)
	{
		serveClient(STDIN_FILENO, RpcAssocGroup);
		return 0;
	}

	// Standalone mode
	for (;;)
	{
		int error;
		SOCKET s_client;

		if ( (s_client = network_accept_any()) == INVALID_SOCKET )
		{
			error = socket_errno;

			if (error == VLMCSD_EINTR || error == VLMCSD_ECONNABORTED) continue;

			#ifdef _NTSERVICE
			if (ServiceShutdown) return 0;
			#endif

			#ifndef NO_LOG
			logger("Fatal: %s\n",vlmcsd_strerror(error));
			#endif

			return error;
		}

		RpcAssocGroup++;
		serveClientAsync(s_client, RpcAssocGroup);
	}
	#endif // NO_SOCKETS

	return 0;
}

#endif // USE_MSRPC
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
	uint_fast16_t _v;

	if (Ctx != *Ndr64Ctx)
		_v = LE16(((WORD*)Request->Ndr.Data)[1]) - 4;
	else
		_v = LE16(((WORD*)Request->Ndr64.Data)[1]) - 4;

	// Only KMS v4, v5 and v6 are supported
	if (_v >= vlmcsd_countof(_Versions))
	{
#		ifndef NO_LOG
		logger("Fatal: KMSv%i unsupported\n", _v + 4);
#		endif // NO_LOG
		return 0;
	}

	// Could check for equality but allow bigger requests to support buggy RPC clients (e.g. wine)
	// Buffer overrun is check by caller.
	return (requestSize >= _Versions[_v].RequestSize);
}


/*
 * Handles the actual KMS request from the client.
 * Calls KMS functions (CreateResponseV4 or CreateResponseV6) in kms.c
 * Returns size of the KMS response packet or 0 on failure.
 *
 * The RPC packet size (excluding header) is actually in Response->AllocHint
 */
static int rpcRequest(const RPC_REQUEST64 *const Request, RPC_RESPONSE64 *const Response, const DWORD RpcAssocGroup_unused, const SOCKET sock_unused, WORD* NdrCtx, WORD* Ndr64Ctx, BYTE packetType, const char* const ipstr)
{
	uint_fast16_t _v;
	int ResponseSize;
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

	_v = LE16(((WORD*)requestData)[1]) - 4;

	if (!(ResponseSize = _Versions[_v].CreateResponse(requestData, responseData, ipstr)))
	{
		return 0;
	}

	if (Ctx != *Ndr64Ctx)
	{
		Response->Ndr.DataSizeMax = LE32(0x00020000);
		Response->Ndr.DataLength  =	Response->Ndr.DataSizeIs = LE32(ResponseSize);
		len = ResponseSize + sizeof(Response->Ndr);
	}
	else
	{
		Response->Ndr64.DataSizeMax = LE64(0x00020000ULL);
		Response->Ndr64.DataLength  = Response->Ndr64.DataSizeIs = LE64((uint64_t)ResponseSize);
		len = ResponseSize + sizeof(Response->Ndr64);
	}

	pRpcReturnCode = ((BYTE*)&Response->Ndr) + len;
	UA32(pRpcReturnCode) = 0; //LE32 not needed for 0
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
		if (!_Actions[_a].CheckRequestSize(requestBuffer, request_len, &NdrCtx, &Ndr64Ctx)) return;

		// Unable to create a valid response from request
		if (!(response_len = _Actions[_a].GetResponse(requestBuffer, rpcResponse, RpcAssocGroup, sock, &NdrCtx, &Ndr64Ctx, rpcRequestHeader.PacketType, ipstr))) return;

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
		status = !0;
	}

	if (Header->DataRepresentation != BE32(0x10000000))
	{
		p("Fatal: RPC response does not conform to Microsoft's limited support of DCE RPC\n");
		status = !0;
	}

	if (Header->AuthLength != 0)
	{
		p("Fatal: RPC response requests authentication\n");
		status = !0;
	}

	// vlmcsd does not support fragmented packets (not yet neccassary)
	if ( (Header->PacketFlags & (RPC_PF_FIRST | RPC_PF_LAST)) != (RPC_PF_FIRST | RPC_PF_LAST) )
	{
		p("Fatal: RPC packet flags RPC_PF_FIRST and RPC_PF_LAST are not both set.\n");
		status = !0;
	}

	if (Header->PacketFlags & RPC_PF_CANCEL_PENDING)	p("Warning: %s should not be set\n", "RPC_PF_CANCEL_PENDING");
	if (Header->PacketFlags & RPC_PF_RESERVED)			p("Warning: %s should not be set\n", "RPC_PF_RESERVED");
	if (Header->PacketFlags & RPC_PF_NOT_EXEC)			p("Warning: %s should not be set\n", "RPC_PF_NOT_EXEC");
	if (Header->PacketFlags & RPC_PF_MAYBE)				p("Warning: %s should not be set\n", "RPC_PF_MAYBE");
	if (Header->PacketFlags & RPC_PF_OBJECT)			p("Warning: %s should not be set\n", "RPC_PF_OBJECT");

	if (Header->VersionMajor != 5 || Header->VersionMinor != 0)
	{
		p("Fatal: Expected RPC version 5.0 and got %u.%u\n", Header->VersionMajor, Header->VersionMinor);
		status = !0;
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

		status = !0;
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
			errorout("\nFatal: Could not send RPC request\n");
			status = !0;
			break;
		}

		if (!_recv(sock, &ResponseHeader, sizeof(RPC_HEADER)))
		{
			errorout("\nFatal: No RPC response received from server\n");
			status = !0;
			break;
		}

		if ((status = checkRpcResponseHeader(&ResponseHeader, RequestHeader, RPC_PT_RESPONSE, &errorout))) break;

		size = useNdr64 ? sizeof(RPC_RESPONSE64) : sizeof(RPC_RESPONSE);

		if (size > LE16(ResponseHeader.FragLength) - sizeof(ResponseHeader))
			size = LE16(ResponseHeader.FragLength) - sizeof(ResponseHeader);

		if (!_recv(sock, &_Response, size))
		{
			errorout("\nFatal: RPC response is incomplete\n");
			status = !0;
			break;
		}

		if (_Response.CancelCount != 0)
		{
			errorout("\nFatal: RPC response cancel count is not 0\n");
			status = !0;
		}

		if (_Response.ContextId != (useNdr64 ? LE16(1) : 0))
		{
			errorout("\nFatal: RPC response context id %u is not bound\n", (unsigned int)LE16(_Response.ContextId));
			status = !0;
		}

		int_fast8_t sizesMatch;

		if (useNdr64)
		{
			*responseSize = (size_t)LE64(_Response.Ndr64.DataLength);
			responseSize2 = (size_t)LE64(_Response.Ndr64.DataSizeIs);

			if (!*responseSize || !_Response.Ndr64.DataSizeMax)
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

			if (!*responseSize || !_Response.Ndr.DataSizeMax)
			{
				status = (int)LE32(_Response.Ndr.status);
				break;
			}

			sizesMatch = (size_t)LE32(_Response.Ndr.DataLength) == responseSize2;
		}

		if (!sizesMatch)
		{
			errorout("\nFatal: NDR data length (%u) does not match NDR data size (%u)\n",
					(uint32_t)*responseSize,
					(uint32_t)LE32(_Response.Ndr.DataSizeIs)
			);

			status = !0;
		}

		*KmsResponse = (BYTE*)vlmcsd_malloc(*responseSize + MAX_EXCESS_BYTES);

		// If RPC stub is too short, assume missing bytes are zero (same ill behavior as MS RPC)
		memset(*KmsResponse, 0, *responseSize + MAX_EXCESS_BYTES);

		// Read up to 16 bytes more than bytes expected to detect faulty KMS emulators
		if ((bytesread = recv(sock, (char*)*KmsResponse, *responseSize + MAX_EXCESS_BYTES, 0)) < (int)*responseSize)
		{
			errorout("\nFatal: No or incomplete KMS response received. Required %u bytes but only got %i\n",
					(uint32_t)*responseSize,
					(int32_t)(bytesread < 0 ? 0 : bytesread)
			);

			status = !0;
			break;
		}

		DWORD *pReturnCode;

		size_t len = *responseSize + (useNdr64 ? sizeof(_Response.Ndr64) : sizeof(_Response.Ndr)) + sizeof(*pReturnCode);
		size_t pad = ((~len & 3) + 1) & 3;

		if (len + pad != LE32(_Response.AllocHint))
		{
			errorout("\nWarning: RPC stub size is %u, should be %u (probably incorrect padding)\n", (uint32_t)LE32(_Response.AllocHint), (uint32_t)(len + pad));
		}
		else
		{
			size_t i;
			for (i = 0; i < pad; i++)
			{
				if (*(*KmsResponse + *responseSize + sizeof(*pReturnCode) + i))
				{
					errorout("\nWarning: RPC stub data not padded to zeros according to Microsoft standard\n");
					break;
				}
			}
		}

		pReturnCode = (DWORD*)(*KmsResponse + *responseSize + pad);
		status = LE32(UA32(pReturnCode));

		if (status) errorout("\nWarning: RPC stub data reported Error %u\n", (uint32_t)status);

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
		errorout("\nFatal: Sending RPC bind request failed\n");
		return !0;
	}

	if (!_recv(sock, &ResponseHeader, sizeof(RPC_HEADER)))
	{
		errorout("\nFatal: Did not receive a response from server\n");
		return !0;
	}

	if ((status = checkRpcResponseHeader
	(
			&ResponseHeader,
			RequestHeader,
			packetType == RPC_PT_BIND_REQ ? RPC_PT_BIND_ACK : RPC_PT_ALTERCONTEXT_ACK,
			&errorout
	)))
	{
		return status;
	}

	bindResponse = (RPC_BIND_RESPONSE*)vlmcsd_malloc(LE16(ResponseHeader.FragLength) - sizeof(RPC_HEADER));
	BYTE* bindResponseBytePtr = (BYTE*)bindResponse;

	if (!_recv(sock, bindResponse, LE16(ResponseHeader.FragLength) - sizeof(RPC_HEADER)))
	{
		errorout("\nFatal: Incomplete RPC bind acknowledgement received\n");
		free(bindResponseBytePtr);
		return !0;
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
			errorout("\nFatal: Expected %u CTX items but got %u\n",
					(uint32_t)LE32(bindRequest->NumCtxItems),
					(uint32_t)LE32(bindResponse->NumResults)
			);

			status = !0;
		}

		for (i = 0; i < ctxItems; i++)
		{
			const char* transferSyntaxName =
					i == CtxBTFN ? "BTFN" :  i == CtxNDR64 ? "NDR64" : "NDR32";

			if (bindResponse->Results[i].AckResult == RPC_BIND_NACK) // transfer syntax was declined
			{
				if (!IsNullGuid((BYTE*)&bindResponse->Results[i].TransferSyntax))
				{
					errorout(
						"\nWarning: Rejected transfer syntax %s did not return NULL Guid\n",
						transferSyntaxName
					);
				}

				if (bindResponse->Results[i].SyntaxVersion)
				{
					errorout(
						"\nWarning: Rejected transfer syntax %s did not return syntax version 0 but %u\n",
						transferSyntaxName,
						LE32(bindResponse->Results[i].SyntaxVersion)
					);
				}

				if (bindResponse->Results[i].AckReason == RPC_ABSTRACTSYNTAX_UNSUPPORTED)
				{
					errorout(
						"\nWarning: Transfer syntax %s does not support KMS activation\n",
						transferSyntaxName
					);
				}
				else if (bindResponse->Results[i].AckReason != RPC_SYNTAX_UNSUPPORTED)
				{
					errorout(
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
					errorout("\nWarning: BTFN did not respond with RPC_BIND_ACK or RPC_BIND_NACK\n");
				}

				if (bindResponse->Results[i].AckReason != LE16(3))
				{
					errorout("\nWarning: BTFN did not return expected feature mask 0x3 but 0x%X\n", (unsigned int)LE16(bindResponse->Results[i].AckReason));
				}

				if (verbose) printf("... BTFN ");
				RpcFlags.HasBTFN = TRUE;

				continue;
			}

			// NDR32 or NDR64 Ctx
			if (bindResponse->Results[i].AckResult != RPC_BIND_ACCEPT)
			{
				errorout(
					"\nFatal: transfer syntax %s returned an invalid status, neither RPC_BIND_ACCEPT nor RPC_BIND_NACK\n",
					transferSyntaxName
				);

				status = !0;
			}

			if (!IsEqualGUID(&bindResponse->Results[i].TransferSyntax, &bindRequest->CtxItems[i].TransferSyntax))
			{
				errorout(
					"\nFatal: Transfer syntax of RPC bind request and response does not match\n"
				);

				status = !0;
			}

			if (bindResponse->Results[i].SyntaxVersion != bindRequest->CtxItems[i].SyntaxVersion)
			{
				errorout("\nFatal: Expected transfer syntax version %u for %s but got %u\n",
						(uint32_t)LE32(bindRequest->CtxItems[0].SyntaxVersion),
						transferSyntaxName,
						(uint32_t)LE32(bindResponse->Results[0].SyntaxVersion)
				);

				status = !0;
			}

			// The ack reason field is actually undefined here but Microsoft sets this to 0
			if (bindResponse->Results[i].AckReason != 0)
			{
				errorout(
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
		errorout("\nFatal: Could neither negotiate NDR32 nor NDR64 with the RPC server\n");
		status = !0;
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
#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#if !defined(_CRYPTO_OPENSSL) && !defined(_CRYPTO_POLARSSL) && !defined(_CRYPTO_WINDOWS)
#include "crypto_internal.h"
#include "endian.h"

#define F0(x, y, z)  ( ((x) & (y)) | (~(x) & (z)) )
#define F1(x, y, z)  ( ((x) & (y)) | ((x) & (z)) | ((y) & (z)) )

#define SI1(x)  ( ROR32(x, 2 ) ^ ROR32(x, 13) ^ ROR32(x, 22) )
#define SI2(x)  ( ROR32(x, 6 ) ^ ROR32(x, 11) ^ ROR32(x, 25) )
#define SI3(x)  ( ROR32(x, 7 ) ^ ROR32(x, 18) ^ ((x) >> 3 ) )
#define SI4(x)  ( ROR32(x, 17) ^ ROR32(x, 19) ^ ((x) >> 10) )

static const DWORD k[] = {
	0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1,
	0x923F82A4, 0xAB1C5ED5, 0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
	0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174, 0xE49B69C1, 0xEFBE4786,
	0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
	0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147,
	0x06CA6351, 0x14292967, 0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
	0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85, 0xA2BFE8A1, 0xA81A664B,
	0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
	0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A,
	0x5B9CCA4F, 0x682E6FF3, 0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
	0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};


static void Sha256Init(Sha256Ctx *Ctx)
{
	Ctx->State[0] = 0x6A09E667;
	Ctx->State[1] = 0xBB67AE85;
	Ctx->State[2] = 0x3C6EF372;
	Ctx->State[3] = 0xA54FF53A;
	Ctx->State[4] = 0x510E527F;
	Ctx->State[5] = 0x9B05688C;
	Ctx->State[6] = 0x1F83D9AB;
	Ctx->State[7] = 0x5BE0CD19;
	Ctx->Len = 0;
}


static void Sha256ProcessBlock(Sha256Ctx *Ctx, BYTE *block)
{
	unsigned int  i;
	DWORD  w[64], temp1, temp2;
	DWORD  a = Ctx->State[0];
	DWORD  b = Ctx->State[1];
	DWORD  c = Ctx->State[2];
	DWORD  d = Ctx->State[3];
	DWORD  e = Ctx->State[4];
	DWORD  f = Ctx->State[5];
	DWORD  g = Ctx->State[6];
	DWORD  h = Ctx->State[7];

	for (i = 0; i < 16; i++)
		//w[ i ] = GET_UAA32BE(block, i);
		w[i] = BE32(((DWORD*)block)[i]);

	for (i = 16; i < 64; i++)
		w[ i ] = SI4(w[ i - 2 ]) + w[ i - 7 ] + SI3(w[ i - 15 ]) + w[ i - 16 ];

	for (i = 0; i < 64; i++)
	{
		temp1 = h + SI2(e) + F0(e, f, g) + k[ i ] + w[ i ];
		temp2 = SI1(a) + F1(a, b, c);

		h = g;
		g = f;
		f = e;
		e = d + temp1;
		d = c;
		c = b;
		b = a;
		a = temp1 + temp2;
	}

	Ctx->State[0] += a;
	Ctx->State[1] += b;
	Ctx->State[2] += c;
	Ctx->State[3] += d;
	Ctx->State[4] += e;
	Ctx->State[5] += f;
	Ctx->State[6] += g;
	Ctx->State[7] += h;
}


static void Sha256Update(Sha256Ctx *Ctx, BYTE *data, size_t len)
{
	unsigned int  b_len = Ctx->Len & 63,
								r_len = (b_len ^ 63) + 1;

	Ctx->Len += len;

	if ( len < r_len )
	{
		memcpy(Ctx->Buffer + b_len, data, len);
		return;
	}

	if ( r_len < 64 )
	{
		memcpy(Ctx->Buffer + b_len, data, r_len);
		len  -= r_len;
		data += r_len;
		Sha256ProcessBlock(Ctx, Ctx->Buffer);
	}

	for (; len >= 64; len -= 64, data += 64)
		Sha256ProcessBlock(Ctx, data);

	if ( len ) memcpy(Ctx->Buffer, data, len);
}


static void Sha256Finish(Sha256Ctx *Ctx, BYTE *hash)
{
	unsigned int  i, b_len = Ctx->Len & 63;

	Ctx->Buffer[ b_len ] = 0x80;
	if ( b_len ^ 63 ) memset(Ctx->Buffer + b_len + 1, 0, b_len ^ 63);

	if ( b_len >= 56 )
	{
		Sha256ProcessBlock(Ctx, Ctx->Buffer);
		memset(Ctx->Buffer, 0, 56);
	}

	//PUT_UAA64BE(Ctx->Buffer, (unsigned long long)(Ctx->Len * 8), 7);
	((uint64_t*)Ctx->Buffer)[7] = BE64((uint64_t)Ctx->Len << 3);
	Sha256ProcessBlock(Ctx, Ctx->Buffer);

	for (i = 0; i < 8; i++)
		//PUT_UAA32BE(hash, Ctx->State[i], i);
		((DWORD*)hash)[i] = BE32(Ctx->State[i]);

}


void Sha256(BYTE *data, size_t len, BYTE *hash)
{
	Sha256Ctx Ctx;

	Sha256Init(&Ctx);
	Sha256Update(&Ctx, data, len);
	Sha256Finish(&Ctx, hash);
}


static void _Sha256HmacInit(Sha256HmacCtx *Ctx, BYTE *key, size_t klen)
{
	BYTE  IPad[64];
	unsigned int  i;

	memset(IPad, 0x36, sizeof(IPad));
	memset(Ctx->OPad, 0x5C, sizeof(Ctx->OPad));

	if ( klen > 64 )
	{
		BYTE *temp = (BYTE*)alloca(32);
		Sha256(key, klen, temp);
		klen = 32;
		key  = temp;
	}

	for (i = 0; i < klen; i++)
	{
		IPad[ i ]      ^= key[ i ];
		Ctx->OPad[ i ] ^= key[ i ];
	}

	Sha256Init(&Ctx->ShaCtx);
	Sha256Update(&Ctx->ShaCtx, IPad, sizeof(IPad));
}


static void _Sha256HmacUpdate(Sha256HmacCtx *Ctx, BYTE *data, size_t len)
{
	Sha256Update(&Ctx->ShaCtx, data, len);
}


static void _Sha256HmacFinish(Sha256HmacCtx *Ctx, BYTE *hmac)
{
	BYTE  temp[32];

	Sha256Finish(&Ctx->ShaCtx, temp);
	Sha256Init(&Ctx->ShaCtx);
	Sha256Update(&Ctx->ShaCtx, Ctx->OPad, sizeof(Ctx->OPad));
	Sha256Update(&Ctx->ShaCtx, temp, sizeof(temp));
	Sha256Finish(&Ctx->ShaCtx, hmac);
}



int_fast8_t Sha256Hmac(BYTE* key, BYTE* restrict data, DWORD len, BYTE* restrict hmac)
{
	Sha256HmacCtx Ctx;
	_Sha256HmacInit(&Ctx, key, 16);
	_Sha256HmacUpdate(&Ctx, data, len);
	_Sha256HmacFinish(&Ctx, hmac);
	return TRUE;
}


#endif // No external Crypto

/*
 * dns_srv.c
 *
 * This file contains the code for KMS SRV record lookup in DNS (_vlmcs._tcp.example.com IN SRV 0 0 1688 mykms.example.com)
 *
 */

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifndef NO_DNS

#include "dns_srv.h"

#include <string.h>
#include <stdio.h>
#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
//#ifndef DNS_PARSER_INTERNAL
#if __ANDROID__
#include <netinet/in.h>
#include "nameser.h"
#include "resolv.h"
#else // other Unix non-Android
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#endif // other Unix non-Android
//#endif // DNS_PARSER_INTERNAL
#else // WIN32
#include <windns.h>
#endif // WIN32

#include "helpers.h"
#include "output.h"
#include "endian.h"

#if defined(DNS_PARSER_INTERNAL) && !defined(_WIN32)

#include "ns_name.h"
#include "ns_parse.h"

// Define macros to redirect DNS parser functions to internal versions

#undef ns_msg
#undef ns_initparse
#undef ns_parserr
#undef ns_rr
#undef ns_name_uncompress
#undef ns_msg_base
#undef ns_msg_end
#undef ns_rr_rdata
#undef ns_rr_type
#undef ns_msg_count
#undef ns_rr_class
#undef ns_s_an
#define ns_msg ns_msg_vlmcsd
#define ns_initparse ns_initparse_vlmcsd
#define ns_parserr ns_parserr_vlmcsd
#define ns_rr ns_rr_vlmcsd
#define ns_name_uncompress ns_name_uncompress_vlmcsd
#define ns_msg_base ns_msg_base_vlmcsd
#define ns_msg_end ns_msg_end_vlmcsd
#define ns_rr_rdata ns_rr_rdata_vlmcsd
#define ns_rr_type ns_rr_type_vlmcsd
#define ns_msg_count ns_msg_count_vlmcsd
#define ns_rr_class ns_rr_class_vlmcsd
#define ns_s_an ns_s_an_vlmcsd

#ifndef NS_MAXLABEL
#define NS_MAXLABEL 63
#endif

#endif // defined(DNS_PARSER_INTERNAL) && !defined(_WIN32)


//TODO: maybe move to helpers.c
static unsigned int isqrt(unsigned int n)
{
	unsigned int c = 0x8000;
	unsigned int g = 0x8000;

	for(;;)
	{
		if(g*g > n)
			g ^= c;

		c >>= 1;

		if(c == 0) return g;

		g |= c;
	}
}


/*
 * Compare function for qsort to sort SRV records by priority and weight
 * random_weight must be product of weight from SRV record and square root of a random number
 */
static int kmsServerListCompareFunc1(const void* a, const void* b)
{
	if ( !a && !b) return 0;
	if ( a && !b) return -1;
	if ( !a && b) return 1;

	int priority_order =  (int)((*(kms_server_dns_ptr*)a)->priority) - ((int)(*(kms_server_dns_ptr*)b)->priority);

	if (priority_order) return priority_order;

	return (int)((*(kms_server_dns_ptr*)b)->random_weight) - ((int)(*(kms_server_dns_ptr*)a)->random_weight);
}

/* Sort resulting SRV records */
void sortSrvRecords(kms_server_dns_ptr* serverlist, const int answers)
{
	int i;

	for (i = 0; i < answers; i++)
	{
		serverlist[i]->random_weight = (rand32() % 256) * isqrt(serverlist[i]->weight * 1000);
	}

	qsort(serverlist, answers, sizeof(kms_server_dns_ptr), kmsServerListCompareFunc1);
}


#define RECEIVE_BUFFER_SIZE 2048
#ifndef _WIN32 // UNIX resolver

/*
 * Retrieves a raw DNS answer (a buffer of what came over the net)
 * Result must be parsed
 */
static int getDnsRawAnswer(const char *restrict query, unsigned char** receive_buffer)
{
	if (res_init() < 0)
	{
		errorout("Cannot initialize resolver: %s", strerror(errno));
		return 0;
	}

	//if(!(*receive_buffer = (unsigned char*)malloc(RECEIVE_BUFFER_SIZE))) OutOfMemory();
	*receive_buffer = (unsigned char*)vlmcsd_malloc(RECEIVE_BUFFER_SIZE);

	int bytes_received;

	if (*query == '.')
	{
#		if __ANDROID__ || __GLIBC__ /* including __UCLIBC__*/ || __APPLE__ || __CYGWIN__ || __FreeBSD__ || __NetBSD__ || __DragonFly__ || __OpenBSD__ || __sun__
			bytes_received = res_querydomain("_vlmcs._tcp", query + 1, ns_c_in,	ns_t_srv, *receive_buffer, RECEIVE_BUFFER_SIZE);
#		else
			char* querystring = (char*)alloca(strlen(query) + 12);
			strcpy(querystring, "_vlmcs._tcp");
			strcat(querystring, query);
			bytes_received = res_query(querystring, ns_c_in, ns_t_srv, *receive_buffer, RECEIVE_BUFFER_SIZE);
#		endif
	}
	else
	{
		bytes_received = res_search("_vlmcs._tcp", ns_c_in, ns_t_srv, *receive_buffer, RECEIVE_BUFFER_SIZE);
	}

	if (bytes_received < 0)
	{
		errorout("Fatal: DNS query to %s%s failed: %s\n", "_vlmcs._tcp",	*query == '.' ? query : "", hstrerror(h_errno));
		return 0;
	}

	return bytes_received;
}

/*
 * Retrieves an unsorted array of SRV records (Unix / Posix)
 */
int getKmsServerList(kms_server_dns_ptr** serverlist, const char *restrict query)
{
	unsigned char* receive_buffer;
	*serverlist = NULL;

	int bytes_received = getDnsRawAnswer(query, &receive_buffer);

	if (bytes_received == 0) return 0;

	ns_msg msg;

	if (ns_initparse(receive_buffer, bytes_received, &msg) < 0)
	{
		errorout("Fatal: Incorrect DNS response: %s\n", strerror(errno));
		free(receive_buffer);
		return 0;
	}

	uint16_t i, answers = ns_msg_count(msg, ns_s_an);
	//if(!(*serverlist = (kms_server_dns_ptr*)malloc(answers * sizeof(kms_server_dns_ptr)))) OutOfMemory();
	*serverlist = (kms_server_dns_ptr*)malloc(answers * sizeof(kms_server_dns_ptr));

	memset(*serverlist, 0, answers * sizeof(kms_server_dns_ptr));

	for (i = 0; i < answers; i++)
	{
		ns_rr rr;

		if (ns_parserr(&msg, ns_s_an, i, &rr) < 0)
		{
			errorout("Warning: Error in DNS resource record: %s\n", strerror(errno));
			continue;
		}

		if (ns_rr_type(rr) != ns_t_srv)
		{
			errorout("Warning: DNS server returned non-SRV record\n");
			continue;
		}

		if (ns_rr_class(rr) != ns_c_in)
		{
			errorout("Warning: DNS server returned non-IN class record\n");
			continue;
		}

		dns_srv_record_ptr srvrecord = (dns_srv_record_ptr)ns_rr_rdata(rr);
		kms_server_dns_ptr kms_server = (kms_server_dns_ptr)vlmcsd_malloc(sizeof(kms_server_dns_t));

		(*serverlist)[i] = kms_server;

		if (ns_name_uncompress(ns_msg_base(msg), ns_msg_end(msg), srvrecord->name, kms_server->serverName, sizeof(kms_server->serverName)) < 0)
		{
			errorout("Warning: No valid DNS name returned in SRV record: %s\n", strerror(errno));
			continue;
		}

        sprintf(kms_server->serverName + strlen(kms_server->serverName), ":%hu", GET_UA16BE(&srvrecord->port));
        kms_server->priority = GET_UA16BE(&srvrecord->priority);
        kms_server->weight = GET_UA16BE(&srvrecord->weight);

	}

	free(receive_buffer);
	return answers;
}

#else // WIN32 (Windows Resolver)

/*
 * Retrieves an unsorted array of SRV records (Windows)
 */
int getKmsServerList(kms_server_dns_ptr** serverlist, const char *const restrict query)
{
#	define MAX_DNS_NAME_SIZE 254
	*serverlist = NULL;
	PDNS_RECORD receive_buffer;
	char dnsDomain[MAX_DNS_NAME_SIZE];
	char FqdnQuery[MAX_DNS_NAME_SIZE];
	DWORD size = MAX_DNS_NAME_SIZE;
	DNS_STATUS result;
	int answers = 0;
	PDNS_RECORD dns_iterator;

	if (*query == '-')
	{
		if (!GetComputerNameExA(ComputerNamePhysicalDnsDomain, dnsDomain, &size))
		{
			errorout("Fatal: Could not determine computer's DNS name: %s\n", vlmcsd_strerror(GetLastError()));
			return 0;
		}

		strcpy(FqdnQuery, "_vlmcs._tcp.");
		strncat(FqdnQuery, dnsDomain, MAX_DNS_NAME_SIZE - 12);
	}
	else
	{
		strcpy(FqdnQuery, "_vlmcs._tcp");
		strncat(FqdnQuery, query, MAX_DNS_NAME_SIZE - 11);
	}

	if ((result = DnsQuery_UTF8(FqdnQuery, DNS_TYPE_SRV, 0, NULL, &receive_buffer, NULL)) != 0)
	{
		errorout("Fatal: DNS query to %s failed: %s\n", FqdnQuery, vlmcsd_strerror(result));
		return 0;
	}

	for (dns_iterator = receive_buffer; dns_iterator; dns_iterator = dns_iterator->pNext)
	{
		if (dns_iterator->Flags.S.Section != 1) continue;

		if (dns_iterator->wType != DNS_TYPE_SRV)
		{
			errorout("Warning: DNS server returned non-SRV record\n");
			continue;
		}

		answers++;
	}

	*serverlist = (kms_server_dns_ptr*)vlmcsd_malloc(answers * sizeof(kms_server_dns_ptr));

	for (answers = 0, dns_iterator = receive_buffer; dns_iterator; dns_iterator = dns_iterator->pNext)
	{
		if (dns_iterator->wType != DNS_TYPE_SRV) continue;

		kms_server_dns_ptr kms_server = (kms_server_dns_ptr)vlmcsd_malloc(sizeof(kms_server_dns_t));

		memset(kms_server, 0, sizeof(kms_server_dns_t));

		snprintf(kms_server->serverName, sizeof(kms_server->serverName), "%s:%hu", dns_iterator->Data.SRV.pNameTarget, dns_iterator->Data.SRV.wPort);
		kms_server->priority = dns_iterator->Data.SRV.wPriority;
		kms_server->weight = dns_iterator->Data.SRV.wWeight;

		(*serverlist)[answers++] = kms_server;
	}

	//sortSrvRecords(*serverlist, answers, NoSrvRecordPriority);

	DnsRecordListFree(receive_buffer, DnsFreeRecordList);

	return answers;
#	undef MAX_DNS_NAME_SIZE
}
#endif // _WIN32
#undef RECEIVE_BUFFER_SIZE
#endif // NO_DNS


/*
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Modified by Hotbird64 for use with vlmcs.
 */

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifdef DNS_PARSER_INTERNAL
#ifndef NO_DNS

/* Import. */

#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "types.h"
#include "endian.h"
#include "ns_name.h"
#include "ns_parse.h"

/* Macros. */

#define NS_GET16_VLMCSD(s, cp) do { \
	(s) = GET_UA16BE(cp); \
	(cp) += NS_INT16SZ; \
} while (0)

#define NS_GET32_VLMCSD(l, cp) do { \
	(l) = GET_UA32BE(cp); \
	(cp) += NS_INT32SZ; \
} while (0)

#define RETERR(err) do { errno = (err); return (-1); } while (0)

/* Forward. */

static void	setsection_vlmcsd(ns_msg_vlmcsd *msg, ns_sect_vlmcsd sect);


static int dn_skipname_vlmcsd(const unsigned char *s, const unsigned char *end)
{
	const unsigned char *p;
	for (p=s; p<end; p++)
		if (!*p) return p-s+1;
		else if (*p>=192)
			{if (p+1<end) return p-s+2;
			else break;}
	return -1;
}

static int
ns_skiprr_vlmcsd(const uint8_t *ptr, const uint8_t *eom, ns_sect_vlmcsd section, int count) {
	const uint8_t *optr = ptr;

	for ((void)NULL; count > 0; count--) {
		int b, rdlength;

		b = dn_skipname_vlmcsd(ptr, eom);
		if (b < 0)
			RETERR(EMSGSIZE);
		ptr += b/*Name*/ + NS_INT16SZ/*Type*/ + NS_INT16SZ/*Class*/;
		if (section != ns_s_qd_vlmcsd) {
			if (ptr + NS_INT32SZ + NS_INT16SZ > eom)
				RETERR(EMSGSIZE);
			ptr += NS_INT32SZ/*TTL*/;
			NS_GET16_VLMCSD(rdlength, ptr);
			ptr += rdlength/*RData*/;
		}
	}
	if (ptr > eom)
		RETERR(EMSGSIZE);
	return (ptr - optr);
}

int
ns_initparse_vlmcsd(const uint8_t *msg, int msglen, ns_msg_vlmcsd *handle) {
	const uint8_t *eom = msg + msglen;
	int i;

	memset(handle, 0x5e, sizeof *handle);
	handle->_msg = msg;
	handle->_eom = eom;
	if (msg + NS_INT16SZ > eom)
		RETERR(EMSGSIZE);
	NS_GET16_VLMCSD(handle->_id, msg);
	if (msg + NS_INT16SZ > eom)
		RETERR(EMSGSIZE);
	NS_GET16_VLMCSD(handle->_flags, msg);
	for (i = 0; i < ns_s_max_vlmcsd; i++) {
		if (msg + NS_INT16SZ > eom)
			RETERR(EMSGSIZE);
		NS_GET16_VLMCSD(handle->_counts[i], msg);
	}
	for (i = 0; i < ns_s_max_vlmcsd; i++)
		if (handle->_counts[i] == 0)
			handle->_sections[i] = NULL;
		else {
			int b = ns_skiprr_vlmcsd(msg, eom, (ns_sect_vlmcsd)i,
					  handle->_counts[i]);

			if (b < 0)
				return (-1);
			handle->_sections[i] = msg;
			msg += b;
		}
	if (msg > eom)
		RETERR(EMSGSIZE);
	handle->_eom = msg;
	setsection_vlmcsd(handle, ns_s_max_vlmcsd);
	return (0);
}

int
ns_parserr_vlmcsd(ns_msg_vlmcsd *handle, ns_sect_vlmcsd section, int rrnum, ns_rr_vlmcsd *rr) {
	int b;

	/* Make section right. */
	if (section >= ns_s_max_vlmcsd)
		RETERR(ENODEV);
	if (section != handle->_sect)
		setsection_vlmcsd(handle, section);

	/* Make rrnum right. */
	if (rrnum == -1)
		rrnum = handle->_rrnum;
	if (rrnum < 0 || rrnum >= handle->_counts[(int)section])
		RETERR(ENODEV);
	if (rrnum < handle->_rrnum)
		setsection_vlmcsd(handle, section);
	if (rrnum > handle->_rrnum) {
		b = ns_skiprr_vlmcsd(handle->_msg_ptr, handle->_eom, section,
			      rrnum - handle->_rrnum);

		if (b < 0)
			return (-1);
		handle->_msg_ptr += b;
		handle->_rrnum = rrnum;
	}

	/* Do the parse. */
	b = ns_name_uncompress_vlmcsd(handle->_msg, handle->_eom,
		      handle->_msg_ptr, rr->name, NS_MAXDNAME);
	if (b < 0)
		return (-1);
	handle->_msg_ptr += b;
	if (handle->_msg_ptr + NS_INT16SZ + NS_INT16SZ > handle->_eom)
		RETERR(EMSGSIZE);
	NS_GET16_VLMCSD(rr->type, handle->_msg_ptr);
	NS_GET16_VLMCSD(rr->rr_class, handle->_msg_ptr);
	if (section == ns_s_qd_vlmcsd) {
		rr->ttl = 0;
		rr->rdlength = 0;
		rr->rdata = NULL;
	} else {
		if (handle->_msg_ptr + NS_INT32SZ + NS_INT16SZ > handle->_eom)
			RETERR(EMSGSIZE);
		NS_GET32_VLMCSD(rr->ttl, handle->_msg_ptr);
		NS_GET16_VLMCSD(rr->rdlength, handle->_msg_ptr);
		if (handle->_msg_ptr + rr->rdlength > handle->_eom)
			RETERR(EMSGSIZE);
		rr->rdata = handle->_msg_ptr;
		handle->_msg_ptr += rr->rdlength;
	}
	if (++handle->_rrnum > handle->_counts[(int)section])
		setsection_vlmcsd(handle, (ns_sect_vlmcsd)((int)section + 1));

	/* All done. */
	return (0);
}

/* Private. */

static void
setsection_vlmcsd(ns_msg_vlmcsd *msg, ns_sect_vlmcsd sect) {
	msg->_sect = sect;
	if (sect == ns_s_max_vlmcsd) {
		msg->_rrnum = -1;
		msg->_msg_ptr = NULL;
	} else {
		msg->_rrnum = 0;
		msg->_msg_ptr = msg->_sections[(int)sect];
	}
}

#endif // !NO_DNS
#endif // DNS_PARSER_INTERNAL
/*
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Modified by Hotbird64 for use with vlmcs.
 */

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifdef DNS_PARSER_INTERNAL
#ifndef NO_DNS

#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "types.h"
#include "ns_name.h"

#ifdef SPRINTF_CHAR
# define SPRINTF(x) strlen(sprintf/**/x)
#else
# define SPRINTF(x) ((size_t)sprintf x)
#endif

#define NS_TYPE_ELT			0x40 /* EDNS0 extended label type */
#define DNS_LABELTYPE_BITSTRING		0x41

#define NS_MAXCDNAME 255
#define NS_CMPRSFLGS 0xc0

/* Data. */

static char	digits[] = "0123456789";


/* Forward. */

static int		special_vlmcsd(int);
static int		printable_vlmcsd(int);
static int		labellen_vlmcsd(const uint8_t *);
static int		decode_bitstring_vlmcsd(const char **, char *, const char *);

/*
 * ns_name_ntop(src, dst, dstsiz)
 *	Convert an encoded domain name to printable ascii as per RFC1035.
 * return:
 *	Number of bytes written to buffer, or -1 (with errno set)
 * notes:
 *	The root is returned as "."
 *	All other domains are returned in non absolute form
 */
static int
ns_name_ntop_vlmcsd(const uint8_t *src, char *dst, size_t dstsiz)
{
	const uint8_t *cp;
	char *dn, *eom;
	uint8_t c;
	uint32_t n;
	int l;

	cp = src;
	dn = dst;
	eom = dst + dstsiz;

	while ((n = *cp++) != 0) {
		if ((n & NS_CMPRSFLGS) == NS_CMPRSFLGS) {
			/* Some kind of compression pointer. */
			errno = EMSGSIZE;
			return (-1);
		}
		if (dn != dst) {
			if (dn >= eom) {
				errno = EMSGSIZE;
				return (-1);
			}
			*dn++ = '.';
		}
		if ((l = labellen_vlmcsd(cp - 1)) < 0) {
			errno = EMSGSIZE; /* XXX */
			return(-1);
		}
		if (dn + l >= eom) {
			errno = EMSGSIZE;
			return (-1);
		}
		if ((n & NS_CMPRSFLGS) == NS_TYPE_ELT) {
			int m;

			if (n != DNS_LABELTYPE_BITSTRING) {
				/* XXX: labellen should reject this case */
				errno = EINVAL;
				return(-1);
			}
			if ((m = decode_bitstring_vlmcsd((const char **)&cp, dn, eom)) < 0)
			{
				errno = EMSGSIZE;
				return(-1);
			}
			dn += m; 
			continue;
		}
		for ((void)NULL; l > 0; l--) {
			c = *cp++;
			if (special_vlmcsd(c)) {
				if (dn + 1 >= eom) {
					errno = EMSGSIZE;
					return (-1);
				}
				*dn++ = '\\';
				*dn++ = (char)c;
			} else if (!printable_vlmcsd(c)) {
				if (dn + 3 >= eom) {
					errno = EMSGSIZE;
					return (-1);
				}
				*dn++ = '\\';
				*dn++ = digits[c / 100];
				*dn++ = digits[(c % 100) / 10];
				*dn++ = digits[c % 10];
			} else {
				if (dn >= eom) {
					errno = EMSGSIZE;
					return (-1);
				}
				*dn++ = (char)c;
			}
		}
	}
	if (dn == dst) {
		if (dn >= eom) {
			errno = EMSGSIZE;
			return (-1);
		}
		*dn++ = '.';
	}
	if (dn >= eom) {
		errno = EMSGSIZE;
		return (-1);
	}
	*dn++ = '\0';
	return (dn - dst);
}

static int
ns_name_unpack_vlmcsd(const uint8_t *msg, const uint8_t *eom, const uint8_t *src,
	       uint8_t *dst, size_t dstsiz)
{
	const uint8_t *srcp, *dstlim;
	uint8_t *dstp;
	int n, len, checked, l;

	len = -1;
	checked = 0;
	dstp = dst;
	srcp = src;
	dstlim = dst + dstsiz;
	if (srcp < msg || srcp >= eom) {
		errno = EMSGSIZE;
		return (-1);
	}
	/* Fetch next label in domain name. */
	while ((n = *srcp++) != 0) {
		/* Check for indirection. */
		switch (n & NS_CMPRSFLGS) {
		case 0:
		case NS_TYPE_ELT:
			/* Limit checks. */
			if ((l = labellen_vlmcsd(srcp - 1)) < 0) {
				errno = EMSGSIZE;
				return(-1);
			}
			if (dstp + l + 1 >= dstlim || srcp + l >= eom) {
				errno = EMSGSIZE;
				return (-1);
			}
			checked += l + 1;
			*dstp++ = n;
			memcpy(dstp, srcp, l);
			dstp += l;
			srcp += l;
			break;

		case NS_CMPRSFLGS:
			if (srcp >= eom) {
				errno = EMSGSIZE;
				return (-1);
			}
			if (len < 0)
				len = srcp - src + 1;
			srcp = msg + (((n & 0x3f) << 8) | (*srcp & 0xff));
			if (srcp < msg || srcp >= eom) {  /* Out of range. */
				errno = EMSGSIZE;
				return (-1);
			}
			checked += 2;
			/*
			 * Check for loops in the compressed name;
			 * if we've looked at the whole message,
			 * there must be a loop.
			 */
			if (checked >= eom - msg) {
				errno = EMSGSIZE;
				return (-1);
			}
			break;

		default:
			errno = EMSGSIZE;
			return (-1);			/* flag error */
		}
	}
	*dstp = '\0';
	if (len < 0)
		len = srcp - src;
	return (len);
}


/*
 * ns_name_uncompress_vlmcsd(msg, eom, src, dst, dstsiz)
 *	Expand compressed domain name to presentation format.
 * return:
 *	Number of bytes read out of `src', or -1 (with errno set).
 * note:
 *	Root domain returns as "." not "".
 */
int
ns_name_uncompress_vlmcsd(const uint8_t *msg, const uint8_t *eom, const uint8_t *src,
		   char *dst, size_t dstsiz)
{
	uint8_t tmp[NS_MAXCDNAME];
	int n;
	
	if ((n = ns_name_unpack_vlmcsd(msg, eom, src, tmp, sizeof tmp)) == -1)
		return (-1);
	if (ns_name_ntop_vlmcsd(tmp, dst, dstsiz) == -1)
		return (-1);
	return (n);
}

/*
 * special(ch)
 *	Thinking in noninternationalized USASCII (per the DNS spec),
 *	is this characted special ("in need of quoting") ?
 * return:
 *	boolean.
 */
static int
special_vlmcsd(int ch) {
	switch (ch) {
	case 0x22: /* '"' */
	case 0x2E: /* '.' */
	case 0x3B: /* ';' */
	case 0x5C: /* '\\' */
	case 0x28: /* '(' */
	case 0x29: /* ')' */
	/* Special modifiers in zone files. */
	case 0x40: /* '@' */
	case 0x24: /* '$' */
		return (1);
	default:
		return (0);
	}
}

/*
 * printable(ch)
 *	Thinking in noninternationalized USASCII (per the DNS spec),
 *	is this character visible and not a space when printed ?
 * return:
 *	boolean.
 */
static int
printable_vlmcsd(int ch) {
	return (ch > 0x20 && ch < 0x7f);
}

static int
decode_bitstring_vlmcsd(const char **cpp, char *dn, const char *eom)
{
	const char *cp = *cpp;
	char *beg = dn, tc;
	int b, blen, plen;

	if ((blen = (*cp & 0xff)) == 0)
		blen = 256;
	plen = (blen + 3) / 4;
	plen += sizeof("\\[x/]") + (blen > 99 ? 3 : (blen > 9) ? 2 : 1);
	if (dn + plen >= eom)
		return(-1);

	cp++;
	dn += SPRINTF((dn, "\\[x"));
	for (b = blen; b > 7; b -= 8, cp++)
		dn += SPRINTF((dn, "%02x", *cp & 0xff));
	if (b > 4) {
		tc = *cp++;
		dn += SPRINTF((dn, "%02x", tc & (0xff << (8 - b))));
	} else if (b > 0) {
		tc = *cp++;
		dn += SPRINTF((dn, "%1x",
			       ((tc >> 4) & 0x0f) & (0x0f << (4 - b)))); 
	}
	dn += SPRINTF((dn, "/%d]", blen));

	*cpp = cp;
	return(dn - beg);
}

static int
labellen_vlmcsd(const uint8_t *lp)
{
	int bitlen;
	uint8_t l = *lp;

	if ((l & NS_CMPRSFLGS) == NS_CMPRSFLGS) {
		/* should be avoided by the caller */
		return(-1);
	}

	if ((l & NS_CMPRSFLGS) == NS_TYPE_ELT) {
		if (l == DNS_LABELTYPE_BITSTRING) {
			if ((bitlen = *(lp + 1)) == 0)
				bitlen = 256;
			return((bitlen + 7 ) / 8 + 1);
		}
		return(-1);	/* unknwon ELT */
	}
	return(l);
}

#endif // !NO_DNS
#endif // DNS_PARSER_INTERNAL
