noinst_LTLIBRARIES=libfreegcs.la

libfreegcs_la_SOURCES = \
		GCS.cpp \
		GCS.h \
		Util.h \
		Geo.h \
		Constraints.cpp \
		Constraints.h \
		SubSystem.cpp \
		SubSystem.h \
		qp_eq.cpp \
		qp_eq.h

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/Mod/Sketcher/App \
		-I$(top_builddir)/src -I$(top_builddir)/src/Mod/Sketcher/App $(all_includes) \
        -I$(EIGEN3_INC)
