#!/bin/sh
# **************************************************************************

srcdir=$1
srcdirpath=$2

if test x"$srcdirpath" = x""; then
  echo "Usage: $0 basedir sourcefile"
  exit
fi

# **************************************************************************

token=`echo $srcdirpath | sed -e 's/[\/\\]*//g' | sed -e 's/^\.*//g'`

case $token in
*.cpp)
  token=`basename $token .cpp`
  class=`basename $srcdirpath .cpp`
  ;;
*.c)
  token=`basename $token .c`
  class=`basename $srcdirpath .c`
  ;;
*.icc)
  token=`basename $token .icc`
  class=`basename $srcdirpath .icc`
  ;;
*.ic)
  token=`basename $token .ic`
  class=`basename $srcdirpath .ic`
  ;;
esac

case $token in
srcInventorWin* | libInventorWin*)
  token=`echo $token | cut -c15-`
  ;;
srcInventorQt* | libInventorQt*)
  token=`echo $token | cut -c14-`
  ;;
srcInventor* | libInventor*)
  token=`echo $token | cut -c12-`
  ;;
src* | lib*)
  token=`echo $token | cut -c4-`
  ;;
*)
  ;;
esac

exec 5>${token}.cpp

# include the declaration header for the current class
cat $srcdir/$srcdirpath | grep "^#include" | grep -v config.h | head -1 >&5

cat >&5 <<EOF

#include "CoinTest.h"
#include <Inventor/misc/SoRefPtr.h>

#include <cassert>
#include <cstdio>
#include <iostream>

#include <TestSuiteUtils.h>
#include <TestSuiteMisc.h>

EOF

# include all includes inside the TEST_SUITE scope up here
cat $srcdir/$srcdirpath | \
  sed -n -e '/^#if.*COIN_TEST_SUITE/,/^#endif.*COIN_TEST_SUITE/ p' | \
  egrep "^#include" >&5

cat >&5 <<EOF

using namespace SIM::Coin3D::Coin;

BOOST_AUTO_TEST_SUITE(${class}_TestSuite);

EOF

# extract the testsuite parts of the .cpp file, strip out the #ifdef
# wrapper, and insert #line directives to make error messages point to
# the original source file instead of the generated one.

cat $srcdir/$srcdirpath | egrep -n "*" | \
  sed -n -e '/:#if.*COIN_TEST_SUITE/,/:#endif.*COIN_TEST_SUITE/ p' | \
  sed -e 's,\([0-9]*\):#ifdef.*COIN_TEST_SUITE,#line \1 "'$srcdirpath'",' | \
  sed -e 's,\([0-9]*\):#include.*,#line \1 "'$srcdirpath'",' | \
  egrep -v ":#.*COIN_TEST_SUITE" | \
  sed -e '/#line/ a\
\
' | \
  sed -e 's,[0-9]*:,,' >&5

echo "" >&5
echo "BOOST_AUTO_TEST_SUITE_END();" >&5

exec 5>/dev/null
