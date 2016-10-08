#ifndef INCLUDED_NETWORK_H
#define INCLUDED_NETWORK_H

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "types.h"
#include "output.h"

#if _MSC_VER
//typedef signed char int_fast8_t;
//typedef unsigned char BYTE;
//typedef UINT_PTR size_t;
//typedef unsigned long DWORD;
#define STDIN_FILENO 0
#endif

int_fast8_t sendrecv(SOCKET sock, BYTE *data, int len, int_fast8_t do_send);

#define _recv(s, d, l)  sendrecv(s, (BYTE *)d, l,  0)
#define _send(s, d, l)  sendrecv(s, (BYTE *)d, l, !0)

#ifndef NO_SOCKETS

void closeAllListeningSockets();
#ifdef SIMPLE_SOCKETS
int listenOnAllAddresses();
#endif // SIMPLE_SOCKETS
BOOL addListeningSocket(const char *const addr);
__pure int_fast8_t checkProtocolStack(const int addressfamily);

#if HAVE_GETIFADDR
void getPrivateIPAddresses(int* numAddresses, char*** ipAddresses);
#endif // HAVE_GETIFADDR

#endif // NO_SOCKETS

int runServer();
SOCKET connectToAddress(const char *const addr, const int AddressFamily, int_fast8_t showHostName);
int_fast8_t isDisconnected(const SOCKET s);

#endif // INCLUDED_NETWORK_H
