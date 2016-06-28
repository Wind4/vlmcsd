################################################################################

.PHONY: clean

PROGRAM_NAME ?= vlmcsd
CLIENT_NAME ?= vlmcs
MULTI_NAME ?= vlmcsdmulti
OBJ_NAME ?= libkms-static.o
A_NAME ?= libkms.a
CONFIG ?= config.h
COMPILER_LANGUAGE ?= c

# crypto library to use for standard algos, could save ~1-2kb ;)
# can be either 'openssl', 'polarssl' or anything other for internal impl
CRYPTO ?= internal

# use DNS_PARSER=internal if your OS doesn't supply the DNS parser routines
DNS_PARSER ?= OS

# You should supply your own version string here

VLMCSD_VERSION ?= $(shell test -d .svn && echo svn`svnversion`)

FEATURES ?= full
VERBOSE ?= NO

################################################################################

CC ?= gcc
TARGETPLATFORM := $(shell LANG=en_US.UTF-8 $(CC) -v 2>&1 | grep '^Target: ' | cut -f 2 -d ' ')

ifneq (,$(findstring darwin,$(TARGETPLATFORM)))
  DARWIN := 1
  UNIX := 1
endif

ifneq (,$(findstring androideabi,$(TARGETPLATFORM)))
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
  DLL_NAME ?= cygkms.dll
else ifeq ($(WIN),1)
  DLL_NAME ?= libkms.dll
else ifeq ($(DARWIN),1)
  DLL_NAME ?= libkms.dylib
else
  DLL_NAME ?= libkms.so
endif

BASECFLAGS = -DVLMCSD_COMPILER=\"$(notdir $(CC))\" -DVLMCSD_PLATFORM=\"$(TARGETPLATFORM)\" -DCONFIG=\"$(CONFIG)\" -DBUILD_TIME=$(shell date '+%s') -g -Os -fno-strict-aliasing -fomit-frame-pointer -ffunction-sections -fdata-sections
BASELDFLAGS = 
STRIPFLAGS =
CLIENTLDFLAGS =
SERVERLDFLAGS =

ifndef SAFE_MODE
  BASECFLAGS += -fvisibility=hidden -pipe -fno-common -fno-exceptions -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -fmerge-all-constants
  
  ifeq ($(ELF),1)
    BASELDFLAGS += -Wl,-z,norelro
  endif

  ifneq (,$(findstring gcc,$(notdir $(CC))))
    BASECFLAGS += -flto
  endif

endif

ifeq ($(NOLIBS),1)
  NOLRESOLV=1
  NOLPTHREAD=1
endif

ifneq ($(NO_DNS),1)
  ifneq ($(ANDROID),1)
  ifneq ($(NOLRESOLV),1)

    ifeq ($(MINGW),1)
      CLIENTLDFLAGS += -ldnsapi
    endif

    ifeq ($(LINUX),1)
      CLIENTLDFLAGS += -lresolv
    endif

    ifeq ($(HURD),1)
      CLIENTLDFLAGS += -lresolv
    endif

    ifeq ($(DARWIN),1)
      CLIENTLDFLAGS += -lresolv
    endif

    ifeq ($(CYGWIN),1)
      DNS_PARSER := internal
      CLIENTLDFLAGS += -lresolv
    endif

    ifeq ($(OPENBSD),1)
      DNS_PARSER := internal
    endif

    ifeq ($(SOLARIS),1)
      CLIENTLDFLAGS += -lresolv
    endif

  endif
  endif
else
  BASECFLAGS += -DNO_DNS
endif 

ifneq ($(CAT),2)
  BASECFLAGS += "-Wall"
endif  

ifeq ($(DARWIN), 1)
  STRIPFLAGS += -Wl,-S -Wl,-x
  BASECFLAGS += -Wno-deprecated-declarations
else ifeq ($(shell uname), SunOS)
  STRIPFLAGS += -s
  ifeq ($(notdir $(LD_ALTEXEC)),gld)
    BASELDFLAGS += -Wl,--gc-sections
  endif
  BASELDFLAGS += -lsocket
else
  ifneq ($(CC),tcc)
  	BASELDFLAGS += -Wl,--gc-sections
  endif
  STRIPFLAGS += -s
endif

