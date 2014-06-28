noinst_LTLIBRARIES=libLanguage.la

BUILT_SOURCES=\
		moc_Translator.cpp \
		qrc_translation.cpp

nodist_libLanguage_la_SOURCES=\
		qrc_translation.cpp

libLanguage_la_SOURCES=\
		Translator.cpp \
		Translator.h

EXTRA_DIST = \
		translation.qrc \
		FreeCAD.po \
		FreeCAD_af.qm \
		FreeCAD_af.ts \
		FreeCAD_cs.qm \
		FreeCAD_cs.ts \
		FreeCAD_de.qm \
		FreeCAD_de.ts \
		FreeCAD_es-ES.qm \
		FreeCAD_es-ES.ts \
		FreeCAD_fi.qm \
		FreeCAD_fi.ts \
		FreeCAD_fr.qm \
		FreeCAD_fr.ts \
		FreeCAD_hr.qm \
		FreeCAD_hr.ts \
		FreeCAD_hu.qm \
		FreeCAD_hu.ts \
		FreeCAD_it.qm \
		FreeCAD_it.ts \
		FreeCAD_ja.qm \
		FreeCAD_ja.ts \
		FreeCAD_nl.qm \
		FreeCAD_nl.ts \
		FreeCAD_no.qm \
		FreeCAD_no.ts \
		FreeCAD_pl.qm \
		FreeCAD_pl.ts \
		FreeCAD_pt-BR.qm \
		FreeCAD_pt-BR.ts \
		FreeCAD_pt-PT.qm \
		FreeCAD_pt-PT.ts \
		FreeCAD_ro.qm \
		FreeCAD_ro.ts \
		FreeCAD_ru.qm \
		FreeCAD_ru.ts \
		FreeCAD_sk.qm \
		FreeCAD_sk.ts \
		FreeCAD_sv-SE.qm \
		FreeCAD_sv-SE.ts \
		FreeCAD_tr.qm \
		FreeCAD_tr.ts \
		FreeCAD.ts \
		FreeCAD_uk.qm \
		FreeCAD_uk.ts \
		FreeCAD_zh-CN.qm \
		FreeCAD_zh-CN.ts \
		FreeCAD_zh-TW.qm \
		FreeCAD_zh-TW.ts \
		qt_de.qm \
		qt_es-ES.qm \
		qt_fr.qm \
		qt_it.qm \
		qt_jp.qm \
		qt_pl.qm \
		qt_pt-BR.qm \
		qt_ru.qm \
		qt_sv-SE.qm \
		qt_uk.qm \
		qt_zh-CN.qm


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

