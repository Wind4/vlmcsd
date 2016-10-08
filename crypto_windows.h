/*
 * crypto_windows.h
 */

#ifdef _CRYPTO_WINDOWS
#ifndef CRYPTO_WINDOWS_H_
#define CRYPTO_WINDOWS_H_

#if !_WIN32 && !__CYGWIN__
#error You cannot use Windows CryptoAPI on non-Windows platforms
#else // _WIN32 || __CYGWIN__

#include "types.h"
#if _MSC_VER
#include "Wincrypt.h"
#endif

typedef struct _Sha2356HmacCtx
{
	HCRYPTHASH hHmac;
	HCRYPTKEY hKey;
} Sha256HmacCtx;

int_fast8_t Sha256(BYTE* restrict data, DWORD DataSize, BYTE* restrict hash);
int_fast8_t Sha256Hmac(const BYTE* key, BYTE* restrict data, DWORD len, BYTE* restrict hmac);

/*int_fast8_t Sha256HmacInit(Sha256HmacCtx *Ctx, BYTE *key, uint8_t keySize);
int_fast8_t Sha256HmacUpdate(Sha256HmacCtx *Ctx, BYTE *data, DWORD len);
int_fast8_t Sha256HmacFinish(Sha256HmacCtx *Ctx, BYTE *hmac);*/


#endif // _WIN32 || __CYGWIN__
#endif /* CRYPTO_WINDOWS_H_ */
#endif // _CRYPTO_WINDOWS
