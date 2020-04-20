.NOTPARALLEL:

MAX_THREADS ?= 16

PROGRAM_NAME ?= bin/vlmcsd
CLIENT_NAME ?= bin/vlmcs
MULTI_NAME ?= bin/vlmcsdmulti
OBJ_NAME ?= build/libkms-static.o
A_NAME ?= lib/libkms.a

BASE_PROGRAM_NAME=$(notdir $(PROGRAM_NAME))
BASE_CLIENT_NAME=$(notdir $(CLIENT_NAME))
BASE_MULTI_NAME=$(notdir $(MULTI_NAME))
BASE_DLL_NAME=$(notdir $(DLL_NAME))
BASE_A_NAME=$(notdir $(A_NAME))

TARGETPLATFORM := $(shell LANG=en_US.UTF-8 $(CC) -v 2>&1 | grep '^Target: ' | cut -f 2 -d ' ')

ifneq (,$(findstring darwin,$(TARGETPLATFORM)))
  DARWIN := 1
  UNIX := 1
endif

ifneq (,$(findstring android,$(TARGETPLATFORM)))
  ANDROID := 1
  UNIX := 1
  ELF := 1
endif

ifneq (,$(findstring minix,$(TARGETPLATFORM)))
  MINIX := 1
  UNIX := 1
  ELF := 1
endif

ifneq (,$(findstring mingw,$(TARGETPLATFORM)))
  MINGW := 1
  WIN := 1
  PE := 1 
endif

ifneq (,$(findstring cygwin,$(TARGETPLATFORM)))
  CYGWIN := 1
  WIN := 1
  PE := 1
endif

ifneq (,$(findstring cygnus,$(TARGETPLATFORM)))
  CYGWIN := 1
  WIN := 1
  PE := 1
endif

ifneq (,$(findstring freebsd,$(TARGETPLATFORM)))
  FREEBSD := 1
  UNIX := 1
  BSD := 1
  ELF := 1
endif

ifneq (,$(findstring netbsd,$(TARGETPLATFORM)))
  NETBSD := 1
  UNIX := 1
  BSD := 1
  ELF := 1
endif

ifneq (,$(findstring openbsd,$(TARGETPLATFORM)))
  OPENBSD := 1
  UNIX := 1
  BSD := 1
  ELF := 1
endif

ifneq (,$(findstring solaris,$(TARGETPLATFORM)))
  SOLARIS := 1
  UNIX := 1
  ELF := 1
endif

ifneq (,$(findstring linux,$(TARGETPLATFORM)))
  LINUX := 1
  UNIX := 1
  ELF := 1
endif

ifneq (,$(findstring gnu,$(TARGETPLATFORM)))
ifeq (,$(findstring linux,$(TARGETPLATFORM)))
  UNIX := 1
  HURD := 1
  ELF := 1
endif
endif

ifeq ($(CYGWIN),1)
  DLL_NAME ?= lib/cygkms.dll
else ifeq ($(WIN),1)
  DLL_NAME ?= lib/libkms.dll
else ifeq ($(DARWIN),1)
  DLL_NAME ?= lib/libkms.dylib
else
  DLL_NAME ?= lib/libkms.so
endif

.DEFAULT:
	+@(test -d bin || mkdir bin) & (test -d lib || mkdir lib) & (test -d build || mkdir build)
	+@$(MAKE) -j$(MAX_THREADS) -C src $@ FROM_PARENT=1 PROGRAM_NAME=$(PROGRAM_NAME) CLIENT_NAME=$(CLIENT_NAME) MULTI_NAME=$(MULTI_NAME) DLL_NAME=$(DLL_NAME) A_NAME=$(A_NAME)

all:
	+@(test -d bin || mkdir bin) & (test -d lib || mkdir lib) & (test -d build || mkdir build)
	+@$(MAKE) -j$(MAX_THREADS) -C src $@ FROM_PARENT=1 PROGRAM_NAME=$(PROGRAM_NAME) CLIENT_NAME=$(CLIENT_NAME) MULTI_NAME=$(MULTI_NAME) DLL_NAME=$(DLL_NAME) A_NAME=$(A_NAME)

