#ifndef __crypto_polarssl_h
#define __crypto_polarssl_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include <polarssl/version.h>
#include "crypto.h"

#if POLARSSL_VERSION_NUMBER >= 0x01030000

#include <polarssl/sha256.h>

#define Sha256(d, l, h)  sha256(d, l, h, 0)

#define Sha256HmacCtx              sha256_context
#define Sha256HmacInit(c, k, l)    ( sha256_hmac_starts(c, k, l, 0),     !0 )
#define Sha256HmacUpdate(c, d, l)  ( sha256_hmac_update(c, d, l),        !0 )
#define Sha256HmacFinish(c, h)     ( sha256_hmac_finish(c, h),           !0 )
#define Sha256Hmac(k, d, l, h)     ( sha256_hmac(k, 16, d, l, h, FALSE), !0 )

#else // POLARSSL_VERSION_NUMBER

#include <polarssl/sha2.h>

#define Sha256(d, l, h)  sha2(d, l, h, 0)

#define Sha256HmacCtx				sha2_context
#define Sha256HmacInit(c, k, l)		( sha2_hmac_starts(c, k, l, 0),     !0 )
#define Sha256HmacUpdate(c, d, l)	( sha2_hmac_update(c, d, l),        !0 )
#define Sha256HmacFinish(c, h)		( sha2_hmac_finish(c, h),           !0 )
#define Sha256Hmac(k, d, l, h)		( sha2_hmac(k, 16, d, l, h, FALSE), !0 )

#endif // POLARSSL_VERSION_NUMBER
#endif // __crypto_polarssl_h


