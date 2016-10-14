#ifndef HELPERS_H
#define HELPERS_H

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include <stdint.h>
#include "types.h"

#define GUID_LE 0
#define GUID_BE 1
#define GUID_SWAP 2

BOOL stringToInt(const char *const szValue, const unsigned int min, const unsigned int max, unsigned int *const value);
unsigned int getOptionArgumentInt(const char o, const unsigned int min, const unsigned int max);
void optReset(void);
char* win_strerror(const int message);
int ucs2_to_utf8_char (const WCHAR ucs2_le, char *utf8);
size_t utf8_to_ucs2(WCHAR* const ucs2_le, const char* const utf8, const size_t maxucs2, const size_t maxutf8);
WCHAR utf8_to_ucs2_char (const unsigned char * input, const unsigned char ** end_ptr);
BOOL ucs2_to_utf8(const WCHAR* const ucs2_le, char* utf8, size_t maxucs2, size_t maxutf8);
int_fast8_t string2Uuid(const char *const restrict input, GUID *const restrict guid);
void randomNumberInit();
void LEGUID(GUID *const restrict result, const GUID* const restrict guid);
void parseAddress(char *const addr, char** szHost, char** szPort);
__noreturn void OutOfMemory(void);
void* vlmcsd_malloc(size_t len);
void hex2bin(BYTE *const bin, const char *hex, const size_t maxbin);
__pure BOOL getArgumentBool(int_fast8_t *result, const char *const argument);


#endif // HELPERS_H
