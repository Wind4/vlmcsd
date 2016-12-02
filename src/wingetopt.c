/*
POSIX getopt for Windows

AT&T Public License

Code given out at the 1985 UNIFORUM conference in Dallas.
Modified for vlmcsd by Hotbird64
*/

#ifdef _MSC_VER

#include "wingetopt.h"
//#include <stdio.h>
#include <string.h>

#define EOF	(-1)
#define ERR(s, c)	if(opterr){\
	char errbuf[2];\
	errbuf[0] = c; errbuf[1] = '\n';\
	fputs(argv[0], stderr);\
	fputs(s, stderr);\
	fputc(c, stderr);}
//(void) write(2, argv[0], (unsigned)strlen(argv[0]));\
	//(void) write(2, s, (unsigned)strlen(s));\
	//(void) write(2, errbuf, 2);}

int	opterr = 1;
int	optind = 1;
int	optopt;
char* optarg;

int getopt(int argc, char * const argv[], const char *opts)
{
	static int sp = 1;
	register int c;
	register char *cp;

	if (sp == 1)
		if (optind >= argc ||
			argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if (strcmp(argv[optind], "--") == 0) {
			optind++;
			return(EOF);
		}
		optopt = c = argv[optind][sp];
		if (c == ':' || (cp = strchr(opts, c)) == NULL) {
			//ERR(": illegal option -- ", (char)c);
			if (argv[optind][++sp] == '\0') {
				optind++;
				sp = 1;
			}
			return('?');
		}
		if (*++cp == ':') {
			if (argv[optind][sp + 1] != '\0')
				optarg = (char*)&argv[optind++][sp + 1];
			else if (++optind >= argc) {
				//ERR(": option requires an argument -- ", (char)c);
				sp = 1;
				return('?');
			}
			else
				optarg = (char*)argv[optind++];
			sp = 1;
		}
		else {
			if (argv[optind][++sp] == '\0') {
				sp = 1;
				optind++;
			}
			optarg = NULL;
		}
		return(c);
}

#endif  // _MSC_VER

