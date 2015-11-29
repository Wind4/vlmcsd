#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#if defined(_CRYPTO_OPENSSL)

#include "crypto.h"
#include "crypto_openssl.h" // Required for Eclipse only
#include <stdint.h>
#include "endian.h"


#ifndef _OPENSSL_NO_HMAC

int Sha256HmacInit_OpenSSL(HMAC_CTX *c, const void *k, int l)
{
	HMAC_CTX_init(c);
	#if OPENSSL_VERSION_NUMBER >= 0x10000000L
		int result =
	#else
		int result = TRUE;
	#endif
	HMAC_Init_ex(c, k, l, EVP_sha256(), NULL);
	return result;
}

int Sha256HmacFinish_OpenSSL(HMAC_CTX *c, unsigned char *h, unsigned int *l)
{
	#if OPENSSL_VERSION_NUMBER >= 0x10000000L
		int result =
	#else
		int result = !0;
	#endif
	HMAC_Final(c, h, l);
	HMAC_CTX_cleanup(c);
	return result;
}

int_fast8_t Sha256Hmac(BYTE* key, BYTE* restrict data, DWORD len, BYTE* restrict hmac)
{
	HMAC_CTX Ctx;

#	if OPENSSL_VERSION_NUMBER >= 0x10000000L

	return
		Sha256HmacInit_OpenSSL(&Ctx, key, 16) &&
		HMAC_Update(&Ctx, data, len) &&
		Sha256HmacFinish_OpenSSL(&Ctx, hmac, NULL);

#	else // OpenSSL 0.9.x

	Sha256HmacInit_OpenSSL(&Ctx, key, 16);
	HMAC_Update(&Ctx, data, len);
	Sha256HmacFinish_OpenSSL(&Ctx, hmac, NULL);
	return TRUE;

#	endif
}

#else // _OPENSSL_NO_HMAC (some routers have OpenSSL without support for HMAC)

int _Sha256HmacInit(Sha256HmacCtx *Ctx, BYTE *key, size_t klen)
{
	BYTE  IPad[64];
	unsigned int  i;

	memset(IPad, 0x36, sizeof(IPad));
	memset(Ctx->OPad, 0x5C, sizeof(Ctx->OPad));

	if ( klen > 64 )
	{
		BYTE *temp = (BYTE*)alloca(32);
		SHA256(key, klen, temp);
		klen = 32;
		key  = temp;
	}

	for (i = 0; i < klen; i++)
	{
		IPad[ i ]      ^= key[ i ];
		Ctx->OPad[ i ] ^= key[ i ];
	}

	SHA256_Init(&Ctx->ShaCtx);
	return SHA256_Update(&Ctx->ShaCtx, IPad, sizeof(IPad));
}

int _Sha256HmacUpdate(Sha256HmacCtx *Ctx, BYTE *data, size_t len)
{
	int rc = SHA256_Update(&Ctx->ShaCtx, data, len);
	return rc;
}

int _Sha256HmacFinish(Sha256HmacCtx *Ctx, BYTE *hmac, void* dummy)
{
	BYTE temp[32];

	SHA256_Final(temp, &Ctx->ShaCtx);
	SHA256_Init(&Ctx->ShaCtx);
	SHA256_Update(&Ctx->ShaCtx, Ctx->OPad, sizeof(Ctx->OPad));
	SHA256_Update(&Ctx->ShaCtx, temp, sizeof(temp));
	return SHA256_Final(hmac, &Ctx->ShaCtx);
}

int_fast8_t Sha256Hmac(BYTE* key, BYTE* restrict data, DWORD len, BYTE* restrict hmac)
{
	Sha256HmacCtx Ctx;
	_Sha256HmacInit(&Ctx, key, 16);
	_Sha256HmacUpdate(&Ctx, data, len);
	_Sha256HmacFinish(&Ctx, hmac, NULL);
	return TRUE;
}
#endif

#if defined(_USE_AES_FROM_OPENSSL)
void TransformOpenSslEncryptKey(AES_KEY *k, const AesCtx *const Ctx)
{
	uint32_t *rk_OpenSSL = k->rd_key, *rk_vlmcsd = (uint32_t*)Ctx->Key;
	k->rounds = Ctx->rounds;

	for (; rk_OpenSSL < k->rd_key + ((k->rounds + 1) << 2); rk_OpenSSL++, rk_vlmcsd++)
	{
		#ifdef _OPENSSL_SOFTWARE
			*rk_OpenSSL = BE32(*rk_vlmcsd);
		#else
			*rk_OpenSSL = LE32(*rk_vlmcsd);
		#endif
	}
}

