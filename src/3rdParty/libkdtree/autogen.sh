#!/bin/sh -ex

#libtoolize --force &> /dev/null
aclocal
autoheader
automake --add-missing --no-force
autoconf
# To be run by the library users
#exec ./configure $@

# COPYRIGHT --
#
# This file is part of libkdtree++, a C++ template KD-Tree sorting container.
# libkdtree++ is (c) 2004-2007 Martin F. Krafft <libkdtree@pobox.madduck.net>
# and distributed under the terms of the Artistic License 2.0.
# See the ./COPYING file in the source tree root for more information.
#
# THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
# OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
