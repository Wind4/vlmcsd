################################################################################

.PHONY: clean

PROGRAM_NAME ?= ../bin/vlmcsd
CLIENT_NAME ?= ../bin/vlmcs
MULTI_NAME ?= ../bin/vlmcsdmulti
OBJ_NAME ?= ../build/libkms-static.o
A_NAME ?= ../lib/libkms.a
CONFIG ?= config.h
COMPILER_LANGUAGE ?= c

BASE_PROGRAM_NAME=$(notdir $(PROGRAM_NAME))
BASE_CLIENT_NAME=$(notdir $(CLIENT_NAME))
BASE_MULTI_NAME=$(notdir $(MULTI_NAME))
BASE_DLL_NAME=$(notdir $(DLL_NAME))
BASE_A_NAME=$(notdir $(A_NAME))

ifeq (1,$(FROM_PARENT))

  CLIENT_NAME_TEST=$(patsubst /%,/,$(CLIENT_NAME))
  MULTI_NAME_TEST=$(patsubst /%,/,$(MULTI_NAME))
  DLL_NAME_TEST=$(patsubst /%,/,$(DLL_NAME))
  A_NAME_TEST=$(patsubst /%,/,$(A_NAME))
  PROGRAM_NAME_TEST=$(patsubst /%,/,$(PROGRAM_NAME))

  ifneq (/,$(PROGRAM_NAME_TEST))
    PROGRAM_PREFIX=../
  endif

  ifneq (/,$(CLIENT_NAME_TEST))
    CLIENT_PREFIX=../
  endif

  ifneq (/,$(MULTI_NAME_TEST))
    MULTI_PREFIX=../
  endif

  ifneq (/,$(DLL_NAME_TEST))
    DLL_PREFIX=../
  endif

  ifneq (/,$(A_NAME_TEST))
    A_PREFIX=../
  endif

endif

REAL_PROGRAM_NAME=$(PROGRAM_PREFIX)$(PROGRAM_NAME)
REAL_CLIENT_NAME=$(CLIENT_PREFIX)$(CLIENT_NAME)
REAL_MULTI_NAME=$(MULTI_PREFIX)$(MULTI_NAME)
REAL_DLL_NAME=$(DLL_PREFIX)$(DLL_NAME)
REAL_A_NAME=$(A_PREFIX)$(A_NAME)


# crypto library to use for standard algos, could save ~1-2kb ;)
# can be either 'openssl', 'polarssl' or anything other for internal impl
CRYPTO ?= internal

# use DNS_PARSER=internal if your OS doesn't supply the DNS parser routines
DNS_PARSER ?= OS

# You should supply your own version string here

VLMCSD_VERSION ?= $(shell test -d ../.git && git describe)

FEATURES ?= full
VERBOSE ?= NO

################################################################################

CC ?= gcc
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
  DLL_NAME ?= ../lib/cygkms.dll
else ifeq ($(WIN),1)
  DLL_NAME ?= ../lib/libkms.dll
else ifeq ($(DARWIN),1)
  DLL_NAME ?= ../lib/libkms.dylib
else
  DLL_NAME ?= ../lib/libkms.so
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

ifeq ($(ELF), 1)
  PICFLAGS += -fPIC
endif

ifeq ($(NOLIBS),1)
  NOLRESOLV=1
  NOLPTHREAD=1
endif

ifneq ($(NOLIBS),1)
  ifeq ($(MINGW),1)
    BASELDFLAGS += -lws2_32 -liphlpapi -lshlwapi
  endif
endif 

ifneq ($(NOLIBS),1)
  ifeq ($(CYGWIN),1)
    BASELDFLAGS += -liphlpapi
  endif
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

LIBRARY_CFLAGS = -DSIMPLE_SOCKETS -DNO_TIMEOUT -DNO_SIGHUP -DNO_CL_PIDS -DNO_LOG -DNO_RANDOM_EPID -DNO_INI_FILE -DNO_HELP -DNO_CUSTOM_INTERVALS -DNO_PID_FILE -DNO_USER_SWITCH -DNO_VERBOSE_LOG -DNO_LIMIT -DNO_VERSION_INFORMATION -DNO_PRIVATE_IP_DETECT -DNO_STRICT_MODES -DNO_CLIENT_LIST -DNO_TAP -UNO_SOCKETS -USIMPLE_RPC

