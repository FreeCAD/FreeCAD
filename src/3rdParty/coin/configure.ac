# **************************************************************************

# FIXME: try to fix all problems with spaces in directory names;
# whether it's the src dir, the install/build dir or directories with
# dependent libraries. Alternatively, we should at least detect and
# warn/exit. Ditto goes for all our other software modules aswell, of
# course. 20020519 mortene.

# FIXME: need to figure out when libstdc++ is necessary to link. GCC
# 3.2 on Solaris seems to demand it, or we'll get unresolved symbols.
# 20021128 mortene.

# **************************************************************************

# release version number info
m4_define([COIN_MAJOR], [4])
m4_define([COIN_MINOR], [0])
m4_define([COIN_MICRO], [7])
m4_define([COIN_BETA], [])

# This is probably a more correct setup, but will be harder to keep if
# we want major and minor to grow slowly (but is that any point?)
# 20010807 larsa
m4_define([COIN_ABI_CURRENT], [m4_eval((COIN_MAJOR*20)+COIN_MINOR)])
m4_define([COIN_ABI_REVISION], [COIN_MICRO])
m4_define([COIN_ABI_AGE], [COIN_MINOR])

# only used on Linux for debian packages
m4_define([SO_NUMBER], [m4_eval(COIN_MAJOR*20)])

# For Mac OS X Compiler Frameworks:
m4_define([MAC_FRAMEWORK_NAME_DEFAULT], [Inventor])
m4_define([MAC_FRAMEWORK_PREFIX_DEFAULT], [/Library/Frameworks])
m4_define([MAC_FRAMEWORK_VERSION_DEFAULT], [D])

# **************************************************************************

AC_PREREQ(2.50)

AC_INIT([Coin], [COIN_MAJOR.COIN_MINOR.COIN_MICRO[]COIN_BETA],
  [coin-support@coin3d.org])

AC_CONFIG_AUX_DIR(cfg)
AC_CONFIG_SRCDIR(src/misc/SoDB.cpp)

# FIXME: detect and exit upon this problem (as seen reported on
# coin-bugs): the install prefix is set to the same directory as the
# build directory (or also as the srccode dir). 20020129 mortene.


AH_TOP([
#ifndef COIN_INTERNAL
#error this is a private header file
#endif

#include <unconfig.h>
#include <setup.h>
])

AC_CANONICAL_SYSTEM

# For include/Inventor/system/inttypes.h.
AC_DEFINE_UNQUOTED(COIN_CONFIGURE_BUILD, SIM_AS_TR_CPP([$build]), [Build system.])
AC_DEFINE_UNQUOTED(COIN_CONFIGURE_HOST, SIM_AS_TR_CPP([$host]), [Build host system.])
AC_DEFINE_UNQUOTED(COIN_CONFIGURE_TARGET, SIM_AS_TR_CPP([$target]), [Build target system.])

# date used by debian setup
SIM_AC_DATE_RFC822([RFC822_DATE])
AC_SUBST([RFC822_DATE])

# date used by redhat setup
SIM_AC_ISO8601_DATE([coin_configure_date])
AC_SUBST([coin_configure_date])

SIM_AC_MSVC_DSP_ENABLE_OPTION

# If the Microsoft Visual C++ cl.exe compiler is available, set us up for
# compiling with it and to generate an MSWindows .dll file.

SIM_AC_SETUP_MSVCPP_IFELSE

# **************************************************************************
# Mac OS X: gcc

# old-compat
AC_ARG_WITH([framework],
  [  --without-framework     (deprecated: use --disable-framework)],
  [case "$withval" in
  no | false ) enable_framework=false ;;
  * ) ;;
  esac])

sim_ac_framework_name="MAC_FRAMEWORK_NAME_DEFAULT"
AC_ARG_ENABLE([framework],
  [AC_HELP_STRING([--disable-framework], 
                  [Do 'UNIX-style' installation on Mac OS X])],
  [case $enableval in
  no | false ) sim_ac_framework=false ;;
  * ) sim_ac_framework=true ;;
  esac],
  [case $host_os in
  darwin* ) sim_ac_framework=true ;;
  * ) sim_ac_framework=false ;;
  esac])

AC_ARG_WITH([framework-prefix],
  [  --with-framework-prefix=<path>  (default: "MAC_FRAMEWORK_PREFIX_DEFAULT")],
  [sim_ac_framework_prefix="$withval"
  if test x"$sim_ac_framework_prefix" = x""; then
    sim_ac_framework_prefix=\${prefix}
  fi],
  [sim_ac_framework_prefix="MAC_FRAMEWORK_PREFIX_DEFAULT"])

sim_ac_framework_version="MAC_FRAMEWORK_VERSION_DEFAULT"

AC_SUBST([MAC_FRAMEWORK], [$sim_ac_framework])
AC_SUBST([MAC_FRAMEWORK_NAME], [$sim_ac_framework_name])
AC_SUBST([MAC_FRAMEWORK_PREFIX], [$sim_ac_framework_prefix])
AC_SUBST([MAC_FRAMEWORK_VERSION], [$sim_ac_framework_version])

AC_SUBST([nop], [])

frameworkdir=
if $sim_ac_framework; then
  frameworkdir="${sim_ac_framework_prefix}/${sim_ac_framework_name}.framework"
  includedir="$frameworkdir/Resources/include"
fi

ivincludedir=$includedir

AC_SUBST([ivincludedir])
AC_SUBST([frameworkdir])

# **************************************************************************

ac_build_year=$(date +%Y)


AC_SUBST([COIN_BUILD_YEAR], $ac_build_year)
AC_SUBST([COIN_MAJOR_VERSION], [COIN_MAJOR])
AC_SUBST([COIN_MINOR_VERSION], [COIN_MINOR])
AC_SUBST([COIN_MICRO_VERSION], [COIN_MICRO])
AC_SUBST([COIN_BETA_VERSION], [COIN_BETA])
AC_SUBST([COIN_VERSION], [$COIN_MAJOR_VERSION.$COIN_MINOR_VERSION.$COIN_MICRO_VERSION$COIN_BETA_VERSION])
VERSION=$COIN_VERSION

SIM_AC_CONFIGURATION_SETTING([Library version], [$COIN_VERSION])

# Libtool versioning
AC_SUBST([LT_CURRENT], [COIN_ABI_CURRENT])
AC_SUBST([LT_REVISION], [COIN_ABI_REVISION])
AC_SUBST([LT_AGE], [COIN_ABI_AGE])

AC_SUBST([COIN_SO_VERSION], [SO_NUMBER])

#  These are for include/Inventor/C/basic.h.

AC_DEFINE_UNQUOTED([COIN_MAJOR_VERSION], [$COIN_MAJOR_VERSION],
  [define this to the libCoin major version number])
AC_DEFINE_UNQUOTED([COIN_MINOR_VERSION], [$COIN_MINOR_VERSION],
  [define this to the libCoin minor version number])
AC_DEFINE_UNQUOTED([COIN_MICRO_VERSION], [$COIN_MICRO_VERSION],
  [define this to the libCoin release version number])
AC_DEFINE_UNQUOTED([COIN_VERSION], ["$COIN_VERSION"],
  [define this to the full libCoin major.minor.micro version number])
if test x"$COIN_BETA_VERSION" = x""; then :; else
  AC_DEFINE_UNQUOTED([COIN_BETA_VERSION], [$COIN_BETA_VERSION],
    [define this to the libCoin beta version letter])
fi

# *******************************************************************
# Miscellaneous options and initializations.

coin_build_dir=`pwd`
coin_src_dir=`cd "$srcdir"; pwd`
coin_w32_build_dir=`cygpath -w "$coin_build_dir" 2>/dev/null || echo "$coin_build_dir"`
coin_w32_src_dir=`cygpath -w "$coin_src_dir" 2>/dev/null || echo "$coin_src_dir"`

SIM_AC_RELATIVE_SRC_DIR

# *******************************************************************
# set up configure options for "make distcheck" mode - we want to
# ensure that all options that can be turned on works...

AC_MSG_CHECKING([for distcheck mode])
case "$prefix:$coin_build_dir" in
*/=inst:*/=build )
  enable_threads=yes
  enable_threadsafe=yes
  enable_html=yes
  enable_man=yes
  enable_3ds_import=yes
  enable_debug=no
  AC_MSG_RESULT([yes])
  ;;
* )
  AC_MSG_RESULT([no])
  ;;
esac

# **************************************************************************
#  Locate C++ compiler and set C++ as the default language to use
#  in tests. The configure script will automatically terminate if
#  it doesn't find a C++ compiler.
AC_PROG_CC

# AC_OBJEXT
# AC_EXEEXT

# **************************************************************************
# NOTE: if we touch CFLAGS/CXXFLAGS before AC_PROG_CC is invoked, the -g
# flag will be lost  20020104 larsa

if $BUILD_WITH_MSVC; then
  SIM_AC_SETUP_MSVCRT([
    CPPFLAGS="$CPPFLAGS $sim_ac_msvcrt_CPPFLAGS"
    CFLAGS="$CFLAGS $sim_ac_msvcrt_CFLAGS"
    CXXFLAGS="$CXXFLAGS $sim_ac_msvcrt_CXXFLAGS"
    LDFLAGS="$LDFLAGS $sim_ac_msvcrt_LDFLAGS"
    LIBS="$LIBS $sim_ac_msvcrt_LIBS"
    # sim_ac_msvcrt_LIBLIBS is set up at the end
    # sim_ac_msvcrt_LIBLDFLAGS is set up at the end
  ])
fi

AC_SUBST([COIN_MSVC_LIBC], [$sim_ac_msvcrt])
AC_SUBST([BUILD_WITH_MSVC])

# **************************************************************************
# Universal Binary support (Mac OS X)

SIM_AC_UNIVERSAL_BINARIES

# **************************************************************************

AM_INIT_AUTOMAKE(Coin, $VERSION)

#  Default to not building a static library.
#  Can be overridden by the user with --enable-static.
AM_DISABLE_STATIC

#  Turn off default maintainer make-rules -- use ./bootstrap instead.
AM_MAINTAINER_MODE

if $BUILD_WITH_MSVC; then
  RANLIB=:
  export RANLIB
fi

# **************************************************************************
#  Locate C++ compiler and set C++ as the default language to use
#  in tests. The configure script will automatically terminate if
#  it doesn't find a C++ compiler.
#
#  Note that due to bugs in the libtool.m4 set of checks (of libtool v1.4.2)
#  that makes AC_PROG_LIBTOOL dependent on the current compiler being a C
#  compiler, we don't activate the C++ compiler until _after_ running
#  AC_PROG_LIBTOOL.
# 
#  (The specific checks we know not to work are the checks for "-c -o"
#  compatibility, where only CFLAGS is set up (and not CXXFLAGS) before the
#  check is run).

AC_PROG_CXX
AC_PROG_CXXCPP

# Since the plan is to not compile pure C anymore, we can simplify by always
# setting C++ mode in Coin's configure script.
AC_LANG([C++])

if $BUILD_WITH_MSVC; then
  SIM_AC_MSC_VERSION

  case "$sim_ac_msc_major_version" in
  8 | 9) CPPFLAGS="$CPPFLAGS -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS" ;;
  *)
  esac
fi

SIM_AC_STRIP_EXIT_DECLARATION

# **************************************************************************

SIM_AC_MACOS10_DEPLOYMENT_TARGET

if test x"$sim_ac_macos10_deployment_target_major_version" = x"10"; then
  # AC_DEFINE() COIN_MACOS_10_# as needed here...
  sim_ac_macosx=true
  sim_ac_macosx_10_2ff=false
  sim_ac_macosx_10_3ff=false
  AC_DEFINE([COIN_MACOS_10], 1, [Define for Mac OS X builds])
  if test 1 -le $sim_ac_macos10_deployment_target_minor_version; then
    AC_DEFINE([COIN_MACOS_10_1], 1, [Define for Mac OS 10.1 builds])
    sim_ac_macosx_10_2ff=false
    sim_ac_macosx_10_3ff=false
  fi
  if test 2 -le $sim_ac_macos10_deployment_target_minor_version; then
    AC_DEFINE([COIN_MACOS_10_2], 1, [Define for Mac OS 10.2 builds])
    sim_ac_macosx_10_2ff=true
    sim_ac_macosx_10_3ff=false
    sim_ac_macosx_name="Jaguar"
  fi
  if test 3 -le $sim_ac_macos10_deployment_target_minor_version; then
    #AC_DEFINE([COIN_MACOS_10_3], 1, [Define for Mac OS 10.3 builds])
    sim_ac_macosx_10_2ff=true
    sim_ac_macosx_10_3ff=true
    sim_ac_macosx_name="Panther"
  fi
  if test 4 -le $sim_ac_macos10_deployment_target_minor_version; then
    #AC_DEFINE([COIN_MACOS_10_4], 1, [Define for Mac OS 10.4 builds])
    sim_ac_macosx_10_2ff=true
    sim_ac_macosx_10_3ff=true
    sim_ac_macosx_name="Tiger"
  fi
  if test 5 -le $sim_ac_macos10_deployment_target_minor_version; then
    #AC_DEFINE([COIN_MACOS_10_5], 1, [Define for Mac OS 10.5 builds])
    sim_ac_macosx_10_2ff=true
    sim_ac_macosx_10_3ff=true
    sim_ac_macosx_name="Leopard"
  fi
