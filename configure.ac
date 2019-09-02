dnl
dnl file: configure.ac
dnl Build configure for the GNU automake/autoconf tools.
dnl
dnl Copyright (C) 2004 Michael Hubbard <mhubbard@digital-fallout.com>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
dnl

dnl read version from VERSION and remove trailing shell newline
define([GRIST_VERSION],patsubst(esyscmd(cat VERSION),[\([0-9.]*\(-devel\)?\)\(\w\|\W\)*],[\1]))dnl version

dnl general requirements
dnl 

AC_PREREQ(2.59)
#AC_INIT([grist], patsubst(esyscmd([cat VERSION]),[$],[]), [bugs@digital-fallout.com])
AC_INIT([grist], GRIST_VERSION,[bugs@digital-fallout.com])

# Configure options
AC_ARG_ENABLE([memwatch],
	      AC_HELP_STRING([--enable-memwatch],
	      		     [include memwatch analysis routines]),
	      [with_memwatch=yes])

AC_ARG_ENABLE([debug],
	      AC_HELP_STRING([--enable-debug],
	      		     [build with debug code enabled]),
	      [with_debug=yes])


# Stuff
AM_CONDITIONAL([COND_MEMWATCH], [test "$with_memwatch" = yes])

if test "x$with_debug" = "xyes"; then
	CFLAGS="-D_DEBUG ${CFLAGS}"
fi

if test "x$with_memwatch" = "xyes"; then
	CFLAGS="-DMEMWATCH ${CFLAGS}"
fi

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR([src/main.c])
AM_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# libdbi check
#
AC_CHECK_LIB([dbi], [dbi_initialize], [], 
	AC_MSG_ERROR([libdbi not found. 

Please make sure that you have libdbi properly installed. If you have installed
libdbi to a non-standard include directory you can alter the CFLAGS and LDFLAGS
variables passed to configure as follows:

./configure CFLAGS=-I/<path-to-libdbi>/include LDFLAGS=-L/<path-to-libdbi>/lib 
	
For more information about libdbi visit: http://libdbi.sourceforge.net.
]))
#
# End libdbi check

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stddef.h stdlib.h stdarg.h string.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit bzero memset strdup])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