void TransformOpenSslDecryptKey(AES_KEY *k, const AesCtx *const Ctx)
{
	uint_fast8_t i;

	#ifdef _DEBUG_OPENSSL
	AES_set_decrypt_key((BYTE*)Ctx->Key, 128, k);
	errorout("Correct V5 round key:");

	for (i = 0; i < (Ctx->rounds + 1) << 4; i++)
	{
		if (!(i % 16)) errorout("\n");
		if (!(i % 4)) errorout(" ");
		errorout("%02X", ((BYTE*)(k->rd_key))[i]);
	}

	errorout("\n");
	#endif

	k->rounds = Ctx->rounds;

	/* invert the order of the round keys blockwise (1 Block = AES_BLOCK_SIZE = 16): */

	for (i = 0; i < (Ctx->rounds + 1) << 2; i++)
	{
		#ifdef _OPENSSL_SOFTWARE
			k->rd_key[((Ctx->rounds-(i >> 2)) << 2) + (i & 3)] = BE32(Ctx->Key[i]);
		#else
			k->rd_key[((Ctx->rounds-(i >> 2)) << 2) + (i & 3)] = LE32(Ctx->Key[i]);
		#endif
	}

    /* apply the inverse MixColumn transform to all round keys but the first and the last: */

	uint32_t *rk = k->rd_key + 4;

	for (i = 0; i < (Ctx->rounds - 1); i++)
	{
    	MixColumnsR((BYTE*)(rk + (i << 2)));
    }

	#ifdef _DEBUG_OPENSSL
	errorout("Real round key:");

	for (i = 0; i < (Ctx->rounds + 1) << 4; i++)
	{
		if (!(i % 16)) errorout("\n");
		if (!(i % 4)) errorout(" ");
		errorout("%02X", ((BYTE*)(k->rd_key))[i]);
	}

	errorout("\n");
	#endif
}

static BYTE NullIV[AES_BLOCK_SIZE + 8]; // OpenSSL may overwrite bytes behind IV

void AesEncryptCbc(const AesCtx *const Ctx, BYTE *iv, BYTE *data, size_t *len)
{
	AES_KEY k;

	// OpenSSL overwrites IV plus 4 bytes
	BYTE localIV[24]; // 4 spare bytes for safety
	if (iv) memcpy(localIV, iv, AES_BLOCK_SIZE);

	// OpenSSL Low-Level APIs do not pad. Could use EVP API instead but needs more code to access the expanded key
	uint_fast8_t pad = (~*len & (AES_BLOCK_SIZE - 1)) + 1;

	#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ == 8) // gcc 4.8 memset bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56977
	    size_t i;
		for (i = 0; i < pad; i++) data[*len + i] = pad;
	#else
		memset(data + *len, pad, pad);
	#endif
	*len += pad;

	memset(NullIV, 0, sizeof(NullIV));

	TransformOpenSslEncryptKey(&k, Ctx);

	AES_cbc_encrypt(data, data, *len, &k, iv ? localIV : NullIV, AES_ENCRYPT);
}

void AesDecryptBlock(const AesCtx *const Ctx, BYTE *block)
{
	AES_KEY k;

	TransformOpenSslDecryptKey(&k, Ctx);
	AES_decrypt(block, block, &k);
}

#if defined(_CRYPTO_OPENSSL) && defined(_USE_AES_FROM_OPENSSL) && !defined(_OPENSSL_SOFTWARE)
void AesEncryptBlock(const AesCtx *const Ctx, BYTE *block)
{
	AES_KEY k;

	TransformOpenSslEncryptKey(&k, Ctx);
	AES_encrypt(block, block, &k);
}
#endif

void AesDecryptCbc(const AesCtx *const Ctx, BYTE *iv, BYTE *data, size_t len)
{
	AES_KEY k;

	memset(NullIV, 0, sizeof(NullIV));

	TransformOpenSslDecryptKey(&k, Ctx);
	AES_cbc_encrypt(data, data, len, &k, iv ? iv : NullIV, AES_DECRYPT);
}

#ifndef _OPENSSL_SOFTWARE
void AesCmacV4(BYTE *Message, size_t MessageSize, BYTE *HashOut)
{
    size_t i;
    BYTE hash[AES_BLOCK_BYTES];
    AesCtx Ctx;
    AES_KEY k;

    AesInitKey(&Ctx, AesKeyV4, FALSE, V4_KEY_BYTES);
    TransformOpenSslEncryptKey(&k, &Ctx);

    memset(hash, 0, sizeof(hash));
    memset(Message + MessageSize, 0, AES_BLOCK_BYTES);
    Message[MessageSize] = 0x80;

    for (i = 0; i <= MessageSize; i += AES_BLOCK_BYTES)
    {
        XorBlock(Message + i, hash);
        AES_encrypt(hash, hash, &k);
    }

    memcpy(HashOut, hash, AES_BLOCK_BYTES);
}
#endif // !_OPENSSL_SOFTWARE

#endif // defined(_USE_AES_FROM_OPENSSL)

#endif // _CRYPTO_OPENSSL
