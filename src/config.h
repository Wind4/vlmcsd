#ifndef CONFIG_H_
#define CONFIG_H_

/* Don't change anything ABOVE this line */

/*
 * As a best practice do not change the original config.h as distributed with vlmcsd.
 * Instead make a copy, e.g. myconfig.h, customize it and type 'make CONFIG=myconfig.h'
 * to build vlmcsd. This prevents your copy being overwritten when you upgrade to a
 * new version.
 */



 /*
  * ----------------------------------------------------------------------------------------
  * Useful customizations. These options are mandatory. You cannot comment them out.
  * Feel free to change them to fit your needs.
  * ----------------------------------------------------------------------------------------
  */

#ifndef VERSION
  /*
   * Define your own version identifier here, e.g. '#define VERSION "my vlmcsd based on 1103"'
   */

#define VERSION "private build"

#endif // VERSION





#ifndef HWID // HwId from the Ratiborus VM
#define HWID 0x3A, 0x1C, 0x04, 0x96, 0x00, 0xB6, 0x00, 0x76
#endif





/*
 * Anything below this line is optional. If you want to use any of these options
 * uncomment one or more lines starting with "//#define"
 */



 /*
  * -------------------------------
  * Defaults
  * -------------------------------
  */



#ifndef INI_FILE
  /*
   * Uncomment and customize the following line if you want vlmcsd to look for an ini file
   * at a default location.
   */

   //#define INI_FILE "/etc/vlmcsd.ini"

#endif // INI_FILE





#ifndef DATA_FILE
/*
 * Uncomment and customize the following line if you want vlmcsd to look for a KMS data file
 * at a custom default location.
 */

 //#define DATA_FILE "/etc/vlmcsd.kmd"

#endif // DATA_FILE


/*
 * ----------------------------------------------------------------------------------------
 * Troubleshooting options. Please note that disabling features may also help troubleshooting.
 * If you have an old OS that does not support features like pthreads, shared memory or
 * semaphores, uncomment "#define NO_LIMIT" and "#define NO_SIGHUP" and leave "#define USE_THREADS"
 * commented out.
 * ----------------------------------------------------------------------------------------
 */


#ifndef CHILD_HANDLER
 /*
  * Uncomment the following #define if you are compiling for a platform that does
  * not correctly handle the SA_NOCLDWAIT flag when ignoring SIGCHLD, i.e. forked
  * processes remain as "zombies" after dying. This option will add a SIGCHLD handler that
  * "waits" for a child that has terminated. This is only required for a few
  * unixoid OSses.
  */

  //#define CHILD_HANDLER

#endif // CHILD_HANDLER


#ifndef NO_TIMEOUT
/*
 * Uncomment the following #define if you are compiling for a platform that does
 * not support custom socket send or receive timeouts.
 */

 //#define NO_TIMEOUT

#endif // NO_TIMEOUT


#ifndef NO_DNS
/*
 * Uncomment the following #define if you have trouble with accessing routines
 * from libresolv. If enabled, vlmcs will be compiled without support for
 * detecting KMS servers via DNS.
 */

 //#define NO_DNS

#endif // NO_DNS


#ifndef TERMINAL_FIXED_WIDTH
/*
 * Uncomment the following #define and optionally change its value if you are compiling for
 * a platform that cannot properly determine the width of a terminal/command prompt.
 * This affects the output of "vlmcsd -x" only. It should be rarely necessary to use this.
 */

 //#define TERMINAL_FIXED_WIDTH 80

#endif // TERMINAL_FIXED_WIDTH




#ifndef _PEDANTIC
/*
 * Uncomment the following #define if you want to do vlmcs and vlmcsd more checks on the data
 * it receives over the network. They are normally not necessary but may provide useful if
 * you are testing any KMS server or client emulator that may send malformed KMS packets.
 */

 //#define _PEDANTIC

#endif // _PEDANTIC




#ifndef NO_PROCFS
/*
 * Cygwin, Linux, Android, NetBSD, DragonflyBSD:
 *    Do not rely on a properly mounted proc filesystem and use the less reliable
 *    argv[0] to determine the program's executable name.
 *    Use only if absolutely necessary (very old versions of these OSses).
 *
 * Minix, OpenBSD:
 *    This option has no effect since the OS always must use the less reliable argv[0].
 *
 * FreeBSD, Mac OS X, iOS, Solaris, Windows:
 *    This option is not neccessary (and has no effect) since these OSses provide
 *    a reliable way to determine the executable name.
 *
 */

 //#define NO_PROCFS

#endif // NO_PROCFS




