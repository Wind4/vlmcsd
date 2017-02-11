#ifndef HELPERS_H
#define HELPERS_H

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include <stdint.h>
#include "types.h"

#if __ANDROID__
#include <sys/syscall.h>
#endif // __ANDROID__

#define GUID_LE 0
#define GUID_BE 1
#define GUID_SWAP 2

BOOL stringToInt(const char *const szValue, const unsigned int min, const unsigned int max, unsigned int *const value);
unsigned int getOptionArgumentInt(const char o, const unsigned int min, const unsigned int max);
void optReset(void);
__pure DWORD timeSpanString2Seconds(const char *const restrict argument);
#define timeSpanString2Minutes(x) (timeSpanString2Seconds(x) / 60)
char* win_strerror(const int message);
int ucs2_to_utf8_char (const WCHAR ucs2_le, char *utf8);
size_t utf8_to_ucs2(WCHAR* const ucs2_le, const char* const utf8, const size_t maxucs2, const size_t maxutf8);
WCHAR utf8_to_ucs2_char (const unsigned char * input, const unsigned char ** end_ptr);
BOOL ucs2_to_utf8(const WCHAR* const ucs2_le, char* utf8, size_t maxucs2, size_t maxutf8);
int_fast8_t string2UuidLE(const char *const restrict input, GUID *const restrict guid);
void randomNumberInit();
void parseAddress(char *const addr, char** szHost, char** szPort);
__noreturn void OutOfMemory(void);
void* vlmcsd_malloc(size_t len);
void hex2bin(BYTE *const bin, const char *hex, const size_t maxbin);
void loadKmsData();
#if !defined(DATA_FILE) || !defined(NO_SIGHUP)
void getExeName();
#endif // !defined(DATA_FILE) || !defined(NO_SIGHUP)
__pure BOOL getArgumentBool(int_fast8_t *result, const char *const argument);
char* vlmcsd_strdup(const char* src);

#if defined(NO_SOCKETS) || IS_LIBRARY
#define exitOnWarningLevel(x)
#else // !NO_SOCKETS
void exitOnWarningLevel(const int_fast8_t level);
#endif // !NO_SOCKETS


#if __ANDROID__ && !defined(USE_THREADS) // Bionic does not wrap these syscalls (intentionally because Google fears, developers don't know how to use it)
int shmget(key_t key, size_t size, int shmflg);
void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmdt(const void *shmaddr);
int shmctl(int shmid, int cmd, /*struct shmid_ds*/void *buf);
#endif // __ANDROID__ && !defined(USE_THREADS)

#endif // HELPERS_H