else
  sim_ac_macosx=false
  sim_ac_macosx_10_2ff=false
  sim_ac_macosx_10_3ff=false
  sim_ac_macosx_name=""
fi

AM_CONDITIONAL([MAC_FRAMEWORK], [$sim_ac_framework])
if $sim_ac_framework; then
  AC_DEFINE([COIN_MACOSX_FRAMEWORK], 1, [Define when building Mac OS X framework])
fi

AM_CONDITIONAL([MACOSX_10_2FF], [$sim_ac_macosx_10_2ff])
AM_CONDITIONAL([MACOSX_10_3FF], [$sim_ac_macosx_10_3ff])
AM_CONDITIONAL([MACOSX], [$sim_ac_macosx])
AC_SUBST([MACOSX_10_2FF], [$sim_ac_macosx_10_2ff])
AC_SUBST([MACOSX_NAME], [$sim_ac_macosx_name])

# **************************************************************************
#  Initialize libtool
#
# FIXME: this must be done even when we're not using libtool (as when
# building with MSVC++), because that's where at least the enable_static
# and enable_shared flags are set up. 20030324 mortene.
AC_PROG_LIBTOOL

# **************************************************************************
#  These are used for constructing the coin-config file.
AC_SUBST([COIN_EXTRA_CPPFLAGS], [$CPPFLAGS])
AC_SUBST([COIN_EXTRA_CFLAGS], [$CFLAGS])
AC_SUBST([COIN_EXTRA_CXXFLAGS], [$CXXFLAGS])
AC_SUBST([COIN_EXTRA_LDFLAGS], [$LDFLAGS])
AC_SUBST([COIN_EXTRA_LIBS], [$LIBS])

# *******************************************************************
# Compiler checking.

## Make it possible for the user to turn off optimization flags for
## the compiler before running the fatal error checks below.
SIM_AC_COMPILER_OPTIMIZATION

## Smoke out some known defunct compilers.
SIM_AC_COMPILER_CPLUSPLUS_FATAL_ERRORS

## See which way the compiler knows the name of the current function. ####
SIM_AC_CHECK_VAR_FUNCTIONNAME

SIM_AC_MAC_CPP_ADJUSTMENTS

if test x"$prefix" = x"NONE"; then
  prefix=/usr/local
fi
#
# *******************************************************************
# Enable/disable separate Coin modules

sim_ac_enable_java_wrapper=false
AC_ARG_ENABLE(
  [java-wrapper],
  AC_HELP_STRING([--enable-java-wrapper], [enable building java wrapper]), [
  case $enableval in
    yes | true) sim_ac_enable_java_wrapper=true ;;
    no | false) sim_ac_enable_java_wrapper=false ;;
    *) ;;
  esac])

AM_CONDITIONAL([BUILD_JAVA_WRAPPER], [$sim_ac_enable_java_wrapper])

if $sim_ac_enable_java_wrapper; then
  AC_DEFINE([HAVE_JAVA_WRAPPER], 1, [Define this if you build the Java wrapper])
  SIM_AC_CONFIGURATION_SETTING([Generate Java wrapper], [Yes (not really)])
else
  SIM_AC_CONFIGURATION_SETTING([Generate Java wrapper], [No (default)])
fi

# *******************************************************************
# See if we should build with 3ds import capabilities.

sim_ac_enable_3ds_import=false
AC_ARG_ENABLE(
  [3ds-import],
  AC_HELP_STRING([--enable-3ds-import], [enable 3ds import capabilities]), [
  case $enableval in
    yes | true) sim_ac_enable_3ds_import=true ;;
    no | false) sim_ac_enable_3ds_import=false ;;
    *) ;;
  esac])

HAVE_3DS_IMPORT_CAPABILITIES=1

if $sim_ac_enable_3ds_import ; then
  AC_DEFINE([HAVE_3DS_IMPORT_CAPABILITIES],, [for setup.h])
  SIM_AC_CONFIGURATION_SETTING([3ds import capabilities], [Yes (warning: experimental)])
else
  HAVE_3DS_IMPORT_CAPABILITIES=0
  SIM_AC_CONFIGURATION_SETTING([3ds import capabilities], [No  (enable with --enable-3ds-import)])
fi
AC_SUBST(HAVE_3DS_IMPORT_CAPABILITIES)

AM_CONDITIONAL([BUILD_3DS_CAPABILITIES], [$sim_ac_enable_3ds_import])


# *******************************************************************
# Enable/disable system expat

sim_ac_enable_system_expat=false
AC_ARG_ENABLE(
  [system-expat],
  AC_HELP_STRING([--enable-system-expat], [enable use of system-expat]), [
  case $enableval in
    yes | true) sim_ac_enable_system_expat=true ;;
    no | false) sim_ac_enable_system_expat=false ;;
    *) ;;
  esac])

AM_CONDITIONAL([USE_SYSTEM_EXPAT], [$sim_ac_enable_system_expat])

if $sim_ac_enable_system_expat; then
  sim_ac_save_LIBS=$LIBS
  LIBS="$LIBS -lexpat"
  AC_MSG_CHECKING([for system installation of expat])
  AC_TRY_LINK(
   [#include <expat.h>],
   [(void)XML_ParserCreate(0);],
   [AC_DEFINE([HAVE_SYSTEM_EXPAT], 1, [Define this if you want to use the system installation of Expat])
    COIN_EXTRA_LIBS="$COIN_EXTRA_LIBS -lexpat"
    AC_MSG_RESULT([available])],
   [AC_MSG_ERROR([expat library not found. (If you do not want to enable using the system installation of Expat library in Coin, do not specify the "--enable-system-expat" option to the configure script.)])
    LIBS=$sim_ac_save_LIBS])

  SIM_AC_CONFIGURATION_SETTING([Use system expat], [Yes])
else
  SIM_AC_CONFIGURATION_SETTING([Use system expat], [No (default)])
fi

# *******************************************************************
# See if we should build with javascript capabilities.

sim_ac_enable_javascript=true
AC_ARG_ENABLE(
  [javascript-api],
  AC_HELP_STRING([--disable-javascript-api], [disable javascript capabilities]), [
  case $enableval in
    yes | true) sim_ac_enable_javascript=true ;;
    no | false) sim_ac_enable_javascript=false ;;
    *) ;;
  esac])

HAVE_JAVASCRIPT=1

if $sim_ac_enable_javascript ; then
  AC_DEFINE([COIN_HAVE_JAVASCRIPT],, [for setup.h])
  SIM_AC_CONFIGURATION_SETTING([javascript capabilities], [Yes])
else
  HAVE_JAVASCRIPT=0
  SIM_AC_CONFIGURATION_SETTING([javascript capabilities], [No (enable with --enable-javascript-api)])
fi
AC_SUBST(HAVE_JAVASCRIPT)

# *******************************************************************
# Query enabling/disabling various subsystems

sim_ac_enable_nodekits=true

AC_ARG_ENABLE(
  [nodekits],
  AC_HELP_STRING([--disable-nodekits], [disable nodekit support]), [
  case $enableval in
    yes | true) sim_ac_enable_nodekits=true ;;
    no | false) sim_ac_enable_nodekits=false ;;
    *) ;;
  esac])

sim_ac_enable_draggers=true

AC_ARG_ENABLE(
  [draggers],
  AC_HELP_STRING([--disable-draggers], [disable dragger support]), [
  case $enableval in
    yes | true) sim_ac_enable_draggers=true ;;
    no | false) sim_ac_enable_draggers=false ;;
    *) ;;
  esac])

sim_ac_enable_manipulators=true

AC_ARG_ENABLE(
  [manipulators],
  AC_HELP_STRING([--disable-manipulators], [disable manipulator support]), [
  case $enableval in
    yes | true) sim_ac_enable_manipulators=true ;;
    no | false) sim_ac_enable_manipulators=false ;;
    *) ;;
  esac])

HAVE_NODEKITS=1
HAVE_DRAGGERS=1
HAVE_MANIPULATORS=1

if $sim_ac_enable_nodekits; then
  AC_DEFINE([HAVE_NODEKITS],, [for setup.h])
  SIM_AC_CONFIGURATION_SETTING([nodekit support], [Yes])
else
  HAVE_NODEKITS=0
  HAVE_DRAGGERS=0
  HAVE_MANIPULATORS=0
  SIM_AC_CONFIGURATION_SETTING([nodekit support], [No  (this severely cripples Coin)])
fi

if $sim_ac_enable_draggers && test x"$HAVE_NODEKITS" = x"1"; then
  AC_DEFINE([HAVE_DRAGGERS],, [for setup.h])
  SIM_AC_CONFIGURATION_SETTING([dragger support], [Yes])
else
  HAVE_DRAGGERS=0
  HAVE_MANIPULATORS=0
  SIM_AC_CONFIGURATION_SETTING([dragger support], [No  (this severely cripples Coin)])
fi

if $sim_ac_enable_manipulators && test x"$HAVE_DRAGGERS" = x"1"; then
  AC_DEFINE([HAVE_MANIPULATORS],, [for setup.h])
  SIM_AC_CONFIGURATION_SETTING([manipulator support], [Yes])
else
  HAVE_MANIPULATORS=0
  SIM_AC_CONFIGURATION_SETTING([manipulator support], [No  (this severely cripples Coin)])
fi

AC_SUBST(HAVE_NODEKITS)
AC_SUBST(HAVE_DRAGGERS)
AC_SUBST(HAVE_MANIPULATORS)

# *******************************************************************

sim_ac_enable_vrml97=true

AC_ARG_ENABLE(
  [vrml97],
  AC_HELP_STRING([--disable-vrml97], [disable VRML97 support]), [
  case $enableval in
    yes | true) sim_ac_enable_vrml97=true ;;
    no | false) sim_ac_enable_vrml97=false ;;
    *) ;;
  esac])

HAVE_VRML97=1

if $sim_ac_enable_vrml97 ; then
  AC_DEFINE([HAVE_VRML97],, [for setup.h])
  SIM_AC_CONFIGURATION_SETTING([VRML97 support], [Yes])
else
  HAVE_VRML97=0
  SIM_AC_CONFIGURATION_SETTING([VRML97 support], [No  (enable with --enable-vrml97)])
fi
AC_SUBST(HAVE_VRML97)

# *******************************************************************
# Search for the "local GLU" extension

enable_superglu=false

if test -x ${srcdir}/src/extensions/superglu/configure; then
  enable_superglu=true
  AC_DEFINE(HAVE_SUPERGLU,1, [*EXPERIMENTAL* to use the GLU we've "appropriated" from SGI])
  HAVE_SUPERGLU=1
  AC_CONFIG_SUBDIRS([src/extensions/superglu])
  CPPFLAGS="-I$coin_build_dir/src/extensions/superglu/include -I$coin_src_dir/src/extensions/superglu/include $CPPFLAGS"
else
  enable_superglu=false
  HAVE_SUPERGLU=0
fi
AC_SUBST([HAVE_SUPERGLU])

AC_SUBST([SUPERGLUPREFIX], [Super])
export SUPERGLUPREFIX

AM_CONDITIONAL(BUILD_WITH_SUPERGLU, $enable_superglu)

# *******************************************************************

AC_ARG_ENABLE([threads],
  [AC_HELP_STRING([--disable-threads], [disable platform-independent multithreading abstraction classes])],
  [case $enableval in
  yes | true ) sim_ac_enable_threads=true ;;
  no | false ) sim_ac_enable_threads=false ;;
  *) AC_MSG_ERROR([invalid arg "$enableval" for --enable-threads option]) ;;
  esac],
  [sim_ac_enable_threads=true])

HAVE_THREADS=0

