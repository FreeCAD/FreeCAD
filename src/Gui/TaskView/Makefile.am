noinst_LTLIBRARIES=libTaskView.la

BUILT_SOURCES=\
		moc_TaskAppearance.cpp \
		moc_TaskSelectLinkProperty.cpp \
		moc_TaskDialog.cpp \
		moc_TaskWatcher.cpp \
		moc_TaskEditControl.cpp \
		moc_TaskView.cpp \
		ui_TaskAppearance.h \
		ui_TaskSelectLinkProperty.h \
		ui_TaskEditControl.h


libTaskView_la_UI=\
		TaskAppearance.ui \
		TaskSelectLinkProperty.ui \
		TaskEditControl.ui

libTaskView_la_SOURCES=\
		TaskAppearance.cpp \
		TaskSelectLinkProperty.cpp \
		TaskDialog.cpp \
		TaskDialogPython.cpp \
		TaskWatcher.cpp \
		TaskEditControl.cpp \
		TaskView.cpp

includedir = @includedir@/Gui/TaskView

include_HEADERS=\
		TaskAppearance.h \
		TaskSelectLinkProperty.h \
		TaskDialog.h \
		TaskDialogPython.h \
		TaskWatcher.h \
		TaskEditControl.h \
		TaskView.h

EXTRA_DIST = \
		$(libTaskView_la_UI)

%.cpp: %.xml $(top_srcdir)/src/Tools/generateTemplates/templateClassPyExport.py
	$(PYTHON) $(top_srcdir)/src/Tools/generate.py --outputPath $(@D) $<

# rule for Qt MetaObject Compiler:
moc_%.cpp: %.h
	$(QT_MOC) $< -o $(@F)

# rules for Qt User Interface Compiler:
ui_%.h: %.ui
	$(QT_UIC) $< -o $(@F)

# rules for Qt Resource Compiler:
qrc_%.cpp: %.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)


# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(srcdir)/.. $(QT_CXXFLAGS) $(all_includes) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)

CLEANFILES = $(BUILT_SOURCES)