ifeq ($(FEATURES), embedded)
  BASECFLAGS += -DNO_HELP -DNO_USER_SWITCH -DNO_CUSTOM_INTERVALS -DNO_PID_FILE -DNO_VERBOSE_LOG -DNO_VERSION_INFORMATION
else ifeq ($(FEATURES), autostart)
  BASECFLAGS += -DNO_HELP -DNO_VERSION_INFORMATION
else ifeq ($(FEATURES), minimum)
  BASECFLAGS += -DSIMPLE_RPC -DSIMPLE_SOCKETS -DNO_TIMEOUT -DNO_SIGHUP -DNO_CL_PIDS -DNO_LOG -DNO_RANDOM_EPID -DNO_INI_FILE -DNO_HELP -DNO_CUSTOM_INTERVALS -DNO_PID_FILE -DNO_USER_SWITCH -DNO_VERBOSE_LOG -DNO_LIMIT -DNO_VERSION_INFORMATION -DNO_PRIVATE_IP_DETECT -DSMALL_AES -DNO_STRICT_MODES -DNO_TAP -DNO_CLIENT_LIST -DUNSAFE_DATA_LOAD -DNO_EXTERNAL_DATA -UFULL_INTERNAL_DATA -U_PEDANTIC
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

ifdef DATA
  BASECFLAGS += -DDATA_FILE=\"$(DATA)\"
endif

ifeq ($(NO_GETIFADDRS), 1)
  BASECFLAGS += -DNO_GETIFADDRS
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

ifdef OFFICE2016
  BASECFLAGS += -DEPID_OFFICE2016=\"$(OFFICE2016)\"
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
ifneq ($(DARWIN), 1)

  ifeq ($(THREADS), 1)
    SERVERLDFLAGS += -lpthread
  endif
  
  ifeq (,$(findstring NO_LIMIT,$(CFLAGS) $(BASECFLAGS)))  
    SERVERLDFLAGS += -lpthread
  endif

endif
endif
endif
endif

$(REAL_MULTI_NAME): BASECFLAGS += -DMULTI_CALL_BINARY=1

all: $(REAL_CLIENT_NAME) $(REAL_PROGRAM_NAME)

allmulti: $(REAL_CLIENT_NAME) $(REAL_PROGRAM_NAME) $(REAL_MULTI_NAME)

vlmcsd: $(REAL_PROGRAM_NAME)
	+@true

vlmcs: $(REAL_CLIENT_NAME)
	+@true

vlmcsdmulti: $(REAL_MULTI_NAME)
	+@true

libkms: $(REAL_DLL_NAME)
	+@true

libkms-static: $(REAL_A_NAME)

ifneq ($(strip $(VLMCSD_VERSION)),)
  BASECFLAGS += -DVERSION=\"$(VLMCSD_VERSION),\ built\ $(shell date -u '+%Y-%m-%d %H:%M:%S' | sed -e 's/ /\\ /g')\ UTC\" 
endif

ifdef CAT
  BASECFLAGS += -DONE_FILE
endif

SRCS = crypto.c kms.c endian.c output.c shared_globals.c helpers.c
HEADERS = $(CONFIG) types.h rpc.h vlmcsd.h endian.h crypto.h kms.h network.h output.h shared_globals.h vlmcs.h helpers.h kmsdata.h
DEPS = $(patsubst %,../build/%,$(MULTI_SRCS:.c=.d))

VLMCSD_SRCS = vlmcsd.c kmsdata.c $(SRCS)
VLMCSD_OBJS = $(patsubst %,../build/%,$(VLMCSD_SRCS:.c=.o))

VLMCS_SRCS = vlmcs.c kmsdata-full.c $(SRCS)
VLMCS_OBJS = $(patsubst %,../build/%,$(VLMCS_SRCS:.c=.o))

MULTI_SRCS = vlmcsd.c vlmcs.c vlmcsdmulti.c kmsdata-full.c $(SRCS)
MULTI_OBJS = $(patsubst %,../build/%,$(SRCS:.c=.o)) ../build/kmsdata-full.o ../build/vlmcsd-m.o ../build/vlmcs-m.o ../build/vlmcsdmulti-m.o

DLL_SRCS = libkms.c vlmcs.c $(SRCS)
DLL_OBJS = $(patsubst %,../build/%,$(DLL_SRCS:.c=-l.o))
A_OBJS = $(patsubst %,../build/%,$(DLL_SRCS:.c=-a.o))