#ifndef USE_AUXV
/*
 * Linux only:
 *    Use the process' ELF aux vector to determine the executable name.
 *    This is actually the best method but is supported only with
 *
 *        * the musl library
 *        * the glbic library 2.16 or newer
 *
 *    It does NOT work with uclibc (most routers and other small devices) and glibc < 2.16.
 *    Use it only if your system supports it and you do not plan to use the binary on older systems.
 *    It won't work on debian 7 or Red Hat 6.x.
 *
 *    It is safe to try this by yourself. vlmcsd won't compile if your system doesn't support it.
 */

 //#define USE_AUXV

#endif // USE_AUXV




#ifndef _OPENSSL_NO_HMAC
/*
 * If you configured vlmcsd to use OpenSSL (which you shouldn't) you may use this option
 * to calculate the KMSv6 HMAC with internal code instead of using OpenSSL.
 *
 * This may be necessary for some embedded devices that have OpenSSL without HMAC support.
 */

 //#define _OPENSSL_NO_HMAC

#endif // _OPENSSL_NO_HMAC




/*
 * ----------------------------------------------------------------------------------------
 * Modes of operation
 * ----------------------------------------------------------------------------------------
 */


#ifndef USE_THREADS
 /*
  * Do not use fork() but threads to serve your clients.
  *
  * Unix-like operarting systems:
  *    You may use this or not. Entirely your choice. Threads do not require explicitly allocating
  *    a shared memory segment which might be a problem on some systems. Using fork() is more robust
  *    although the threaded version of vlmcsd is rock solid too.
  *
  *    Some older unixoid OSses may not have pthreads. Do NOT use USE_THREADS and define NO_SIGHUP
  *    and NO_LIMIT instead to disable use of the pthreads, shared memory and semaphores.
  *
  * Cygwin:
  *    It is recommended to use threads since fork() is extremely slow (no copy on write) and somewhat
  *    unstable.
  *
  * Windows:
  *    This option has no effect since fork() is not supported.
  */

  //#define USE_THREADS

#endif // USE_THREADS




#ifndef _CRYPTO_POLARSSL
/*
 * Not available on native Windows. Can be used with Cygwin.
 *
 * Use PolarSSL for crypto routines if possible and if it is safe. There is not much benefit by using this
 * options since it can be used for SHA256 and HMAC_SHA256 only. It cannot be used for AES calculations because
 * KMSv6 uses a modified algorithm that PolarSSL does not support. KMSv4 CMAC is also unsupported since it uses
 * a Rijndael keysize (160 bits) that is not part of the AES standard.
 *
 * It is strongly recommended not to use an external crypto library.
 *
 * Do not define both _CRYPTO_OPENSSL and _CRYPTO_POLARSSL
 */

 //#define _CRYPTO_POLARSSL

#endif // _CRYPTO_POLARSSL




#ifndef _CRYPTO_OPENSSL
/*
 * Not available on native Windows. Can be used with Cygwin.
 *
 * Use OpenSSL for crypto routines if possible and if it is safe. There is not much benefit by using this
 * options since it can be used for SHA256 and HMAC_SHA256 only. It cannot be used for AES calculations because
 * KMSv6 uses a modified algorithm that OpenSSL does not support. KMSv4 CMAC is also unsupported since it uses
 * a Rijndael keysize (160 bits) that is not part of the AES standard.
 *
 * It is strongly recommended not to use an external crypto library.
 *
 * Do not define both _CRYPTO_OPENSSL and _CRYPTO_POLARSSL
 */

 //#define _CRYPTO_OPENSSL

#endif // _CRYPTO_OPENSSL




#ifndef _USE_AES_FROM_OPENSSL
/*
 * DANGEROUS: Tweak OpenSSL to perform KMSv4 CMAC and KMSv6 modified AES. This option creates the expanded
 * AES key by itself and then applies modifications to it. OpenSSL will then perfom modified AES operations.
 *
 * This options tampers with internal structures of OpenSSL that are subject to change or may have a platform
 * specific implementation. In this case your resulting binary can only perform KMSv5 operations.
 *
 * This option has no effect if _CRYPTO_OPENSSL is not defined.
 *
 * Don't use this except for your own research on the internals of OpenSSL.
 */

 //#define _USE_AES_FROM_OPENSSL

#endif // _USE_AES_FROM_OPENSSL





#ifndef _OPENSSL_SOFTWARE
/*
 * Use this only if you have defined _CRYPTO_OPENSSL and _USE_AES_FROM_OPENSSL. It has no effect otherwise.
 *
 * This options assumes a different internal AES expanded key in OpenSSL which is used mostly if OpenSSL is
 * compiled without support for hardware accelerated AES. It's worth a try if _USE_AES_FROM_OPENSSL doesn't work.
 */

 //#define _OPENSSL_SOFTWARE

