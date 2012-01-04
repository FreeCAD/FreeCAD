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
		translations/StartPage_es.qm \
		translations/StartPage_es.ts \
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
		translations/StartPage_pt.qm \
		translations/StartPage_pt.ts \
		translations/StartPage_ru.qm \
		translations/StartPage_ru.ts \
		translations/StartPage_se.qm \
		translations/StartPage_se.ts \
		translations/StartPage_uk.qm \
		translations/StartPage_uk.ts \
		translations/StartPage_zh.qm \
		translations/StartPage_zh.ts \
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
