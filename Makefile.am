ACLOCAL_AMFLAGS = -I m4

SUBDIRS=data src

EXTRA_DIST = \
		cMake/ConfigureChecks.cmake \
		cMake/FindCoin3D.cmake \
		cMake/FindCoin3DDoc.cmake \
		cMake/FindEigen2.cmake \
		cMake/FindEigen3.cmake \
		cMake/FindF2C.cmake \
		cMake/FindODE.cmake \
		cMake/FindOpenCasCade.cmake \
		cMake/FindOpenCV.cmake \
		cMake/FindSoQt.cmake \
		cMake/FindSpnav.cmake \
		cMake/FindXercesC.cmake \
		cMake/FreeCadMacros.cmake \
		cMake/UseLibPack6x.cmake \
		cMake/UseLibPack7x.cmake \
		cMake/UseLibPackCustom.cmake \
		config.h.cmake \
		CMakeLists.txt \
		autogen.sh \
		BuildAll.bat \
		build.sh \
		ChangeLog.txt \
		copying.lib \
		README.Linux \
		README.Win32

debian-package:
	$(top_srcdir)/package/makedebian.sh

if HAVE_DOXYGEN
devdoc:
	doxygen $(top_builddir)/src/Doc/BuildDevDoc.cfg
endif

