dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.50)

dnl Init stuff
dnl **************************************************************************
dnl release version number info
m4_include([revision.m4])

AC_INIT(FreeCAD,[FREECAD_MAJOR.FREECAD_MINOR.FREECAD_MICRO],[wmayer@users.sourceforge.net], FreeCAD)
AC_CONFIG_AUX_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar foreign])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG_CPLUSPLUS

dnl Default install directory is your home directory
dnl **************************************************************************
AC_PREFIX_DEFAULT($HOME/FreeCAD)

dnl Version number
dnl **************************************************************************
dnl Shared library versioning
dnl GENERIC_LIBRARY_VERSION=1:2:0
dnl                         | | |
dnl                  +------+ | +---+
dnl                  |        |     |
dnl               current:revision:age
dnl                  |        |     |
dnl                  |        |     +- increment if interfaces have been added
dnl                  |        |        set to zero if interfaces have been removed
dnl                  |        |        or changed
dnl                  |        +- increment if source code has changed
dnl                  |           set to zero if current is incremented
dnl                  +- increment if interfaces have been added, removed or changed

LIB_CURRENT=2
LIB_REVISION=0
LIB_AGE=0
AC_SUBST(LIB_CURRENT)
AC_SUBST(LIB_REVISION)
AC_SUBST(LIB_AGE)

