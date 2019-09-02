#!/bin/sh
#
# file: autogen.sh 
# Build configure script using the GNU automake/autoconf tools.
#
# Copyright (C) 2004 Michael Hubbard <mhubbard@digital-fallout.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#

sh tools/getrev.sh > VERSION

if [ "$1" == "clean" ]; then
	rm -f ./aclocal.m4
	rm -f ./configure
	rm -f ./config.*
	rm -f ./missing
	rm -f ./install-sh
	rm -f ./mkinstalldirs
	rm -f ./depcomp

	rm -f ./src/*.in
	rm -f ./src/Makefile

	rm -fr ./autom4te.cache

	rm -f stamp-h1

	rm -f COPYING Makefile Makefile.in
	
	exit
fi

aclocal
autoheader
automake --gnu -ac
autoconf

