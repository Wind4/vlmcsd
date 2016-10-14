#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#if !defined(_CRYPTO_OPENSSL) && !defined(_CRYPTO_POLARSSL) && !defined(_CRYPTO_WINDOWS)
#include "crypto_internal.h"
#include "endian.h"

#define F0(x, y, z)  ( ((x) & (y)) | (~(x) & (z)) )
#define F1(x, y, z)  ( ((x) & (y)) | ((x) & (z)) | ((y) & (z)) )

#define SI1(x)  ( ROR32(x, 2 ) ^ ROR32(x, 13) ^ ROR32(x, 22) )
#define SI2(x)  ( ROR32(x, 6 ) ^ ROR32(x, 11) ^ ROR32(x, 25) )
#define SI3(x)  ( ROR32(x, 7 ) ^ ROR32(x, 18) ^ ((x) >> 3 ) )
#define SI4(x)  ( ROR32(x, 17) ^ ROR32(x, 19) ^ ((x) >> 10) )

static const DWORD k[] = {
	0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1,
	0x923F82A4, 0xAB1C5ED5, 0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
	0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174, 0xE49B69C1, 0xEFBE4786,
	0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
	0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147,
	0x06CA6351, 0x14292967, 0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
	0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85, 0xA2BFE8A1, 0xA81A664B,
	0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
	0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A,
	0x5B9CCA4F, 0x682E6FF3, 0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
	0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};


static void Sha256Init(Sha256Ctx *Ctx)
{
	Ctx->State[0] = 0x6A09E667;
	Ctx->State[1] = 0xBB67AE85;
	Ctx->State[2] = 0x3C6EF372;
	Ctx->State[3] = 0xA54FF53A;
	Ctx->State[4] = 0x510E527F;
	Ctx->State[5] = 0x9B05688C;
	Ctx->State[6] = 0x1F83D9AB;
	Ctx->State[7] = 0x5BE0CD19;
	Ctx->Len = 0;
}


static void Sha256ProcessBlock(Sha256Ctx *Ctx, BYTE *block)
{
	unsigned int  i;
	DWORD  w[64], temp1, temp2;
	DWORD  a = Ctx->State[0];
	DWORD  b = Ctx->State[1];
	DWORD  c = Ctx->State[2];
	DWORD  d = Ctx->State[3];
	DWORD  e = Ctx->State[4];
	DWORD  f = Ctx->State[5];
	DWORD  g = Ctx->State[6];
	DWORD  h = Ctx->State[7];

	for (i = 0; i < 16; i++)
		//w[ i ] = GET_UAA32BE(block, i);
		w[i] = BE32(((DWORD*)block)[i]);

	for (i = 16; i < 64; i++)
		w[ i ] = SI4(w[ i - 2 ]) + w[ i - 7 ] + SI3(w[ i - 15 ]) + w[ i - 16 ];

	for (i = 0; i < 64; i++)
	{
		temp1 = h + SI2(e) + F0(e, f, g) + k[ i ] + w[ i ];
		temp2 = SI1(a) + F1(a, b, c);

		h = g;
		g = f;
		f = e;
		e = d + temp1;
		d = c;
		c = b;
		b = a;
		a = temp1 + temp2;
	}

	Ctx->State[0] += a;
	Ctx->State[1] += b;
	Ctx->State[2] += c;
	Ctx->State[3] += d;
	Ctx->State[4] += e;
	Ctx->State[5] += f;
	Ctx->State[6] += g;
	Ctx->State[7] += h;
}


static void Sha256Update(Sha256Ctx *Ctx, BYTE *data, size_t len)
{
	unsigned int  b_len = Ctx->Len & 63,
								r_len = (b_len ^ 63) + 1;

	Ctx->Len += (unsigned int)len;

	if ( len < r_len )
	{
		memcpy(Ctx->Buffer + b_len, data, len);
		return;
	}

	if ( r_len < 64 )
	{
		memcpy(Ctx->Buffer + b_len, data, r_len);
		len  -= r_len;
		data += r_len;
		Sha256ProcessBlock(Ctx, Ctx->Buffer);
	}

	for (; len >= 64; len -= 64, data += 64)
		Sha256ProcessBlock(Ctx, data);

	if ( len ) memcpy(Ctx->Buffer, data, len);
}


static void Sha256Finish(Sha256Ctx *Ctx, BYTE *hash)
{
	unsigned int  i, b_len = Ctx->Len & 63;

	Ctx->Buffer[ b_len ] = 0x80;
	if ( b_len ^ 63 ) memset(Ctx->Buffer + b_len + 1, 0, b_len ^ 63);

	if ( b_len >= 56 )
	{
		Sha256ProcessBlock(Ctx, Ctx->Buffer);
		memset(Ctx->Buffer, 0, 56);
	}

	//PUT_UAA64BE(Ctx->Buffer, (unsigned long long)(Ctx->Len * 8), 7);
	((uint64_t*)Ctx->Buffer)[7] = BE64((uint64_t)Ctx->Len << 3);
	Sha256ProcessBlock(Ctx, Ctx->Buffer);

	for (i = 0; i < 8; i++)
		//PUT_UAA32BE(hash, Ctx->State[i], i);
		((DWORD*)hash)[i] = BE32(Ctx->State[i]);

}


void Sha256(BYTE *data, size_t len, BYTE *hash)
{
	Sha256Ctx Ctx;

	Sha256Init(&Ctx);
	Sha256Update(&Ctx, data, len);
	Sha256Finish(&Ctx, hash);
}


static void _Sha256HmacInit(Sha256HmacCtx *Ctx, BYTE *key, size_t klen)
{
	BYTE  IPad[64];
	unsigned int  i;

	memset(IPad, 0x36, sizeof(IPad));
	memset(Ctx->OPad, 0x5C, sizeof(Ctx->OPad));

	if ( klen > 64 )
	{
		BYTE *temp = (BYTE*)alloca(32);
		Sha256(key, klen, temp);
		klen = 32;
		key  = temp;
	}

	for (i = 0; i < klen; i++)
	{
		IPad[ i ]      ^= key[ i ];
		Ctx->OPad[ i ] ^= key[ i ];
	}

	Sha256Init(&Ctx->ShaCtx);
	Sha256Update(&Ctx->ShaCtx, IPad, sizeof(IPad));
}


static void _Sha256HmacUpdate(Sha256HmacCtx *Ctx, BYTE *data, size_t len)
{
	Sha256Update(&Ctx->ShaCtx, data, len);
}


static void _Sha256HmacFinish(Sha256HmacCtx *Ctx, BYTE *hmac)
{
	BYTE  temp[32];

	Sha256Finish(&Ctx->ShaCtx, temp);
	Sha256Init(&Ctx->ShaCtx);
	Sha256Update(&Ctx->ShaCtx, Ctx->OPad, sizeof(Ctx->OPad));
	Sha256Update(&Ctx->ShaCtx, temp, sizeof(temp));
	Sha256Finish(&Ctx->ShaCtx, hmac);
}



int_fast8_t Sha256Hmac(BYTE* key, BYTE* restrict data, DWORD len, BYTE* restrict hmac)
{
	Sha256HmacCtx Ctx;
	_Sha256HmacInit(&Ctx, key, 16);
	_Sha256HmacUpdate(&Ctx, data, len);
	_Sha256HmacFinish(&Ctx, hmac);
	return TRUE;
}


#endif // No external Crypto