if $sim_ac_enable_threads; then

  sim_ac_threads_api="none"

  # Make it possible to explicitly turn off Win32 threads, to for instance
  # use POSIX threads instead under Win32.
  AC_ARG_ENABLE([w32threads],
    [AC_HELP_STRING([--disable-w32threads], [never use Win32 threads, even when available])],
    [case $enableval in
    yes | true ) sim_ac_w32_enable_threads=true ;;
    no | false ) sim_ac_w32_enable_threads=false ;;
    *) AC_MSG_ERROR([invalid arg "$enableval" for --disable-w32threads option]) ;;
    esac],
    [sim_ac_w32_enable_threads=true])

  sim_ac_win32_threads_available=false

  # Check for platform-native Win32 thread API first.
  if $sim_ac_w32_enable_threads; then
    AC_MSG_CHECKING([for Win32 threads])
    AC_TRY_LINK(
     [#include <windows.h>],
     [HANDLE h = CreateThread(NULL, 0, NULL, NULL, 0, NULL);
      (void)SetThreadPriority(h, 0);
      ExitThread(0);],
     [sim_ac_win32_threads_available=true
      sim_ac_threads_api="native Win32"
      AC_DEFINE([USE_W32THREAD], , [define to use the Win32 threads API])
      AC_DEFINE([COIN_THREADID_TYPE], DWORD, [System dependent thread ID type])])
      # (we just ignore failure, as we fall through to POSIX threads)
    AC_MSG_RESULT($sim_ac_win32_threads_available)
  fi

  if ! $sim_ac_win32_threads_available; then
    SIM_AC_CHECK_PTHREAD([
      AC_DEFINE([USE_PTHREAD], , [define to use the POSIX threads API])
      AC_DEFINE([COIN_THREADID_TYPE], pthread_t, [System dependent thread ID type])
      sim_ac_threads_api="POSIX"
      COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_pthread_cppflags"
      COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_pthread_ldflags"
      COIN_EXTRA_LIBS="$sim_ac_pthread_libs $COIN_EXTRA_LIBS"
    ])
  fi

  if test "$sim_ac_threads_api" = "none"; then
    AC_MSG_ERROR([Could not find any usable native thread-handling API/library/devkit!  (If you do not want to enable the platform-independent thread-handling classes in Coin, specify the "--disable-threads" option to the configure script.)])
  fi

  HAVE_THREADS=1
  AC_DEFINE([HAVE_THREADS],, [to use the platform-independent thread-handling abstractions])
else
  sim_ac_threads_api="Disabled  (enable with --enable-threads)"
fi

AC_SUBST([HAVE_THREADS])
SIM_AC_CONFIGURATION_SETTING([System threads API], [$sim_ac_threads_api])

AM_CONDITIONAL([BUILD_WITH_THREADS], [$sim_ac_enable_threads])

# *******************************************************************
# Thread safe library

AC_ARG_ENABLE([threadsafe],
  AC_HELP_STRING([--enable-threadsafe], [enable thread safe traversals]),
  [case $enableval in
  yes | true ) sim_ac_enable_threadsafe=true ;;
  no | false ) sim_ac_enable_threadsafe=false ;;
  *) AC_MSG_ERROR([invalid arg "$enableval" for --enable-threadsafe option]) ;;
  esac],
  [sim_ac_enable_threadsafe=false])

HAVE_THREADSAFE=0

if $sim_ac_enable_threadsafe; then
  if test "$HAVE_THREADS" = "0"; then
    AC_MSG_ERROR([You can not enable thread safe action traversals while at the same time disabling the portable thread-handling abstractions. You must use "--enable-threadsafe" with "--enable-threads".])
  fi

  HAVE_THREADSAFE=1
  AC_DEFINE([COIN_THREADSAFE], 1, [for setup.h])
  SIM_AC_CONFIGURATION_SETTING([Thread safe traversals], [Yes  (in development)])
else
  SIM_AC_CONFIGURATION_SETTING([Thread safe traversals], [No  (enable with --enable-threadsafe)])
fi

# for coin.cfg
AC_SUBST([HAVE_THREADSAFE])

# *******************************************************************
# Headers we might want to use

# We expect these to include their dependencies.
#
# A non-empty last argument indicates to Autoconf that we would like to heed
# the result from compilation, not just pre-processing. (A space is enough
# to indicate non-emptiness.)
AC_CHECK_HEADERS(
  [unistd.h sys/types.h inttypes.h stdint.h sys/param.h sys/time.h sys/timeb.h time.h io.h windows.h libgen.h direct.h strings.h ieeefp.h],
  [], [], [])

AC_MSG_CHECKING([for flex file adjustments])
if test x"$ac_cv_header_unistd_h" != x"yes"; then
  AC_MSG_RESULT([-DYY_NO_UNISTD_H])
  CPPFLAGS="$CPPFLAGS -DYY_NO_UNISTD_H"
else
  AC_MSG_RESULT([none])
fi

SIM_AC_CHECK_HEADER_TLHELP32_H
SIM_AC_CHECK_FUNC__SPLITPATH

SIM_AC_CHECK_WIN32_API

# Various systems (like MKS NuTCracker) needs explicit gdi32 linkage.
if $sim_ac_have_win32_api; then
  LIBS="$LIBS -lgdi32"
  COIN_EXTRA_LIBS="$COIN_EXTRA_LIBS -lgdi32"
fi

# A non-empty last argument indicates to Autoconf that we would like to heed
# the result from compilation, not just pre-processing. (A space is enough
# to indicate non-emptiness.)
AC_CHECK_HEADERS(
  [sys/unistd.h],
  [sim_ac_have_sys_unistd_h=true],
  [sim_ac_have_sys_unistd_h=false],
  [ ])