clean:
	+@$(MAKE) -j$(MAX_THREADS) -C src $@ FROM_PARENT=1 PROGRAM_NAME=$(PROGRAM_NAME) CLIENT_NAME=$(CLIENT_NAME) MULTI_NAME=$(MULTI_NAME) DLL_NAME=$(DLL_NAME) A_NAME=$(A_NAME)
	+@$(MAKE) -j$(MAX_THREADS) -C man $@

alldocs:
	+@$(MAKE) -j$(MAX_THREADS) -C man $@

dosdocs:
	+@$(MAKE) -j$(MAX_THREADS) -C man $@

unixdocs:
	+@$(MAKE) -j$(MAX_THREADS) -C man $@

htmldocs:
	+@$(MAKE) -j$(MAX_THREADS) -C man $@

pdfdocs:
	+@$(MAKE) -j$(MAX_THREADS) -C man $@

GNUmakefile:

help:
	@echo "Type"
	@echo "    ${MAKE}               - to build $(BASE_PROGRAM_NAME) and $(BASE_CLIENT_NAME)"
	@echo "    ${MAKE} clean         - to remove all targets and temporary files"
	@echo "    ${MAKE} pdfdocs       - Create PDF versions of the documentation (Requires groff with PDF support)."
	@echo "    ${MAKE} htmldocs      - Create HTML versions of the documentation."
	@echo "    ${MAKE} unixdocs      - Create Unix TXT versions of the documentation."
	@echo "    ${MAKE} dosdocs       - Create DOS/Windows TXT versions of the documentation."
	@echo "    ${MAKE} alldocs       - Create all versions of the documentation."
	@echo "    ${MAKE} vlmcsd        - to build KMS server $(PROGRAM_NAME)"
	@echo "    ${MAKE} vlmcs         - to build KMS client $(CLIENT_NAME)"
	@echo "    ${MAKE} vlmcsdmulti   - to build $(BASE_PROGRAM_NAME) and $(BASE_CLIENT_NAME) in a single multi-call binary $(MULTI_NAME)"
	@echo "    ${MAKE} libkms        - to build the shared library $(DLL_NAME)"
	@echo "    ${MAKE} libkms-static - to build the static library $(A_NAME)"
	@echo ""
	@echo "Options"
	@echo "    CONFIG=<x>                   Compile <x> as instead of config.h."
	@echo "    INI=<x>                      Compile $(BASE_PROGRAM_NAME) with default ini file <x>"
	@echo "    DATA=<x>                     Compile $(BASE_PROGRAM_NAME) and $(BASE_CLIENT_NAME) with default KMS data file <x>"
	@echo "    PROGRAM_NAME=<x>             Use <x> as output file name for the KMS server. Defaults to vlmcsd."
	@echo "    CLIENT_NAME=<x>              Use <x> as output file name for the KMS client. Defaults to vlmcs."
	@echo "    MULTI_NAME=<x>               Use <x> as output file name for the multi-call binary. Defaults to vlmcsdmulti."
	@echo "    DEPENDENCIES=1               Create dependency files."
	@echo "    CRYPTO=openssl               Use openssl for SHA256/HMAC calculations."
	@echo "    CRYPTO=openssl_with_aes      EXPERIMENTAL: Use openssl for SHA256/HMAC and AES calculations (hardware, e.g. AES-NI on x86)."
	@echo "    CRYPTO=openssl_with_aes_soft EXPERIMENTAL: Use openssl for SHA256/HMAC and AES calculations (software)."
	@echo "    CRYPTO=polarssl              Use polarssl instead of internal crypto code for SHA256/HMAC calculations."
	@echo "    CRYPTO=windows               Use Windows CryptoAPI instead of internal crypto code for SHA256/HMAC calculations."
	@echo "    MSRPC=1                      Use Microsoft RPC instead of vlmcsd's internal RPC. Only works with Windows and Cygwin targets."
	@echo "    CC=<x>                       Use compiler <x>. Supported compilers are gcc, icc, tcc and clang. Others may or may not work."
	@echo "    AR=<x>                       Use <x> instead of ar to build $(BASE_A_NAME). Set to gcc-ar if you want to use gcc's LTO feature."
	@echo "    COMPILER_LANGUAGE=<x>        May be c or c++."
	@echo "    TERMINAL_WIDTH=<x>           Assume a fixed terminal width of <x> columns. Use in case of problems only."  
	@echo "    VLMCSD_VERSION=<x>           Sets <x> as your version identifier. Defaults to \"private build\"."
	@echo "    CFLAGS=<x>                   Pass <x> as additional arguments to the compiler."
	@echo "    LDFLAGS=<x>                  Pass <x> as additional arguments to the linker."
	@echo "    PLATFORMFLAGS=<x>            Pass <x> as additional arguments to the compiler and the linker."
	@echo "    BASECFLAGS=<x>               Pass only <x> as arguments to the compiler (advanced users only)."
	@echo "    BASELDFLAGS=<x>              Pass only <x> as arguments to the linker (advanced users only)."
	@echo "    STRIP=0                      Don't strip debug information from $(BASE_PROGRAM_NAME) and $(BASE_CLIENT_NAME) (for developers)."
	@echo "    VERBOSE=1                    Be verbose when making targets."
	@echo "    VERBOSE=3                    Show name of compiler."
	@echo "    THREADS=1                    Use threads instead of fork(). Automatically set for native Windows. Recommended for Cygwin."
	@echo "    HWID=<x>                     Use <x> as the default HWID (when it can't be found in an ini file)."
	@echo "    FEATURES=full                Compile $(BASE_PROGRAM_NAME) with all features (default)."
	@echo "    FEATURES=most                Compile $(BASE_PROGRAM_NAME) without rarely used features."
	@echo "    FEATURES=embedded            Compile $(BASE_PROGRAM_NAME) with typical features for embedded systems."
	@echo "    FEATURES=autostart           Removes features typically not needed if you place $(BASE_PROGRAM_NAME) in an autostart script."
	@echo "    FEATURES=inetd               Compile $(BASE_PROGRAM_NAME) for running through an internet superserver only."
	@echo "    FEATURES=minimum             Compiles only basic features of $(BASE_PROGRAM_NAME)."
	@echo "    FEATURES=fixedepids          $(BASE_PROGRAM_NAME) only uses bultin internal ePIDs."
	@echo ""
	@echo "Useful CFLAGS to save memory when running $(BASE_PROGRAM_NAME) on very small embedded devices (finer control than FEATURES=)"
	@echo "    -DNO_STRICT_MODES            Don't support enhanced emulator detection prevention."
	@echo "    -DNO_CLIENT_LIST             Don't support maintaining a client list (CMIDs)."
	@echo "    -DNO_VERBOSE_LOG             Don't support verbose logging. Removes -v option."
	@echo "    -DNO_LOG                     Don't add support for logging. Implies -DNO_VERBOSE_LOG."
	@echo "    -DNO_RANDOM_EPID             Don't support random ePIDs."
	@echo "    -DNO_INI_FILE                Don't support reading ePIDs/HWIDs from a file."
	@echo "    -DNO_PID_FILE                Don't support writing a PID file. Removes -p option."
	@echo "    -DNO_USER_SWITCH             Don't support changing uid/gid after program start. Removes -u and -g options."
	@echo "    -DNO_HELP                    Don't support command line help."
	@echo "    -DNO_CUSTOM_INTERVALS        Don't support custom intervals for retry and refresh activation. Removes -A and -R options."
	@echo "    -DNO_FREEBIND                Don't support binding to foreign IP addresses. Removes -F0 and -F1 options. Only affects FreeBSD and Linux."
	@echo "    -DNO_SOCKETS                 Don't support standalone operation. Requires an internet superserver to start $(BASE_PROGRAM_NAME)."
	@echo "    -DSIMPLE_SOCKETS             Compile $(BASE_PROGRAM_NAME) with basic socket support only. Removes -L option."
	@echo "    -DSIMPLE_RPC                 Don't support RPC with NDR64 and BTFN in $(BASE_PROGRAM_NAME) (but do in $(BASE_CLIENT_NAME)). Makes emulator detection easy."
	@echo "    -DNO_TAP                     Compile $(BASE_PROGRAM_NAME) without VPN support (Windows and Cygwin only)."
	@echo "    -DNO_CL_PIDS                 Don't support specifying ePIDs and HwId from the command line in $(BASE_PROGRAM_NAME)."
	@echo "    -DNO_LIMIT                   Don't support limiting concurrent clients in $(BASE_PROGRAM_NAME)."
	@echo "    -DNO_SIGHUP                  Don't support SIGHUP handling in $(BASE_PROGRAM_NAME)."
	@echo "    -DNO_VERSION_INFORMATION     Don't support displaying version information in $(BASE_PROGRAM_NAME) and $(BASE_CLIENT_NAME). Removes -V option."
	@echo "    -DNO_PRIVATE_IP_DETECT       Don't support protection against clients with public IP addresses in $(BASE_PROGRAM_NAME)"	
	@echo "    -DSMALL_AES                  Use a smaller (saves about 200 bytes) but slower implementation of AES."
	@echo "    -DNO_EXTERNAL_DATA           Don't support loading an external database. Mutually exclusive with -DNO_INTERNAL_DATA"
	@echo "    -DNO_INTERNAL_DATA           Don't compile an internal database. Mutually exclusive with -DNO_EXTERNAL_DATA"
	@echo "    -DUNSAFE_DATA_LOAD           Don't check the KMS data file for integrity. Saves some bytes but is dangerous."
	@echo ""
	@echo "Troubleshooting options"
	@echo "    CAT=1                        Combine all sources in a single in-memory file and compile directly to target."
	@echo "    NOPROCFS=1                   Don't rely on a properly mounted proc filesystem in /proc."
	@echo "    AUXV=1                       Use /proc/self/auxv (requires Linux with glibc >= 2.16 or musl.)"
	@echo "    NOLPTHREAD=1                 Disable detection if -lpthread is required (for use with Android NDK)."
	@echo "    NOLRESOLV=1                  Disable detection if -lresolv is required  (for use with Android NDK)."
	@echo "    NOLIBS=1                     Do not attempt to autodetect any library dependencies."
	@echo "    OPENSSL_HMAC=0               Compile for openssl versions that don't have HMAC support (required on some embedded devices)."
	@echo "    NO_TIMEOUT=1                 Do not set timeouts for sockets (for systems that don't support it)."
	@echo "    CHILD_HANDLER=1              Install a handler for SIGCHLD (for systems that don't support SA_NOCLDWAIT)."
	@echo "    NO_DNS=1                     Compile $(BASE_CLIENT_NAME) without support for detecting KMS servers via DNS."
	@echo "    NO_GETIFADDRS=1              Compile $(BASE_PROGRAM_NAME) without using getifaddrs()."
	@echo "    GETIFADDRS=musl              Compile $(BASE_PROGRAM_NAME) with its own implementation of getifaddrs() based on musl."
	@echo "    DNS_PARSER=internal          Use $(BASE_CLIENT_NAME) internal DNS parsing routines. No effect on MingW (native Windows)."
	@echo ""
	@echo "Other useful CFLAGS"
	@echo "    -DNO_COMPILER_UAA            Do not use compiler support for byte swapping and unaligned access"
	@echo "    -DFULL_INTERNAL_DATA         Embed full internal KMS data in $(BASE_PROGRAM_NAME)."
	@echo "    -DSUPPORT_WINE               Add code that the Windows version of $(BASE_PROGRAM_NAME) runs on Wine if MSRPC=1"
	@echo "    -D_PEDANTIC                  Report rare error/warning conditions instead of silently ignoring them."
	@echo "    -DFD_SETSIZE=<x>             Allow <x> -L statements in $(BASE_PROGRAM_NAME) (default: 64 on Windows, 1024 on most Unixes)."

