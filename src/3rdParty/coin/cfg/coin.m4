############################################################################
# Usage:
#   SIM_AC_HAVE_COIN_IFELSE( IF-FOUND, IF-NOT-FOUND )
#
# Description:
#   This macro locates the Coin development system.  If it is found,
#   the set of variables listed below are set up as described and made
#   available to the configure script.
#
#   The $sim_ac_coin_desired variable can be set to false externally to
#   make Coin default to be excluded.
#
# Autoconf Variables:
# > $sim_ac_coin_desired     true | false (defaults to true)
# < $sim_ac_coin_avail       true | false
# < $sim_ac_coin_cppflags    (extra flags the preprocessor needs)
# < $sim_ac_coin_cflags      (extra flags the C compiler needs)
# < $sim_ac_coin_cxxflags    (extra flags the C++ compiler needs)
# < $sim_ac_coin_ldflags     (extra flags the linker needs)
# < $sim_ac_coin_libs        (link library flags the linker needs)
# < $sim_ac_coin_datadir     (location of Coin data files)
# < $sim_ac_coin_includedir  (location of Coin headers)
# < $sim_ac_coin_version     (the libCoin version)
# < $sim_ac_coin_msvcrt      (the MSVC++ C library Coin was built with)
# < $sim_ac_coin_configcmd   (the path to coin-config or "false")
#
# Authors:
#   Lars J. Aas, <larsa@sim.no>
#   Morten Eriksen, <mortene@sim.no>
#
# TODO:
#

AC_DEFUN([SIM_AC_HAVE_COIN_IFELSE], [
AC_PREREQ([2.14a])

# official variables
sim_ac_coin_avail=false
sim_ac_coin_cppflags=
sim_ac_coin_cflags=
sim_ac_coin_cxxflags=
sim_ac_coin_ldflags=
sim_ac_coin_libs=
sim_ac_coin_datadir=
sim_ac_coin_includedir=
sim_ac_coin_version=

# internal variables
: ${sim_ac_coin_desired=true}
sim_ac_coin_extrapath=

AC_ARG_WITH([coin],
AC_HELP_STRING([--with-coin], [enable use of Coin [[default=yes]]])
AC_HELP_STRING([--with-coin=DIR], [give prefix location of Coin]),
  [ case $withval in
    no)  sim_ac_coin_desired=false ;;
    yes) sim_ac_coin_desired=true ;;
    *)   sim_ac_coin_desired=true
         sim_ac_coin_extrapath=$withval ;;
    esac],
  [])

case $build in
*-mks ) sim_ac_pathsep=";" ;;
* )     sim_ac_pathsep="${PATH_SEPARATOR}" ;;
esac