LIBRARY_CFLAGS = -DSIMPLE_SOCKETS -DNO_TIMEOUT -DNO_SIGHUP -DNO_CL_PIDS -DNO_EXTENDED_PRODUCT_LIST -DNO_BASIC_PRODUCT_LIST -DNO_LOG -DNO_RANDOM_EPID -DNO_INI_FILE -DNO_INI_FILE -DNO_HELP -DNO_CUSTOM_INTERVALS -DNO_PID_FILE -DNO_USER_SWITCH -DNO_VERBOSE_LOG -DNO_LIMIT -DNO_VERSION_INFORMATION

ifeq ($(FEATURES), embedded)
  BASECFLAGS += -DNO_HELP -DNO_USER_SWITCH -DNO_BASIC_PRODUCT_LIST -DNO_CUSTOM_INTERVALS -DNO_PID_FILE -DNO_VERBOSE_LOG -DNO_VERSION_INFORMATION
else ifeq ($(FEATURES), autostart)
  BASECFLAGS += -DNO_HELP -DNO_VERSION_INFORMATION
else ifeq ($(FEATURES), minimum)
  BASECFLAGS += $(LIBRARY_CFLAGS)
else ifeq ($(FEATURES), most)
  BASECFLAGS += -DNO_SIGHUP -DNO_PID_FILE -DNO_LIMIT
else ifeq ($(FEATURES), inetd)
  BASECFLAGS += -DNO_SIGHUP -DNO_SOCKETS -DNO_PID_FILE -DNO_LIMIT -DNO_VERSION_INFORMATION
else ifeq ($(FEATURES), fixedepids)
  BASECFLAGS += -DNO_SIGHUP -DNO_CL_PIDS -DNO_RANDOM_EPID -DNO_INI_FILE
endif

ifdef INI
  BASECFLAGS += -DINI_FILE=\"$(INI)\"
endif

ifeq ($(THREADS), 1)
  BASECFLAGS += -DUSE_THREADS
endif

ifeq ($(CHILD_HANDLER), 1)
  BASECFLAGS += -DCHILD_HANDLER
endif

ifeq ($(NO_TIMEOUT), 1)
  BASECFLAGS += -DNO_TIMEOUT
endif

ifdef WINDOWS
  BASECFLAGS += -DEPID_WINDOWS=\"$(WINDOWS)\"
endif

ifdef OFFICE2010
  BASECFLAGS += -DEPID_OFFICE2010=\"$(OFFICE2010)\"
endif

ifdef OFFICE2013
  BASECFLAGS += -DEPID_OFFICE2013=\"$(OFFICE2013)\"
endif

ifdef HWID
  BASECFLAGS += -DHWID=$(HWID)
endif

ifdef TERMINAL_WIDTH
  BASECFLAGS += -DTERMINAL_FIXED_WIDTH=$(TERMINAL_WIDTH) -DDISPLAY_WIDTH=\"$(TERMINAL_WIDTH)\"
endif

ifeq ($(NOPROCFS), 1)
  BASECFLAGS += -DNO_PROCFS
endif

ifeq ($(AUXV), 1)
  BASECFLAGS += -DUSE_AUXV
endif

ifneq ($(ANDROID), 1)
ifneq ($(MINIX), 1)
ifneq ($(NOLPTHREAD), 1)

  ifeq ($(THREADS), 1)
    SERVERLDFLAGS += -lpthread
  endif
  
  ifeq (,$(findstring NO_LIMIT,$(CFLAGS) $(BASECFLAGS)))  
    SERVERLDFLAGS += -lpthread
  endif

endif
endif
endif

$(MULTI_NAME): BASECFLAGS += -DMULTI_CALL_BINARY=1

all: $(CLIENT_NAME) $(PROGRAM_NAME)

#ifdef CAT
  allmulti: $(CLIENT_NAME) $(PROGRAM_NAME) $(MULTI_NAME)
#endif

ifneq ($(strip $(VLMCSD_VERSION)),)
  BASECFLAGS += -DVERSION=\"$(VLMCSD_VERSION),\ built\ $(shell date -u '+%Y-%m-%d %H:%M:%S' | sed -e 's/ /\\ /g')\ UTC\" 
endif

ifdef CAT
  BASECFLAGS += -DONE_FILE
endif

SRCS = crypto.c kms.c endian.c output.c shared_globals.c helpers.c
HEADERS = $(CONFIG) types.h rpc.h vlmcsd.h endian.h crypto.h kms.h network.h output.h shared_globals.h vlmcs.h helpers.h
DEPS = $(MULTI_SRCS:.c=.d)

VLMCSD_SRCS = vlmcsd.c $(SRCS)
VLMCSD_OBJS = $(VLMCSD_SRCS:.c=.o)

