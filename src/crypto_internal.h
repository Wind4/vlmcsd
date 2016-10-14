#ifndef __crypto_internal_h
#define __crypto_internal_h

#if !defined(_CRYPTO_OPENSSL) && !defined(_CRYPTO_POLARSSL) && !defined(_CRYPTO_WINDOWS)

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "crypto.h"

typedef struct {
	DWORD  State[8];
	BYTE   Buffer[64];
	unsigned int  Len;
} Sha256Ctx;

typedef struct {
	Sha256Ctx  ShaCtx;
	BYTE  OPad[64];
} Sha256HmacCtx;

void Sha256(BYTE *data, size_t len, BYTE *hash);
int_fast8_t Sha256Hmac(BYTE* key, BYTE* restrict data, DWORD len, BYTE* restrict hmac);

//void _Sha256HmacInit(Sha256HmacCtx *Ctx, BYTE *key, size_t klen);
//void _Sha256HmacUpdate(Sha256HmacCtx *Ctx, BYTE *data, size_t len);
//void _Sha256HmacFinish(Sha256HmacCtx *Ctx, BYTE *hmac);

//#define Sha256HmacInit(c, k, l)    ( _Sha256HmacInit(c, k, l),   !0 )
//#define Sha256HmacUpdate(c, d, l)  ( _Sha256HmacUpdate(c, d, l), !0 )
//#define Sha256HmacFinish(c, h)     ( _Sha256HmacFinish(c, h),    !0 )


#endif // !defined(_CRYPTO_OPENSSL) && !defined(_CRYPTO_POLARSSL) && !defined(_CRYPTO_WINDOWS)

#endif // __crypto_internal_h
