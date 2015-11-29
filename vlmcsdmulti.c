/* Multi-Call Binary for vlmcs and vlmcsd */

#ifndef CONFIG
#define CONFIG "config.h"
#endif // CONFIG
#include CONFIG

#if MULTI_CALL_BINARY < 1
#error "Please define MULTI_CALL_BINARY=1 when compiling this file."
#endif

#include <libgen.h>
#include <stdio.h>

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
		if (!strcmp((char*)argv[1],"vlmcsd"))
			return server_main(argc - 1, argv + 1);

		if (!strcmp((char*)argv[1],"vlmcs"))
			return client_main(argc - 1, argv + 1);
	}

	errorout(
			"vlmcsdmulti %s\n\n"
			"Usage:\n"
			"\t%s vlmcsd [<vlmcsd command line>]\n"
			"\t%s vlmcs [<vlmcs command line>]\n\n",
			Version, *argv, *argv
	);

	return !0;
}