#endif // _OPENSSL_SOFTWARE




#ifndef FULL_INTERNAL_DATA
/*
 * Includes the full database in vlmcsd.
 */

 //#define FULL_INTERNAL_DATA
#endif // FULL_INTERNAL_DATA




/*
 * ----------------------------------------------------------------------------------------
 * Removal of features. Allows you to remove features of vlmcsd you do not need or want.
 * Use it to get smaller binaries. This is especially useful on very small embedded devices.
 * ----------------------------------------------------------------------------------------
 */


#ifndef NO_FREEBIND
 /*
  * Do not compile support for FREEBIND (Linux) and IP_BINDANY (FreeBSD). This disables the -F1 command
  * line option and you can bind only to (listen on) IP addresses that are currently up and running on
  * your system.
  */

//#define NO_FREEBIND

#endif // NO_FREEBIND




#ifndef NO_TAP
 /*
  * Do not compile support for using a VPN adapter under Windows. Disables -O command line option.
  */

//#define NO_TAP

#endif // NO_TAP




#ifndef NO_VERSION_INFORMATION
/*
 * Removes the -V option from vlmcsd and vlmcs that displays the version information
 */

 //#define NO_VERSION_INFORMATION

#endif // NO_VERSION_INFORMATION




#ifndef NO_VERBOSE_LOG
/*
 * Removes the ability to do verbose logging and disables -v and -q in vlmcsd. It does not remove the -v
 * option in the vlmcs client. Disables ini file directive LogVerbose.
 */

 //#define NO_VERBOSE_LOG

#endif // NO_VERBOSE_LOG




#ifndef NO_LOG
/*
 * Disables logging completely. You can neither log to a file nor to the console. -D and -f will
 * start vlmcsd in foreground. -e will not be available. Disables ini file directive LogFile.
 * Implies NO_VERBOSE_LOG.
 */

 //#define NO_LOG

#endif // NO_LOG




#ifndef NO_STRICT_MODES
/*
 * Disables emulator detection protection. Removes -M0, -M1, -E0, -E1, -K0, -K1, -K2 and -K3 from
 * vlmcsd command line options and WhitelistingLevel from INI file parameters. vlmcsd always behaves
 * as if it was started with -K0, -M0.
 */

 //#define NO_STRICT_MODES

#endif // NO_STRICT_MODES





#ifndef NO_CLIENT_LIST
/*
 * Disables the ability to maintain a list of Client Machine IDs (CMIDs). Removes -M0, -M1, -E0 and -E1
 * from vlmcsd command line options.
 */

 //#define NO_CLIENT_LIST

#endif // !NO_CLIENT_LIST



#ifndef NO_RANDOM_EPID
 /*
  * Disables the ability to generate random ePIDs. Useful if you managed to grab ePID/HWID from a
  * real KMS server and want to use these. Removes -r from the vlmcsd command line and the ini
  * file directive RandomizationLevel (The randomization level will be harcoded to 0).
  */

  //#define NO_RANDOM_EPID

#endif // NO_RANDOM_EPID




#ifndef NO_INI_FILE
/*
 * Disables the ability to use a configuration file (aka ini file). Removes -i from the command line.
 */

 //#define NO_INI_FILE

#endif // NO_INI_FILE




#ifndef NO_PID_FILE
 /*
 * Disables the abilty to write a pid file containing the process id of vlmcsd. If your init system
 * does not need this feature, you can safely disables this but it won't save much space. Disables
 * the use of -p from the command line and PidFile from the ini file.
 */

 //#define NO_PID_FILE

#endif // NO_PID_FILE




#ifndef NO_EXTERNAL_DATA
 /*
 * Disables the abilty to load external KMS data from a file. Disables command line options -j
 * and ini file parameter KmsData. Implies UNSAFE_DATA_LOAD.
 */

 //#define NO_EXTERNAL_DATA

#endif // NO_EXTERNAL_DATA




#ifndef NO_INTERNAL_DATA
 /*
 * Compiles vlmcsd and vlmcs without an internal database. If no database is found at
 * either the default location or the file specified with command line option -j.,
 * the program exits with an error message.
 */

 //#define NO_INTERNAL_DATA

#endif // NO_INTERNAL_DATA




#ifndef UNSAFE_DATA_LOAD
 /*
 * Does not check an external KMS data file for integrity.
 * This save some bytes but it dangerous if you load a KMS data file from an unknown source.
 */

 //#define UNSAFE_DATA_LOAD

#endif // UNSAFE_DATA_LOAD