if $sim_ac_have_sys_unistd_h; then
  AC_MSG_CHECKING([for IN_PATH define conflict])
  AC_TRY_COMPILE(
   [#include <sys/unistd.h>],
   [void * v; v = (void*) IN_PATH;],
   [sim_ac_in_path_defined=true],
   [sim_ac_in_path_defined=false])
  if $sim_ac_in_path_defined; then
    AC_MSG_RESULT([yes])
    AC_DEFINE([COIN_UNDEF_IN_PATH_HACK],1,
     [define to "remove" conflicting IN_PATH define from <sys/unistd.h>])
  else
    AC_MSG_RESULT([no])
  fi
fi
    
# According to Coin user Ralf Corsepius, at least SunOS4 needs
# to include sys/types.h before netinet/in.h. There have also
# been a problem report for FreeBSD which seems to indicate
# the same dependency on that platform aswell.
#
# FIXME: should we make this into a general macro
# SIM_AC_CHECK_DEPENDENT_HEADER(HEADER, DEPENDONHEADERS..)? 20001025 mortene.
#
# FIXME2: is this really necessary? AC _CHECK_HEADER(S) tests by using the
# preprocessor, not by doing a compile, so we could probably just
# include netinet/in.h in the above list to AC _CHECK_HEADERS. We wouldn't
# catch problems quiet so early on, though. 20001025 mortene.

AC_CACHE_CHECK([for netinet/in.h], sim_cv_netinet_in_h, [
  AC_TRY_CPP([
    #ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
    #endif /* HAVE_SYS_TYPES_H */
    #include <netinet/in.h>],
    [sim_cv_netinet_in_h=true], [sim_cv_netinet_in_h=false])])

if $sim_cv_netinet_in_h; then
  AC_DEFINE_UNQUOTED([HAVE_NETINET_IN_H], [1],
    [Define if you have the <netinet/in.h> header file.])
fi

# For Inventor/system/inttypes.h.in.
#
# Note: this assumes AC_CHECK_HEADERS(sys/types.h stdint.h) has been done.
#
# FIXME: the header file checking should be AC_REQUIREd, if possible.
# 20010711 mortene.

SIM_AC_DEFINE_BYTESIZE_TYPES

# *******************************************************************

AC_MSG_CHECKING([for fstat() function])
AC_TRY_LINK(
 [#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif],
 [struct stat sb;
  int result = fstat(0, &sb);],
 [AC_DEFINE(HAVE_FSTAT, 1, [define if fstat() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])])

# *******************************************************************
# We want to use BSD 4.3's isinf(), isnan(), finite() if they are
# available.

AC_MSG_CHECKING([for BSD 4.3 isinf() function])
AC_TRY_LINK(
 [#include <math.h>],
 [int result = isinf(42.0);],
 [AC_DEFINE(HAVE_ISINF, 1, [define if isinf() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])])

AC_MSG_CHECKING([for BSD 4.3 isnan() function])
AC_TRY_LINK(
 [#include <math.h>],
 [int result = isnan(42.0);],
 [AC_DEFINE(HAVE_ISNAN, 1, [whether or not isnan() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])

  AC_MSG_CHECKING([for _isnan() function])
  AC_TRY_LINK(
   [#include <math.h>],
   [int result = _isnan(42.0);],
   [AC_DEFINE(HAVE__ISNAN, 1, [whether or not _isnan() is available])
    AC_MSG_RESULT([available])],
   [AC_MSG_RESULT([not available])])
])

AC_MSG_CHECKING([for BSD 4.3 finite() function])
AC_TRY_LINK(
 [#include <math.h>],
 [int result = finite(42.0);],
 [AC_DEFINE(HAVE_FINITE, 1, [whether or not finite() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])

  AC_MSG_CHECKING([for _finite() function])
  AC_TRY_LINK(
   [#include <math.h>],
   [int result = _finite(42.0);],
   [AC_DEFINE(HAVE__FINITE, 1, [whether or not _finite() is available])
    AC_MSG_RESULT([available])],
   [AC_MSG_RESULT([not available])])
])

AC_MSG_CHECKING([for fpclass() function])
AC_TRY_LINK(
[#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif],
 [int result = fpclass(42.0);],
 [AC_DEFINE(HAVE_FPCLASS, 1, [whether or not fpclass() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])])

AC_MSG_CHECKING([for _fpclass() function])
AC_TRY_LINK(
 [#include <math.h>],
 [int result = _fpclass(42.0);],
 [AC_DEFINE(HAVE__FPCLASS, 1, [whether or not _fpclass() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])])

SIM_AC_CHECK_TYPEOF_STRUCT_MEMBER([
#include <sys/types.h>
#ifdef HAVE_TIME_H
#include <time.h> // struct timeval (Linux)
#endif // HAVE_TIME_H
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> // struct timeval (IRIX)
#endif // HAVE_SYS_TIME_H
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H
], [timeval], [tv_sec], [SIM_TIMEVAL_TV_SEC_T], [])

SIM_AC_CHECK_TYPEOF_STRUCT_MEMBER([
#include <sys/types.h>
#ifdef HAVE_TIME_H
#include <time.h> // struct timeval (Linux)
#endif // HAVE_TIME_H
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> // struct timeval (IRIX)
#endif // HAVE_SYS_TIME_H
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H
], [timeval], [tv_usec], [SIM_TIMEVAL_TV_USEC_T], [])

# *******************************************************************

sim_ac_build_library=true
AC_ARG_ENABLE(
  [build],
  [AC_HELP_STRING([--disable-build], [disable configuration for library build])],
  [case $enableval in
  no | false)
   sim_ac_build_library=false
   SIM_AC_CONFIGURATION_SETTING([Coin build type], [build disabled])
   ;;
  esac])

AM_CONDITIONAL(BUILD_LIBRARY, $sim_ac_build_library)

# *******************************************************************
# * Do we want to build the documentation?
# *******************************************************************

AC_ARG_ENABLE(html,
  AC_HELP_STRING([--enable-html], [build and install Coin HTML documentation]),
  [case $enableval in
    yes | true) want_html=yes ;;
    *)          want_html=no ;;
  esac],
  [want_html=no])

case $htmldir in
/*)
  # do nothing - absolute path
  ;;
*)
  # expand $docdir and append /html
  htmldir=`eval echo ${docdir}`/html
  ;;
esac

AC_SUBST(htmldir)

AC_ARG_ENABLE(man,
  AC_HELP_STRING([--enable-man], [build and install Coin man pages]),
  [case $enableval in
    yes | true) want_man=yes ;;
    *)          want_man=no ;;
  esac],
  [want_man=no])

# Used in the Doxygen parameter file.
AC_SUBST([COIN_DOC_HTML], [`echo $want_html | tr '[a-z]' '[A-Z]'`])
AC_SUBST([coin_html_dir], [`pwd`/html])
AC_SUBST([COIN_DOC_MAN], [`echo $want_man | tr '[a-z]' '[A-Z]'`])
AC_SUBST([coin_man_dir], [`pwd`/man])

AM_CONDITIONAL(BUILD_MANPAGES, test x"$want_man" = x"yes")
AM_CONDITIONAL(BUILD_HTMLPAGES, test x"$want_html" = x"yes")

if test x"$want_man" = x"yes"; then
  SIM_AC_CONFIGURATION_SETTING([manpage installation], [$mandir])
fi

if test x"$want_html" = x"yes"; then
  SIM_AC_CONFIGURATION_SETTING([HTML installation], [$htmldir])
fi

# Perl is used for the Doxygen-based doc generation, plus when making
# NSIS installation packages.
AC_PATH_PROG(sim_ac_perl_exe, perl, false, $PATH)

if test x"$want_html" != xno -o x"$want_man" != xno; then
  SIM_AC_DOXYGEN_TOOL([], [SIM_AC_ERROR([no-doxygen])])
  if test x"$sim_ac_perl_exe" = xfalse; then
    AC_MSG_ERROR([Could not find the Perl executable, which is needed for Doxygen doc generation.])
  fi
fi

# path_tag is used to identify paths in docs/coin.doxygen that needs to be
# transformed using cygpath under cygwin.

case $host in
*-cygwin) path_tag="<PATH>" ;;
*)        path_tag= ;;
esac

AC_SUBST(path_tag)

#Get all the doxygen source files
. ${coin_src_dir}/docs/coin_doxygenfiles.inc

#The AC_SUBST command will not accept multiline strings, so trick this
#into a singleline string
DOXYGEN_INPUT=$(echo ${DOXYGEN_INPUT})

AC_SUBST(DOXYGEN_INPUT)

sim_ac_makensis_exe=false
case $host in
*-cygwin)
  AC_PATH_PROG([sim_ac_makensis_exe], [makensis], [false], [$PATH])
  ;;
esac

AC_PATH_PROG([sim_ac_dpkg_buildpackage_exe],
  [dpkg-buildpackage], [false], [$PATH])

sim_ac_windows_packagable=true
if test x"$sim_ac_perl_exe" = xfalse || test x"$sim_ac_makensis_exe" = xfalse; then
  sim_ac_windows_packageable=false
fi

AM_CONDITIONAL([WIN_PACKAGEABLE], $sim_ac_windows_packageable)

sim_ac_debian_packageable=true
if test x"$sim_ac_dpkg_buildpackage_exe" = xfalse; then
  sim_ac_debian_packageable=false
fi

AM_CONDITIONAL([DEB_PACKAGEABLE], [$sim_ac_debian_packageable])

SIM_AC_PACKAGEMAKER_APP
AC_SUBST([sim_ac_packagemaker_app])
sim_ac_macosx_packageable=true
if test x"$sim_ac_packagemaker_app" = xfalse; then
  sim_ac_macosx_packageable=false
fi

AM_CONDITIONAL([MACOSX_PACKAGEABLE], [$sim_ac_macosx_packageable])

# *******************************************************************
# Configure an alternate installation?

AC_ARG_WITH(
  [alternate],
  [AC_HELP_STRING([--with-alternate=string],
                  [specify name of alternate configuration])],
  [CONFIG=$withval],
  [CONFIG=default])

AC_ARG_WITH(
  [suffix],
  [AC_HELP_STRING([--with-suffix=string],
                  [specify library name suffix (e.g. "_g")])],
  [SUFFIX=$withval],
  [SUFFIX=])

AC_SUBST(CONFIG)
AC_SUBST(SUFFIX)

# *******************************************************************
# * Check compiler features, bugs, etc.
# *******************************************************************

# FIXME: the extra strict warning settings for GCC below should be
# placed in a macro, so they can be easily used from other src-code
# modules aswell. 20021218 mortene.

SIM_AC_COMPILE_DEBUG([
  CPPFLAGS="$CPPFLAGS -DCOIN_DEBUG=1"
], [
  CPPFLAGS="$CPPFLAGS -DCOIN_DEBUG=0"
])

SIM_AC_DEBUGSYMBOLS
SIM_AC_RTTI_SUPPORT
SIM_AC_EXCEPTION_HANDLING
SIM_AC_PROFILING_SUPPORT
SIM_AC_COMPILER_WARNINGS
SIM_AC_DETECT_COMMON_COMPILER_FLAGS

# Disable bool type with MSVC.
#
# "/noBool" should not be part of the SIM_AC_DETECT_COMMON_COMPILER_FLAGS
# macro, because some projects will not compile properly with it.
# (Like at least SoQt, since the Qt headers defines the bool type, which
# will then cause a compiler error when q[whatever].h is included.)

# The setting of /noBool for MSVC has been disabled because of problems with
# using <limits> with Visual C++ 6.  2007-10-03 larsa
# SIM_AC_COMPILER_NOBOOL([
#   sim_ac_compiler_CXXFLAGS="$sim_ac_compiler_CXXFLAGS $sim_ac_nobool_CXXFLAGS"
# ])

#  Yep, this is important in case we want to add code to make
#  the API interface "Open Inventor strict", meaning the user must
#  use the exact same include files as under "real" Open Inventor.
#  We would then need to know that we are actually allowed to include
#  any file during internal build.
#
#  The COIN_INTERNAL define is also in use to detect
#  "inefficient including" internally.

CPPFLAGS="$CPPFLAGS -DCOIN_INTERNAL"

if $BUILD_WITH_MSVC; then
  case ${enable_static-no} in
  yes | true)  COIN_STATIC=true;
               enable_shared=no ;; # --enable-static-problem with autoconf 2.58
  *)           COIN_STATIC=false ;;
  esac
  case ${enable_shared-default} in
  default)     ;;
  yes | true)  COIN_STATIC=false ;;
  *)           COIN_STATIC=true ;;
  esac
fi

# **************************************************************************
# Check for various system specifics.

# Try to find snprintf() and vsnprintf() on the system.
SIM_AC_CHECK_NPRINTF

# C99 va_copy() available?
AC_MSG_CHECKING([for va_copy() stdarg macro])
AC_TRY_LINK([#include <stdarg.h>
             static void hepp(int dummy, ...) {
             va_list l, c;
             va_start(l, dummy); va_copy(c, l); va_end(c); va_end(l); }
            ],
            [hepp(0, 1, 2); ],
  [AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_VA_COPY_MACRO], 1, [Define to use va_copy() when available])],
  [AC_MSG_RESULT([no])])

# How to string quote.
SIM_AC_CHECK_MACRO_QUOTE([], [SIM_AC_ERROR([no-cpp-quoting])])
if test x"$sim_ac_quote_hash" = xyes; then
  AC_DEFINE([HAVE_HASH_QUOTING],1,
    [define if preprocessor can quote arguments with the hash symbol])
else
  if test x"$sim_ac_quote_apostrophes" = xyes; then
    AC_DEFINE([HAVE_APOSTROPHES_QUOTING],1,
      [define if preprocessor can quote arguments with apostophes])
  fi
fi

# *******************************************************************
# Instead of getenv() on most Win32 platforms (not Windows CE).

AC_MSG_CHECKING([for GetEnvironmentVariable() function])
AC_TRY_LINK([#include <windows.h>],
            [(void)GetEnvironmentVariable("FOOBAR", NULL, 0);],
  [AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_GETENVIRONMENTVARIABLE],1, [Define to use GetEnvironmentVariable() instead of getenv()])],
  [AC_MSG_RESULT([no])])

# *******************************************************************
#

sim_ac_save_LIBS=$LIBS
LIBS="$LIBS -lwinmm"
AC_MSG_CHECKING([for timeGetTime() function])
AC_TRY_LINK([
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
], [
  (void)timeGetTime();
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_TIMEGETTIME],, [Define to use timeGetTime()])
  COIN_EXTRA_LIBS="$COIN_EXTRA_LIBS -lwinmm"
], [
  AC_MSG_RESULT([no])
  LIBS=$sim_ac_save_LIBS
])

# *******************************************************************
# The MessageBox() Win32 API call is used in SoDB.cpp to throw up
# an error message when multiple Coin-instances are detected in the
# same process image.

sim_ac_save_LIBS=$LIBS
LIBS="$LIBS -luser32"
AC_MSG_CHECKING([for MessageBox() function])
AC_TRY_LINK([
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
], [
  (void)MessageBox(NULL, NULL, NULL, 0);
], [
  AC_MSG_RESULT([yes])
  COIN_EXTRA_LIBS="$COIN_EXTRA_LIBS -luser32"
], [
  AC_MSG_RESULT([no])
  LIBS=$sim_ac_save_LIBS
])

# *******************************************************************
# Test for misc functions to get systemtime with.

AC_MSG_CHECKING([for QueryPerformanceCounter() function])
AC_TRY_LINK([
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
], [
  LARGE_INTEGER l;
  (void)QueryPerformanceFrequency(&l);
  (void)QueryPerformanceCounter(&l);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_QUERYPERFORMANCECOUNTER],1, [Define to use Win32 QueryPerformanceCounter()])
], [
  AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for _ftime() function])
AC_TRY_LINK([
#include <windows.h>
#include <sys/types.h>
#include <sys/timeb.h>
], [
  struct _timeb timebuffer;
  _ftime(&timebuffer);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE__FTIME],1, [Define to use Win32 _ftime()])
], [
  AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for ftime() function])
AC_TRY_LINK([
#include <sys/timeb.h>
], [
  struct timeb timebuffer;
  ftime(&timebuffer);
], [
  # ftime() is not available in libSystem on Mac OS X < 10.4,
  # so don't use it when building the binary SDK.
  case ${MACOSX_DEPLOYMENT_TARGET} in
    10.[[0123]])
      AC_MSG_RESULT([yes (unused)])
      ;;
    *)
      AC_MSG_RESULT([yes])
      AC_DEFINE([HAVE_FTIME],1, [Define to use ftime()])
      ;;
  esac
], [
  AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for _getcwd() function])
AC_TRY_LINK([
#include <direct.h>
], [
  char buf[512];
  _getcwd(buf, 512);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE__GETCWD],1, [Define to use Win32 _getcwd()])
], [
  AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for getcwd() function])
AC_TRY_LINK([
#include <unistd.h>
], [
  char buf[512];
  getcwd(buf, 512);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_GETCWD],1, [Define to use getcwd()])
], [
  AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for gettimeofday() function])
AC_TRY_LINK([
#ifdef HAVE_TIME_H
#include <time.h>
#endif /* HAVE_TIME_H */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SYS_UNISTD_H
#include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */
], [
  struct timeval tmp;
  (void)gettimeofday(&tmp, NULL);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_GETTIMEOFDAY],1, [Define to use SVr4 / BSD4.3 gettimeofday()])
], [
  AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for localtime_s() function])
AC_TRY_COMPILE(
 [#include <time.h>], [
  time_t t;
  struct tm buf;
  (void) localtime_s(&buf, &t);
 ], [AC_DEFINE(HAVE_LOCALTIME_S, 1, [define if localtime_s() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])])

AC_MSG_CHECKING([for localtime_r() function])
AC_TRY_COMPILE(
 [#include <time.h>], [
  time_t t;
  struct tm buf;
  (void) localtime_r(&t, &buf);
], [AC_DEFINE(HAVE_LOCALTIME_R, 1, [define if localtime_r() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])])

# *******************************************************************
# Test for misc functions, typically not part of neither ISO C or POSIX.

AC_MSG_CHECKING([for strncasecmp() function])
AC_TRY_LINK([
#include <string.h>
], [
  (void)strncasecmp(NULL, NULL, 0);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_STRNCASECMP],1, [Define to use BSD4.3 strncasecmp()])
], [
  AC_MSG_RESULT([no])
])

AC_MSG_CHECKING([for memmove() function])
AC_TRY_COMPILE(
 [#include <string.h>],
 [(void) memmove(0, 0, 0);],
 [AC_DEFINE(HAVE_MEMMOVE, 1, [define if memmove() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])])

AC_MSG_CHECKING([for bcopy() function])
AC_TRY_COMPILE(
 [#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif],
 [(void) bcopy(0, 0, 0);],
 [AC_DEFINE(HAVE_BCOPY, 1, [define if bcopy() is available])
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])])

AC_MSG_CHECKING([for _logb() function])
AC_TRY_LINK([
#include <math.h>
#include <float.h>
], [
  (void)_logb(1.0);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE__LOGB],1, [Define to use Win32 _logb()])
], [
  AC_MSG_RESULT([no])
])


# *******************************************************************
# * Test to see if dynamic linking through either the (Linux/IRIX/...)
# * dl library or the Win32 LoadLibrary() call is available.
# *******************************************************************

coin_can_do_runtime_linking=false

# On Mac OS X < 10.4, dlopen() is missing and has to be emulated; from
# 10.4 on, we can use dlopen() directly. Note that the latter does not 
# obsolete the dyld check, since we also use other dyld functionility.
SIM_AC_CHECK_DYLD([
  AC_DEFINE_UNQUOTED([HAVE_DYLD_RUNTIME_BINDING],1,
                     [define if the Mac OS X dyld can be used for runtime binding])
    coin_can_do_runtime_linking=true
])

case ${MACOSX_DEPLOYMENT_TARGET} in
  10.[[0123]])
    # Do *not* pick up dlopen() support when building on 10.4 for earlier
    # Mac OS X versions (< 10.4 did not support dlopen(), so we have to use 
    # our own implementation based on built-in dyld functions there).
    ;;
  *)
    SIM_AC_CHECK_DL([
      AC_DEFINE_UNQUOTED([HAVE_DL_LIB],1, [define if you have the dl library])
      COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_dl_cppflags"
      COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_dl_ldflags"
      COIN_EXTRA_LIBS="$sim_ac_dl_libs $COIN_EXTRA_LIBS"
      coin_can_do_runtime_linking=true
    ])
    ;;
esac

if ! $coin_can_do_runtime_linking; then
  SIM_AC_CHECK_LOADLIBRARY([
    AC_DEFINE_UNQUOTED([HAVE_WINDLL_RUNTIME_BINDING],1,
                       [define if the Win32 LoadLibrary method is available])
    coin_can_do_runtime_linking=true
  ])
fi

if ! $coin_can_do_runtime_linking; then
  SIM_AC_CHECK_DLD([
    AC_DEFINE_UNQUOTED([HAVE_DLD_LIB],1,
                       [define if the DLD shared library loader is available])
    COIN_EXTRA_LIBS="$sim_ac_dld_libs $COIN_EXTRA_LIBS"
    coin_can_do_runtime_linking=true
  ])
fi

if $coin_can_do_runtime_linking; then
  AC_DEFINE_UNQUOTED([HAVE_DYNAMIC_LINKING], 1,
                     [should be defined if there is some way of doing dynamic linking])
fi

# *******************************************************************
# * Set us up for using the simage library (if wanted, and if
# * available).
# *******************************************************************

AC_ARG_ENABLE([dl-simage],
  AC_HELP_STRING([--enable-dl-simage],
                 [use runtime dynamic linking for libsimage if possible [[default=yes]]]),
  [case "${enableval}" in
    yes | true) enable_dl_simage=true ;;
    no | false) enable_dl_simage=false ;;
    *) SIM_AC_ENABLE_ERROR([--enable-dl-simage]) ;;
  esac],
  [enable_dl_simage=true])

