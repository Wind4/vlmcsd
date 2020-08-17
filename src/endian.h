#ifndef __endian_h
#define __endian_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

//
// Unaligned access
//

#if !defined(NO_COMPILER_UAA)
#define UAA16(p, i) (((PACKED16*)p)->val[i])
#define UAA32(p, i) (((PACKED32*)p)->val[i])
#define UAA64(p, i) (((PACKED64*)p)->val[i])
#endif 

//
//Byteswap: Use compiler support if available
//
#ifndef NO_COMPILER_UAA
#ifdef __has_builtin // Clang supports this

#if __has_builtin(__builtin_bswap16)
#define BS16(x) __builtin_bswap16(x)
#endif

#if __has_builtin(__builtin_bswap32)
#define BS32(x) __builtin_bswap32(x)
#endif

#if __has_builtin(__builtin_bswap64)
#define BS64(x) __builtin_bswap64(x)
#endif

#endif // has_builtin

#ifdef __GNUC__ // GNU C >= 4.3 has bswap32 and bswap64. GNU C >= 4.8 also has bswap16
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ > 2)

#ifndef BS32
#define BS32(x) __builtin_bswap32(x)
#endif

#ifndef BS64
#define BS64(x) __builtin_bswap64(x)
#endif

#if (__GNUC__ > 4) || (__GNUC_MINOR__ > 7)

#ifndef BS16
#define BS16(x) __builtin_bswap16(x)
#endif

#endif // GNU C > 4.7
#endif // __GNUC__ > 4
#endif // __GNUC__
#endif // NO_COMPILER_UAA

//
// Byteorder
//
#if defined(__linux__) || defined(__GLIBC__) || defined(__CYGWIN__)

#include <endian.h>
#include <byteswap.h>

#ifndef BS16
#define BS16(x) bswap_16(x)
#endif

#ifndef BS32
#define BS32(x) bswap_32(x)
#endif

#ifndef BS64
#define BS64(x) bswap_64(x)
#endif

#elif defined(__sun__)

#include <sys/byteorder.h>

#ifndef BS16
#define BS16(x) BSWAP_16(x)
#endif

#ifndef BS32
#define BS32(x) BSWAP_32(x)
#endif

#ifndef BS64
#define BS64(x) BSWAP_64(x)
#endif

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321

#ifdef _LITTLE_ENDIAN
#define __BYTE_ORDER __LITTLE_ENDIAN
#else
#define __BYTE_ORDER __BIG_ENDIAN
#endif

#elif __minix__ || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__OpenBSD__)

#include <sys/types.h>
#include <sys/endian.h>

#define __BYTE_ORDER     _BYTE_ORDER
#define __LITTLE_ENDIAN  _LITTLE_ENDIAN
#define __BIG_ENDIAN     _BIG_ENDIAN

#ifdef __OpenBSD__

#ifndef BS16
#define BS16  swap16
#endif

#ifndef BS32
#define BS32  swap32
#endif

#ifndef BS64
#define BS64  swap64
#endif

#else // !__OpenBSD__

#ifndef BS16
#define BS16  bswap16
#endif

#ifndef BS32
#define BS32  bswap32
#endif

#ifndef BS64
#define BS64  bswap64
#endif

#endif // !__OpenBSD__

#elif defined(__APPLE__)

#include <sys/types.h>
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>

#define __BYTE_ORDER     _BYTE_ORDER
#define __LITTLE_ENDIAN  _LITTLE_ENDIAN
#define __BIG_ENDIAN     _BIG_ENDIAN

#ifndef BS16
#define BS16 OSSwapInt16
#endif

#ifndef BS32
#define BS32 OSSwapInt32
#endif

#ifndef BS64
#define BS64 OSSwapInt64
#endif

#elif defined(_WIN32)

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN

#include <stdlib.h>

#ifndef BS16
#define BS16 _byteswap_ushort
#endif

#ifndef BS32
#define BS32 _byteswap_ulong
#endif

#ifndef BS64
#define BS64 _byteswap_uint64
#endif

#endif // Byteorder in different OS


#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && defined(__LITTLE_ENDIAN)  \
	&& defined(BS16) && defined(BS32) && defined(BS64)

#if __BYTE_ORDER == __LITTLE_ENDIAN

#define __BE16(x)  BS16(x)
#define __LE16(x)  (x)
#define __BE32(x)  BS32(x)
#define __LE32(x)  (x)
#define __BE64(x)  BS64(x)
#define __LE64(x)  (x)