dnl Required headers
dnl (mainly for OpenCASCADE 5.2, we need also config.h for cfg's)
dnl **************************************************************************
AC_LANG([C++])
AC_MSG_CHECKING([ for C++ header files ])
AC_CHECK_HEADERS(istream ostream istream fstream ios iomanip iostream)
AC_CHECK_HEADERS(iomanip.h limits.h values.h float.h)
AC_CHECK_HEADERS(siginfo.h bits/sigset.h bstring.h sys/types.h sys/select.h)
AC_CHECK_HEADERS(sys/filio.h sys/mman.h libc.h)
AC_CHECK_HEADERS([sstream],,[AC_MSG_ERROR([This header is needed. Bye.])])

AC_CXX_HAVE_STD_IOSTREAM

dnl Checking for C/C++ compiler
dnl **************************************************************************
AC_PROG_CXX
AC_PROG_CC
AC_ENABLE_SHARED(yes)
AC_ENABLE_STATIC(no)

dnl Checking for Fortran compiler
dnl **************************************************************************
AC_PROG_F77()
AC_FC_SRCEXT(f)
AC_FC_LIBRARY_LDFLAGS
AC_FC_WRAPPERS

dnl Checking for programs
dnl **************************************************************************
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_PROG_LN_S
#AC_PROG_YACC
#AM_PROG_LEX


AC_PATH_PROG(SWIG, swig, false)
if test "$SWIG" = false ; then
   AC_MSG_WARN([Can't find SWIG installation])
   HAVE_SWIG=0
   AM_CONDITIONAL(HAVE_SWIG_FOUND, false)
else
   HAVE_SWIG=1
   AM_CONDITIONAL(HAVE_SWIG_FOUND, true)
fi
AC_SUBST(HAVE_SWIG)

dnl Defines in config.h
dnl **************************************************************************
AC_DEFINE_UNQUOTED(HAVE_GETENVIRONMENTVARIABLE, 1, [Define to use GetEnvironmentVariable() instead of getenv()])
AC_DEFINE_UNQUOTED(HAVE_GL_GL_H, 1, [define if the GL header should be included as GL/gl.h])
AC_DEFINE_UNQUOTED(HAVE_QGLFORMAT_EQ_OP, 1, [Define this to 1 if operator==(QGLFormat&, QGLFormat&) is available])
AC_DEFINE_UNQUOTED(HAVE_QGLFORMAT_SETOVERLAY, 1, [Define this to 1 if QGLFormat::setOverlay() is available])
AC_DEFINE_UNQUOTED(HAVE_QGLWIDGET_SETAUTOBUFFERSWAP, 1, [Define this to 1 if QGLWidget::setAutoBufferSwap() is available])
AC_DEFINE_UNQUOTED(HAVE_QT_KEYPAD_DEFINE, 1, [Define this if Qt::Keypad is available])
AC_DEFINE_UNQUOTED(HAVE_QWIDGET_SHOWFULLSCREEN, 1, [Define this if QWidget::showFullScreen() is available])
AC_DEFINE_UNQUOTED(HAVE_SYS_TYPES_H, 1, [Define to 1 if you have the <sys/types.h> header file.])
AC_DEFINE_UNQUOTED(USE_STD_IOSTREAM, 1, [Define to 1 to build zipios++ sources with iostream.])
AC_DEFINE_UNQUOTED(OCE_HAVE_CLIMITS, 1, [Define to 1 to build with OCE instead of OCC.])
AC_DEFINE_UNQUOTED(OCE_HAVE_IOSTREAM, 1, [Define to 1 to build with OCE instead of OCC.])
AC_DEFINE_UNQUOTED(OCE_HAVE_IOMANIP, 1, [Define to 1 to build with OCE instead of OCC.])

dnl Check if you want to use GUI, or not (currently disabled)
dnl **************************************************************************
dnl
#AC_ARG_ENABLE([gui],
#  AC_HELP_STRING([--enable-gui], [Enable GUI (you can disable this feature to use FreeCAD in server mode)]),
#  [case $enableval in
#    no | false) fc_set_gui=false ;;
#    *)          fc_set_gui=true ;;
#  esac],
#  [fc_set_gui=true])
#
#if $fc_set_gui; then
#	echo "not yet done"
#fi
#
#if test "x$use_glx" = "xyes"; then
#  AM_CONDITIONAL(FREECAD_BUILD_GUI, true)
#else
#  AM_CONDITIONAL(FREECAD_BUILD_GUI, false)
#fi
#if FREECAD_BUILD_GUI
# ... in Makefile.am
#endif

## DEFAULT INCLUDE/LIB PATHS
#all_includes="$all_includes -I/usr/include -I/usr/local/include"
#all_libraries="$all_libraries -L/usr/lib -L/usr/local/lib"

dnl *************************************************************************
dnl
dnl Checking for libraries.
dnl
dnl *************************************************************************

dnl checking for zlib
dnl **************************************************************************
AC_CHECK_LIB(z, inflate, [cv_libz=yes], [cv_libz=no])

if test "$cv_libz" != "yes"; then
  AC_MSG_ERROR([
  **** Cannot find the zlib library. ****
  ])
fi

dnl checking for Python
dnl **************************************************************************
dnl AC_MSG_CHECKING([for Python])
dnl fc_py_ver=`python -c "import sys; print sys.version[[:3]]"`;
dnl if test x$fc_py_ver = x; then
dnl 	AC_MSG_ERROR([
dnl 	**** Cannot find Python interpreter. ****
dnl   ])
dnl fi;
dnl AC_MSG_RESULT([yes])
dnl 
dnl dnl Additional test to force version number of >= 2.5
dnl AC_MSG_CHECKING([for Python version >= 2.5])
dnl prog="import sys
dnl s=0x02050000
dnl sys.exit(sys.hexversion < s)"
dnl AS_IF([AM_RUN_LOG([python -c "$prog"])],
dnl       [AC_MSG_RESULT([yes])], 
dnl       [AC_MSG_ERROR([
dnl 	**** Install Python version 2.5 or later ****
dnl       ])])

AC_ARG_WITH(python-version,
	AC_HELP_STRING([--with-python-version=VER], [Choose the required Python version]),
	[fc_py_ver=$withval],
	[fc_py_ver=2.5])

dnl At least version 2.5 required
AM_PATH_PYTHON($fc_py_ver)


AC_ARG_WITH(python-include,
	AC_HELP_STRING([--with-python-include=DIR], [Path to the Python header files]),
	[fc_py_incs=$withval],
	[fc_py_incs=`$PYTHON -c "import distutils.sysconfig; print distutils.sysconfig.get_python_inc()"`])

AC_ARG_WITH(python-lib,
	AC_HELP_STRING([--with-python-lib=DIR], [Path to the Python library files]),
	[fc_py_libs=$withval],
	[fc_py_libs=`$PYTHON -c "import distutils.sysconfig; print distutils.sysconfig.get_config_var('LIBDIR')"`])

fc_py_ac_save_cppflags=$CPPFLAGS
fc_py_ac_save_ldflags=$LDFLAGS
fc_py_ac_save_libs=$LIBS
CPPFLAGS="$CPPFLAGS -I$fc_py_incs"
LDFLAGS="$LDFLAGS -L$fc_py_libs"
LIBS="-lpython$PYTHON_VERSION -lpthread -ldl -lutil -lm"

dnl Small test program that only works with Python 2.5 and higher
fc_cv_lib_py_avail=no
AC_CHECK_HEADER(Python.h,[
AC_MSG_CHECKING([for libpython$PYTHON_VERSION])
AC_TRY_LINK([#include "Python.h"],
	[Py_Initialize(); 
	 const char* sys = "sys"; 
	 PyImport_AddModule(sys);
	 Py_Finalize();],
	[fc_cv_lib_py_avail=yes
	AC_MSG_RESULT(yes)],
	AC_MSG_RESULT(no))])

CPPFLAGS=$fc_py_ac_save_cppflags
LDFLAGS=$fc_py_ac_save_ldflags
LIBS=$fc_py_ac_save_libs

if test x"$fc_cv_lib_py_avail" = xyes; then
	all_includes="$all_includes -I$fc_py_incs"
	all_libraries="$all_libraries -L$fc_py_libs"

	AC_SUBST([PYTHON_LIB], [python$PYTHON_VERSION])
else
	AC_MSG_ERROR([
	**** Cannot find Python$PYTHON_VERSION devel files. ****
	])
fi


dnl checking for PyQt4 utilities
dnl **************************************************************************
#AC_PATH_PROG(PYUIC4, pyuic4, false)
#if test "$PYUIC4" = false ; then
#   AC_MSG_ERROR([Can't find pyuic4 utility])
#fi
#AC_PATH_PROG(PYRCC4, pyrcc4, true)
#if test "$PYRCC4" = false ; then
#   AC_MSG_ERROR([Can't find pyrcc4 utility])
#fi
#AC_SUBST(PYUIC4)
#AC_SUBST(PYRCC4)


dnl checking for PyCXX & zipios++
dnl **************************************************************************
fc_make_no_dfsg_package=yes
AC_ARG_ENABLE(no-dfsg-tarball,
	AC_HELP_STRING([--enable-no-dfsg-tarball], [Use local sources of PyCXX & zipios++ packages [[default=yes]]]),
	[fc_make_no_dfsg_package=$enableval],[fc_make_no_dfsg_package=yes])
# check if local versions of PyCXX & zipios++ are part of these sources
if test x"$fc_make_no_dfsg_package" = xyes; then
    fc_py_ac_save_cppflags=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS -I$srcdir -I$srcdir/src"
    AC_TRY_COMPILE([#include "src/CXX/Version.hxx"], [],
        [], [fc_make_no_dfsg_package=no])
    CPPFLAGS=$fc_py_ac_save_cppflags
fi
AM_CONDITIONAL(MAKE_NO_DFSG_PACKAGE, test x"$fc_make_no_dfsg_package" = xyes)
AC_MSG_RESULT(Use local sources of PyCXX & zipios++... $fc_make_no_dfsg_package)


dnl checking for existence of (system-wide) PyCXX & zipios++ headers
dnl **************************************************************************
ZIPIOS_LIB=""
if test x"$fc_make_no_dfsg_package" != xyes; then
    fc_py_ac_save_cppflags=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS -I$fc_py_incs"
    ZIPIOS_LIB="-lzipios"
    AC_MSG_CHECKING(for PyCXX headers)
    AC_TRY_COMPILE([#include <CXX/Version.hxx>], [],
        [AC_MSG_RESULT(yes)],
        [AC_MSG_RESULT(no);AC_MSG_ERROR(Cannot find the PyCXX headers)])
    AC_MSG_CHECKING(for zipios++ headers)
    AC_TRY_COMPILE([#include <zipios++/zipios_defs.h>], [],
        [AC_MSG_RESULT(yes)],
        [AC_MSG_RESULT(no);AC_MSG_ERROR(Cannot find the zipios++ headers)])
    CPPFLAGS=$fc_py_ac_save_cppflags
fi
AC_SUBST(ZIPIOS_LIB)


dnl checking for xerces-c
dnl **************************************************************************
AC_MSG_CHECKING([for xerces-c])
AC_ARG_WITH(xercesc-include,
	AC_HELP_STRING([--with-xercesc-include=DIR], [Path to the xerces-c header files]),
	[fc_xer_incs=$withval],
	[fc_xer_incs=/usr/include])

AC_CHECK_FILE($fc_xer_incs/xercesc,,
	[AC_CHECK_FILE(/usr/local/include/xercesc,[fc_xer_incs=/usr/local/include])])

AC_ARG_WITH(xercesc-lib,
	AC_HELP_STRING([--with-xercesc-lib=DIR], [Path to the xerces-c library files]),
	[fc_xer_libs=$withval],
	[fc_xer_libs=/usr/lib])

fc_xer_ac_save_cppflags=$CPPFLAGS
fc_xer_ac_save_ldflags=$LDFLAGS
fc_xer_ac_save_libs=$LIBS
CPPFLAGS="$CPPFLAGS -I$fc_xer_incs"
LDFLAGS="$LDFLAGS -L$fc_xer_libs"
LIBS="-lxerces-c"

fc_cv_lib_xer_avail=no
AC_CHECK_HEADER(xercesc/framework/XMLBuffer.hpp,[
AC_MSG_CHECKING([whether xerces lib is available])
AC_TRY_LINK([#include <xercesc/framework/XMLBuffer.hpp>],
	[XERCES_CPP_NAMESPACE_USE
	 XMLBuffer buf;],
	[fc_cv_lib_xer_avail=yes
	AC_MSG_RESULT(yes)],
	AC_MSG_RESULT(no))])

CPPFLAGS=$fc_xer_ac_save_cppflags
LDFLAGS=$fc_xer_ac_save_ldflags
LIBS=$fc_xer_ac_save_libs

if test x"$fc_cv_lib_xer_avail" = xyes; then
	all_includes="$all_includes -I$fc_xer_incs"
	all_libraries="$all_libraries -L$fc_xer_libs"
else
	AC_MSG_ERROR([
	**** Cannot find xerces devel files. ****
   ])
fi


dnl checking for ANN
dnl **************************************************************************
dnl AC_MSG_CHECKING([for ANN])
dnl AC_ARG_WITH(ann-include,
dnl 	AC_HELP_STRING([--with-ann-include=DIR], [Path to the ANN header files]),
dnl 	[fc_ann_incs=$withval],
dnl 	[fc_ann_incs=/usr/include])
dnl 
dnl AC_CHECK_FILE($fc_ann_incs/ANN,,
dnl 	[AC_CHECK_FILE(/usr/local/include/ANN,[fc_ann_incs=/usr/local/include])])
dnl 
dnl AC_ARG_WITH(ann-lib,
dnl 	AC_HELP_STRING([--with-ann-lib=DIR], [Path to the ANN library files]),
dnl 	[fc_ann_libs=$withval],
dnl 	[fc_ann_libs=/usr/lib])
dnl 
dnl fc_ann_ac_save_cppflags=$CPPFLAGS
dnl fc_ann_ac_save_ldflags=$LDFLAGS
dnl fc_ann_ac_save_libs=$LIBS
dnl CPPFLAGS="$CPPFLAGS -I$fc_ann_incs"
dnl LDFLAGS="$LDFLAGS -L$fc_ann_libs"
dnl LIBS="-lann"
dnl 
dnl fc_cv_lib_ann_avail=no
dnl AC_CHECK_HEADER(ANN/ANN.h,[
dnl AC_MSG_CHECKING([whether ANN lib is available])
dnl AC_TRY_LINK([#include <ANN/ANN.h>],
dnl 	[ANNkd_tree ann;],
dnl 	[fc_cv_lib_ann_avail=yes
dnl 	AC_MSG_RESULT(yes)],
dnl 	AC_MSG_RESULT(no))])
dnl 
dnl CPPFLAGS=$fc_ann_ac_save_cppflags
dnl LDFLAGS=$fc_ann_ac_save_ldflags
dnl LIBS=$fc_ann_ac_save_libs
dnl 
dnl if test x"$fc_cv_lib_ann_avail" = xyes; then
dnl 	all_includes="$all_includes -I$fc_ann_incs"
dnl 	all_libraries="$all_libraries -L$fc_ann_libs"
dnl else
dnl 	AC_MSG_ERROR([
dnl 	**** Cannot find ANN devel files. ****
dnl    ])
dnl fi


dnl checking for eigen2
dnl **************************************************************************
dnl AC_MSG_CHECKING([for eigen2])
dnl AC_ARG_WITH(eigen2-include,
dnl 	AC_HELP_STRING([--with-eigen2-include=DIR], [Path to the eigen2 header files]),
dnl 	[fc_eig_incs=$withval],
dnl 	[fc_eig_incs=/usr/include/eigen2])
dnl 
dnl AC_CHECK_FILE($fc_eig_incs/Eigen,,
dnl 	[AC_CHECK_FILE(/usr/local/include/eigen2,[fc_eig_incs=/usr/local/include/eigen2])])
dnl 
dnl fc_eig_ac_save_cppflags=$CPPFLAGS
dnl CPPFLAGS="$CPPFLAGS -I$fc_eig_incs"
dnl 
dnl # Check a file which is present in Eigen2 but not in Eigen3
dnl fc_cv_lib_eig_avail=no
dnl AC_CHECK_HEADER(Eigen/NewStdVector,
dnl 	fc_cv_lib_eig_avail=yes,)
dnl 
dnl CPPFLAGS=$fc_eig_ac_save_cppflags
dnl 
dnl if test x"$fc_cv_lib_eig_avail" = xyes; then
dnl 	AC_SUBST([EIGEN2_INC], [$fc_eig_incs])
dnl else
dnl 	AC_MSG_WARN([
dnl 	**** Cannot find eigen2 devel files.
dnl 	Modules that depend on this library cannot be built. ****
dnl    ])
dnl fi

dnl AM_CONDITIONAL(HAVE_EIGEN2, test x"$fc_cv_lib_eig_avail" = xyes)


dnl checking for eigen3
dnl **************************************************************************
AC_MSG_CHECKING([for eigen3])
AC_ARG_WITH(eigen3-include,
	AC_HELP_STRING([--with-eigen3-include=DIR], [Path to the eigen3 header files]),
	[fc_eig_incs=$withval],
	[fc_eig_incs=/usr/include/eigen3])

AC_CHECK_FILE($fc_eig_incs/Eigen,,
	[AC_CHECK_FILE(/usr/local/include/eigen3,[fc_eig_incs=/usr/local/include/eigen3])])

fc_eig_ac_save_cppflags=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -I$fc_eig_incs"

# Check a file which is present in Eigen3 but not in Eigen2
fc_cv_lib_eig_avail=no
AC_CHECK_HEADER(Eigen/Eigenvalues,
	fc_cv_lib_eig_avail=yes,)

CPPFLAGS=$fc_eig_ac_save_cppflags

if test x"$fc_cv_lib_eig_avail" = xyes; then
	AC_SUBST([EIGEN3_INC], [$fc_eig_incs])
else
	AC_MSG_WARN([
	**** Cannot find eigen3 devel files.
	Modules that depend on this library cannot be built. ****
   ])
fi

AM_CONDITIONAL(HAVE_EIGEN3, test x"$fc_cv_lib_eig_avail" = xyes)

dnl checking for boost
dnl ******************

FREECAD_AC_HAVE_BOOST


dnl checking for Qt
dnl ***************

FREECAD_AC_HAVE_QT(4.3)


dnl checking for Coin
dnl **************************************************************************

SIM_AC_HAVE_COIN_IFELSE(,AC_MSG_ERROR([
	**** Cannot find Coin devel files. ****
   ]))

AC_SUBST([sim_ac_coin_includedir])
AC_SUBST([sim_ac_coin_cppflags])
AC_SUBST([sim_ac_coin_ldflags])
AC_SUBST([sim_ac_coin_libs])

dnl checking for SoQt
dnl **************************************************************************

SIM_AC_HAVE_SOQT_IFELSE(,AC_MSG_ERROR([
	**** Cannot find SoQt devel files. ****
   ]))

AC_SUBST([sim_ac_soqt_includedir])
AC_SUBST([sim_ac_soqt_cppflags])
AC_SUBST([sim_ac_soqt_ldflags])
AC_SUBST([sim_ac_soqt_libs])

dnl **************************************************************************
dnl checking for optional libraies
dnl **************************************************************************

dnl checking for OpenGL libs
dnl **************************************************************************
case $host_os in
  mingw32*)
    GL_LIBS="-lopengl32 -lglu32"
    ;;
  darwin*)
    GL_LIBS="-Wl,-F/System/Library/Frameworks -Wl,-framework,OpenGL"
    ;;
  linux*|kfreebsd*-gnu*)
    GL_LIBS="-lGL -lGLU"
    ;;
esac
AC_SUBST(GL_LIBS)

dnl checking for OpenCascade
dnl **************************************************************************
dnl Check if CASROOT is set and estimate where the include and libs could be
if test x"$CASROOT" != x; then
  fc_occ_incs_test="$CASROOT/inc"
  fc_occ_libs_test="$CASROOT/Linux/lib"
else
  fc_occ_incs_test=/usr/include/opencascade
  fc_occ_libs_test=/usr/lib
fi

AC_CHECKING([OpenCascade])
AC_ARG_WITH(occ-include,
	AC_HELP_STRING([--with-occ-include=DIR], [Path to the OpenCascade header files]),
	[fc_occ_incs=$withval],
	[fc_occ_incs=$fc_occ_incs_test])

AC_ARG_WITH(occ-lib,
	AC_HELP_STRING([--with-occ-lib=DIR], [Path to the OpenCascade library files]),
	[fc_occ_libs=$withval],
	[fc_occ_libs=$fc_occ_libs_test])

fc_occ_ac_save_cppflags=$CPPFLAGS
fc_occ_ac_save_ldflags=$LDFLAGS
fc_occ_ac_save_libs=$LIBS
CPPFLAGS="$CPPFLAGS -I$fc_occ_incs"
LDFLAGS="$LDFLAGS -L$fc_occ_libs"
LIBS="-ldl -lTKernel"

fc_cv_lib_occ_avail=no
AC_CHECK_HEADER(Standard.hxx,[
AC_MSG_CHECKING([whether OCC libs are available])
AC_TRY_LINK([#define HAVE_IOSTREAM 1
	#include <gp_Pnt.hxx>],
	[gp_Pnt pt(0,0,0);],
	[fc_cv_lib_occ_avail=yes
	AC_MSG_RESULT(yes)],
	AC_MSG_RESULT(no))])

# Checking for version >= 6.1
LIBS="-ldl -lTKernel -lTKMesh -lTKG2d -lTKG3d -lTKTopAlgo -lTKMath -lTKBRep -lTKGeomBase -lTKGeomAlgo"
fc_cv_lib_occ_ver_6=no
if test x"$fc_cv_lib_occ_avail" = xyes; then
	AC_CHECK_HEADER(Standard.hxx,[
	AC_MSG_CHECKING([whether OCC is >= 6.1])
	AC_TRY_LINK([#define HAVE_IOSTREAM 1
		#include <gp_Pnt.hxx>],
		[gp_Pnt pt(0,0,0);],
		[fc_cv_lib_occ_ver_6=yes
		AC_MSG_RESULT(yes)],
		AC_MSG_RESULT(no))])
fi

CPPFLAGS=$fc_occ_ac_save_cppflags
LDFLAGS=$fc_occ_ac_save_ldflags
LIBS=$fc_occ_ac_save_libs

if test x"$fc_cv_lib_occ_avail" = xyes; then
	AC_SUBST([OCC_INC], [$fc_occ_incs])
	AC_SUBST([OCC_LIB], [$fc_occ_libs])
else
	AC_MSG_WARN([
	**** Cannot find OpenCASCADE devel files.
	Modules that depend on this library cannot be built. ****
  ])
fi

AM_CONDITIONAL(HAVE_OPENCASCADE, test x"$fc_cv_lib_occ_avail" = xyes)
AM_CONDITIONAL(OCC_VERSION6, test x"$fc_cv_lib_occ_ver_6" = xyes)


dnl checking for Salome SMESH
dnl **************************************************************************
#AC_MSG_CHECKING([for Salome SMESH])
#AC_ARG_WITH(smesh-include,
#	AC_HELP_STRING([--with-smesh-include=DIR], [Path to the Salome SMESH header files]),
#	[fc_smesh_incs=$withval],
#	[fc_smesh_incs=/usr/include])
#
#AC_CHECK_FILE($fc_smesh_incs/SMESH_Gen.hxx,,
#	[AC_CHECK_FILE(/usr/local/include/smesh,[fc_smesh_incs=/usr/local/include])])
#
#AC_ARG_WITH(smesh-lib,
#	AC_HELP_STRING([--with-smesh-lib=DIR], [Path to the Salome SMESH library files]),
#	[fc_smesh_libs=$withval],
#	[fc_smesh_libs=/usr/lib])
#
#fc_smesh_ac_save_cppflags=$CPPFLAGS
#fc_smesh_ac_save_ldflags=$LDFLAGS
#fc_smesh_ac_save_libs=$LIBS
#CPPFLAGS="$CPPFLAGS -I$fc_smesh_incs -I$OCC_INC"
#LDFLAGS="$LDFLAGS -L$fc_smesh_libs"
#LIBS="-lSMESH"
#
#fc_cv_lib_smesh_avail=no
#AC_CHECK_HEADER(SMESH_Gen.hxx,[
#AC_MSG_CHECKING([whether SMESH lib is available])
#AC_TRY_LINK([#include <SMESH_Gen.hxx>],
#	[SMESH_Gen meshgen;],
#	[fc_cv_lib_smesh_avail=yes
#	AC_MSG_RESULT(yes)],
#	AC_MSG_RESULT(no))])
#
#CPPFLAGS=$fc_smesh_ac_save_cppflags
#LDFLAGS=$fc_smesh_ac_save_ldflags
#LIBS=$fc_smesh_ac_save_libs
#
#if test x"$fc_cv_lib_smesh_avail" = xyes; then
#	AC_SUBST([SMESH_INCLUDE], [$fc_smesh_incs])
#	AC_SUBST([SMESH_LIBRARY], [$fc_smesh_libs])
#else
#	AC_MSG_WARN([
#	**** Cannot find Salome SMESH devel files. ****
#   ])
#fi
#
#AM_CONDITIONAL(HAVE_SALOMESMESH, test x"$fc_cv_lib_smesh_avail" = xyes)

# check whether netgen support has to be enabled
AC_ARG_ENABLE(netgen,[--enable-netgen use netgen [default=no]],enable_netgen=$enableval,enable_netgen=no)
if test "$enable_netgen" = "yes"; then
   AC_CHECK_HEADERS([nglib.h])
fi
AM_CONDITIONAL(NETGEN, test "$enable_netgen" = "yes")

AC_TRY_COMPILE( [], 
 [int array[((int)sizeof(void*))-6];],
 [enable_stdmeshers_64bit="yes"],
 [enable_stdmeshers_64bit="no"] )
AM_CONDITIONAL(STDMESHERS64BIT, test "$enable_stdmeshers_64bit" = "yes")


dnl checking for spnav library (for 3d mouse support)
dnl **************************************************************************
AC_MSG_CHECKING([for spnav])
AC_ARG_WITH(spnav-include,
	AC_HELP_STRING([--with-spnav-include=DIR], [Path to the spnav header files]),
	[fc_spnav_incs=$withval],
	[fc_spnav_incs=/usr/include])

fc_spnav_ac_save_cppflags=$CPPFLAGS
CPPFLAGS="$CPPFLAGS -I$fc_spnav_incs"

fc_cv_lib_spnav_avail=no
AC_CHECK_HEADER(spnav.h,
	fc_cv_lib_spnav_avail=yes,)

CPPFLAGS=$fc_spnav_ac_save_cppflags

if test x"$fc_cv_lib_spnav_avail" = xyes; then
	AC_SUBST([SPNAV_INC], [$fc_spnav_incs])
else
	AC_MSG_WARN([
	**** Cannot find spnav devel files.
	No support for 3D SpaceNavigator. ****
   ])
fi

AM_CONDITIONAL(HAVE_SPNAV_FOUND, test x"$fc_cv_lib_spnav_avail" = xyes)


dnl checking for matplotlib
dnl **************************************************************************
fc_matplotlib_avail=no
AC_MSG_CHECKING([for matplotlib])
fc_matplotlib_ver=`python -c "import matplotlib as m; print m.__version__;"`;
if test x$fc_py_ver = x; then
	AC_MSG_WARN([
	**** Cannot find matplotlib Python module.
	Plot Module will not available until matplotlib is installed ****
    ])
else
	fc_matplotlib_avail=yes
fi;
AC_MSG_RESULT([yes])
AM_CONDITIONAL(HAVE_MATPLOTLIB, test x"$fc_matplotlib_avail" = xyes)

#---------------------------------------------------------------------
#
# Check if 64-bit platform
#  
#---------------------------------------------------------------------
AC_MSG_CHECKING([if platform is 64-bit (-D_OCC64)])
AC_TRY_COMPILE( [], 
 [int array[((int)sizeof(void*))-6];],
 [AC_MSG_RESULT([yes]); CPPFLAGS="$CPPFLAGS -D_OCC64"; case $platform in Linux*) CPPFLAGS="$CPPFLAGS -m64";; esac],
 [AC_MSG_RESULT([no])] )

#AC_MSG_CHECKING([if platform is 64-bit (-D_OCC64)]) 
#AC_TRY_COMPILE([],[int array[6-(int)sizeof(void*)];],[AC_MSG_RESULT(no)], 
#               [AC_MSG_RESULT(yes); OCC_INC_FLAG="$OCC_INC_FLAG -D_OCC64"])

dnl checking for WildMagic3
dnl **************************************************************************
dnl AC_CHECKING([WildMagic3])
dnl AC_ARG_WITH(wm3-include,
dnl 	AC_HELP_STRING([--with-wm3-include=DIR], [Path to the WildMagic3 header files]),
dnl 	[fc_wm3_incs=$withval],
dnl 	[fc_wm3_incs=/usr/include/WildMagic3/Include])
dnl 
dnl AC_ARG_WITH(wm3-lib,
dnl 	AC_HELP_STRING([--with-wm3-lib=DIR], [Path to the WildMagic3 library files]),
dnl 	[fc_wm3_libs=$withval],
dnl 	[fc_wm3_libs=/usr/lib])
dnl 
dnl AC_CHECK_FILE($fc_wm3_incs/Wm3Query.h,fc_wm3_ver=340,fc_wm3_ver=330)
dnl 
dnl Set the appropriate library name
dnl if test x"$fc_wm3_ver" = x"330"; then
dnl 	fc_wm3_lib="WildMagic3"
dnl 	fc_wm3_txt="Version <= 3.3.0"
dnl else
dnl 	fc_wm3_lib="Wm3Foundation"
dnl 	fc_wm3_txt="Version >= 3.4.0"
dnl fi
dnl 
dnl fc_wm3_ac_save_cppflags=$CPPFLAGS
dnl fc_wm3_ac_save_ldflags=$LDFLAGS
dnl fc_wm3_ac_save_libs=$LIBS
dnl CPPFLAGS="$CPPFLAGS -I$fc_wm3_incs"
dnl LDFLAGS="$LDFLAGS -L$fc_wm3_libs"
dnl LIBS="-l$fc_wm3_lib"
dnl 
dnl fc_cv_lib_wm3_avail=no
dnl AC_CHECK_HEADER(Wm3Math.h,[
dnl AC_MSG_CHECKING([whether WildMagic lib is available])
dnl AC_TRY_LINK([#include <Wm3Math.h>],
dnl 	[double val = Wm3::Math<double>::Cos(0.0);],
dnl 	[fc_cv_lib_wm3_avail=yes
dnl 	AC_MSG_RESULT(yes)],
dnl 	AC_MSG_RESULT(no))])
dnl 
dnl CPPFLAGS=$fc_wm3_ac_save_cppflags
dnl LDFLAGS=$fc_wm3_ac_save_ldflags
dnl LIBS=$fc_wm3_ac_save_libs
dnl 
dnl if test x"$fc_cv_lib_wm3_avail" = xyes; then
dnl 	AC_SUBST([WM3_CFLAGS], [-I$fc_wm3_incs])
dnl 	AC_SUBST([WM3_LDFLAGS], [-L$fc_wm3_libs])
dnl 	AC_SUBST([WM3_LIBS], [-l$fc_wm3_lib])
dnl 	AC_SUBST([WM3_VERSION], [$fc_wm3_ver])
dnl else
dnl 	AC_MSG_WARN([
dnl 	**** Cannot find WildMagic devel files.
dnl 	Modules that depend on this library cannot be built. ****
dnl   ])
dnl fi
dnl 
dnl AM_CONDITIONAL(WM3_LIBRARY_FILES, test x"$fc_cv_lib_wm3_avail" = xyes)

dnl checking for GTS
dnl **************************************************************************
dnl
dnl AC_PATH_PROG(GTSCONFIG, gts-config, false, $PATH)
dnl if test x"$GTSCONFIG" != xfalse; then
dnl 	fc_gts_incs=`gts-config --cflags`
dnl 	fc_gts_libs=`gts-config --libs`
dnl 
dnl 	AC_SUBST([GTS_CFLAGS], [$fc_gts_incs])
dnl 	AC_SUBST([GTS_LIBS], [$fc_gts_libs])
dnl else
dnl 	AC_MSG_WARN([
dnl 	**** Cannot find GTS library files.
dnl 	Modules that depend on this library cannot be built. ****
dnl   ])
dnl fi
dnl
dnl AM_CONDITIONAL(HAVE_GTS, test x"$GTSCONFIG" != xfalse)

dnl checking for OpenCV
dnl **************************************************************************

dnl HAVE_OPENCV=false
dnl PKG_CHECK_MODULES(opencv, opencv, [HAVE_OPENCV=true], [true])
dnl if test x$HAVE_OPENCV = xfalse; then
dnl 	AC_MSG_WARN([
dnl 	**** Cannot find OpenCV library files.
dnl 	Modules that depend on this library cannot be built. ****
dnl   ])
dnl fi
dnl
dnl AM_CONDITIONAL([HAVE_OPENCV], [test x$HAVE_OPENCV = xtrue])

dnl **************************************************************************

AC_SUBST(all_includes)
AC_SUBST(all_libraries)

dnl Check if you want to have log info, or not
dnl **************************************************************************

AC_ARG_ENABLE([loginfo],
  AC_HELP_STRING([--enable-loginfo], [Enable log information (disabled by default)]),
  [case $enableval in
    no | false) fc_set_loginfo=false ;;
    *)          fc_set_loginfo=true ;;
  esac],
  [fc_set_loginfo=false])

if $fc_set_loginfo; then
	CPPFLAGS="$CPPFLAGS -DFC_DEBUG"
fi

AC_ARG_ENABLE([template],
  AC_HELP_STRING([--enable-template], [Enable the build of the _TEMPLATE_ module (disabled by default)]),
  [case $enableval in
    no | false) fc_set_template=false ;;
    *)          fc_set_template=true ;;
  esac],
  [fc_set_template=false])

AM_CONDITIONAL(BUILD_TEMPLATE, test x"$fc_set_template" = xtrue)

AC_ARG_ENABLE([sandbox],
  AC_HELP_STRING([--enable-sandbox], [Enable the build of the Sandbox module (disabled by default)]),
  [case $enableval in
    no | false) fc_set_sandbox=false ;;
    *)          fc_set_sandbox=true ;;
  esac],
  [fc_set_sandbox=false])

AM_CONDITIONAL(BUILD_SANDBOX, test x"$fc_set_sandbox" = xtrue)

AC_ARG_ENABLE([assembly],
  AC_HELP_STRING([--enable-assembly], [Enable the build of the Assembly module (disabled by default)]),
  [case $enableval in
    no | false) fc_set_assembly=false ;;
    *)          fc_set_assembly=true ;;
  esac],
  [fc_set_assembly=false])

