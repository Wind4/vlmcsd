/* Multi-Call Binary for vlmcs and vlmcsd */

#define _CRT_SECURE_NO_WARNINGS

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#if MULTI_CALL_BINARY < 1
#error "Please define MULTI_CALL_BINARY=1 when compiling this file."
#endif

#include <stdio.h>

#if !_MSC_VER
#include <libgen.h>
#else // _MSC_VER
#include <stdlib.h>
#include "helpers.h"
#endif // _MSC_VER

#include "vlmcs.h"
#include "vlmcsd.h"
#include "types.h"
#include "shared_globals.h"
#include "output.h"

#if (defined(_WIN32) || defined(__CYGWIN__))
#define compare strcasecmp // Best for case-preserving (but otherwise case-insensitive) filesystems
#else // native Unix
#define compare strcmp // for case-sensitive filesystems
#endif // native Unix

#if _MSC_VER
static char* basename(const char* fullname)
{
	size_t len = strlen(fullname);
	char* filename = (char*)vlmcsd_malloc(len + 1);
	char* extension = (char*)vlmcsd_malloc(len + 1);
	static char result[64];

	_splitpath(fullname, NULL, NULL, filename, extension);

	if (strlen(filename) + strlen(extension) > 63)
	{
		*result = 0;
		goto finally;
	}

	strcpy(result, filename);
	strcat(result, extension);

	finally:
	free(filename);
	free(extension);

	return result;
}
#endif // _MSC_VER

int main(int argc, CARGV argv)
{
	multi_argv = argv;
	multi_argc = argc;

	if (!compare(basename((char*)*argv), "vlmcsd"))
		return server_main(argc, argv);

	if (!compare(basename((char*)*argv), "vlmcs"))
		return client_main(argc, argv);

#ifdef _WIN32
	if (!compare(basename((char*)*argv), "vlmcsd.exe"))
		return server_main(argc, argv);

	if (!compare(basename((char*)*argv), "vlmcs.exe"))
		return client_main(argc, argv);
#endif // _WIN32

	if (argc > 1)
	{
		if (!strcmp((char*)argv[1], "vlmcsd"))
			return server_main(argc - 1, argv + 1);

		if (!strcmp((char*)argv[1], "vlmcs"))
			return client_main(argc - 1, argv + 1);
	}

	errorout(
		"vlmcsdmulti %s\n\n"
		"Usage:\n"
		"\t%s vlmcsd [<vlmcsd command line>]\n"
		"\t%s vlmcs [<vlmcs command line>]\n\n",
		Version, *argv, *argv
	);

	return VLMCSD_EINVAL;
}


#if _MSC_VER && !defined(_DEBUG)
int __stdcall WinStartUp(void)
{
	WCHAR **szArgList;
	int argc;
	szArgList = CommandLineToArgvW(GetCommandLineW(), &argc);

	int i;
	char **argv = (char**)vlmcsd_malloc(sizeof(char*)*argc);

	for (i = 0; i < argc; i++)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, szArgList[i], -1, argv[i], 0, NULL, NULL);
		argv[i] = (char*)vlmcsd_malloc(size);
		WideCharToMultiByte(CP_UTF8, 0, szArgList[i], -1, argv[i], size, NULL, NULL);
	}

	exit(main(argc, argv));
}
#endif // _MSC_VER && !defined(_DEBUG)&& !MULTI_CALL_BINARY
