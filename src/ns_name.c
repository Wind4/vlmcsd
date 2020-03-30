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
ns_name_uncompress_vlmcsd(uint8_t *msg, uint8_t *eom, uint8_t *src,
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