if $enable_dl_simage && $coin_can_do_runtime_linking; then
  AC_DEFINE_UNQUOTED([SIMAGE_RUNTIME_LINKING],1,
    [define for runtime linking with simage])
  SIM_AC_CONFIGURATION_SETTING([simage linkage], [run-time binding])
else
  SIM_AC_HAVE_SIMAGE_IFELSE([
    AC_DEFINE([HAVE_LIBSIMAGE],1,
      [define if you have the simage library])
    CPPFLAGS="$CPPFLAGS $sim_ac_simage_cppflags"
    LDFLAGS="$LDFLAGS $sim_ac_simage_ldflags"
    LIBS="$sim_ac_simage_libs $LIBS"
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_simage_cppflags"
    COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_simage_ldflags"
    COIN_EXTRA_LIBS="$sim_ac_simage_libs $COIN_EXTRA_LIBS"
    SIM_AC_CONFIGURATION_SETTING([simage linkage], [link-time binding])
  ], [
    if $sim_ac_simage_desired; then
      AC_MSG_WARN(simage development system not found);
    fi
    SIM_AC_CONFIGURATION_SETTING([simage linkage], [no simage support])
  ])
fi

# Check if sound is enabled

sim_ac_enable_sound=true

AC_ARG_ENABLE(
  [sound],
  AC_HELP_STRING([--disable-sound], [disable sound support]), [
  case $enableval in
    no | false) sim_ac_enable_sound=false ;;
    *) ;;
  esac])

#  Check for openal (required for sound)

AC_ARG_ENABLE([dl-openal],
  AC_HELP_STRING([--enable-dl-openal],
                 [use runtime dynamic linking for OpenAL if possible [[default=yes]]]),
  [case "${enableval}" in
    yes | true) enable_dl_openal=true ;;
    no | false) enable_dl_openal=false ;;
    *) SIM_AC_ENABLE_ERROR([--enable-dl-openal]) ;;
  esac],
  [enable_dl_openal=true])

if $sim_ac_enable_sound && $enable_dl_openal && $coin_can_do_runtime_linking; then
  AC_DEFINE_UNQUOTED([OPENAL_RUNTIME_LINKING],1,
    [define for runtime linking with OpenAL])
  SIM_AC_CONFIGURATION_SETTING([OpenAL support], [Yes, run-time binding])
else
SIM_AC_HAVE_OPENAL_IFELSE([
  CPPFLAGS="$CPPFLAGS $sim_ac_openal_cppflags"
  LDFLAGS="$LDFLAGS $sim_ac_openal_ldflags"
  LIBS="$LIBS $sim_ac_openal_libs"
  COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_openal_cppflags"
  COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_openal_ldflags"
  COIN_EXTRA_LIBS="$COIN_EXTRA_LIBS $sim_ac_openal_libs"
  AC_DEFINE(HAVE_OPENAL, 1, [openal extensions available])
  SIM_AC_CONFIGURATION_SETTING([OpenAL support], [Yes, link-time binding])
], [
  if $sim_ac_enable_sound; then
    AC_MSG_WARN([openal is needed for sound])
    sim_ac_enable_sound=false
  fi
  SIM_AC_CONFIGURATION_SETTING([OpenAL support], [No])
])
fi

HAVE_SOUND=1

if $sim_ac_enable_sound ; then
  AC_DEFINE([HAVE_SOUND],, [for setup.h])
  SIM_AC_CONFIGURATION_SETTING([Sound support], [Yes])
else
  HAVE_SOUND=0
  SIM_AC_CONFIGURATION_SETTING([Sound support], [No  (enable with --enable-sound)])
fi
AC_SUBST(HAVE_SOUND)

AM_CONDITIONAL([BUILD_WITH_SOUND], [$sim_ac_enable_sound])

# ********************************************************************
# * Set us up for using the Fontconfig font library (if wanted, and if
# * available).
# ********************************************************************

AC_ARG_ENABLE([dl-fontconfig],
  AC_HELP_STRING([--disable-dl-fontconfig],
                 [disable runtime dynamic linking for fontconfig if possible [[default=--enable-dl-fontconfig]]]),
  [case "${enableval}" in
    yes | true) enable_dl_fontconfig=true ;;
    no | false) enable_dl_fontconfig=false ;;
    *) SIM_AC_ENABLE_ERROR([--enable-dl-fontconfig]) ;;
  esac],
  [enable_dl_fontconfig=true])

sim_ac_have_fontconfig=false
if $enable_dl_fontconfig && $coin_can_do_runtime_linking; then
  AC_DEFINE_UNQUOTED([FONTCONFIG_RUNTIME_LINKING], 1,
    [define for runtime linking with fontconfig])
  SIM_AC_CONFIGURATION_SETTING([Fontconfig support], [Yes, run-time binding])
else
  SIM_AC_HAVE_FONTCONFIG_IFELSE([
    AC_DEFINE(HAVE_FONTCONFIG, 1,
      [define if you have the fontconfig library])
    CPPFLAGS="$CPPFLAGS $sim_ac_fontconfig_cppflags"
    LDFLAGS="$LDFLAGS $sim_ac_fontconfig_ldflags"
    LIBS="$sim_ac_fontconfig_libs $LIBS"
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_fontconfig_cppflags"
    COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_fontconfig_ldflags"
    COIN_EXTRA_LIBS="$sim_ac_fontconfig_libs $COIN_EXTRA_LIBS"
    SIM_AC_CONFIGURATION_SETTING([Fontconfig support], [Yes, link-time binding])
  ], [
    if $sim_ac_fontconfig_desired; then
      SIM_AC_CONFIGURATION_SETTING([Fontconfig support], [No])
    fi
  ])
fi

AM_CONDITIONAL(HAVE_FONTCONFIG, $sim_ac_have_fontconfig)

# ********************************************************************
# * Set us up for using the SpiderMonkey javascript library (if wanted,
# * and if available).
# ********************************************************************

AC_ARG_ENABLE([dl-spidermonkey],
  AC_HELP_STRING([--disable-dl-spidermonkey],
                 [disable runtime dynamic linking for SpiderMonkey if possible [[default=--enable-dl-spidermonkey]]]),
  [case "${enableval}" in
    yes | true) enable_dl_spidermonkey=true ;;
    no | false) enable_dl_spidermonkey=false ;;
    *) SIM_AC_ENABLE_ERROR([--enable-dl-spidermonkey]) ;;
  esac],
  [enable_dl_spidermonkey=true])

sim_ac_have_spidermonkey=false
if $enable_dl_spidermonkey && $coin_can_do_runtime_linking; then
  AC_DEFINE_UNQUOTED([SPIDERMONKEY_RUNTIME_LINKING], 1,
    [define for runtime linking with SpiderMonkey])
  if $sim_ac_enable_javascript ; then     
    SIM_AC_CONFIGURATION_SETTING([SpiderMonkey support], [Yes, run-time binding])
  fi      
else
  SIM_AC_HAVE_SPIDERMONKEY_IFELSE([
    AC_DEFINE(HAVE_SPIDERMONKEY_VIA_LINKTIME_LINKING, 1,
      [define if you have the SpiderMonkey library])
    CPPFLAGS="$CPPFLAGS $sim_ac_spidermonkey_cppflags"
    LDFLAGS="$LDFLAGS $sim_ac_spidermonkey_ldflags"
    LIBS="$sim_ac_spidermonkey_libs $LIBS"
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_spidermonkey_cppflags"
    COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_spidermonkey_ldflags"
    COIN_EXTRA_LIBS="$sim_ac_spidermonkey_libs $COIN_EXTRA_LIBS"
    if $sim_ac_enable_javascript ; then
      SIM_AC_CONFIGURATION_SETTING([SpiderMonkey support], [Yes, link-time binding])
      sim_ac_have_spidermonkey=true
    fi
  ], [
    if $sim_ac_spidermonkey_desired; then
      SIM_AC_CONFIGURATION_SETTING([SpiderMonkey support], [No])
    fi
  ])
fi

AM_CONDITIONAL(HAVE_SPIDERMONKEY_VIA_LINKTIME_LINKING, $sim_ac_have_spidermonkey)


# **************************************************************************
# Set us up for using TrueType fonts. Through the FreeType library (if
# wanted, and if available), and / or the Win32 API.

if $sim_ac_have_win32_api; then
  SIM_AC_CONFIGURATION_SETTING([Win32 TrueType fonts], [Yes])
fi

AC_ARG_ENABLE([dl-freetype],
  AC_HELP_STRING([--disable-dl-freetype],
                 [disable runtime dynamic linking for freetype if possible [[default=--enable-dl-freetype]]]),
  [case "${enableval}" in
    yes | true) enable_dl_freetype=true ;;
    no | false) enable_dl_freetype=false ;;
    *) SIM_AC_ENABLE_ERROR([--enable-dl-freetype]) ;;
  esac],
  [enable_dl_freetype=true])

sim_ac_have_freetype=false

if $enable_dl_freetype && $coin_can_do_runtime_linking; then
  AC_DEFINE_UNQUOTED([FREETYPE_RUNTIME_LINKING], 1,
    [define for runtime linking with freetype])
  SIM_AC_CONFIGURATION_SETTING([FreeType library], [Yes, run-time binding])
else
  SIM_AC_HAVE_FREETYPE_IFELSE([
    AC_DEFINE(HAVE_FREETYPE, 1, [define if you have the freetype library])
    CPPFLAGS="$CPPFLAGS $sim_ac_freetype_cppflags"
    LDFLAGS="$LDFLAGS $sim_ac_freetype_ldflags"
    LIBS="$sim_ac_freetype_libs $LIBS"
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_freetype_cppflags"
    COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_freetype_ldflags"
    COIN_EXTRA_LIBS="$sim_ac_freetype_libs $COIN_EXTRA_LIBS"
    SIM_AC_CONFIGURATION_SETTING([FreeType library], [Yes, link-time binding])
  ], [
    if $sim_ac_freetype_desired; then
      SIM_AC_CONFIGURATION_SETTING([FreeType library], [No])
    fi
  ])
fi

AM_CONDITIONAL(HAVE_FREETYPE, $sim_ac_have_freetype)

# **************************************************************************
# Set us up for using TrueType fonts through the Core Text framework
# on the Mac.

#
# disabled until CoreText implementation is in place. 20100410 tamer.
#

#AC_ARG_ENABLE([coretext],
#  AC_HELP_STRING([--disable-coretext],
#                 [disable usage of the CoreText framework on OS X [[default=--enable-coretext]]]),
#  [case "${enableval}" in
#    yes | true) enable_coretext=true ;;
#    no | false) enable_coretext=false ;;
#    *) SIM_AC_ENABLE_ERROR([--enable-coretext]) ;;
#  esac],
#  [enable_coretext=true])
#
#if $enable_coretext; then
#  SIM_AC_HAVE_CORETEXT_IFELSE([
#    AC_DEFINE(HAVE_CORETEXT, 1, [define if you have the CoreText framework])
#    CPPFLAGS="$CPPFLAGS $sim_ac_coretext_cppflags"
#    LDFLAGS="$LDFLAGS $sim_ac_coretext_ldflags"
#    LIBS="$sim_ac_coretext_libs $LIBS"
#    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_coretext_cppflags"
#    COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_coretext_ldflags"
#    COIN_EXTRA_LIBS="$sim_ac_coretext_libs $COIN_EXTRA_LIBS"
#    SIM_AC_CONFIGURATION_SETTING([Core Text framework], [Yes])
#  ], [
#    SIM_AC_CONFIGURATION_SETTING([Coretext framework], [No])
#  ])
#fi
#
#AM_CONDITIONAL(HAVE_CORETEXT, $sim_ac_have_coretext)

# *******************************************************************
# * Math library
# *******************************************************************

SIM_AC_CHECK_MATHLIB(
  [COIN_EXTRA_LIBS="$COIN_EXTRA_LIBS $sim_ac_libm"],
  [SIM_AC_ERROR([no-math-library])])

# *******************************************************************
# * Check for ilogb() after mathlib test
# *******************************************************************
AC_MSG_CHECKING([for ilogb() function])
AC_TRY_LINK([
#include <math.h>
], [
  (void)ilogb(1.0);
], [
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_ILOGB],1, [Define to use ilogb()])
], [
  AC_MSG_RESULT([no])
])

# *******************************************************************
# * Handle the detection and inclusion of compression libraries
# *******************************************************************

AC_ARG_ENABLE([dl-zlib],
  AC_HELP_STRING([--disable-dl-zlib],
                 [disable runtime dynamic linking for libz if possible [[default=--enable-dl-zlib]]]),
  [case "${enableval}" in
    yes | true) enable_dl_zlib=true ;;
    no | false) enable_dl_zlib=false ;;
    *) SIM_AC_ENABLE_ERROR([--enable-dl-zlib]) ;;
  esac],
  [enable_dl_zlib=true])

if $enable_dl_zlib && $coin_can_do_runtime_linking; then
  AC_DEFINE_UNQUOTED([ZLIB_RUNTIME_LINKING],1,
    [define for runtime linking with zlib])
  SIM_AC_CONFIGURATION_SETTING([zlib support], [Yes, run-time binding])
