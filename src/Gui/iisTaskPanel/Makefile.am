noinst_LTLIBRARIES=libTaskPanel.la

BUILT_SOURCES=\
		moc_iisiconlabel.cpp \
		moc_iistaskbox.cpp \
		moc_iistaskgroup.cpp \
		moc_iistaskheader.cpp \
		qrc_iisTaskPanel.cpp

libTaskPanel_la_SOURCES=\
		src/iisfreecadscheme.cpp \
		src/iisiconlabel.cpp \
		src/iistaskbox.cpp \
		src/iistaskgroup.cpp \
		src/iistaskheader.cpp \
		src/iistaskpanel.cpp \
		src/iistaskpanelscheme.cpp \
		src/iiswinxptaskpanelscheme.cpp \
		include/iisTaskPanel

nodist_libTaskPanel_la_SOURCES=\
		$(BUILT_SOURCES)

includedir = @includedir@/Gui/iisTaskPanel/src

include_HEADERS=\
		src/iisfreecadscheme.h \
		src/iisiconlabel.h \
		src/iistaskbox.h \
		src/iistaskgroup.h \
		src/iistaskheader.h \
		src/iistaskpanel.h \
		src/iistaskpanel_global.h \
		src/iistaskpanelscheme.h \
		src/iiswinxptaskpanelscheme.h

EXTRA_DIST = \
		src/Resources/headerButtonFold.png \
		src/Resources/headerButtonFold.xpm \
		src/Resources/headerButtonFoldOver.png \
		src/Resources/headerButtonFoldOver_XPBlue1.png \
		src/Resources/headerButtonFoldOver_XPBlue2.png \
		src/Resources/headerButtonFold_XPBlue1.png \
		src/Resources/headerButtonFold_XPBlue2.png \
		src/Resources/headerButtonUnfold.png \
		src/Resources/headerButtonUnfoldOver.png \
		src/Resources/headerButtonUnfoldOver_XPBlue1.png \
		src/Resources/headerButtonUnfoldOver_XPBlue2.png \
		src/Resources/headerButtonUnfold_XPBlue1.png \
		src/Resources/headerButtonUnfold_XPBlue2.png \
		src/Resources/screen2.psd \
		src/iisTaskPanel.qrc

# rule for Qt MetaObject Compiler:
moc_%.cpp: src/%.h
	$(QT_MOC) $< -o $(@F)

# rules for Qt Resource Compiler:
qrc_%.cpp: src/%.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)


# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(srcdir)/.. $(QT_CXXFLAGS) $(all_includes)

CLEANFILES = $(BUILT_SOURCES)

