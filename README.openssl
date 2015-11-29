IMPORTANT
=========

1. Do not use any of the OpenSSL binaries
2. Do not compile OpenSSL binaries yourself

(except for doing some research into the deep internals of OpenSSL)

REASONS
=======

All OpenSSL binaries included are highly experimental and are likely to fail
in many cases. To get some real benefit from OpenSSL (or PolarSSL) it should
handle all crypting/hashing.

However this is not possible because Microsoft has slightly altered AES
encryption in KMSv6 and uses a non-AES variant of the Rijndael CMAC in
KMSv4. OpenSSL is not able to handle this if you use it correctly.

This means OpenSSL can be used safely only for SHA256 and HMAC SHA256
calculations used in KMSv5 and KMSv6 but the code size benefit is only
100 to 300 bytes (depending on the architecture).

To benefit more from OpenSSL (getting it performing the AES stuff) I do
the first phase of AES encryption/decryption (called key expansion) with my
own code. I then poke the expanded key into internal OpenSSL structs to make
it behave in a way not intended by the OpenSSL developers but in a way to
perform non-standard AES crypting as required by KMSv4 and KMSv6. KMSv5 is
the only protocol that could use OpenSSL without hacking the OpenSSL internals.

That means vlmcsd still needs about 40% of the internal AES code plus some
OpenSSL hacking code to poke the expanded key into OpenSSL.

The entire OpenSSL hacking does not work in every case because the internal
OpenSSL structs differ depending on the OpenSSL version, OpenSSL configuration
at compile time (whether it is configured to use compiled C code or assembler
code), CPU architecture and CPU features (whether it can perform AES in
hardware).

SUMMARY
=======

If you use OpenSSL in a safe way (compile with CRYPTO=openssl), there is not
much benefit from it. The binary may become bigger or smaller and you
definitely need more RAM when you run vlmcsd or vlmcs.

If you use hacked OpenSSL (compile with CRYPTO=openssl_with_aes or
CRYPTO=openssl_with_aes_soft) you risk malfunction of vlmcs/vlmcsd even if it
performed correctly several times before.

Both vlmcs and vlmcsd do not have more features when compiled with OpenSSL
support. It may be faster (especially on CPUs with hardware assisted AES) but
uses more memory and may fail or perform unreliably.


