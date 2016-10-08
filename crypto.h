#ifndef __crypto_h
#define __crypto_h

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "endian.h"
#include <stdint.h>

#define AES_KEY_BYTES   (16) // 128 Bits
#define AES_BLOCK_BYTES (16)
#define AES_BLOCK_WORDS (AES_BLOCK_BYTES / sizeof(DWORD))
#define AES_KEY_DWORDS  (AES_KEY_BYTES / sizeof(DWORD))
#define V4_KEY_BYTES	(20) // 160 Bits

#define ROR32(v, n)  ( (v) << (32 - n) | (v) >> n )

void XorBlock(const BYTE *const in, const BYTE *out);

void AesCmacV4(BYTE *data, size_t len, BYTE *hash);

extern const BYTE AesKeyV5[];
extern const BYTE AesKeyV6[];

typedef struct {
	DWORD  Key[48]; // Supports a maximum of 160 key bits!
	uint_fast8_t rounds;
} AesCtx;

void AesInitKey(AesCtx *Ctx, const BYTE *Key, int_fast8_t IsV6, int AesKeyBytes);
void AesEncryptBlock(const AesCtx *const Ctx, BYTE *block);
void AesDecryptBlock(const AesCtx *const Ctx, BYTE *block);
void AesEncryptCbc(const AesCtx *const Ctx, BYTE *restrict iv, BYTE *restrict data, size_t *restrict len);
void AesDecryptCbc(const AesCtx *const Ctx, BYTE *iv, BYTE *data, size_t len);
void MixColumnsR(BYTE *restrict state);

#if defined(_CRYPTO_OPENSSL)
#include "crypto_openssl.h"

#elif defined(_CRYPTO_POLARSSL)
#include "crypto_polarssl.h"

#elif defined(_CRYPTO_WINDOWS)
#include "crypto_windows.h"

#else
#include "crypto_internal.h"

#endif
#endif // __crypto_h
