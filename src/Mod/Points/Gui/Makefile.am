
lib_LTLIBRARIES=libPointsGui.la PointsGui.la

BUILT_SOURCES=\
		ui_DlgPointsRead.h \
		moc_DlgPointsReadImp.cpp \
		qrc_Points.cpp 

libPointsGui_la_SOURCES=\
		Command.cpp \
		DlgPointsReadImp.cpp \
		DlgPointsReadImp.h \
		PreCompiled.cpp \
		PreCompiled.h \
		ViewProvider.cpp \
		Workbench.cpp

includedir = @includedir@/Mod/Points/Gui

include_HEADERS=\
		ViewProvider.h \
		Workbench.h

# the library search path.
libPointsGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App $(QT_LIBS) \
		$(sim_ac_coin_libs) $(sim_ac_soqt_libs) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libPointsGui_la_CPPFLAGS = -DPointsAppExport= -DPointsGuiExport=

libPointsGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lPoints

#--------------------------------------------------------------------------------------
# Loader of libPointsGui

PointsGui_la_SOURCES=\
		AppPointsGui.cpp

# the library search path.
PointsGui_la_LDFLAGS = $(libPointsGui_la_LDFLAGS) -module -avoid-version
PointsGui_la_CPPFLAGS = $(libPointsGui_la_CPPFLAGS)

PointsGui_la_LIBADD   = \
		$(libPointsGui_la_LIBADD) \
		-lPointsGui

PointsGui_la_DEPENDENCIES = libPointsGui.la

#--------------------------------------------------------------------------------------

# rule for Qt MetaObject Compiler:
moc_%.cpp: %.h
	$(QT_MOC) $< -o $(@F)

# rule for Qt MetaObject Compiler:
%.moc: %.h
	$(QT_MOC) $< -o $(@F)

# rules for Qt User Interface Compiler:
ui_%.h: %.ui
	$(QT_UIC) $< -o $(@F)

# rules for Qt Resource Compiler:
qrc_%.cpp: Resources/%.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)

libdir = $(prefix)/Mod/Points

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		DlgPointsRead.ui \
		CMakeLists.txt \
		Resources/Points.qrc \
		Resources/translations/Points_af.qm \
		Resources/translations/Points_af.ts \
		Resources/translations/Points_de.qm \
		Resources/translations/Points_de.ts \
		Resources/translations/Points_es-ES.qm \
		Resources/translations/Points_es-ES.ts \
		Resources/translations/Points_fi.qm \
		Resources/translations/Points_fi.ts \
		Resources/translations/Points_fr.qm \
		Resources/translations/Points_fr.ts \
		Resources/translations/Points_hr.qm \
		Resources/translations/Points_hr.ts \
		Resources/translations/Points_it.qm \
		Resources/translations/Points_it.ts \
		Resources/translations/Points_nl.qm \
		Resources/translations/Points_nl.ts \
		Resources/translations/Points_no.qm \
		Resources/translations/Points_no.ts \
		Resources/translations/Points_pl.qm \
		Resources/translations/Points_pl.ts \
		Resources/translations/Points_pt-BR.qm \
		Resources/translations/Points_pt-BR.ts \
		Resources/translations/Points_ru.qm \
		Resources/translations/Points_ru.ts \
		Resources/translations/Points_sv-SE.qm \
		Resources/translations/Points_sv-SE.ts \
		Resources/translations/Points_uk.qm \
		Resources/translations/Points_uk.ts \
		Resources/translations/Points_zh-TW.qm \
		Resources/translations/Points_zh-TW.ts \
		Resources/translations/Points_ro.qm \
		Resources/translations/Points_ro.ts \
		Resources/translations/Points_ja.qm \
		Resources/translations/Points_ja.ts \
		Resources/translations/Points_tr.qm \
		Resources/translations/Points_tr.ts \
		Resources/translations/Points_cs.qm \
		Resources/translations/Points_cs.ts \
		Resources/translations/Points_sk.qm \
		Resources/translations/Points_sk.ts \
		Resources/translations/Points_hu.qm \
		Resources/translations/Points_hu.ts \
		Resources/translations/Points_zh-CN.qm \
		Resources/translations/Points_zh-CN.ts
		
