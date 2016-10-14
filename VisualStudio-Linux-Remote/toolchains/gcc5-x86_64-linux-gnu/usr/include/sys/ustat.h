/* Header describing obsolete `ustat' interface.
   Copyright (C) 1996-2016 Free Software Foundation, Inc.
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

/*
 * This interface is obsolete.  Use <sys/statfs.h> instead.
 */

#ifndef _SYS_USTAT_H
#define	_SYS_USTAT_H	1

#include <features.h>

#include <sys/types.h>
#include <bits/ustat.h>

__BEGIN_DECLS

extern int ustat (__dev_t __dev, struct ustat *__ubuf) __THROW;

__END_DECLS

#endif /* sys/ustat.h */
