
BUILT_SOURCES = \
		Version.h

Version.h: Version.h.in
	$(PYTHON) $(top_srcdir)/src/Tools/SubWCRev.py --srcdir=$(top_srcdir) --bindir=$(top_builddir)


include_HEADERS=\
		Version.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		BuildVersion.bat \
		Version.h.in \
		CMakeLists.txt

