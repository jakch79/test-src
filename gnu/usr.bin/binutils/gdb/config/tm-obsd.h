/* Target machine sub-description for OpenBSD.
   This is included by other tm-*.h files to specify OpenBSD-specific stuff.
   Copyright 1993, 1994 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef SVR4_SHARED_LIBS

/* Return non-zero if we are in a shared library trampoline code stub. */

#define IN_SOLIB_CALL_TRAMPOLINE(pc, name) \
  (name && !strcmp(name, "_DYNAMIC"))

#endif /* !SVR4_SHARED_LIBS */
