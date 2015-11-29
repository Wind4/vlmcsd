#ifndef __crypto_openssl_h
#define __crypto_openssl_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include <openssl/opensslv.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include "crypto.h"

#define Sha256(d, l, h)  SHA256(d, l, h)
int_fast8_t Sha256Hmac(BYTE* key, BYTE* restrict data, DWORD len, BYTE* restrict hmac);

#ifndef _OPENSSL_NO_HMAC
#define Sha256HmacCtx    HMAC_CTX
#else
typedef struct {
	SHA256_CTX  ShaCtx;
	BYTE  OPad[64];
} Sha256HmacCtx;
#endif

#ifndef _OPENSSL_NO_HMAC

#define Sha256HmacInit(c, k, l)    Sha256HmacInit_OpenSSL(c, k, l)
#define Sha256HmacFinish(c, h)     Sha256HmacFinish_OpenSSL(c, h, NULL)

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
#define Sha256HmacUpdate(c, d, l)  HMAC_Update(c, d, l)
#else // OPENSSL_VERSION_NUMBER < 0x10000000L
#define Sha256HmacUpdate(c, d, l)  (HMAC_Update(c, d, l), !0)
#endif // OPENSSL_VERSION_NUMBER >= 0x10000000L

int Sha256HmacInit_OpenSSL(HMAC_CTX *c, const void *k, int l);
int Sha256HmacFinish_OpenSSL(HMAC_CTX *c, unsigned char *h, unsigned int *l);

#else // _OPENSSL_NO_HMAC

int _Sha256HmacInit(Sha256HmacCtx *Ctx, BYTE *key, size_t klen);
int _Sha256HmacUpdate(Sha256HmacCtx *Ctx, BYTE *data, size_t len);
int _Sha256HmacFinish(Sha256HmacCtx *Ctx, BYTE *hmac, void* dummy);
#define Sha256HmacInit(c, k, l)    _Sha256HmacInit(c, k, l)
#define Sha256HmacFinish(c, h)     _Sha256HmacFinish(c, h, NULL)
#define Sha256HmacUpdate(c, d, l)  _Sha256HmacUpdate(c, d, l)

#endif // _OPENSSL_NO_HMAC

extern const BYTE AesKeyV4[];
#endif // __crypto_openssl_h
