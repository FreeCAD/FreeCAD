
lib_LTLIBRARIES=libQtUnitGui.la QtUnitGui.la

BUILT_SOURCES=\
		ui_UnitTest.h \
		moc_UnitTestImp.cpp \
		qrc_Test.cpp

libQtUnitGui_la_SOURCES=\
		PreCompiled.cpp \
		PreCompiled.h \
		UnitTestImp.cpp \
		UnitTestImp.h \
		UnitTestPy.cpp \
		UnitTestPy.h

# the library search path.
libQtUnitGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui $(QT_LIBS) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libQtUnitGui_la_CPPFLAGS = -DQtUnitAppExport= -DQtUnitGuiExport=

libQtUnitGui_la_LIBADD   = \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui

#--------------------------------------------------------------------------------------
# Loader of libQtUnitGui

QtUnitGui_la_SOURCES=\
		AppTestGui.cpp

# the library search path.
QtUnitGui_la_LDFLAGS = $(libQtUnitGui_la_LDFLAGS) -module -avoid-version
QtUnitGui_la_CPPFLAGS = $(libQtUnitGui_la_CPPFLAGS)

QtUnitGui_la_LIBADD   = \
		$(libQtUnitGui_la_LIBADD) \
		-lQtUnitGui

QtUnitGui_la_DEPENDENCIES = libQtUnitGui.la

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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS)

libdir = $(prefix)/Mod/Test

CLEANFILES = $(BUILT_SOURCES)

# Change data dir from default ($(prefix)/share) to $(prefix)
datadir = $(prefix)/Mod/Test
data_DATA = \
		qtunittest.py

EXTRA_DIST = \
		$(data_DATA) \
		CMakeLists.txt \
		UnitTest.ui \
		Resources/Test.qrc \
		Resources/translations/Test_af.qm \
		Resources/translations/Test_af.ts \
		Resources/translations/Test_de.qm \
		Resources/translations/Test_de.ts \
		Resources/translations/Test_es-ES.qm \
		Resources/translations/Test_es-ES.ts \
		Resources/translations/Test_fi.qm \
		Resources/translations/Test_fi.ts \
		Resources/translations/Test_fr.qm \
		Resources/translations/Test_fr.ts \
		Resources/translations/Test_hr.qm \
		Resources/translations/Test_hr.ts \
		Resources/translations/Test_hu.qm \
		Resources/translations/Test_hu.ts \
		Resources/translations/Test_it.qm \
		Resources/translations/Test_it.ts \
		Resources/translations/Test_ja.qm \
		Resources/translations/Test_ja.ts \
		Resources/translations/Test_nl.qm \
		Resources/translations/Test_nl.ts \
		Resources/translations/Test_no.qm \
		Resources/translations/Test_no.ts \
		Resources/translations/Test_pl.qm \
		Resources/translations/Test_pl.ts \
		Resources/translations/Test_pt-BR.qm \
		Resources/translations/Test_pt-BR.ts \
		Resources/translations/Test_ru.qm \
		Resources/translations/Test_ru.ts \
		Resources/translations/Test_sv-SE.qm \
		Resources/translations/Test_sv-SE.ts \
		Resources/translations/Test_uk.qm \
		Resources/translations/Test_uk.ts \
		Resources/translations/Test_zh-CN.qm \
		Resources/translations/Test_zh-CN.ts \
		Resources/translations/Test_zh-TW.qm \
		Resources/translations/Test_zh-TW.ts \
		Resources/translations/Test_ro.qm \
		Resources/translations/Test_ro.ts \
		Resources/translations/Test_cs.qm \
		Resources/translations/Test_cs.ts \
		Resources/translations/Test_sk.qm \
		Resources/translations/Test_sk.ts \
		Resources/translations/Test_tr.qm \
		Resources/translations/Test_tr.ts \
		qtunittest.py