PDFDOCS = vlmcs.1.pdf vlmcsd.7.pdf vlmcsd.8.pdf vlmcsdmulti.1.pdf vlmcsd.ini.5.pdf vlmcsd-floppy.7.pdf
HTMLDOCS = $(PDFDOCS:.pdf=.html)
UNIXDOCS = $(PDFDOCS:.pdf=.unix.txt)
DOSDOCS = $(PDFDOCS:.pdf=.dos.txt)

ifneq ($(NO_DNS),1)

  VLMCS_SRCS += dns_srv.c
  MULTI_SRCS += dns_srv.c
  MULTI_OBJS += ../build/dns_srv.o

ifeq ($(DNS_PARSER),internal)
ifneq ($(MINGW),1)
  VLMCS_SRCS += ns_parse.c ns_name.c
  MULTI_SRCS += ns_parse.c ns_name.c
  MULTI_OBJS += ../build/ns_parse.o ../build/ns_name.o
  BASECFLAGS += "-DDNS_PARSER_INTERNAL"
endif
endif

endif

ifeq ($(MSRPC),1)
  VLMCSD_SRCS += msrpc-server.c
  VLMCS_SRCS += msrpc-client.c
  MULTI_SRCS += msrpc-server.c msrpc-client.c
  MULTI_OBJS += ../build/msrpc-server-m.o ../build/msrpc-client-m.o
  DLL_SRCS += msrpc-server.c
  BASECFLAGS += -DUSE_MSRPC -Wno-unknown-pragmas
  BASELDFLAGS += -lrpcrt4
else
  SRCS += network.c rpc.c  
endif

ifeq ($(GETIFADDRS),musl)
ifneq ($(NO_GETIFADDRS),1)
  BASECFLAGS += -DGETIFADDRS_MUSL
  VLMCSD_SRCS += getifaddrs-musl.c
  MULTI_SRCS += getifaddrs-musl.c
  VLMCS_SRCS += getifaddrs-musl.c
  DLL_SRCS += getifaddrs-musl.c
  MULTI_OBJS += ../build/getifaddrs-musl.o
endif
endif

ifeq ($(ANDROID),1)
ifneq ($(NO_GETIFADDRS),1)
  VLMCSD_SRCS += ifaddrs-android.c
  MULTI_SRCS += ifaddrs-android.c
  DLL_SRCS += ifaddrs-android.c
  MULTI_OBJS += ../build/ifaddrs-android.o
endif
endif

ifeq "$(WIN)" "1"
	VLMCSD_SRCS += ntservice.c wintap.c
	MULTI_SRCS += ntservice.c wintap.c
	MULTI_OBJS += ../build/ntservice.o ../build/wintap.o
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

../build/%.o: %.c 
  ifeq ($(VERBOSE),1)
	+$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -c $< -o $@
  ifeq ($(DEPENDENCIES),1)
	+$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -MM -MF $*.d $< -MT $@
  endif
  else
	+@echo "$(COMPILER)	CC	$(notdir $@) <- $<"
	+@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -c $< -o $@
  ifeq ($(DEPENDENCIES),1)
	+@echo "$(COMPILER)	DEP	$*.d <- $<"
	+@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -MM -MF $*.d $< -MT $@
  endif
  endif

../build/%-m.o: %.c
  ifeq ($(VERBOSE),1)
	+$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -o $@ -c $<
  ifeq ($(DEPENDENCIES),1)
	+$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -MM -MF $*.d $< -MT $@
  endif
  else
	+@echo "$(COMPILER)	CC	$(notdir $@) <- $<"
	+@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -o $@ -c $<
  ifeq ($(DEPENDENCIES),1)
	+@echo "$(COMPILER)	DEP	$*.d <- $<"
	+@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) -MM -MF $*.d $< -MT $@
  endif
  endif

../build/%-a.o: %.c
  ifeq ($(VERBOSE),1)
	+$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC -o $@ -c $<
  ifeq ($(DEPENDENCIES),1)
	+$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC -MM -MF $*.d $<
  endif
  else
	+@echo "$(COMPILER)	CC	$(notdir $@) <- $<"
	+@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC -o $@ -c $<
  ifeq ($(DEPENDENCIES),1)
	+@echo "$(COMPILER)	DEP	$*.d <- $<"
	+@$(CC) -x$(COMPILER_LANGUAGE) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC -MM -MF $*.d $<
  endif
  endif

