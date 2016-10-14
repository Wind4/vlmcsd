/* Copyright (C) 1994-2016 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#if !defined _STDIO_H && !defined __need_FOPEN_MAX && !defined __need_IOV_MAX
# error "Never include <bits/stdio_lim.h> directly; use <stdio.h> instead."
#endif

#ifdef _STDIO_H
# define L_tmpnam 20
# define TMP_MAX 238328
# define FILENAME_MAX 4096

# ifdef __USE_POSIX
#  define L_ctermid 9
#  if !defined __USE_XOPEN2K || defined __USE_GNU
#   define L_cuserid 9
#  endif
# endif
#endif

#if defined __need_FOPEN_MAX || defined _STDIO_H
# undef  FOPEN_MAX
# define FOPEN_MAX 16
#endif

#if defined __need_IOV_MAX && !defined IOV_MAX
# define IOV_MAX 1024
#endif