#ifndef NO_USER_SWITCH
/*
 * Disables switching to another uid and/or gid after starting the program and setting up the sockets.
 * You cannot use -u anf -g on the command line as well as User and Group in the ini file. If your init system
 * supports starting daemons as another uid/gid (user/group) you can disable this feature in vlmcsd as long as you
 * do not need to run vlmcsd on a privileged port ( < 1024 on most systems).
 *
 * This setting has no effect on native Windows since -u and -g is not available anyway. It may be used with
 * Cygwin.
 */

 //#define NO_USER_SWITCH

#endif // NO_USER_SWITCH




#ifndef NO_HELP
/*
 * Disables display of help in both vlmcsd and vlmcs. Saves some bytes but only makes sense if you have
 * access to the man files vlmcsd.8 and vlmcs.1
 */

 //#define NO_HELP

#endif // NO_HELP




#ifndef NO_CUSTOM_INTERVALS
/*
 * Disables the ability to specify custom interval for renewing and retrying activation. Newer versions of the Microsoft's
 * KMS activation client (as in Win 8.1) do not honor these parameters anyway. Disable them to save some bytes. Removes
 * -A and -R from the command line as well as ActivationInterval and RenewalInterval in the ini file.
 */

 //#define NO_CUSTOM_INTERVALS

#endif // NO_CUSTOM_INTERVALS




#ifndef NO_PRIVATE_IP_DETECT
/*
 * Disables the ability to protect vlmcsd against KMS requests from public IP addresses.
 * Removes -o from the command line.
 */

 //#define NO_PRIVATE_IP_DETECT

#endif // NO_PRIVATE_IP_DETECT




#ifndef NO_SOCKETS
/*
 * Disables standalone startup of vlmcsd. If you use this config directive, you must start vlmcsd from an internet
 * superserver like inetd, xinetd, systemd or launchd. Disables -m, -t, -4, -6, -e, -f, -P and -L in the vlmcsd
 * command line. Socket setup is the job of your superserver.
 */

 //#define NO_SOCKETS

#endif // NO_SOCKETS




#ifndef NO_CL_PIDS
/*
 * Disables the ability to specify ePIDs and HWID at the command line. You still may use them in the ini file.
 * Removes -0, -3, -w and -H from the vlmcsd command line.
 */

 //#define NO_CL_PIDS

#endif // NO_CL_PIDS




#ifndef NO_LIMIT
/*
 * Disables the ability to limit the number of worker threads or processes that vlmcsd uses. While you should set a
 * limit whenever possible, you may save some bytes by enabling that setting. If you do not use a limit, use vlmcsd
 * in a "friendly" environment only, i.e. do not run it without a reasonable limit on the internet.
 *
 * Removes the ability to use -m in the vlmcsd command line and MaxWorkers in the ini file.
 *
 * Some older unixoid OSses may not have pthreads. Do NOT use USE_THREADS and define NO_SIGHUP
 * and NO_LIMIT instead to disable use of the pthreads, shared memory and semaphores.
 */

 //#define NO_LIMIT

#endif // NO_LIMIT




#ifndef NO_SIGHUP
/*
 * Disables the ability to signal hangup (SIGHUP) to vlmcsd to restart it (rereading the ini file). The SIGHUP
 * handler makes heavy use of OS specific code. It should not cause any trouble on Solaris, Mac OS X and iOS. On Linux
 * use "#define USE_AUXV" (see troubleshooting options) if this is supported by your C runtime library.
 *
 * You may save some bytes by enabling this option. Use it also if the SIGHUP handler causes any trouble on your
 * platform. Please note that with no SIGHUP handler at all. vlmcsd will simply terminate (uncleanly) if it receives
 * the SIGHUP signal. This is the same behavior as with most other signals.
 *
 * This option has no effect on native Windows since Posix signaling is not supported. It can be used with Cygwin.
 */

 //#define NO_SIGHUP

#endif // NO_SIGHUP




#ifndef SIMPLE_RPC
/*
 * Uses a simple version of the RPC protocol which does not support NDR64 and BTFN.
 * Supports RPC with the features present in Windows XP and earlier only. Using this creates
 * smaller binaries but makes emulator detection easier.
 */

 //#define SIMPLE_RPC
#endif // !SIMPLE_RPC




#ifndef SIMPLE_SOCKETS
/*
 * Disables the ability to choose IP addresses using the -L option in vlmcsd. vlmcsd will listen on all IP addresses.
 * It still supports IPv4 and IPv6.
 */

 //#define SIMPLE_SOCKETS

#endif // SIMPLE_SOCKETS



/* Don't change anything BELOW this line */


#endif /* CONFIG_H_ */
