#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "endian.h"

#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && defined(__LITTLE_ENDIAN)  \
	&& defined(BS16) && defined(BS32) && defined(BS64) && !defined(NO_COMPILER_UAA)

#else // ! defined(__BYTE_ORDER)

void PUT_UAA64BE(void *p, unsigned long long v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned long long *)p)[i];
	_p[ 0 ] = (unsigned char)(v >> 56);
	_p[ 1 ] = (unsigned char)(v >> 48);
	_p[ 2 ] = (unsigned char)(v >> 40);
	_p[ 3 ] = (unsigned char)(v >> 32);
	_p[ 4 ] = (unsigned char)(v >> 24);
	_p[ 5 ] = (unsigned char)(v >> 16);
	_p[ 6 ] = (unsigned char)(v >> 8);
	_p[ 7 ] = (unsigned char)(v);
}

void PUT_UAA32BE(void *p, unsigned int v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned int *)p)[i];
	_p[ 0 ] = (unsigned char)(v >> 24);
	_p[ 1 ] = (unsigned char)(v >> 16);
	_p[ 2 ] = (unsigned char)(v >> 8);
	_p[ 3 ] = (unsigned char)(v);
}

void PUT_UAA16BE(void *p, unsigned short v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned short *)p)[i];
	_p[ 0 ] = (unsigned char)(v >> 8);
	_p[ 1 ] = (unsigned char)(v);
}


void PUT_UAA64LE(void *p, unsigned long long v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned long long *)p)[i];
	_p[ 0 ] = (unsigned char)(v);
	_p[ 1 ] = (unsigned char)(v >> 8);
	_p[ 2 ] = (unsigned char)(v >> 16);
	_p[ 3 ] = (unsigned char)(v >> 24);
	_p[ 4 ] = (unsigned char)(v >> 32);
	_p[ 5 ] = (unsigned char)(v >> 40);
	_p[ 6 ] = (unsigned char)(v >> 48);
	_p[ 7 ] = (unsigned char)(v >> 56);
}

void PUT_UAA32LE(void *p, unsigned int v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned int *)p)[i];
	_p[ 0 ] = (unsigned char)(v);
	_p[ 1 ] = (unsigned char)(v >> 8);
	_p[ 2 ] = (unsigned char)(v >> 16);
	_p[ 3 ] = (unsigned char)(v >> 24);
}

void PUT_UAA16LE(void *p, unsigned short v, unsigned int i)
{
	unsigned char *_p = (unsigned char *)&((unsigned short *)p)[i];
	_p[ 0 ] = (unsigned char)(v);
	_p[ 1 ] = (unsigned char)(v >> 8);
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
#endif


#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && defined(__LITTLE_ENDIAN)  \
	&& defined(BS16) && defined(BS32) && defined(BS64)
#else
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

unsigned long long LE64(unsigned long long x)
{
	return GET_UAA64LE(&x, 0);
}

#endif // defined(__BYTE_ORDER)
