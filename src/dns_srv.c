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

	for (;;)
	{
		if (g*g > n)
			g ^= c;

		c >>= 1;

		if (c == 0) return g;

		g |= c;
	}
}


/*
 * Compare function for qsort to sort SRV records by priority and weight
 * random_weight must be product of weight from SRV record and square root of a random number
 */
static int kmsServerListCompareFunc1(const void* a, const void* b)
{
	if (!a && !b) return 0;
	if (a && !b) return -1;
	if (!a && b) return 1;

	int priority_order = (int)((*(kms_server_dns_ptr*)a)->priority) - ((int)(*(kms_server_dns_ptr*)b)->priority);

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
		bytes_received = res_querydomain("_vlmcs._tcp", query + 1, ns_c_in, ns_t_srv, *receive_buffer, RECEIVE_BUFFER_SIZE);
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
		errorout("Fatal: DNS query to %s%s failed: %s\n", "_vlmcs._tcp", *query == '.' ? query : "", hstrerror(h_errno));
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
	* serverlist = NULL;
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

		vlmcsd_snprintf(kms_server->serverName, sizeof(kms_server->serverName), "%s:%hu", dns_iterator->Data.SRV.pNameTarget, dns_iterator->Data.SRV.wPort);
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


