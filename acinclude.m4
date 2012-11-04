dnl @synopsis AC_CXX_HAVE_STL
dnl
dnl If the compiler supports the Standard Template Library, define HAVE_STL.
dnl
dnl @version $Id: acinclude.m4,v 1.2 2006/02/24 00:09:19 wmayer Exp $
dnl @author Luc Maisonobe
dnl
AC_DEFUN([AC_CXX_HAVE_STL],
[AC_CACHE_CHECK(whether the compiler supports Standard Template Library,
ac_cv_cxx_have_stl,
[AC_REQUIRE([AC_CXX_NAMESPACES])
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <list>
#include <deque>
#ifdef HAVE_NAMESPACES
using namespace std;
#endif],[list<int> x; x.push_back(5);
list<int>::iterator iter = x.begin(); if (iter != x.end()) ++iter; return 0;],
 ac_cv_cxx_have_stl=yes, ac_cv_cxx_have_stl=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_have_stl" = yes; then
  AC_DEFINE(HAVE_STL,,[define if the compiler supports Standard Template Library])
fi
])
dnl @synopsis AC_CXX_HAVE_STD
dnl
dnl If the compiler supports ISO C++ standard library (i.e., can include the
dnl files iostream, map, iomanip and cmath}), define HAVE_STD.
dnl
dnl @version $Id: acinclude.m4,v 1.2 2006/02/24 00:09:19 wmayer Exp $
dnl @author Luc Maisonobe
dnl
AC_DEFUN([AC_CXX_HAVE_STD],
[AC_CACHE_CHECK(whether the compiler supports ISO C++ standard library,
ac_cv_cxx_have_std,
[AC_REQUIRE([AC_CXX_NAMESPACES])
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <iostream>
#include <map>
#include <iomanip>
#include <cmath>
#ifdef HAVE_NAMESPACES
using namespace std;
#endif],[return 0;],
 ac_cv_cxx_have_std=yes, ac_cv_cxx_have_std=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_have_std" = yes; then
  AC_DEFINE(HAVE_STD,,[define if the compiler supports ISO C++ standard library])
fi
])
dnl @synopsis AC_CXX_NAMESPACES
dnl
dnl If the compiler can prevent names clashes using namespaces, define
dnl HAVE_NAMESPACES.
dnl
dnl @version $Id: acinclude.m4,v 1.2 2006/02/24 00:09:19 wmayer Exp $
dnl @author Luc Maisonobe
dnl
AC_DEFUN([AC_CXX_NAMESPACES],
[AC_CACHE_CHECK(whether the compiler implements namespaces,
ac_cv_cxx_namespaces,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([namespace Outer { namespace Inner { int i = 0; }}],
                [using namespace Outer::Inner; return i;],
 ac_cv_cxx_namespaces=yes, ac_cv_cxx_namespaces=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_namespaces" = yes; then
  AC_DEFINE(HAVE_NAMESPACES,,[define if the compiler implements namespaces])
fi
])
dnl @synopsis AC_CXX_HAVE_SSTREAM
dnl
dnl If sstream (part of Standard C++ Library) exists
dnl define HAVE_SSTREAM.
dnl
dnl @version ac_cxx_have_std.m4 Tue Mar 28 18:20:26 CEST 2000
dnl @author Thomas Sondergaard thomass@deltadata.dk
dnl
AC_DEFUN([AC_CXX_HAVE_SSTREAM],
[AC_CACHE_CHECK(for sstream,
ac_cv_cxx_have_sstream,
[AC_REQUIRE([AC_CXX_NAMESPACES])
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <sstream>
#ifdef HAVE_NAMESPACES
using namespace std;
#endif],[return 0;],
 ac_cv_cxx_have_sstream=yes, ac_cv_cxx_have_sstream=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_have_sstream" = yes; then
  AC_DEFINE(HAVE_SSTREAM,1,[define if the compiler supports sstream])
fi
])
dnl @synopsis AC_CXX_HAVE_STD_IOSTREAM
dnl
dnl If std iostream (part of Standard C++ Library) exists
dnl define HAVE_STD_IOSTREAM.
dnl
dnl @version ac_cxx_have_std.m4 Tue Mar 28 18:20:26 CEST 2000
dnl @author Thomas Sondergaard thomass@deltadata.dk
dnl
AC_DEFUN([AC_CXX_HAVE_STD_IOSTREAM],
[AC_CACHE_CHECK(for std iostream,
ac_cv_cxx_have_std_iostream,
[AC_REQUIRE([AC_CXX_NAMESPACES])
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([
#include <sstream>
#include <streambuf>
#include <ios>
#ifdef HAVE_NAMESPACES
using namespace std;
#endif
],[return 0;],
 ac_cv_cxx_have_std_iostream=yes, ac_cv_cxx_have_std_iostream=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_have_std_iostream" = yes; then
  AC_DEFINE(HAVE_STD_IOSTREAM,,[define if the compiler has std compliant iostream library])
fi
])
dnl @synopsis FREECAD_AC_HAVE_QT(MINIMUM_VERSION)
dnl
dnl @summary Search for Trolltech's Qt GUI framework.
dnl
dnl Checks for the Qt4 library and its tools uic, moc and rcc.
dnl
dnl The following variables are exported:
dnl
dnl   QT_DIR
dnl   QT_CXXFLAGS
dnl   QT_LIBS
dnl   QT_MOC
dnl   QT_UIC
dnl   QT_RCC
dnl
AC_DEFUN([FREECAD_AC_HAVE_QT],
[

AC_ARG_WITH([qt4-dir],
             AC_HELP_STRING([--with-qt4-dir=DIR], [give prefix location of Qt4]),
             [fc_qt4_dir="$withval"], 
             [fc_qt4_dir="/usr/share/qt4"])

AC_ARG_WITH([qt4-include],
             AC_HELP_STRING([--with-qt4-include=DIR], [give include prefix of Qt4]),
             [fc_qt4_include="$withval"], 
             [fc_qt4_include="$fc_qt4_dir/include"])

AC_ARG_WITH([qt4-lib],
             AC_HELP_STRING([--with-qt4-lib=DIR], [give library path to Qt4]),
             [fc_qt4_lib="$withval"], 
             [fc_qt4_lib="$fc_qt4_dir/lib"])

AC_ARG_WITH([qt4-bin],
             AC_HELP_STRING([--with-qt4-bin=DIR], [give path to Qt4 utilities (moc, uic, rcc)]),
             [fc_qt4_bin="$withval"], 
             [fc_qt4_bin="$fc_qt4_dir/bin"])

AC_ARG_WITH([qt4-framework],
             AC_HELP_STRING([--with-qt4-framework], 
             [give prefix path to the Qt4 framework on Mac OS X]),
             [fc_qt4_frm="$withval"], 
             [fc_qt4_frm=""])

AC_MSG_CHECKING(for host)
AC_MSG_RESULT($host_os)
case $host_os in
  mingw32*)
    fc_qt4_lib_core="-L$fc_qt4_lib -lQtCore"
    QT_LIBS="-L$fc_qt4_lib -lopengl32 -lglu32 -lgdi32 -luser32 -lmingw32 -mthreads -Wl,-enable-stdcall-fixup -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc -Wl,-s -Wl,-s -Wl,-subsystem,windows"
    QT_CXXFLAGS="-I$fc_qt4_include -I$fc_qt4_include/QtCore -I$fc_qt4_include/QtGui -I$fc_qt4_include/QtOpenGL -I$fc_qt4_include/QtSvg -DUNICODE -DQT_LARGEFILE_SUPPORT -DQT_DLL -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_THREAD_SUPPORT -DQT_NEEDS_QMAIN -frtti -fexceptions"
    ;;
  darwin*)
    AC_PATH_XTRA
    if test -d $fc_qt4_frm/QtCore.framework; then
      ac_save_ldflags_fw=$LDFLAGS 
      LDFLAGS="$LDFLAGS -F$fc_qt4_frm -framework QtCore"
      AC_CACHE_CHECK(
        [whether Qt is installed as framework],
        ac_cv_have_qt_framework,
        [AC_TRY_LINK([#include <QtCore/qglobal.h>
                      #include <stdio.h>],
                 [printf("%s\n", qVersion());],
                 [ac_cv_have_qt_framework=yes],
                 [ac_cv_have_qt_framework=no])
        ])
      LDFLAGS=$ac_save_ldflags_fw
    else
      ac_cv_have_qt_framework=no
    fi
    if test "$ac_cv_have_qt_framework" = yes; then
    # Qt as framework installed 
    fc_qt4_lib_core="-Wl,-F$fc_qt4_frm -Wl,-framework,QtCore"
    QT_LIBS="-Wl,-F$fc_qt4_frm"
    #QT_LIBS="$QT_LIBS -Wl,-framework,Qt3Support"
    QT_LIBS="$QT_LIBS -Wl,-framework,QtGui"
    QT_LIBS="$QT_LIBS -Wl,-framework,QtOpenGL"
    QT_LIBS="$QT_LIBS -Wl,-framework,QtCore"
    QT_LIBS="$QT_LIBS -Wl,-framework,QtNetwork"
    QT_LIBS="$QT_LIBS -Wl,-framework,QtXml"
    QT_LIBS="$QT_LIBS -Wl,-framework,QtSql"
    QT_LIBS="$QT_LIBS -Wl,-framework,QtSvg"
	# Separated libs
	QT4_CORE_LIBS="-Wl,-F$fc_qt4_frm -Wl,-framework,QtCore"

    QT_CXXFLAGS="-F$fc_qt4_frm -I$fc_qt4_frm/Qt3Support.framework/Headers"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_frm/QtGui.framework/Headers"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_frm/QtCore.framework/Headers"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_frm/QtOpenGL.framework/Headers"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_frm/QtNetwork.framework/Headers"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_frm/QtSvg.framework/Headers"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_frm/QtXml.framework/Headers"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_frm/QtUiTools.framework/Headers"
	# Separated flags
	QT4_CORE_CXXFLAGS="-F$fc_qt4_frm -I$fc_qt4_frm/QtCore.framework/Headers"
    # QtUiTools doesn't seem to be available as framework
    #QT_CXXFLAGS="$QT_CXXFLAGS -I/usr/include/QtUiTools"
    # QtWebKit check
    fc_ac_save_cppflags=$CPPFLAGS
    CPPFLAGS="$QT_CXXFLAGS -I$fc_qt4_frm/QtWebKit.framework/Headers"
    AC_MSG_CHECKING([whether QtWebKit is available])
    AC_TRY_COMPILE([#include <QWebView>], [],
        [
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_frm/QtWebKit.framework/Headers"
    QT_LIBS="$QT_LIBS -Wl,-framework,QtWebKit"
        AC_MSG_RESULT(yes)],
        AC_MSG_RESULT(no))
    CPPFLAGS=$fc_ac_save_cppflags
    else
    # Qt not as framework installed 
    fc_qt4_lib_core="-L$fc_qt4_lib -lQtCore"
    QT_LIBS="-L$fc_qt4_lib -lQtCore -lQtGui -lQt3Support -lQtNetwork -lQtOpenGL -lQtSvg -lQtXml"
    QT_CXXFLAGS="-I$fc_qt4_include -I$fc_qt4_include/Qt3Support"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtGui"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtCore"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtOpenGL"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtNetwork"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtSvg"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtXml"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtUiTools"
	# Separated flags and libs
    QT4_CORE_CXXFLAGS="-I$fc_qt4_include -I$fc_qt4_include/QtCore"
    QT4_CORE_LIBS="-L$fc_qt4_lib -lQtCore"
    # QtWebKit check
    fc_ac_save_cppflags=$CPPFLAGS
    CPPFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtWebKit"
    AC_MSG_CHECKING([whether QtWebKit is available])
    AC_TRY_COMPILE([#include <QWebView>], [],
        [
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtWebKit"
    QT_LIBS="$QT_LIBS -lQtWebKit"
        AC_MSG_RESULT(yes)],
        AC_MSG_RESULT(no))
    CPPFLAGS=$fc_ac_save_cppflags
    fi
    ;;
  *)  # UNIX/Linux based
    AC_PATH_XTRA
    fc_qt4_lib_core="-L$fc_qt4_lib -lQtCore"
    QT_LIBS="-L$fc_qt4_lib -lQtCore -lQtGui -lQt3Support -lQtNetwork -lQtOpenGL -lQtSvg -lQtXml $X_LIBS -lX11 -lXext -lXmu -lXt -lXi $X_EXTRA_LIBS"
    QT_CXXFLAGS="-I$fc_qt4_include -I$fc_qt4_include/Qt3Support"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtGui"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtCore"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtOpenGL"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtNetwork"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtSvg"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtXml"
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtUiTools $X_CFLAGS"
    # QtWebKit check
    fc_ac_save_cppflags=$CPPFLAGS
    CPPFLAGS="-I$fc_qt4_include -I$fc_qt4_include/QtWebKit"
    AC_MSG_CHECKING([whether QtWebKit is available])
    AC_TRY_LINK([#include <QWebView>], [],
        [
    QT_CXXFLAGS="$QT_CXXFLAGS -I$fc_qt4_include/QtWebKit"
    QT_LIBS="$QT_LIBS -lQtWebKit"
        AC_MSG_RESULT(yes)],
        AC_MSG_RESULT(no))
    CPPFLAGS=$fc_ac_save_cppflags
    #QT4_CXXFLAGS="-I$fc_qt4_include"
    #QT4_LIBS="-L$fc_qt4_lib $X_LIBS -lX11 -lXext -lXmu -lXt -lXi $X_EXTRA_LIBS"
	# Separated flags and libs
    QT4_CORE_CXXFLAGS="-I$fc_qt4_include -I$fc_qt4_include/QtCore"
    QT4_CORE_LIBS="-L$fc_qt4_lib -lQtCore"
    #QT4_GUI_CXXFLAGS="-I$fc_qt4_include/QtGui"
    #QT4_GUI_LIBS="-lQtGui"
    #QT4_NETWORK_CFLAGS="-I$fc_qt4_include/QtNetwork"
    #QT4_NETWORK_LIBS="-lQtNetwork"
    #QT4_XML_CFLAGS="-I$fc_qt4_include/QtXml"
    #QT4_XML_LIBS="-lQtXml"
    ;;
esac

min_qt_version=ifelse([$1], ,4.0.0, $1)

AC_MSG_CHECKING(for Qt >= $min_qt_version)
QT_MOC="$fc_qt4_bin/moc"
QT_UIC="$fc_qt4_bin/uic"
QT_RCC="$fc_qt4_bin/rcc"

# Now we check whether we can actually build a Qt app.
cat > myqt.h << EOF
#include <QObject>
class Test : public QObject
{
Q_OBJECT
public:
  Test() {}
  ~Test() {}
public slots:
  void receive() {}
signals:
  void send();
};
EOF

cat > myqt.cpp << EOF
#include "myqt.h"
#include <QCoreApplication>
#include <QByteArray>
#include <QGlobalStatic>
#include <QStringList>
#include <stdio.h>
#include <stdlib.h>
int main( int argc, char **argv )
{
  QCoreApplication app( argc, argv );
  Test t;
  QObject::connect( &t, SIGNAL(send()), &t, SLOT(receive()) );

  // major, minor, patch
  QString version = "$min_qt_version";
  QStringList numbers = version.split('.');

  int shift[[3]] = {16,8,0};
  int minversion = 0, i = 0;
  for (QStringList::ConstIterator it = numbers.begin(); it != numbers.end(); ++it, ++i) {
    bool ok;
    int val = (*it).toInt(&ok);
    if (ok) {
      minversion = minversion + (val << shift[[i]]);
    }
  }

  exit(QT_VERSION < minversion);
}
EOF

bnv_try_1="$QT_MOC myqt.h -o moc_myqt.cpp"
AC_TRY_EVAL(bnv_try_1)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR([Cannot find Qt meta object compiler (moc), bye...])
fi

bnv_try_2="$CXX $QT_CXXFLAGS -c $CXXFLAGS -o moc_myqt.o moc_myqt.cpp"
AC_TRY_EVAL(bnv_try_2)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR([Failed to compile source file created by Qt meta object compiler (moc), bye...])
fi

bnv_try_3="$CXX $QT_CXXFLAGS -c $CXXFLAGS -o myqt.o myqt.cpp"
AC_TRY_EVAL(bnv_try_3)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR([Failed to compile Qt test app, bye...])
fi

# Make sure not to link against X11 libs so that configure succeeds whithout xserver started
bnv_try_4="$CXX myqt.o moc_myqt.o $fc_qt4_lib_core $LIBS -o myqt"
AC_TRY_EVAL(bnv_try_4)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR([Failed to link with Qt, bye...])
fi

AS_IF([AM_RUN_LOG([./myqt])],
      [AC_MSG_RESULT(yes)], 
      [AC_MSG_ERROR([Version of Qt4 found but < $min_qt_version])
])

rm -f moc_myqt.cpp myqt.h myqt.cpp myqt.o myqt moc_myqt.o

if test -d $fc_qt4_dir; then
   QT_DIR="$fc_qt4_dir"
else
   QT_DIR=""
fi

AC_SUBST(QT_DIR)
AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LIBS)
AC_SUBST(QT4_CORE_CXXFLAGS)
AC_SUBST(QT4_CORE_LIBS)
AC_SUBST(QT_UIC)
AC_SUBST(QT_MOC)
AC_SUBST(QT_RCC)
])
dnl @synopsis FREECAD_AC_HAVE_BOOST
dnl
dnl @summary Search for boost header and library files.
dnl
dnl
AC_DEFUN([FREECAD_AC_HAVE_BOOST],
[
AC_MSG_CHECKING(for boost)

AC_ARG_WITH(boost-include,
	AC_HELP_STRING([--with-boost-include=DIR], [Path to the boost header files]),
	[fc_boost_incs=$withval], [fc_boost_incs=/usr/include])

AC_ARG_WITH(boost-lib,
	AC_HELP_STRING([--with-boost-lib=DIR], [Path to the boost library files]),
	[fc_boost_libs=$withval], [fc_boost_libs=/usr/lib])

fc_boost_ac_save_cppflags=$CPPFLAGS
fc_boost_ac_save_ldflags=$LDFLAGS
fc_boost_ac_save_libs=$LIBS
CPPFLAGS="$CPPFLAGS -I$fc_boost_incs"
LDFLAGS="$LDFLAGS -L$fc_boost_libs"
LIBS="-lboost_program_options-mt"

AC_TRY_LINK([#include <boost/program_options.hpp>],
	[namespace po = boost::program_options;
	 po::options_description generic("Generic options");
	 generic.add_options()
	     ("version,v", "print version string")
	     ("help", "produce help message");
	],
	[AC_MSG_RESULT(yes)],
	[AC_MSG_ERROR(failed)])



AC_MSG_CHECKING(for boost >= 1.35.0)

cat > boost.cpp << EOF
#include <boost/version.hpp>
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv)
{
  exit(BOOST_VERSION >= 103500);
}
EOF

# Depending on boost version decide if boost_system is required
boost_try="$CXX boost.cpp $CPPFLAGS -o boost"
AC_TRY_EVAL(boost_try)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR([Failed to get version of boost, bye...])
fi

AS_IF([AM_RUN_LOG([./boost])],
      [ac_cv_boost_system=no], 
      [ac_cv_boost_system=yes
])
AC_MSG_RESULT($ac_cv_boost_system)
rm -f boost.cpp boost

BOOST_FILESYSTEM_LIB="-lboost_filesystem-mt"
BOOST_PROGOPTIONS_LIB="-lboost_program_options-mt"
BOOST_SIGNALS_LIB="-lboost_signals-mt"
BOOST_SYSTEM_LIB=""
BOOST_REGEX_LIB="-lboost_regex-mt"
if test x"$ac_cv_boost_system" = xyes; then
    LIBS="-lboost_system-mt"
    AC_MSG_CHECKING(for boost system library)
    AC_TRY_LINK([#include <boost/system/error_code.hpp>],
        [ boost::system::error_code error_code; std::string message(error_code.message()); return 0; ],
        [BOOST_SYSTEM_LIB="-lboost_system-mt"],
        [BOOST_SYSTEM_LIB=""])
    
    if test "x$BOOST_SYSTEM_LIB" = "x"; then
        AC_MSG_RESULT(no)
        AC_MSG_ERROR(Unable to link with the boost::system library)
    else
        AC_MSG_RESULT(yes)
    fi
fi

AC_SUBST(BOOST_FILESYSTEM_LIB)
AC_SUBST(BOOST_PROGOPTIONS_LIB)
AC_SUBST(BOOST_SIGNALS_LIB)
AC_SUBST(BOOST_SYSTEM_LIB)
AC_SUBST(BOOST_REGEX_LIB)


CPPFLAGS=$fc_boost_ac_save_cppflags
LDFLAGS=$fc_boost_ac_save_ldflags
LIBS=$fc_boost_ac_save_libs

all_includes="$all_includes -I$fc_boost_incs"
all_libraries="$all_libraries -L$fc_boost_libs"
])