VLMCS_SRCS = vlmcs.c $(SRCS)
VLMCS_OBJS = $(VLMCS_SRCS:.c=.o)

MULTI_SRCS = vlmcsd.c vlmcs.c vlmcsdmulti.c $(SRCS)
MULTI_OBJS = $(SRCS:.c=.o) vlmcsd-m.o vlmcs-m.o vlmcsdmulti-m.o

DLL_SRCS = libkms.c $(SRCS)
DLL_OBJS = $(DLL_SRCS:.c=.o)

PDFDOCS = vlmcs.1.pdf vlmcsd.7.pdf vlmcsd.8.pdf vlmcsdmulti.1.pdf vlmcsd.ini.5.pdf vlmcsd-floppy.7.pdf
HTMLDOCS = $(PDFDOCS:.pdf=.html)
UNIXDOCS = $(PDFDOCS:.pdf=.unix.txt)
DOSDOCS = $(PDFDOCS:.pdf=.dos.txt)

ifneq ($(NO_DNS),1)

  VLMCS_SRCS += dns_srv.c
  MULTI_SRCS += dns_srv.c
  MULTI_OBJS += dns_srv.o

ifeq ($(DNS_PARSER),internal)
ifneq ($(MINGW),1)
  VLMCS_SRCS += ns_parse.c ns_name.c
  MULTI_SRCS += ns_parse.c ns_name.c
  MULTI_OBJS += ns_parse.o ns_name.o
  BASECFLAGS += "-DDNS_PARSER_INTERNAL"
endif
endif

endif

ifeq ($(MSRPC),1)
  VLMCSD_SRCS += msrpc-server.c
  VLMCS_SRCS += msrpc-client.c
  MULTI_SRCS += msrpc-server.c msrpc-client.c
  MULTI_OBJS += msrpc-server-m.o msrpc-client-m.o
  DLL_SRCS += msrpc-server.c
  BASECFLAGS += -DUSE_MSRPC -Wno-unknown-pragmas
  BASELDFLAGS += -lrpcrt4
else
  SRCS += network.c rpc.c  
endif

ifeq "$(WIN)" "1"
	VLMCSD_SRCS += ntservice.c
	MULTI_SRCS += ntservice.c
	MULTI_OBJS += ntservice.o
endif

ifeq ($(CRYPTO), openssl_with_aes)
	BASECFLAGS += -D_CRYPTO_OPENSSL -D_USE_AES_FROM_OPENSSL
	BASELDFLAGS += -lcrypto
	SRCS += crypto_openssl.c
else ifeq ($(CRYPTO), openssl_with_aes_soft)
	BASECFLAGS += -D_CRYPTO_OPENSSL -D_USE_AES_FROM_OPENSSL -D_OPENSSL_SOFTWARE
	BASELDFLAGS += -lcrypto
	SRCS += crypto_openssl.c
else ifeq ($(CRYPTO), openssl)
	BASECFLAGS += -D_CRYPTO_OPENSSL
	BASELDFLAGS += -lcrypto
	SRCS += crypto_openssl.c
else ifeq ($(CRYPTO), polarssl)
	BASECFLAGS += -D_CRYPTO_POLARSSL
	BASELDFLAGS += -lpolarssl
else ifeq ($(CRYPTO), windows)
	BASECFLAGS += -D_CRYPTO_WINDOWS
	SRCS += crypto_windows.c
else
	BASECFLAGS += -D_CRYPTO_INTERNAL
	SRCS += crypto_internal.c
endif

ifneq ($(STRIP),0)
	BASELDFLAGS += $(STRIPFLAGS)
endif

ifeq ($(OPENSSL_HMAC),0)
	BASECFLAGS += -D_OPENSSL_NO_HMAC
endif

ifeq ($(DEPENDENCIES),2)
    BASECFLAGS += -MMD
endif

ifeq ($(VERBOSE),3)
    COMPILER := $(shell printf "%-40s" $(notdir $(CC)))
    ARCHIVER := $(shell printf "%-40s" $(notdir $(AR)))
endif

ARCMD := AR

ifdef CAT
    LDCMD := CC/LD
else
    LDCMD := LD    
endif

-include $(MULTI_SRCS:.c=.d)

