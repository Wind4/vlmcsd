#ifndef INCLUDED_NTSERVICE_H
#define INCLUDED_NTSERVICE_H

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#include "types.h"
#ifdef _NTSERVICE

//#include <strsafe.h>

#define NT_SERVICE_NAME "vlmcsd"
#define NT_SERVICE_DISPLAY_NAME "Key Management Server"

extern SERVICE_TABLE_ENTRY NTServiceDispatchTable[];

VOID ReportServiceStatus(const DWORD, const DWORD, const DWORD);
int NtServiceInstallation(const int_fast8_t installService, const char *restrict ServiceUser, const char *const ServicePassword);

#else // !_NTSERVICE

#define ReportServiceStatus(x,y,z)

#endif // _NTSERVICE

#endif // INCLUDED_NTSERVICE_H