#define PUT_UA16(p, v) PUT_UA16LE(p, v)
#define PUT_UA32(p, v) PUT_UA32LE(p, v)
#define PUT_UA64(p, v) PUT_UA64LE(p, v)

#else // __BYTE_ORDER == __BIG_ENDIAN

#define __BE16(x)  (x)
#define __LE16(x)  BS16(x)
#define __BE32(x)  (x)
#define __LE32(x)  BS32(x)
#define __BE64(x)  (x)
#define __LE64(x)  BS64(x)

#define PUT_UA16(p, v) PUT_UA16BE(p, v)
#define PUT_UA32(p, v) PUT_UA32BE(p, v)
#define PUT_UA64(p, v) PUT_UA64BE(p, v)

#endif // __BYTE_ORDER

#define BE16(x)  __BE16(x)
#define LE16(x)  __LE16(x)
#define BE32(x)  __BE32(x)
#define LE32(x)  __LE32(x)
#define BE64(x)  __BE64(x)
#define LE64(x)  __LE64(x)

#else

extern unsigned short BE16(unsigned short x);

extern unsigned short LE16(unsigned short x);

extern unsigned int BE32(unsigned int x);

extern unsigned int LE32(unsigned int x);

extern unsigned long long BE64(unsigned long long x);

extern unsigned long long LE64(unsigned long long x);

#endif // defined(__BYTE_ORDER)

#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && defined(__LITTLE_ENDIAN)  \
	&& defined(BS16) && defined(BS32) && defined(BS64) &&!defined(NO_COMPILER_UAA)

#define PUT_UAA64BE(p, v, i)  ( UAA64(p, i) = __BE64(v) )
#define PUT_UAA32BE(p, v, i)  ( UAA32(p, i) = __BE32(v) )
#define PUT_UAA16BE(p, v, i)  ( UAA16(p, i) = __BE16(v) )

#define PUT_UAA64LE(p, v, i)  ( UAA64(p, i) = __LE64(v) )
#define PUT_UAA32LE(p, v, i)  ( UAA32(p, i) = __LE32(v) )
#define PUT_UAA16LE(p, v, i)  ( UAA16(p, i) = __LE16(v) )

#define GET_UAA64BE(p, i)  __BE64(UAA64(p, i))
#define GET_UAA32BE(p, i)  __BE32(UAA32(p, i))
#define GET_UAA16BE(p, i)  __BE16(UAA16(p, i))

#define GET_UAA64LE(p, i)  __LE64(UAA64(p, i))
#define GET_UAA32LE(p, i)  __LE32(UAA32(p, i))
#define GET_UAA16LE(p, i)  __LE16(UAA16(p, i))


#else // ! defined(__BYTE_ORDER)

extern void PUT_UAA64BE(void* p, unsigned long long v, unsigned int i);

extern void PUT_UAA32BE(void* p, unsigned int v, unsigned int i);

extern void PUT_UAA16BE(void* p, unsigned short v, unsigned int i);


extern void PUT_UAA64LE(void* p, unsigned long long v, unsigned int i);

extern void PUT_UAA32LE(void* p, unsigned int v, unsigned int i);

extern void PUT_UAA16LE(void* p, unsigned short v, unsigned int i);


extern unsigned long long GET_UAA64BE(void* p, unsigned int i);

extern unsigned int GET_UAA32BE(void* p, unsigned int i);

extern unsigned short GET_UAA16BE(void* p, unsigned int i);


extern unsigned long long GET_UAA64LE(void* p, unsigned int i);

extern unsigned int GET_UAA32LE(void* p, unsigned int i);

extern unsigned short GET_UAA16LE(void* p, unsigned int i);
#endif



#define PUT_UA64BE(p, v)  PUT_UAA64BE(p, v, 0)
#define PUT_UA32BE(p, v)  PUT_UAA32BE(p, v, 0)
#define PUT_UA16BE(p, v)  PUT_UAA16BE(p, v, 0)

#define PUT_UA64LE(p, v)  PUT_UAA64LE(p, v, 0)
#define PUT_UA32LE(p, v)  PUT_UAA32LE(p, v, 0)
#define PUT_UA16LE(p, v)  PUT_UAA16LE(p, v, 0)

#define GET_UA64BE(p)  GET_UAA64BE(p, 0)
#define GET_UA32BE(p)  GET_UAA32BE(p, 0)
#define GET_UA16BE(p)  GET_UAA16BE(p, 0)

#define GET_UA64LE(p)  GET_UAA64LE(p, 0)
#define GET_UA32LE(p)  GET_UAA32LE(p, 0)
#define GET_UA16LE(p)  GET_UAA16LE(p, 0)

#endif // __endian_h