else
  SIM_AC_HAVE_LIBZLIB_IFELSE([
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_libzlib_cppflags"
    COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_libzlib_ldflags"
    COIN_EXTRA_LIBS="$sim_ac_libzlib_libs $COIN_EXTRA_LIBS"
    CPPFLAGS="$CPPFLAGS $sim_ac_libzlib_cppflags"
    LDFLAGS="$LDFLAGS $sim_ac_libzlib_ldflags"
    LIBS="$sim_ac_libzlib_libs $LIBS"
    SIM_AC_CONFIGURATION_SETTING([zlib support], [Yes, link-time binding])
    AC_DEFINE([HAVE_ZLIB], 1, [Define to use zlib])
  ],[
    SIM_AC_CONFIGURATION_SETTING([zlib support], [No])
  ])
fi

# Bug in gzdopen() on Mac OS X -> use gzopen() instead...
case $host_os in
  darwin* ) ;;
  * ) AC_DEFINE([HAVE_GZDOPEN], 1, [Define to use gzdopen()])
esac

# *******************************************************************

AC_ARG_ENABLE([dl-libbzip2],
  AC_HELP_STRING([--disable-dl-libbzip2],
                 [disable runtime dynamic linking for libz if possible [[default=--enable-dl-libbzip2]]]),
  [case "${enableval}" in
    yes | true) enable_dl_libbzip2=true ;;
    no | false) enable_dl_libbzip2=false ;;
    *) SIM_AC_ENABLE_ERROR([--enable-dl-libbzip2]) ;;
  esac],
  [enable_dl_libbzip2=true])

if $enable_dl_libbzip2 && $coin_can_do_runtime_linking; then
  AC_DEFINE_UNQUOTED([LIBBZIP2_RUNTIME_LINKING],1,
    [define for runtime linking with zlib])
  SIM_AC_CONFIGURATION_SETTING([bzip2 support], [Yes, run-time binding])
else
  SIM_AC_HAVE_LIBBZIP2_IFELSE([
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_libbzip2_cppflags"
    COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_libbzip2_ldflags"
    COIN_EXTRA_LIBS="$sim_ac_libbzip2_libs $COIN_EXTRA_LIBS"
    CPPFLAGS="$CPPFLAGS $sim_ac_libbzip2_cppflags"
    LDFLAGS="$LDFLAGS $sim_ac_libbzip2_ldflags"
    LIBS="$sim_ac_libbzip2_libs $LIBS"
    SIM_AC_CONFIGURATION_SETTING([bzip2 support], [Yes, link-time binding])
    AC_DEFINE([HAVE_BZIP2], 1, [Define to use bzip2])
  ],[
    SIM_AC_CONFIGURATION_SETTING([bzip2 support], [No])
  ])
fi

# *******************************************************************
# * EXPAT specific stuff.
# *******************************************************************
if test x"$sim_ac_have_win32_api" = x"false" && test x"$sim_ac_enable_system_expat" = x"false"; then
  AC_DEFINE([XML_DEV_URANDOM], 1, [Define to extract entropy from /dev/urandom])
fi

# *******************************************************************
# * Handle the detection and inclusion of X11 libraries.
# *******************************************************************

  SIM_AC_CHECK_X11([
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_x11_cppflags"
    COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_x11_ldflags"
    COIN_EXTRA_LIBS="$sim_ac_x11_libs $COIN_EXTRA_LIBS"])

  if test x"$sim_ac_x11_avail" = xyes; then
    #  If the X11 shared memory extension is available, include it as
    #  the Mesa library has probably been linked against it.
    SIM_AC_CHECK_X11SHMEM(
      COIN_EXTRA_LIBS="$sim_ac_x11shmem_libs $COIN_EXTRA_LIBS"
    )
  fi

  ## We need this extra, final X11 check to set up the HAVE_X11_AVAILABLE
  ## define -- the X_DISPLAY_MISSING define provided by the built-in
  ## Autoconf X11 check is not strict enough.
  SIM_AC_CHECK_X11_READY([AC_DEFINE([HAVE_X11_AVAILABLE],1,
                         [define if it is possible to build against X11])])

# *******************************************************************
# * Test for installation of Mesa or OpenGL development system.
# *******************************************************************

SIM_AC_CHECK_OPENGL(
  [
  AC_DEFINE([HAVE_OPENGL],1, [define if you have OpenGL])

  COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_ogl_cppflags"
  COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_ogl_ldflags"
  COIN_EXTRA_LIBS="$sim_ac_ogl_libs $COIN_EXTRA_LIBS"

  SIM_AC_CHECK_HEADER_GLEXT([
    CPPFLAGS="$CPPFLAGS $sim_ac_glext_cppflags"
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_glext_cppflags"
  ])

  # *******************************************************************
  # Check what OpenGL->windowsystem bindings are available.
  # 
  # If WGL is available, we don't want AGL nor GLX, as that has the
  # potential to cause harm: e.g. while we at one place in the code
  # might use WGL to create a context, other places we might use GLX to
  # make that same context current -- *kaboom*.
  #
  # (It is at least possible that both AGL and GLX are available at the
  # same time, or WGL and GLX, since X11 is a portable window system.)
  #
  # On Mac OS X/Darwin, we want to link either with GLX or AGL, 
  # depending on whether Coin is built against X11 or not. (X11 is 
  # only needed if you want to use SoXt, and must be enabled explicitly
  # by passing the --enable-darwin-x11 configure flag.)
  #
  # Else, the tests for WGL/GLX below are nested, so we're guaranteed 
  # that only one of HAVE_WGL and HAVE_GLX will be set:

  case $host_os in
    darwin* ) 
      if test x"$sim_ac_x11_avail" = xyes; then 
        SIM_AC_HAVE_GLX_IFELSE([
          AC_DEFINE([HAVE_GLX], 1, [define if you have GLX X11 OpenGL bindings])
          ], [SIM_AC_CONFIGURATION_WARNING([Could not find or compile or link against GLX (OpenGL on X11) but X linkage was specified. The SoOffscreenRenderer functionality will be disabled. ])])
      else
        AC_ARG_ENABLE([agl],
          AC_HELP_STRING([--enable-agl],
                         [enable AGL on Mac OS X [[default=--disable-agl]]]),
          [case "${enableval}" in
            yes | true) sim_ac_enable_agl=true ;;
            no | false) sim_ac_enable_agl=false ;;
            *) SIM_AC_ENABLE_ERROR([--enable-agl]) ;;
           esac],
          [sim_ac_enable_agl=false])
        if $sim_ac_enable_agl; then
          SIM_AC_HAVE_AGL_IFELSE([
            AC_DEFINE([HAVE_AGL], 1, [define if you have AGL OpenGL bindings])
            LDFLAGS="$LDFLAGS $sim_ac_agl_ldflags"
            # indirect linking is not supported in static libs
            if $COIN_STATIC; then
              COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_agl_ldflags"
            fi
            AC_CHECK_HEADERS(
              [OpenGL/CGLCurrent.h],
              [],
              [AC_MSG_ERROR([AGL is available but CGL is not? Dude. Something went very wrong here. Please get in touch with us at coin-support@sim.no.])], [ ]
            )
            SIM_AC_HAVE_AGL_PBUFFER([AC_DEFINE_UNQUOTED([HAVE_AGL_PBUFFER],1,[define if AGL pBuffers are available on the system])])
          ], [])
        fi
        SIM_AC_HAVE_CGL_IFELSE(
          [AC_DEFINE([HAVE_CGL], 1, [define if you have CGL OpenGL bindings])],
          [])
        if test x"$sim_cv_have_cgl" != x"true" && test x"$sim_cv_have_agl" != x"false" ; then
          SIM_AC_CONFIGURATION_WARNING([Could not find or compile or link against AGL or CGL. The SoOffscreenRenderer functionality will be disabled.])
        fi
      fi
    ;;
    * )  
    SIM_AC_HAVE_WGL_IFELSE([
      AC_DEFINE([HAVE_WGL], 1, [define if you have WGL Win32 OpenGL bindings])
      LIBS="$LIBS $sim_ac_wgl_libs"
      COIN_EXTRA_LIBS="$COIN_EXTRA_LIBS $sim_ac_wgl_libs"
    ], [SIM_AC_HAVE_GLX_IFELSE([
          AC_DEFINE([HAVE_GLX], 1, [define if you have GLX X11 OpenGL bindings])
        ], [
          SIM_AC_CONFIGURATION_WARNING([Could not find or compile or link against any usable window-system OpenGL binding, neither GLX (OpenGL on X11), nor WGL (OpenGL on Win32) nor AGL (OpenGL on Mac OS). The SoOffscreenRenderer functionality will be disabled. Note: this might be a configure bug. Check the file config.log.])
        ])
      ])
    ;;
  esac

  # *******************************************************************

  if $enable_superglu; then
    SIM_AC_CONFIGURATION_SETTING([GLU linkage], [superglu])
  else
    # *******************************************************************
    # ** Check if GLU should be loaded and linked at runtime (to avoid
    # ** install dependency libCoin -> libGLU).
    # *******************************************************************
    AC_ARG_ENABLE([dl-glu],
      AC_HELP_STRING([--enable-dl-glu],
                     [use runtime dynamic linking for GLU if possible [[default=yes]]]),
      [case "${enableval}" in
        yes | true) enable_dl_glu=true ;;
        no | false) enable_dl_glu=false ;;
        *) SIM_AC_ENABLE_ERROR([--enable-dl-glu]) ;;
      esac],
      [enable_dl_glu=true])
  
    if $coin_can_do_runtime_linking && $enable_dl_glu; then
      # See if GLU is part of GL library (this is the case on Mac OS X 
      # and BeOS, for instance) before setting us up for runtime linking.
      SIM_AC_GLU_READY_IFELSE([
        AC_DEFINE([GLU_IS_PART_OF_GL],1, 
                  [define if GLU is part of the GL library])
        AC_DEFINE_UNQUOTED([GLU_RUNTIME_LINKING],1,
          [define for runtime linking with GLU])
        SIM_AC_CONFIGURATION_SETTING([GLU linkage], 
                  [run-time binding, part of GL library])
      ], [
        AC_DEFINE_UNQUOTED([GLU_RUNTIME_LINKING],1,
          [define for runtime linking with GLU])
        SIM_AC_CONFIGURATION_SETTING([GLU linkage], [run-time binding])
      ])
    else
      # *******************************************************************
      # ** Check if GLU headers and lib is available on build host.
      # *******************************************************************
      SIM_AC_CHECK_GLU([
        AC_DEFINE([HAVE_GLU],1, [define if you have GLU])
        COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_glu_cppflags"
        COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_glu_ldflags"
        COIN_EXTRA_LIBS="$sim_ac_glu_libs $COIN_EXTRA_LIBS"
        SIM_AC_CONFIGURATION_SETTING([GLU linkage], [link-time binding])
      ], [
        SIM_AC_CONFIGURATION_SETTING([GLU linkage], [no GLU support])
      ])
    fi
  fi
  ], # OpenGL-linkage ok, end of block
  [ # Couldn't link with OpenGL:
    SIM_AC_ERROR([no-opengl])
    # FIXME: should perhaps be able to handle systems where OpenGL is not
    # available,  as it would be handy for making e.g. import/export-only
    # versions of the library. 20000123 mortene.
  ]
)

# **************************************************************************
# Make name of dynamic GL library available to Coin source code (used in
# src/glue/dl.c, at least).

# Will set $sim_ac_shlibext. Note: there's a dependency on this later in
# the configure script aswell.
SIM_AC_DYNLIB_EXT

# **************************************************************************
# Cygwin has not got diffutils installed by default. Check if cmp is
# available.
AC_CHECK_PROG(cmp_avail,[cmp],[Yes])
if test x$cmp_avail = "x" ; then
    AC_MSG_ERROR([cmp must be available to complete the configure process.])
fi


AC_DEFINE_UNQUOTED([DYNAMIC_LIBRARY_EXTENSION], ["$sim_ac_shlibext"],
                   [define this to the dynamic library extension suffix on this system])

AC_SUBST([LIBEXT], [$sim_ac_shlibext])

sim_ac_opengl_dll_name=$sim_ac_ogl_lib
if test "$sim_ac_ogl_lib" = "opengl32"; then :; else
  sim_ac_opengl_dll_name="lib${sim_ac_opengl_dll_name}"
fi
sim_ac_opengl_dll_name="${sim_ac_opengl_dll_name}${sim_ac_shlibext}"

AC_DEFINE_UNQUOTED([OPENGL_SYSTEM_LIBRARY_NAME], ["$sim_ac_opengl_dll_name"],
                   [define this to the dynamic library name of OpenGL])

# *******************************************************************
# * Provide this nasty hack to help building Coin on platforms
# * where one can bump into the "Arg list too long" problem
# * (IBM AIX, SGI IRIX 6.2 and IRIX 6.5 with standard settings are
# * known to fail).
# *
# * This nasty hack also have a lot of positive byeffects, such as
# * increasing optimization, and memory usage of the compiler, while
# * reducing the compile time for a release build.
# *******************************************************************

AC_ARG_ENABLE(compact,
  AC_HELP_STRING([--enable-compact], [enable hack for compact compilation. Has the bieffect of increased optimalization, and reduced compile-time, for increased memory usage for a release build. [[default=no]]]),
  [case "${enableval}" in
    yes | true)  enable_compact=yes ;;
    no | false)  enable_compact=no ;;
    *) SIM_AC_ENABLE_ERROR([--enable-compact]) ;;
  esac],
  enable_compact=no)