AM_CONDITIONAL(BUILD_ASSEMBLY, test x"$fc_set_assembly" = xtrue)

AC_ARG_ENABLE([cam],
  AC_HELP_STRING([--enable-cam], [Enable the build of the Cam module (disabled by default)]),
  [case $enableval in
    no | false) fc_set_cam=false ;;
    *)          fc_set_cam=true ;;
  esac],
  [fc_set_cam=false])

AM_CONDITIONAL(BUILD_CAM, test x"$fc_set_cam" = xtrue)

dnl Check if you want debug information enabled, or not
dnl **************************************************************************

AC_ARG_ENABLE([debug],
  AC_HELP_STRING([--enable-debug], [Enable debug information (enabled by default)]),
  [case $enableval in
    no | false) fc_set_debug=false ;;
    *)          fc_set_debug=true ;;
  esac],
  [fc_set_debug=true])
if $fc_set_debug; then
	CPPFLAGS="$CPPFLAGS -g -D_DEBUG"
else
	CPPFLAGS="$CPPFLAGS -O2 -DNDEBUG"
fi

dnl Check if you want thread support, or not (currently disabled, thread support is forced)
dnl **************************************************************************

CPPFLAGS="$CPPFLAGS -D_REENTRANT"
LDFLAGS="$LDFLAGS -lpthread"
fc_set_thread=true