%.o: %.c 
  ifeq ($(VERBOSE),1)
	$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -c $<
  ifeq ($(DEPENDENCIES),1)
	$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -MM -MF $*.d $<
  endif
  else
	@echo "$(COMPILER)	CC	$@ <- $<"
	@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -c $<
  ifeq ($(DEPENDENCIES),1)
	@echo "$(COMPILER)	DEP	$*.d <- $<"
	@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -MM -MF $*.d $<
  endif
  endif

%-m.o: %.c
  ifeq ($(VERBOSE),1)
	$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -o $@ -c $<
  ifeq ($(DEPENDENCIES),1)
	$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -MM -MF $*.d $<
  endif
  else
	@echo "$(COMPILER)	CC	$@ <- $<"
	@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -o $@ -c $<
  ifeq ($(DEPENDENCIES),1)
	@echo "$(COMPILER)	DEP	$*.d <- $<"
	@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -MM -MF $*.d $<
  endif
  endif


ifdef CAT
  BUILDCOMMAND = cat $^ | $(CC) -x$(COMPILER_LANGUAGE) -o $@ -
  VLMCSD_PREREQUISITES = $(VLMCSD_SRCS)
  VLMCS_PREREQUISITES = $(VLMCS_SRCS)
  MULTI_PREREQUISITES = $(MULTI_SRCS)
  DLL_PREREQUISITES = $(DLL_SRCS)
  OBJ_PREREQUISITES = $(DLL_SRCS)
else
  BUILDCOMMAND = $(CC) -o $@ $^
  VLMCSD_PREREQUISITES = $(VLMCSD_OBJS)
  VLMCS_PREREQUISITES = $(VLMCS_OBJS)
  MULTI_PREREQUISITES = $(MULTI_OBJS)
  DLL_PREREQUISITES = $(DLL_OBJS)
  OBJ_PREREQUISITES = $(DLL_OBJS)
endif

ifeq ($(VERBOSE),1)
  BUILDCOMMANDPREFIX = +
else
  BUILDCOMMANDPREFIX = +@
endif

INFOCOMMAND = +@echo "$(COMPILER)	$(LDCMD)	$@ <- $^"
ARINFOCOMMAND = +@echo "$(ARCHIVER)	$(ARCMD)	$@ <. $^"

VLMCSD_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(SERVERLDFLAGS)
VLMCS_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(CLIENTLDFLAGS)
MULTI_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(CLIENTLDFLAGS) $(SERVERLDFLAGS)
DLL_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -shared -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC
OBJ_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC
  
$(PROGRAM_NAME): $(VLMCSD_PREREQUISITES)
  ifneq ($(VERBOSE),1)
	$(INFOCOMMAND)
  endif
	$(VLMCSD_COMMAND)

$(CLIENT_NAME): $(VLMCS_PREREQUISITES)
  ifneq ($(VERBOSE),1)
	$(INFOCOMMAND)
  endif
	$(VLMCS_COMMAND)

$(MULTI_NAME): $(MULTI_PREREQUISITES)
  ifneq ($(VERBOSE),1)
	$(INFOCOMMAND)
  endif
	$(MULTI_COMMAND)

$(DLL_NAME): $(DLL_PREREQUISITES)
  ifneq ($(VERBOSE),1)
	$(INFOCOMMAND)
  endif
	$(DLL_COMMAND)

ifndef CAT
$(OBJ_NAME):
	+@echo Cannot make $@ without CAT defined. Please create $(A_NAME)
else
$(OBJ_NAME): $(OBJ_PREREQUISITES)
  ifneq ($(VERBOSE),1)
	$(INFOCOMMAND)
  endif
	$(OBJ_COMMAND)
endif

ifdef CAT
$(A_NAME): $(OBJ_NAME)
else
$(A_NAME): BASECFLAGS += -fvisibility=hidden -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC
$(A_NAME): $(DLL_OBJS)
endif
  ifneq ($(VERBOSE),1)
	$(ARINFOCOMMAND)
  endif
	+@rm -f $@
	$(BUILDCOMMANDPREFIX)$(AR) rcs $@ $^

%.pdf : %
  ifeq ($(shell uname), Darwin)
	groff -Tps -mandoc -c $< | pstopdf -i -o $@
  else
	groff -Tpdf -mandoc -c $< > $@
  endif

%.html : %
	groff -Thtml -mandoc -c $< > $@ 

%.unix.txt : %
	groff -P -c -Tutf8 -mandoc -c $< | col -bx > $@

%.dos.txt : %.unix.txt
#	unix2dos -n $< $@
#	sed -e 's/$$/\r/' $< > $@
	awk 'sub("$$", "\r")' $< > $@

