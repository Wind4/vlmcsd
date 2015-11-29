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
		!strncasecmp(argument, "on", 4) ||
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

