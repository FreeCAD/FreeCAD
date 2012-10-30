noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_Start.cpp 

nodist_libResources_la_SOURCES=\
		qrc_Start.cpp 

EXTRA_DIST = \
		translations/StartPage_af.qm \
		translations/StartPage_af.ts \
		translations/StartPage_de.qm \
		translations/StartPage_de.ts \
		translations/StartPage_es-ES.qm \
		translations/StartPage_es-ES.ts \
		translations/StartPage_fi.qm \
		translations/StartPage_fi.ts \
		translations/StartPage_fr.qm \
		translations/StartPage_fr.ts \
		translations/StartPage_hr.qm \
		translations/StartPage_hr.ts \
		translations/StartPage_hu.qm \
		translations/StartPage_hu.ts \
		translations/StartPage_it.qm \
		translations/StartPage_it.ts \
		translations/StartPage_ja.qm \
		translations/StartPage_ja.ts \
		translations/StartPage_nl.qm \
		translations/StartPage_nl.ts \
		translations/StartPage_no.qm \
		translations/StartPage_no.ts \
		translations/StartPage_pl.qm \
		translations/StartPage_pl.ts \
		translations/StartPage_pt-BR.qm \
		translations/StartPage_pt-BR.ts \
		translations/StartPage_ru.qm \
		translations/StartPage_ru.ts \
		translations/StartPage_sv-SE.qm \
		translations/StartPage_sv-SE.ts \
		translations/StartPage_uk.qm \
		translations/StartPage_uk.ts \
		translations/StartPage_zh-CN.qm \
		translations/StartPage_zh-CN.ts \
		translations/StartPage_zh-TW.qm \
		translations/StartPage_zh-TW.ts \
		translations/StartPage_cs.qm \
		translations/StartPage_cs.ts \
		translations/StartPage_sk.qm \
		translations/StartPage_sk.ts \
		translations/StartPage_tr.qm \
		translations/StartPage_tr.ts \
		translations/StartPage_hu.qm \
		translations/StartPage_hu.ts \
		translations/StartPage_ro.qm \
		translations/StartPage_ro.ts \
		translations/StartPage_ja.qm \
		translations/StartPage_ja.ts \
		Start.qrc \
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
