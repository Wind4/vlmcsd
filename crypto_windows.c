/*
 * crypto_windows.c
 */
#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifdef _CRYPTO_WINDOWS

#if !_WIN32 && !__CYGWIN__
#error You cannot use Windows CryptoAPI on non-Windows platforms
#else // _WIN32 || __CYGWIN__

#include "crypto_windows.h"


typedef struct _HMAC_KEYBLOB
{
	BLOBHEADER hdr;
	DWORD dwKeySize;
	BYTE KeyData[16];
} HMAC_KEYBLOB;


/*
 * MingW and Cygwin define NULL as ((void*)0) (Posix standard) which you can't assign to
 * non-pointer types without compiler warning. Thus we use the following
 */
#define NULLHANDLE 0
#define NULLFLAGS 0


static HCRYPTPROV hRsaAesProvider = 0; // Needs to be initialized just once per process



static int_fast8_t AcquireCryptContext()
{
	if (!hRsaAesProvider)
	{
		return (int_fast8_t)CryptAcquireContextW
		(
			&hRsaAesProvider,		// Provider handle
			NULL,					// No key container name
			NULL,					// Default provider
			PROV_RSA_AES,			// Provides SHA and AES
			CRYPT_VERIFYCONTEXT		// We don't need access to persistent keys
		);
	}

	return TRUE;
}


int_fast8_t Sha256(BYTE* restrict data, DWORD DataSize, BYTE* restrict hash)
{
	HCRYPTHASH hHash = 0;
	DWORD HashSize = 32;

	int_fast8_t success =
		AcquireCryptContext() &&

		CryptCreateHash
		(
			hRsaAesProvider,// Provider handle
			CALG_SHA_256,	// Algorithm
			NULLHANDLE,		// SHA256 requires no key
			NULLFLAGS,		// Use default flags
			&hHash			// Handle for hashing
		) &&

		CryptHashData
		(
			hHash,			// Handle
			data,			// data to hash
			DataSize,		// size of data
			NULLFLAGS		// Use default flags
		) &&

		CryptGetHashParam
		(
			hHash,			// Handle
			HP_HASHVAL,		// what you actually want to get (the resulting hash)
			hash,			// data to retrieve
			&HashSize,		// size of data
			NULLFLAGS		// currently reserved (as of this writing)
		);

	if (hHash) CryptDestroyHash(hHash);

	return success;
}


int_fast8_t Sha256Hmac(const BYTE* key, BYTE* restrict data, DWORD len, BYTE* restrict hmac)
{
#	ifndef USE_THREADS // In fork() mode thread-safety is not required
	static
#	endif
	HMAC_KEYBLOB hmackeyblob = {
		// Type, Version, Algorithm
		{ PLAINTEXTKEYBLOB, CUR_BLOB_VERSION, 0, CALG_RC2 },
		// Key length
		16
	};

	HCRYPTKEY hKey = NULLHANDLE;
	HCRYPTHASH hHmacHash = NULLHANDLE;
	HMAC_INFO HmacInfo = { 0 };
	DWORD dwHmacSize = 32;

	HmacInfo.HashAlgid = CALG_SHA_256;
	memcpy(hmackeyblob.KeyData, key, sizeof(hmackeyblob.KeyData));

	BOOL success =
		AcquireCryptContext() &&

		CryptImportKey
		(
			hRsaAesProvider,        // provider handle
			(PBYTE)&hmackeyblob,    // the actual key MS blob format
			sizeof(HMAC_KEYBLOB),   // size of the entire blob
			NULLHANDLE,             // password/key for the key store (none required here)
			NULLFLAGS,              // default flags
			&hKey                   // key handle to retrieve (must be kept until you finish hashing)
		) &&

		CryptCreateHash
		(
			hRsaAesProvider,        // provider handle
			CALG_HMAC,              // the actual key MS blob format
			hKey,                   // size of the entire blob
			NULLFLAGS,              // password/key for the key store (none required here)
			&hHmacHash              // default flags
		) &&                        // key handle to retrieve (must be kept until you finish hashing)

		CryptSetHashParam
		(
			hHmacHash,              // hash handle
			HP_HMAC_INFO,           // parameter you want to set
			(PBYTE)&HmacInfo,       // the HMAC parameters (SHA256 with default ipad and opad)
			NULLFLAGS               // flags are reserved up to Windows 8.1
		) &&

		CryptHashData
		(
			hHmacHash,              // hash handle
			data,                   // Pointer to data you want to hash
			len,                    // data length
			NULLFLAGS               // default flags
		) &&

		CryptGetHashParam
		(
			hHmacHash,              // hash handle
			HP_HASHVAL,             // what you actually want to get (the resulting HMAC)
			hmac,                   // data to retrieve
			&dwHmacSize,            // size of data
			NULLFLAGS               // currently reserved (as of this writing)
		);

	if (hKey) CryptDestroyKey(hKey);
	if (hHmacHash) CryptDestroyHash(hHmacHash);

	return (int_fast8_t)success;
}

#endif // _WIN32 || __CYGWIN__
#endif // _CRYPTO_WINDOWS