AM_CONDITIONAL(HACKING_COMPACT_BUILD, test x"$enable_compact" = x"yes")

# *******************************************************************
# * Enable/disable compilation with the "incremental linking through
# * the use of dynamic libraries"-hack.
# *******************************************************************

AC_ARG_ENABLE(hacking,
  AC_HELP_STRING([--enable-hacking],
                 [enable hacks for quick turn-around cycles during development [[default=no]]]),
  [case "${enableval}" in
    yes | true)  enable_hacking=yes ;;
    no | false)  enable_hacking=no ;;
    *) SIM_AC_ENABLE_ERROR([--enable-hacking]) ;;
  esac],
  enable_hacking=no)

AM_CONDITIONAL(HACKING_DYNAMIC_MODULES, test x"$enable_hacking" = x"yes")

# enable_hacking requires runtime symbol lookup, which is not available
# on Mac OS X < 10.3 and requires the deployment target to be >= 10.3.
sim_ac_macosx_use_dynamic_lookup=false

if test x"$enable_hacking" = x"yes"; then

  if test x"$sim_ac_macosx" = x"true"; then
    if test x"$sim_ac_macosx_10_3ff" = x"true"; then
      sim_ac_macosx_use_dynamic_lookup=true
      case ${MACOSX_DEPLOYMENT_TARGET} in
      10.[[345]])
        ;;
      *)
        SIM_AC_CONFIGURATION_WARNING([Using MACOSX_DEPLOYMENT_TARGET=10.3 (required by --enable-hacking)])
      ;;
      esac
      AC_SUBST([MACOSX_DEPLOYMENT_TARGET], [10.3])
    else 
      AC_MSG_ERROR([--enable-hacking requires Mac OS X version >= 10.3])
    fi
  fi 

  for dir in \
    base actions bundles caches details draggers elements \
    elements/GL:glelements engines errors events fields fonts glue io \
    manips misc lists nodekits navigation nodes rendering shapenodes projectors \
    sensors shaders upgraders hardcopy shadows geo foreignfiles collision \
    xml xml/expat:expat profiler 3ds:format3ds scxml soscxml; \
  do
    case $dir in
    *:* )
      lib=`echo $dir | cut -d: -f2`
      dir=`echo $dir | cut -d: -f1`
      ;;
    * )
      lib=$dir
      ;;
    esac
    HACKING_LIBDIRS="$HACKING_LIBDIRS -L${coin_build_dir}/src/$dir/.libs"
    EXTRA_LIBS="$EXTRA_LIBS -l$lib${SUFFIX}LINKHACK"                  
  done
  if $sim_ac_enable_vrml97; then
    HACKING_LIBDIRS="$HACKING_LIBDIRS -L${coin_build_dir}/src/vrml97/.libs"
    EXTRA_LIBS="$EXTRA_LIBS -lvrml97${SUFFIX}LINKHACK"
  fi
  if $enable_superglu; then
    HACKING_LIBDIRS="$HACKING_LIBDIRS -L${coin_build_dir}/src/extensions/superglu/.libs"
    EXTRA_LIBS="$EXTRA_LIBS -l${SUPERGLUPREFIX}GLU"
  fi
  if $sim_ac_enable_threads; then
    HACKING_LIBDIRS="$HACKING_LIBDIRS -L${coin_build_dir}/src/threads/.libs"
    EXTRA_LIBS="$EXTRA_LIBS -lthreads${SUFFIX}LINKHACK"
  fi
  COIN_HACKING_LIBDIRS="$HACKING_LIBDIRS"
  COIN_EXTRA_LIBS="$EXTRA_LIBS $COIN_EXTRA_LIBS"
fi

AC_SUBST(COIN_HACKING_LIBDIRS)

# linker flag -undefined dynamic_lookup, needed for --enable-hacking
AM_CONDITIONAL([MACOSX_USE_DYNAMIC_LOOKUP], 
               [test x$sim_ac_macosx_use_dynamic_lookup = xtrue])


# *******************************************************************
# * For exposing various build, src and installation paths to
# * misc configuration files under the $top_src_dir/build/ directory.
# *******************************************************************

AC_SUBST([coin_build_dir])
AC_SUBST([coin_w32_build_dir])
AC_SUBST([coin_src_dir])
AC_SUBST([coin_w32_src_dir])

# **************************************************************************
# Remove redundant options from certain option lists.

SIM_AC_UNIQIFY_OPTION_LIST(COIN_EXTRA_CPPFLAGS, -I$includedir -I$includedir/Inventor/annex $COIN_EXTRA_CPPFLAGS)
SIM_AC_UNIQIFY_OPTION_LIST(COIN_EXTRA_LDFLAGS, -L$libdir $COIN_EXTRA_LDFLAGS)
SIM_AC_UNIQIFY_OPTION_LIST(COIN_EXTRA_LIBS, $COIN_EXTRA_LIBS)
SIM_AC_UNIQIFY_OPTION_LIST(CPPFLAGS, $CPPFLAGS)
SIM_AC_UNIQIFY_OPTION_LIST(LDFLAGS, $LDFLAGS)
SIM_AC_UNIQIFY_OPTION_LIST(LIBS, $LIBS)

# **************************************************************************
# Remove gcc system directories includes from the CPPFLAGS.

CPP_AC_SEARCH_ORDER_FILTER(CPPFLAGS, $CPPFLAGS)
CPP_AC_SEARCH_ORDER_FILTER(COIN_EXTRA_CPPFLAGS, $COIN_EXTRA_CPPFLAGS)

AC_SUBST([LIBFLAGS], [])
AC_SUBST([COIN_TESTSUITE_EXTRA_CPPFLAGS], [])
AC_SUBST([COIN_TESTSUITE_EXTRA_LDFLAGS], [])

# **************************************************************************
# setup for <Inventor/system/gl.h>

# include windows.h?
if test x"$ac_cv_header_windows_h" = x"yes"; then
  sim_include_windows_h="#include <windows.h>"
else
  sim_include_windows_h="/* #include <windows.h> - not needed on system */"
fi

# how to include gl.h
if test x"$ac_cv_header_GL_gl_h" = x"yes"; then
  sim_include_gl_h="#include <GL/gl.h>"
else
  if test x"$ac_cv_header_OpenGL_gl_h" = x"yes"; then
    sim_include_gl_h="#include <OpenGL/gl.h>"
  else
    sim_include_gl_h="#error \"don't know how to include gl.h header\""
  fi
fi

if $enable_superglu; then
  sim_include_glu_h="/* #include <GL/glu.h> - not used, Coin linked with embedded SuperGLU */"
else
  if test x"$ac_cv_header_GL_glu_h" = x"yes"; then
    sim_include_glu_h="#include <GL/glu.h>"
  else
    if test x"$ac_cv_header_OpenGL_glu_h" = x"yes"; then
      sim_include_glu_h="#include <OpenGL/glu.h>"
    else
      sim_include_glu_h="/* #include <GL/glu.h> - not found on system */"
    fi
  fi
fi

if test x"$ac_cv_header_GL_glext_h" = x"yes"; then
  sim_include_glext_h="#include <GL/glext.h>"
else
  if test x"$ac_cv_header_OpenGL_glext_h" = x"yes"; then
    sim_include_glext_h="#include <OpenGL/glext.h>"
  else
    sim_include_glext_h="/* #include <GL/glext.h> - not found on system */"
  fi
fi

AC_SUBST([SIM_INCLUDE_WINDOWS_H], [$sim_include_windows_h])
AC_SUBST([SIM_INCLUDE_GL_H], [$sim_include_gl_h])
AC_SUBST([SIM_INCLUDE_GLU_H], [$sim_include_glu_h])
AC_SUBST([SIM_INCLUDE_GLEXT_H], [$sim_include_glext_h])

# **************************************************************************

# append compiler flags detected earlier, now that all configure tests
# are done...

CFLAGS="$CFLAGS $sim_ac_compiler_CFLAGS"
CXXFLAGS="$CXXFLAGS $sim_ac_compiler_CXXFLAGS"

# **************************************************************************
# Remaining setup based on platform.

# check if compiler/linker handles __declspec()
sim_ac_win32_declspec_available=false
AC_MSG_CHECKING([for __declspec() support])
AC_TRY_LINK(
 [],
 [__declspec(dllimport) int foo;],
 [sim_ac_win32_declspec_available=true
  AC_MSG_RESULT([available])],
 [AC_MSG_RESULT([not available])])

if $sim_ac_win32_declspec_available; then
  if $COIN_STATIC; then
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS -DCOIN_NOT_DLL"
    COIN_EXTRA_LIBS="$COIN_EXTRA_LIBS $LIBS"
  else
    CPPFLAGS="-DCOIN_MAKE_DLL $CPPFLAGS"
    COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS -DCOIN_DLL"
  fi
fi

AM_CONDITIONAL(BUILD_WITH_MSVC, $BUILD_WITH_MSVC)

COIN_COMPILER="$CXX"

if $BUILD_WITH_MSVC; then
  COIN_TESTSUITE_EXTRA_CPPFLAGS="/Fdtestsuite.pdb /EHsc"
  COIN_TESTSUITE_EXTRA_LDFLAGS="/SUBSYSTEM:console /DEBUG"
  rm -f vc60.pdb
  LIBFLAGS="$LIBFLAGS $sim_ac_msvcrt_LIBLDFLAGS"
  LIBS="$LIBS $sim_ac_msvcrt_LIBLIBS"

  debugfile="`pwd`/src/coin${COIN_MAJOR_VERSION}${SUFFIX}.pdb"
  debugfile=`cygpath -w "$debugfile" | sed 's,\\\\,\\\\\\\\,g'`

  if $COIN_STATIC; then
    LIBS=
    LIBFLAGS="-LIB $LIBFLAGS"
    LIBFLAGS="$LIBFLAGS /OUT:coin$COIN_MAJOR_VERSION$SUFFIX.lib"
    case $enable_symbols in
    no | false)
      SIM_AC_CONFIGURATION_SETTING([Coin build type], [static .lib, no symbols])
      ;;
    *)
      SIM_AC_CONFIGURATION_SETTING([Coin build type], [static .lib, with symbols])
      CFLAGS="-g $CFLAGS"
      CXXFLAGS="-g $CXXFLAGS"
      ;;
    esac
  else
    LIBFLAGS="-DLL /INCREMENTAL:NO $LIBFLAGS"
    case $enable_debug in
    no | false)
      release_string=" (release)"
      debug_flag=" /DEBUG"
      ;;
    *)
      release_string=""
      debug_flag=" /DEBUG"
      ;;
    esac
    case $enable_symbols in
    no | false)
      SIM_AC_CONFIGURATION_SETTING([Coin build type],
                                   [dynamic .dll${release_string}])
      LIBFLAGS="$LIBFLAGS ${debug_flag}"
      ;;
    *)
      SIM_AC_CONFIGURATION_SETTING([Coin build type],
                                   [dynamic .dll${release_string} + .pdb])
      CFLAGS="-g $CFLAGS"
      CXXFLAGS="-g $CXXFLAGS"
      LIBFLAGS="$LIBFLAGS ${debug_flag} /PDB:coin$COIN_MAJOR_VERSION$SUFFIX.pdb"
      ;;
    esac
    LIBFLAGS="$LIBFLAGS /OUT:coin$COIN_MAJOR_VERSION$SUFFIX.dll"
  fi
  SIM_AC_CONFIGURATION_SETTING([C library version], [$sim_ac_msvcrt])

  # We use a version suffix on the .dll-file, so several incompatible
  # (major) versions can be installed on a system.
  #
  # BTW, when linking DLLs, the 3rd-party .lib files will be
  # linked into the .dll file. I believe it is still advisable to
  # list all libs used upon `coin-config --libs`, as we can then
  # also use them from "parent" code (remember that their interfaces
  # is not exposed from the DLL) without any fuss.
  sim_ac_coin_noextname="coin${COIN_MAJOR_VERSION}${SUFFIX}"
  if test -n "$COIN_EXTRA_LIBS"; then
    COIN_EXTRA_LIBS="-l${sim_ac_coin_noextname} $COIN_EXTRA_LIBS"
  else
    COIN_EXTRA_LIBS="-l${sim_ac_coin_noextname}"
  fi

  sim_ac_coin_dll_name="${sim_ac_coin_noextname}${sim_ac_shlibext}"

else
  COIN_EXTRA_LIBS="-lCoin${SUFFIX} $COIN_EXTRA_LIBS"
  sim_ac_coin_dll_name="libCoin${SUFFIX}${sim_ac_shlibext}"
fi

if $BUILD_WITH_MSVC; then
  # we'll use the installed wrapper when we use the coin-config script later
  # so we can remove the source code hierarchy
  COIN_COMPILER="wrapmsvc"
fi

AC_DEFINE_UNQUOTED([COIN_SYSTEM_LIBRARY_NAME], ["$sim_ac_coin_dll_name"],
                   [define this to the dynamic library name of Coin])