if $sim_ac_coin_desired; then
  sim_ac_path=$PATH
  test -z "$sim_ac_coin_extrapath" || ## search in --with-coin path
    sim_ac_path="$sim_ac_coin_extrapath/bin${sim_ac_pathsep}$sim_ac_path"
  test x"$prefix" = xNONE ||          ## search in --prefix path
    sim_ac_path="$sim_ac_path${sim_ac_pathsep}$prefix/bin"

  AC_PATH_PROG(sim_ac_coin_configcmd, coin-config, false, $sim_ac_path)

  if test "X$sim_ac_coin_configcmd" != "Xfalse"; then
    test -n "$CONFIG" &&
      $sim_ac_coin_configcmd --alternate=$CONFIG >/dev/null 2>/dev/null &&
      sim_ac_coin_configcmd="$sim_ac_coin_configcmd --alternate=$CONFIG"
  fi

  if $sim_ac_coin_configcmd; then
    sim_ac_coin_version=`$sim_ac_coin_configcmd --version`
    sim_ac_coin_cppflags=`$sim_ac_coin_configcmd --cppflags`
    sim_ac_coin_cflags=`$sim_ac_coin_configcmd --cflags 2>/dev/null`
    sim_ac_coin_cxxflags=`$sim_ac_coin_configcmd --cxxflags`
    sim_ac_coin_ldflags=`$sim_ac_coin_configcmd --ldflags`
    sim_ac_coin_libs=`$sim_ac_coin_configcmd --libs`
    sim_ac_coin_datadir=`$sim_ac_coin_configcmd --datadir`
    # Hide stderr on the following, as ``--includedir'', ``--msvcrt''
    # and ``--cflags'' options were added late to coin-config.
    sim_ac_coin_includedir=`$sim_ac_coin_configcmd --includedir 2>/dev/null`
    sim_ac_coin_msvcrt=`$sim_ac_coin_configcmd --msvcrt 2>/dev/null`
    sim_ac_coin_cflags=`$sim_ac_coin_configcmd --cflags 2>/dev/null`
    AC_CACHE_CHECK(
      [if we can compile and link with the Coin library],
      sim_cv_coin_avail,
      [sim_ac_save_cppflags=$CPPFLAGS
      sim_ac_save_cxxflags=$CXXFLAGS
      sim_ac_save_ldflags=$LDFLAGS
      sim_ac_save_libs=$LIBS
      CPPFLAGS="$CPPFLAGS $sim_ac_coin_cppflags"
      CXXFLAGS="$CXXFLAGS $sim_ac_coin_cxxflags"
      LDFLAGS="$LDFLAGS $sim_ac_coin_ldflags"
      LIBS="$sim_ac_coin_libs $LIBS"
      AC_LANG_PUSH(C++)

      AC_TRY_LINK(
        [#include <Inventor/SoDB.h>],
        [SoDB::init();],
        [sim_cv_coin_avail=true],
        [sim_cv_coin_avail=false])

      AC_LANG_POP
      CPPFLAGS=$sim_ac_save_cppflags
      CXXFLAGS=$sim_ac_save_cxxflags
      LDFLAGS=$sim_ac_save_ldflags
      LIBS=$sim_ac_save_libs
    ])
    sim_ac_coin_avail=$sim_cv_coin_avail

    if $sim_ac_coin_avail; then :; else
      AC_MSG_WARN([
Compilation and/or linking with the Coin main library SDK failed, for
unknown reason. If you are familiar with configure-based configuration
and building, investigate the 'config.log' file for clues.

If you can not figure out what went wrong, please forward the 'config.log'
file to the email address <coin-support@coin3d.org> and ask for help by
describing the situation where this failed.
])
    fi
  else # no 'coin-config' found

# FIXME: test for Coin without coin-config script here
    if test x"$COINDIR" != x""; then
      sim_ac_coindir=`cygpath -u "$COINDIR" 2>/dev/null || echo "$COINDIR"`
      if test -d $sim_ac_coindir/bin && test -d $sim_ac_coindir/lib && test -d $sim_ac_coindir/include/Inventor; then
        # using newest version (last alphabetically) in case of multiple libs
        sim_ac_coin_lib_file=`echo $sim_ac_coindir/lib/coin*.lib | sed -e 's,.* ,,g'`
        if test -f $sim_ac_coin_lib_file; then
          sim_ac_coin_lib_name=`echo $sim_ac_coin_lib_file | sed -e 's,.*/,,g' -e 's,.lib,,'`
          sim_ac_save_cppflags=$CPPFLAGS
          sim_ac_save_libs=$LIBS
          sim_ac_save_ldflags=$LDFLAGS
          CPPFLAGS="$CPPFLAGS -I$sim_ac_coindir/include"
          if test -f $sim_ac_coindir/bin/$sim_ac_coin_lib_name.dll; then
            CPPFLAGS="$CPPFLAGS -DCOIN_DLL"
          fi
          LDFLAGS="$LDFLAGS -L$sim_ac_coindir/lib"
          LIBS="-l$sim_ac_coin_lib_name -lopengl32 $LIBS"

          AC_LANG_PUSH(C++)

          AC_TRY_LINK(
            [#include <Inventor/SoDB.h>],
            [SoDB::init();],
            [sim_cv_coin_avail=true],
            [sim_cv_coin_avail=false])

          AC_LANG_POP
          CPPFLAGS=$sim_ac_save_cppflags
          LDFLAGS=$sim_ac_save_ldflags
          LIBS=$sim_ac_save_libs
          sim_ac_coin_avail=$sim_cv_coin_avail
        fi
      fi
    fi

    if $sim_ac_coin_avail; then
      sim_ac_coin_cppflags=-I$sim_ac_coindir/include
      if test -f $sim_ac_coindir/bin/$sim_ac_coin_lib_name.dll; then
        sim_ac_coin_cppflags="$sim_ac_coin_cppflags -DCOIN_DLL"
      fi
      sim_ac_coin_ldflags=-L$sim_ac_coindir/lib
      sim_ac_coin_libs="-l$sim_ac_coin_lib_name -lopengl32"
      sim_ac_coin_datadir=$sim_ac_coindir/data
    else
      locations=`IFS="${sim_ac_pathsep}"; for p in $sim_ac_path; do echo " -> $p/coin-config"; done`
      AC_MSG_WARN([cannot find 'coin-config' at any of these locations:
$locations])
      AC_MSG_WARN([
Need to be able to run 'coin-config' to figure out how to build and link
against the Coin library. To rectify this problem, you most likely need
to a) install Coin if it has not been installed, b) add the Coin install
bin/ directory to your PATH environment variable.
])
    fi
  fi
fi

if $sim_ac_coin_avail; then
  ifelse([$1], , :, [$1])
else
  ifelse([$2], , :, [$2])
fi
]) # SIM_AC_HAVE_COIN_IFELSE()

