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


static int dn_skipname_vlmcsd(unsigned char *s, unsigned char *end)
{
	unsigned char *p;
	for (p=s; p<end; p++)
		if (!*p) return p-s+1;
		else if (*p>=192)
			{if (p+1<end) return p-s+2;
			else break;}
	return -1;
}

static int
ns_skiprr_vlmcsd(uint8_t *ptr, uint8_t *eom, ns_sect_vlmcsd section, int count) {
	uint8_t *optr = ptr;

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
ns_initparse_vlmcsd(uint8_t *msg, int msglen, ns_msg_vlmcsd *handle) {
	uint8_t *eom = msg + msglen;
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