sim_ac_path_problem=true # until proven otherwise
eval "sim_ac_pb_prefix=${prefix}"
if test x"$sim_ac_pb_prefix" = x"NONE"; then sim_ac_pb_prefix=/usr/local; fi
eval "sim_ac_pb_exec_prefix=${exec_prefix}"
if test x"$sim_ac_pb_exec_prefix" = x"NONE"; then sim_ac_pb_exec_prefix="$prefix"; fi
eval "sim_ac_pb_bindir=${bindir}"
if test -d "${sim_ac_pb_bindir}"; then
  # search by inode id seems safer than strings because of noncanonical paths
  ( eval set dummy `ls -id "${sim_ac_pb_bindir}" 2>/dev/null`;
    findinode=$2;
    IFS=:; for dir in $PATH; do
      # inode=`ls -id "$dir" 2>/dev/null | sed 's/^\([ ]*\)\([0-9]*\).*/\2/g'`;
      eval set dummy `ls -id "$dir" 2>/dev/null`;
      inode=$2;
      test x"$inode" = x"$findinode" && exit 0;
    done; exit 1 ) && sim_ac_path_problem=false
fi
if $sim_ac_path_problem; then
  ( IFS=:; for dir in $PATH; do
      test x"$dir" = x"${sim_ac_pb_bindir}" && exit 0;
    done; exit 1 ) && sim_ac_path_problem=false
fi


# ***********************************************************
# Mac OS X tweaks
#

# We use CoreFoundation calls multiple places in Coin
[case $host_os in
darwin* ) LDFLAGS="$LDFLAGS -Wl,-framework,CoreFoundation" ;;
* ) ;;
esac]

# Disallow static libraries in framework (doesn't make sense)
if $sim_ac_framework; then
  if test x$enable_static = xyes; then
    AC_MSG_ERROR([Creation of static libraries requested, but this is incompatible with the Mac OS X framework setup. Use --without-framework to do UNIX-style build.])
  fi
fi 

if $sim_ac_framework; then
  SIM_AC_CONFIGURATION_SETTING([Coin build type], [Mac OS X framework])
elif $sim_ac_path_problem; then
  SIM_AC_CONFIGURATION_WARNING([Your \$PATH variable does not appear to include \"$prefix/bin\"])
fi

if $sim_ac_framework; then

  # set up the Coin-related flags
  COIN_EXTRA_CPPFLAGS=
  if test x"$sim_ac_framework_prefix" != x"MAC_FRAMEWORK_PREFIX_DEFAULT"; then
    COIN_EXTRA_CPPFLAGS="-F$sim_ac_framework_prefix"
  fi

  COIN_EXTRA_CFLAGS=
  COIN_EXTRA_CXXFLAGS=
  COIN_EXTRA_FP_LDFLAGS=
  if test x"$enable_hacking" = x"yes"; then
    COIN_EXTRA_LDFLAGS="-L$frameworkdir/Libraries"
  else 
    # we don't want -lCoin when doing framework build
    COIN_EXTRA_LIBS=
    COIN_EXTRA_LDFLAGS= 
    if test x"$sim_ac_framework_prefix" != x"MAC_FRAMEWORK_PREFIX_DEFAULT"; then
      COIN_EXTRA_FP_LDFLAGS="-Wl,-F$sim_ac_framework_prefix"
    fi
  fi 
  COIN_EXTRA_LDFLAGS="$COIN_EXTRA_FP_LDFLAGS -Wl,-framework,$MAC_FRAMEWORK_NAME $COIN_EXTRA_LDFLAGS"
  # adjust for OpenGL usage
  COIN_EXTRA_CPPFLAGS="$COIN_EXTRA_CPPFLAGS $sim_ac_ogl_cppflags"
  COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_ogl_ldflags"
  COIN_EXTRA_LIBS="$sim_ac_ogl_libs $COIN_EXTRA_LIBS"
  # adjust for simage usage
  COIN_EXTRA_LDFLAGS="$COIN_EXTRA_LDFLAGS $sim_ac_simage_ldflags"
  COIN_EXTRA_LIBS="$sim_ac_simage_libs $COIN_EXTRA_LIBS"
  # adjust for OpenAL usage
  COIN_EXTRA_LIBS="$COIN_EXTRA_LIBS $sim_ac_openal_libs"
fi

case "$sim_ac_framework:$sim_ac_make_dsp" in
true:* | *:true )
  ;;
* )
  SIM_AC_CONFIGURATION_SETTING([Installation prefix], [$prefix])
  ;;
esac

AC_SUBST([COIN_COMPILER])
AC_SUBST([ac_compiler_gnu])

SIM_AC_MSVC_DSP_SETUP([COIN], [Coin], [coin], [-I$coin_build_dir -I$coin_build_dir/include -I$coin_src_dir/include -I$coin_build_dir/src -I$coin_src_dir/src -I$coin_src_dir/include/Inventor/annex])

# **************************************************************************

AM_CONFIG_HEADER([src/discard.h src/config.h src/setup.h])
AC_CONFIG_FILES([src/Coin.rc])

# SIM_AC_COIN_CONFIG_H_CHECK

# **************************************************************************

AC_CONFIG_FILES([
        Makefile
        bin/Makefile
        include/Makefile
        include/Inventor/Makefile
        include/Inventor/C/Makefile
        include/Inventor/C/XML/Makefile
        include/Inventor/C/base/Makefile
        include/Inventor/C/errors/Makefile
        include/Inventor/C/glue/Makefile
        include/Inventor/C/threads/Makefile
        include/Inventor/VRMLnodes/Makefile
        include/Inventor/actions/Makefile
        include/Inventor/bundles/Makefile
        include/Inventor/caches/Makefile
        include/Inventor/collision/Makefile
        include/Inventor/details/Makefile
        include/Inventor/draggers/Makefile
        include/Inventor/elements/Makefile
        include/Inventor/engines/Makefile
        include/Inventor/errors/Makefile
        include/Inventor/events/Makefile
        include/Inventor/fields/Makefile
        include/Inventor/lists/Makefile
        include/Inventor/lock/Makefile
        include/Inventor/manips/Makefile
        include/Inventor/misc/Makefile
        include/Inventor/navigation/Makefile
        include/Inventor/nodekits/Makefile
        include/Inventor/nodes/Makefile
        include/Inventor/projectors/Makefile
        include/Inventor/sensors/Makefile
        include/Inventor/system/Makefile
        include/Inventor/threads/Makefile
        include/Inventor/tools/Makefile
        include/Inventor/scxml/Makefile
        include/Inventor/annex/Makefile
        include/Inventor/annex/HardCopy/Makefile
        include/Inventor/annex/ForeignFiles/Makefile
        include/Inventor/annex/FXViz/Makefile
        include/Inventor/annex/FXViz/elements/Makefile
        include/Inventor/annex/FXViz/nodes/Makefile
        include/Inventor/annex/Profiler/Makefile
        include/Inventor/annex/Profiler/elements/Makefile
        include/Inventor/annex/Profiler/engines/Makefile
        include/Inventor/annex/Profiler/nodes/Makefile
        include/Inventor/annex/Profiler/nodekits/Makefile
        include/Inventor/annex/Profiler/utils/Makefile
        data/Makefile
        data/draggerDefaults/Makefile
        data/shaders/Makefile
        data/shaders/lights/Makefile
        data/shaders/vsm/Makefile
        data/scxml/Makefile
        data/scxml/navigation/Makefile
        man/Makefile
        man/man1/Makefile
        man/man3/Makefile
        html/Makefile
        src/Makefile
        src/base/Makefile
        src/actions/Makefile
        src/bundles/Makefile
        src/caches/Makefile
        src/collision/Makefile
        src/details/Makefile
        src/draggers/Makefile
        src/elements/Makefile
        src/elements/GL/Makefile
        src/engines/Makefile
        src/errors/Makefile
        src/events/Makefile
        src/fields/Makefile
        src/fonts/Makefile
        src/glue/Makefile
        src/io/Makefile
        src/manips/Makefile
        src/misc/Makefile
        src/rendering/Makefile
        src/lists/Makefile
        src/navigation/Makefile
        src/nodekits/Makefile
        src/nodes/Makefile
        src/projectors/Makefile
        src/3ds/Makefile
        src/sensors/Makefile
        src/upgraders/Makefile
        src/shapenodes/Makefile
        src/threads/Makefile
        src/extensions/Makefile
        src/vrml97/Makefile
        src/hardcopy/Makefile
        src/shaders/Makefile
        src/shadows/Makefile
        src/geo/Makefile
        src/foreignfiles/Makefile
        src/xml/Makefile
        src/xml/expat/Makefile
        src/profiler/Makefile
        src/scxml/Makefile
        src/soscxml/Makefile
        src/doc/Makefile
        testsuite/Makefile
        cfg/gendsp.pl
])

AC_OUTPUT

if test -f cfg/gendsp.pl; then
  chmod +x cfg/gendsp.pl
fi

# in case someone has the old generated SbBasic.h lying about...
if test -f include/Inventor/SbBasic.h; then
  if cmp -s $srcdir/include/Inventor/SbBasic.h include/Inventor/SbBasic.h; then
    :
  else
    rm include/Inventor/SbBasic.h
  fi
fi

# *******************************************************************
# * Platform-specific stuff.
# *******************************************************************

if $sim_ac_build_library; then
  ac_coin_untested_platform=true
  ac_coin_not_sim_platform=true
else
  ac_coin_untested_platform=false
  ac_coin_not_sim_platform=false
fi

case "$host_os" in
  irix6.2)
    case $CXX in
    CC | */CC ) ac_coin_untested_platform=false ;;
    esac
    ;;
  irix6.5)
    case $CXX in
    CC | */CC )
      ac_coin_untested_platform=false
      ac_coin_not_sim_platform=false
      ;;
    esac
    ;;
  aix*)
    case $CXX in
    xlC | */xlC ) ac_coin_untested_platform=false ;;
    esac
    ;;
  solaris*)
    case $CXX in
    g++ ) ac_coin_untested_platform=false ;;
    CC )
      ac_coin_untested_platform=false
      ac_coin_not_sim_platform=false
      ;;
    esac
    ;;
  hpux10.20)
    case $CXX in
    aCC | */aCC ) ac_coin_untested_platform=false ;;
    esac
    ;;
  linux-gnu)
    case "$host_cpu" in
    i?86)
      ac_coin_untested_platform=false
      ac_coin_not_sim_platform=false
    ;;
    x86_64)
      ac_coin_untested_platform=false
      ac_coin_not_sim_platform=false
    ;;
    ia64)
      case "$CXX" in
      g++)
        ac_coin_untested_platform=false
        ;;
      esac
    ;;
    esac
    ;;
  cygwin*)
    case "$CXX" in
    *wrapmsvc* )
      ac_coin_untested_platform=false
      ac_coin_not_sim_platform=false
    ;;
    esac
    ;;
  darwin*)
    case "$CXX" in
      c++ | g++)
        ac_coin_untested_platform=false
        ac_coin_not_sim_platform=false
      ;;
    esac
    ;;
  beos)
    case "$host_cpu" in
      i?86)
        if test "$CXX" = c++; then ac_coin_untested_platform=false; fi
        ;;
    esac
    ;;
esac

case "$CXX" in
*gendsp* )
  ac_coin_untested_platform=false
  ac_coin_not_sim_platform=false
  ;;
esac

if $ac_coin_untested_platform; then
  echo
  echo "************************* WARNING ****************************"
  echo "*"
  echo "* We have not tested Coin on the $host_os $host_cpu"
  echo "* platform with the $CXX C++ compiler. Please report"
  echo "* back to us at <coin-support@coin3d.org> how it works out."
  echo "*"
  echo "**************************************************************"
  echo
else
  if $ac_coin_not_sim_platform; then
    echo
    echo "NOTE: This platform ($host_os $host_cpu with the $CXX C++ compiler)"
    echo "      is not among our internal development systems. Please let us"
    echo "      know at <coin-support@coin3d.org> if you run into any problems."
    echo
  fi
fi

if $sim_ac_build_library; then
  case "$host_os" in
  irix*)
    echo ""
    echo " Detected SGI IRIX. There are some hints regarding this platform"
    echo " in the file "
    echo ""
    echo "           ${srcdir}/README.UNIX"
    echo ""
    echo " Please take a look if this is your first time building on IRIX."
    echo ""
    ;;
  aix*)
    echo ""
    echo " Detected IBM AIX. There are some hints regarding this platform"
    echo " in the file "
    echo ""
    echo "           ${srcdir}/README.UNIX"
    echo ""
    echo " Please take a look if this is your first time building on AIX."
    echo ""
    ;;
  esac
fi

# FIXME: these would be nice to show, but they puck up the configuration
# summary formatting. 20030509 mortene.
#
#SIM_AC_CONFIGURATION_SETTING([CPPFLAGS], [$CPPFLAGS])
#SIM_AC_CONFIGURATION_SETTING([CFLAGS], [$CFLAGS])
#SIM_AC_CONFIGURATION_SETTING([CXXFLAGS], [$CXXFLAGS])
#SIM_AC_CONFIGURATION_SETTING([LDFLAGS], [$LDFLAGS])
#SIM_AC_CONFIGURATION_SETTING([LIBS], [$LIBS])

SIM_AC_CONFIGURATION_SUMMARY

echo ""