#AC_ARG_ENABLE([thread],
#  AC_HELP_STRING([--enable-thread], [Enable thread support]),
#  [case $enableval in
#    no | false) fc_set_thread=false ;;
#    *)          fc_set_thread=true ;;
#  esac],
#  [fc_set_thread=true])
#if $fc_set_thread; then
#	AC_CHECK_HEADER([pthread.h])
#	AC_CHECK_LIB([pthread],[pthread_create])
#	if test "$ac_cv_header_pthread_h" != yes -o \
#        	"$ac_cv_lib_pthread_pthread_create" != yes; then
#  		AC_MSG_ERROR([POSIX threads (pthreads) not working. Bye.])
#	else
##	 	this is apparently needed for some versions of autoconf/automake
##		LIBS="-lpthread $LIBS"
##		CXXFLAGS="$CXXFLAGS -D_REENTRANT"
#		CPPFLAGS="$CPPFLAGS -D_REENTRANT"
#		LDFLAGS="-lpthread"
#	fi
#else
#	CPPFLAGS="$CPPFLAGS -DNO_FC_THREAD"
#fi

dnl Check if you want compiler warnings enabled, or not
dnl **************************************************************************

AC_ARG_ENABLE([warnings],
  AC_HELP_STRING([--enable-warnings], [Enable compiler warnings (enabled by default)]),
  [case $enableval in
    no | false) fc_set_warn=false ;;
    *)          fc_set_warn=true ;;
  esac],
  [fc_set_warn=true])
