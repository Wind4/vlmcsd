/*
POSIX getopt for Windows

AT&T Public License

Code given out at the 1985 UNIFORUM conference in Dallas.
Modified for vlmcsd by Hotbird64
*/

#ifndef _WINGETOPT_H_
#define _WINGETOPT_H_

#ifdef _MSC_VER


#ifdef __cplusplus
extern "C" {
#endif

	extern int opterr;
	extern int optind;
	extern int optopt;
	extern char *optarg;
	extern int getopt(int argc, char * const argv[], const char *optstring);

#ifdef __cplusplus
}
#endif

#endif  // _MSC_VER
#endif // __wingetopt_h