pdfdocs : $(PDFDOCS)

dosdocs : $(DOSDOCS)

unixdocs : $(UNIXDOCS)

htmldocs : $(HTMLDOCS)

alldocs : $(UNIXDOCS) $(HTMLDOCS) $(PDFDOCS) $(DOSDOCS)

clean:
	rm -f *.o *.d *_all.c libkms_all_*.c $(PROGRAM_NAME) $(MULTI_NAME) $(DLL_NAME) $(CLIENT_NAME) $(PDFDOCS) $(DOSDOCS) $(UNIXDOCS) $(HTMLDOCS) $(OBJ_NAME) $(A_NAME) *.a

dnsclean:
	rm -f dns_srv.o

help:
	@echo "Type"
	@echo "    ${MAKE}          - to build $(PROGRAM_NAME) and $(CLIENT_NAME)"
	@echo "    ${MAKE} clean    - to remove $(PROGRAM_NAME) and $(CLIENT_NAME)"
	@echo "    ${MAKE} help     - to see this help"
	@echo "    ${MAKE} pdfdocs  - Create PDF versions of the documentation (Requires groff with PDF support)."
	@echo "    ${MAKE} htmldocs - Create HTML versions of the documentation."
	@echo "    ${MAKE} unixdocs - Create Unix TXT versions of the documentation."
	@echo "    ${MAKE} dosdocs  - Create DOS/Windows TXT versions of the documentation."
	@echo "    ${MAKE} alldocs  - Create all versions of the documentation."
	@echo "    ${MAKE} -j <x>   - Use <x> parallel tasks (SMP support) when compiling $(PROGRAM_NAME) and $(CLIENT_NAME)"
	@echo ""
	@echo "    ${MAKE} $(PROGRAM_NAME) - to build the server only."
	@echo "    ${MAKE} $(CLIENT_NAME) - to build the client only."
	@echo "    ${MAKE} $(MULTI_NAME) - to build $(PROGRAM_NAME) and $(CLIENT_NAME) in a single multi-call binary"
	@echo "    ${MAKE} $(DLL_NAME) - to build the shared library $(DLL_NAME)"
	@echo "    ${MAKE} $(A_NAME) - to build the static library $(A_NAME)"
	@echo ""
	@echo "Options"
	@echo "    CONFIG=<x>                   Compile <x> as instead of config.h."
	@echo "    INI=<x>                      Compile $(PROGRAM_NAME) with default ini file <x>"
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
	@echo "    AR=<x>                       Use <x> instead of ar to build $(A_NAME). Set to gcc-ar if you want to use gcc's LTO feature."
	@echo "    COMPILER_LANGUAGE=<x>        May be c or c++."
	@echo "    TERMINAL_WIDTH=<x>           Assume a fixed terminal width of <x> columns. Use in case of problems only."  
	@echo "    VLMCSD_VERSION=<x>           Sets <x> as your version identifier. Defaults to \"private build\"."
	@echo "    CFLAGS=<x>                   Pass <x> as additional arguments to the compiler."
	@echo "    LDFLAGS=<x>                  Pass <x> as additional arguments to the linker."
	@echo "    PLATFORMFLAGS=<x>            Pass <x> as additional arguments to the compiler and the linker."
	@echo "    BASECFLAGS=<x>               Pass only <x> as arguments to the compiler (advanced users only)."
	@echo "    BASELDFLAGS=<x>              Pass only <x> as arguments to the linker (advanced users only)."
	@echo "    STRIP=0                      Don't strip debug information from $(PROGRAM_NAME) and $(CLIENT_NAME) (for developers)."
	@echo "    VERBOSE=1                    Be verbose when making targets."
	@echo "    VERBOSE=3                    Show name of compiler."
	@echo "    THREADS=1                    Use threads instead of fork(). Automatically set for native Windows. Recommended for Cygwin."
	@echo "    WINDOWS=<x>                  Use <x> as the default ePID for Windows (when using $(PROGRAM_NAME) with -r 0)."
	@echo "    OFFICE2010=<x>               Use <x> as the default ePID for Office2010 (when using $(PROGRAM_NAME) with -r 0)."
	@echo "    OFFICE2013=<x>               Use <x> as the default ePID for Office2013 (when using $(PROGRAM_NAME) with -r 0)."
	@echo "    HWID=<x>                     Use <x> as the default HWID (when it can't be found in an ini file)."
	@echo "    FEATURES=full                Compile $(PROGRAM_NAME) with all features (default)."
	@echo "    FEATURES=most                Compile $(PROGRAM_NAME) without rarely used features."
	@echo "    FEATURES=embedded            Compile $(PROGRAM_NAME) with typical features for embedded systems."
	@echo "    FEATURES=autostart           Removes features typically not needed if you place $(PROGRAM_NAME) in an autostart script."
	@echo "    FEATURES=inetd               Compile $(PROGRAM_NAME) for running through an internet superserver only."
	@echo "    FEATURES=minimum             Compiles only basic features of $(PROGRAM_NAME)."
	@echo "    FEATURES=fixedepids          $(PROGRAM_NAME) only uses bultin internal ePIDs."
	@echo ""
	@echo "Useful CFLAGS to save memory when running $(PROGRAM_NAME) on very small embedded devices (finer control than FEATURES=)"
	@echo "    -DNO_EXTENDED_PRODUCT_LIST   Don't compile the detailed product list."
	@echo "    -DNO_BASIC_PRODUCT_LIST      Don't compile the basic product list."
	@echo "    -DNO_VERBOSE_LOG             Don't support verbose logging. Removes -v option."
	@echo "    -DNO_LOG                     Don't add support for logging. Implies -DNO_VERBOSE_LOG -DNO_EXTENDED_PRODUCT_LIST and -DNO_BASIC_PRODUCT_LIST."
	@echo "    -DNO_RANDOM_EPID             Don't support random ePIDs."
	@echo "    -DNO_INI_FILE                Don't support reading ePIDs/HWIDs from a file."
	@echo "    -DNO_PID_FILE                Don't support writing a PID file. Removes -p option."
	@echo "    -DNO_USER_SWITCH             Don't support changing uid/gid after program start. Removes -u and -g options."
	@echo "    -DNO_HELP                    Don't support command line help."
	@echo "    -DNO_CUSTOM_INTERVALS        Don't support custom intervals for retry and refresh activation. Removes -A and -R options."
	@echo "    -DNO_FREEBIND                Don't support binding to foreign IP addresses. Removes -F0 and -F1 options. Only affects FreeBSD and Linux."
	@echo "    -DSIMPLE_SOCKETS             Compile $(PROGRAM_NAME) with basic socket support only. Removes -L option."
	@echo "    -DNO_SOCKETS                 Don't support standalone operation. Requires an internet superserver to start $(PROGRAM_NAME)."
	@echo "    -DNO_CL_PIDS                 Don't support specifying ePIDs and HwId from the command line in $(PROGRAM_NAME)."
	@echo "    -DNO_LIMIT                   Don't support limiting concurrent clients in $(PROGRAM_NAME)."
	@echo "    -DNO_SIGHUP                  Don't support SIGHUP handling in $(PROGRAM_NAME)."
	@echo "    -DNO_VERSION_INFORMATION     Don't support displaying version information in $(PROGRAM_NAME) and $(CLIENT_NAME). Removes -V option."
	@echo "    -DENABLE_DEPRECATED_OPTIONS  Enable command line options that provide compatibility with previous versions of $(PROGRAM_NAME)."
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
	@echo "    NO_DNS=1                     Compile $(CLIENT_NAME) without support for detecting KMS servers via DNS."
	@echo "    DNS_PARSER=internal          Use $(CLIENT_NAME) internal DNS parsing routines. No effect on MingW (native Windows)."
	@echo ""
	@echo "Other useful CFLAGS"
	@echo "    -DSUPPORT_WINE               Add code that the Windows version of $(PROGRAM_NAME) runs on Wine if MSRPC=1"
	@echo "    -D_PEDANTIC                  Report rare error/warning conditions instead of silently ignoring them."
	@echo "    -DINCLUDE_BETAS              Include SKU / activation IDs for obsolete beta/preview products."
	@echo "    -DFD_SETSIZE=<x>             Allow <x> -L statements in $(PROGRAM_NAME) (default: 64 on Windows, 1024 on most Unixes)."
	@echo "    -flto                        Use link time optimization. Not supported by old compilers (gcc < 4.7). Use whenever supported."
	@echo "    -flto=jobserver              Utilize all CPUs during link time optimization. Requires ${MAKE} -j <cpus>"
	@echo "    -fno-stack-protector         No stack checking. Smaller binaries."
	@echo "    -pipe                        Use pipes instead of temporary files (faster compilation, extends the life of your SSD)." 
