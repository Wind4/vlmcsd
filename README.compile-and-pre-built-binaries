Compilation and pre-built binaries FAQ
======================================

What is the best pre-built binary for my system or device?
----------------------------------------------------------

None. The best binary is compiled by yourself using a toolchain that is
optimized for your system or device in every respect.


How do I compile my own binary?
-------------------------------

On a full blown desktop system this is relativly easy. If not already done so,
install a C compiler (e.g. gcc or clang) through your packet manager, e.g.
"sudo apt-get install gcc" (Debian/Ubuntu) or "sudo yum install gcc"
(RedHat/Fedora).

Then cd to your vlmcsd directory and type "make". vlmcs and vlmcsd will
be built right away for your local system.

If you installed gcc and it is not the default compiler for your OS or
distribution, you may need to type "make CC=gcc" to explicitly select a
specific C compiller.


How do I compile a binary for my embedded device?
-------------------------------------------------

What you need is cross-compiling toolchain for your device. It consists of a
C compiler, libraries, header files and some tools (called binutils). The
toolchain must match the device in processor architecture, endianess, ABI,
library and header files version, library configuration, ...

If the endianess or ABI differs or the version of some library between
toolchain and device differs too much, the resulting binary does not run
on your device.

Once you have a proper toolchain (probably found on the Internet for download),
unpack it to any directory and type

     "make CC=/path/to/toolchain/bindir/c-compiler-binary"

Building vlmcsd for using a cross-compiling toolchain is as easy as building
vlmcsd for your local machine. The only question is, whether this you have
a toolchain that actually matches your device.

Whenever you change any parameter of the make command line, you must "clean"
the source directory from intermediate files and output from previous runs
of make. You can do so by typeing "make clean" or force make to behave as if
the directory were clean by adding -B to the command line, e.g.

     "make -B CC=/path/to/toolchain/bindir/c-compiler-binary"


I have downloaded several promising toolchains for my device but they all
don't work. Can I create my own toolchain?
-------------------------------------------------------------------------

You can use tools like buildroot or OpenWRT. Both are able to create toolchains
for many embedded devices. But this is out of the scope of this document.
If you are unable to walk through thousands of configuration options and make
the right choice, you may probably want to try the pre-built binaries.


How to choose a pre-built binary?
---------------------------------

The directory structure for the binaries is

binaries
+
+--<operating system>
   +
   +--<cpu arch>
      +
      +--<endianess> (omitted if CPU or OS does not allow multi-endianess)
         +
         +--<C-library>

<C-library> can also be "static". That means no special library is required.
Static binaries are much bigger and need more RAM than dynamic binaries but
are more likely to run on your system. Use a static binary only, if none of
the dynmic binaries run.

Don't get confused when a binary is named after an OS or a specific device,
e.g. the name contains "openwrt", "tomato" or "Fritzbox". This does not mean
that the binary will run only on that OS or on that device. It is a hint only
where I got or built the toolchain from.


How to determine the endianess of my system?
--------------------------------------------

- All Intel CPUs (x86, x32, x64) are little-endian only
- Windows is little-endian only even if the CPU support big-endian
- big-endian ARM is extremely uncommon. You can safely assume little-endian
- little-endian PowerPC virtually does not exist since only newer POWER7
  and POWER8 CPUs support it. Always assume big-endian.
- For MIPS both little-endian and big-endian are common. Most Broadcomm and
  TI chips run little-endian. Most Atheros and Ikanos CPUs run big-endian.

Try typing
     echo -n I | od -o | awk 'FNR==1{ print substr($2,6,1)}'

This returns 1 for little-endian systems and 0 for big-endian systems. However
some devices do not have the od command and thus this method won't work.



