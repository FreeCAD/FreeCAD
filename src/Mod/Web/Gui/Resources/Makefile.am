noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_Web.cpp 

nodist_libResources_la_SOURCES=\
		qrc_Web.cpp 

EXTRA_DIST = \
		icons/actions/web-browser.svg \
		icons/actions/web-home.svg \
		icons/actions/web-next.svg \
		icons/actions/web-previous.svg \
		icons/actions/web-refresh.svg \
		icons/actions/web-stop.svg \
		icons/actions/web-zoom-in.svg \
		icons/actions/web-zoom-out.svg \
		Web.qrc \
		UpdateResources.bat


# rule for Qt MetaObject Compiler:
moc_%.cpp: %.h
	$(QT_MOC) $< -o $(@F)

# rule for Qt MetaObject Compiler:
%.moc: %.h
	$(QT_MOC) $< -o $(@F)

# rules for Qt Resource Compiler:
qrc_%.cpp: %.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)


# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(srcdir)/.. $(QT_CXXFLAGS) $(all_includes)

CLEANFILES = $(BUILT_SOURCES)
