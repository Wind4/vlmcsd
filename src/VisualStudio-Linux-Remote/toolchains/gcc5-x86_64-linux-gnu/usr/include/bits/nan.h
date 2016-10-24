/* `NAN' constant for IEEE 754 machines.
   Copyright (C) 1992-2016 Free Software Foundation, Inc.
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

#ifndef _MATH_H
# error "Never use <bits/nan.h> directly; include <math.h> instead."
#endif


/* IEEE Not A Number.  */

#if __GNUC_PREREQ(3,3)

# define NAN	(__builtin_nanf (""))

#elif defined __GNUC__

# define NAN \
  (__extension__							      \
   ((union { unsigned __l __attribute__ ((__mode__ (__SI__))); float __d; })  \
    { __l: 0x7fc00000UL }).__d)

#else

# include <endian.h>

# if __BYTE_ORDER == __BIG_ENDIAN
#  define __qnan_bytes		{ 0x7f, 0xc0, 0, 0 }
# endif
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define __qnan_bytes		{ 0, 0, 0xc0, 0x7f }
# endif

static union { unsigned char __c[4]; float __d; } __qnan_union
  __attribute__ ((__unused__)) = { __qnan_bytes };
# define NAN	(__qnan_union.__d)

#endif	/* GCC.  */