if $fc_set_warn; then
#	CPPFLAGS="$CPPFLAGS -Wall -W"
	CPPFLAGS="$CPPFLAGS -Wall"
else
	CPPFLAGS="$CPPFLAGS -w"
fi

dnl Check if you want precompiled headers (PCH) enabled, or not
dnl ***********************************************************
dnl 
dnl enableval=no
dnl AC_MSG_CHECKING([whether to use precompiled headers])
dnl AC_ARG_ENABLE(pch,
dnl   AC_HELP_STRING([--enable-pch],
dnl     [use precompiled headers, if available]))
dnl case "${enableval}" in
dnl yes)
dnl   AC_MSG_RESULT(yes)
dnl   COMP_FLAGS="$COMP_FLAGS -Winvalid-pch"
dnl   ;;
dnl no)
dnl   AC_MSG_RESULT(no)
dnl   ;;
dnl *)
dnl   AC_MSG_ERROR([bad value ${enableval} for --enable-pch, needs yes or no])
dnl   ;;
dnl esac
dnl AM_CONDITIONAL(USE_PRECOMPILED_HEADERS, test x"$enableval" = xyes)
dnl 
dnl CFLAGS="$COMP_FLAGS $OPT_FLAGS"
dnl CXXFLAGS="$COMP_FLAGS $OPT_FLAGS"
dnl **************************************************************************
dnl Use this Makefile.am
dnl if USE_PRECOMPILED_HEADERS
dnl PreCompiled.h.gch: PreCompiled.h
dnl 	$(CXXCOMPILE) -xc++-header -o $@ $<
dnl 
dnl PRECOMPILED = PreCompiled.h.gch
dnl else
dnl PRECOMPILED =
dnl endif
dnl 
dnl BUILT_SOURCES=\
dnl 		$(PRECOMPILED)
dnl **************************************************************************