../build/%-l.o: %.c
  ifeq ($(VERBOSE),1)
	+$(CC) -x$(COMPILER_LANGUAGE) $(PICFLAGS) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC -o $@ -c $<
  ifeq ($(DEPENDENCIES),1)
	+$(CC) -x$(COMPILER_LANGUAGE) $(PICFLAGS) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC -MM -MF $*.d $<
  endif
  else
	+@echo "$(COMPILER)	CC	$(notdir $@) <- $<"
	+@$(CC) -x$(COMPILER_LANGUAGE) $(PICFLAGS) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC -o $@ -c $<
  ifeq ($(DEPENDENCIES),1)
	+@echo "$(COMPILER)	DEP	$*.d <- $<"
	+@$(CC) -x$(COMPILER_LANGUAGE) $(PICFLAGS) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(PLATFORMFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC -MM -MF $*.d $<
  endif
  endif


ifdef CAT
  BUILDCOMMAND = cat $^ | $(CC) -x$(COMPILER_LANGUAGE) -o $@ -
  VLMCSD_PREREQUISITES = $(VLMCSD_SRCS)
  VLMCS_PREREQUISITES = $(VLMCS_SRCS)
  MULTI_PREREQUISITES = $(MULTI_SRCS)
  DLL_PREREQUISITES = $(DLL_SRCS)
  OBJ_PREREQUISITES = $(DLL_SRCS)
  A_PREREQUISITES = $(DLL_SRCS)
else
  BUILDCOMMAND = $(CC) -o $@ $^
  VLMCSD_PREREQUISITES = $(VLMCSD_OBJS)
  VLMCS_PREREQUISITES = $(VLMCS_OBJS)
  MULTI_PREREQUISITES = $(MULTI_OBJS)
  DLL_PREREQUISITES = $(DLL_OBJS)
  OBJ_PREREQUISITES = $(A_OBJS)
  A_PREREQUISITES = $(A_OBJS)
endif

ifeq ($(VERBOSE),1)
  BUILDCOMMANDPREFIX = +
else
  BUILDCOMMANDPREFIX = +@
endif

INFOCOMMAND = +@echo "$(COMPILER)	$(LDCMD)	$@ <- $(notdir $^)"
ARINFOCOMMAND = +@echo "$(ARCHIVER)	$(ARCMD)	$@ <. $(notdir $^)"

VLMCSD_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(SERVERLDFLAGS)
VLMCS_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(CLIENTLDFLAGS)
MULTI_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(CLIENTLDFLAGS) $(SERVERLDFLAGS)
DLL_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PICFLAGS) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -shared -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC
OBJ_COMMAND = $(BUILDCOMMANDPREFIX)$(BUILDCOMMAND) $(PLATFORMFLAGS) $(BASECFLAGS) $(CFLAGS) $(BASELDFLAGS) $(LDFLAGS) $(SERVERLDFLAGS) -fvisibility=hidden -c -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC
  
$(REAL_PROGRAM_NAME): $(VLMCSD_PREREQUISITES)
  ifneq ($(VERBOSE),1)
	$(INFOCOMMAND)
  endif
	$(VLMCSD_COMMAND)

$(REAL_CLIENT_NAME): $(VLMCS_PREREQUISITES)
  ifneq ($(VERBOSE),1)
	$(INFOCOMMAND)
  endif
	$(VLMCS_COMMAND)

$(REAL_MULTI_NAME): $(MULTI_PREREQUISITES)
  ifneq ($(VERBOSE),1)
	$(INFOCOMMAND)
  endif
	$(MULTI_COMMAND)

$(REAL_DLL_NAME): $(DLL_PREREQUISITES)
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
$(REAL_A_NAME): $(OBJ_NAME)
else
$(REAL_A_NAME): BASECFLAGS += -fvisibility=hidden -DIS_LIBRARY=1 $(LIBRARY_CFLAGS) -UNO_SOCKETS -UUSE_MSRPC
$(REAL_A_NAME): $(A_OBJS)
endif
  ifneq ($(VERBOSE),1)
	$(ARINFOCOMMAND)
  endif
	+@rm -f $@
	$(BUILDCOMMANDPREFIX)$(AR) rcs $@ $^

clean:
	rm -f $(REAL_PROGRAM_NAME) $(REAL_MULTI_NAME) $(REAL_DLL_NAME) $(REAL_CLIENT_NAME)  $(OBJ_NAME) $(REAL_A_NAME) ../bin/* ../build/* *.d

dnsclean:
	rm -f ../build/dns_srv.o

help:
	@echo "Help is available by typing 'make help' in directory $(shell realpath `pwd`/..). Use 'cd ..' to get there."