dnl Define DISTCHECK_CONFIGURE_FLAGS
dnl **************************************************************************
DISTCHECK_CONFIGURE_FLAGS="$DISTCHECK_CONFIGURE_FLAGS CXXFLAGS=\"-fno-strict-aliasing -Wno-write-strings\" LDFLAGS=\"-Wl,-z,defs\""
AC_SUBST(DISTCHECK_CONFIGURE_FLAGS)

CXX=g++
CXXFLAGS="$CXXFLAGS -Wno-deprecated -frtti"

dnl doxygen
dnl **************************************************************************
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
AM_CONDITIONAL(HAVE_DOXYGEN,$HAVE_DOXYGEN)
if test $HAVE_DOXYGEN = "false"; then
        AC_MSG_WARN([doxygen not found])
fi

DOXYGEN_OUTPUT_DIR="."
AC_SUBST(DOXYGEN_OUTPUT_DIR)

DOXYGEN_INPUT_LIST="$srcdir/src/CXX $srcdir/src/zipios++ $srcdir/src/3rdParty $srcdir/src/Build $srcdir/src/Base $srcdir/src/App $srcdir/src/Gui $srcdir/src/Mod $srcdir/src/Main $srcdir/src/Doc"
AC_SUBST(DOXYGEN_INPUT_LIST)

DOXYGEN_EXCLUDE_LIST="$srcdir/src/Tools $srcdir/src/3rdParty/CxImage $srcdir/src/3rdParty/Pivy $srcdir/src/3rdParty/Pivy-0.5"
AC_SUBST(DOXYGEN_EXCLUDE_LIST)

DOXYGEN_IMAGE_PATH="$srcdir/src/Gui/Icons"
AC_SUBST(DOXYGEN_IMAGE_PATH)

DOXYGEN_INCLUDE_PATH="$srcdir/src/Doc/doxygen-headers"
AC_SUBST(DOXYGEN_INCLUDE_PATH)

#AC_CONFIG_FILES([src/Doc/doxygen-headers/Inventor/fields/SoSubField.h:$sim_ac_coin_includedir/Inventor/fields/SoSubField.h])

dnl **************************************************************************

AC_CONFIG_FILES([Makefile 
data/Makefile 
data/examples/Makefile 
src/Makefile 
src/Build/Makefile 
src/3rdParty/Makefile 
src/3rdParty/salomesmesh/Makefile 
src/Base/Makefile 
src/App/Makefile 
src/Gui/Makefile 
src/Gui/Icons/Makefile 
src/Gui/Language/Makefile 
src/Gui/propertyeditor/Makefile 
src/Gui/iisTaskPanel/Makefile 
src/Gui/TaskView/Makefile 
src/Main/Makefile
src/Doc/Makefile
src/Doc/BuildDevDoc.cfg
src/Mod/Makefile
src/Mod/Assembly/App/Makefile
src/Mod/Assembly/Gui/Resources/Makefile
src/Mod/Assembly/Gui/Makefile
src/Mod/Assembly/Makefile
src/Mod/Cam/App/Makefile
src/Mod/Cam/Gui/Makefile
src/Mod/Cam/Makefile
src/Mod/Part/Makefile
src/Mod/Part/App/Makefile
src/Mod/Part/Gui/Makefile
src/Mod/Import/Makefile
src/Mod/Import/Gui/Makefile
src/Mod/Raytracing/Makefile
src/Mod/Raytracing/App/Makefile
src/Mod/Raytracing/Gui/Makefile
src/Mod/Raytracing/Templates/Makefile
src/Mod/Mesh/Makefile
src/Mod/Mesh/App/Makefile
src/Mod/Mesh/Gui/Makefile
src/Mod/MeshPart/Makefile
src/Mod/MeshPart/App/Makefile
src/Mod/MeshPart/Gui/Makefile
src/Mod/MeshPart/Gui/Resources/Makefile
src/Mod/Fem/Makefile
src/Mod/Fem/App/Makefile
src/Mod/Fem/Gui/Makefile
src/Mod/Fem/Gui/Resources/Makefile
src/Mod/PartDesign/Makefile
src/Mod/PartDesign/App/Makefile
src/Mod/PartDesign/Gui/Makefile
src/Mod/PartDesign/Gui/Resources/Makefile
src/Mod/PartDesign/Scripts/Makefile
src/Mod/Sketcher/Makefile
src/Mod/Sketcher/App/Makefile
src/Mod/Sketcher/App/freegcs/Makefile
src/Mod/Sketcher/Gui/Makefile
src/Mod/Sketcher/Gui/Resources/Makefile
src/Mod/Sketcher/Templates/Makefile
src/Mod/Points/Makefile
src/Mod/Points/App/Makefile
src/Mod/Points/Gui/Makefile
src/Mod/Image/Makefile
src/Mod/Image/App/Makefile
src/Mod/Image/Gui/Makefile
src/Mod/Drawing/Makefile
src/Mod/Drawing/App/Makefile
src/Mod/Drawing/Gui/Makefile
src/Mod/Drawing/Gui/Resources/Makefile
src/Mod/Drawing/Templates/Makefile
src/Mod/ReverseEngineering/Makefile
src/Mod/ReverseEngineering/App/Makefile
src/Mod/ReverseEngineering/Gui/Makefile
src/Mod/ReverseEngineering/Gui/Resources/Makefile
src/Mod/Inspection/Makefile
src/Mod/Inspection/App/Makefile
src/Mod/Inspection/Gui/Makefile
src/Mod/Robot/Makefile
src/Mod/Robot/App/Makefile
src/Mod/Robot/Gui/Makefile
src/Mod/Robot/Gui/Resources/Makefile
src/Mod/Robot/Lib/Makefile
src/Mod/Complete/Makefile
src/Mod/Complete/App/Makefile
src/Mod/Complete/Gui/Makefile
src/Mod/Complete/Gui/Resources/Makefile
src/Mod/Draft/Makefile
src/Mod/Arch/Makefile
src/Mod/Test/Makefile
src/Mod/Test/Gui/Makefile
src/Mod/Web/Makefile
src/Mod/Web/Gui/Makefile
src/Mod/Web/Gui/Resources/Makefile
src/Mod/Start/Makefile
src/Mod/Start/App/Makefile
src/Mod/Start/Gui/Makefile
src/Mod/Start/Gui/Resources/Makefile
src/Mod/Start/StartPage/Makefile
src/Mod/Idf/Makefile
src/Mod/TemplatePyMod/Makefile
src/Mod/Sandbox/Makefile
src/Mod/Sandbox/App/Makefile
src/Mod/Sandbox/Gui/Makefile
src/Mod/Ship/Makefile
src/Mod/OpenSCAD/Makefile
src/Mod/Plot/Makefile
src/Tools/Makefile
src/Tools/_TEMPLATE_/Makefile
src/Tools/_TEMPLATE_/App/Makefile
src/Tools/_TEMPLATE_/Gui/Makefile
])
#src/3rdParty/Pivy/Makefile 

AC_OUTPUT

dnl Show a configuration report
dnl **************************************************************************

AC_MSG_NOTICE([

  **************************************************************************

  FreeCAD configuration settings:

  C++ compiler:              $CXX
  Python version:            $PYTHON_VERSION
  Log info enabled:          $fc_set_loginfo
  Debug info enabled:        $fc_set_debug
  Thread enabled (forced):   $fc_set_thread
  RTTI enabled (forced):     true
  Compiler warnings enabled: $fc_set_warn
  installation prefix:       $prefix
  enable-assembly:           $fc_set_assembly
  enable-cam:                $fc_set_cam
  enable-sandbox:            $fc_set_sandbox
  enable-template:           $fc_set_template
  Now, run 'make' to build FreeCAD.

  **************************************************************************
])

dnl **************************************************************************

