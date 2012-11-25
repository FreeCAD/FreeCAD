noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_ReverseEngineering.cpp 

nodist_libResources_la_SOURCES=\
		qrc_ReverseEngineering.cpp 

EXTRA_DIST = \
		icons/actions/FitSurface.svg \
		translations/ReverseEngineering_af.qm \
		translations/ReverseEngineering_af.ts \
		translations/ReverseEngineering_de.qm \
		translations/ReverseEngineering_de.ts \
		translations/ReverseEngineering_es-ES.qm \
		translations/ReverseEngineering_es-ES.ts \
		translations/ReverseEngineering_fi.qm \
		translations/ReverseEngineering_fi.ts \
		translations/ReverseEngineering_fr.qm \
		translations/ReverseEngineering_fr.ts \
		translations/ReverseEngineering_hr.qm \
		translations/ReverseEngineering_hr.ts \
		translations/ReverseEngineering_it.qm \
		translations/ReverseEngineering_it.ts \
		translations/ReverseEngineering_nl.qm \
		translations/ReverseEngineering_nl.ts \
		translations/ReverseEngineering_no.qm \
		translations/ReverseEngineering_no.ts \
		translations/ReverseEngineering_pl.qm \
		translations/ReverseEngineering_pl.ts \
		translations/ReverseEngineering_pt-BR.qm \
		translations/ReverseEngineering_pt-BR.ts \
		translations/ReverseEngineering_ru.qm \
		translations/ReverseEngineering_ru.ts \
		translations/ReverseEngineering_sv-SE.qm \
		translations/ReverseEngineering_sv-SE.ts \
		translations/ReverseEngineering_uk.qm \
		translations/ReverseEngineering_uk.ts \
		translations/ReverseEngineering_hu.qm \
		translations/ReverseEngineering_hu.ts \
		translations/ReverseEngineering_cs.qm \
		translations/ReverseEngineering_cs.ts \
		translations/ReverseEngineering_sk.qm \
		translations/ReverseEngineering_sk.ts \
		translations/ReverseEngineering_tr.qm \
		translations/ReverseEngineering_tr.ts \
		translations/ReverseEngineering_ro.qm \
		translations/ReverseEngineering_ro.ts \
		translations/ReverseEngineering_ja.qm \
		translations/ReverseEngineering_ja.ts \
		translations/ReverseEngineering_zh-TW.qm \
		translations/ReverseEngineering_zh-TW.ts \
		translations/ReverseEngineering_zh-CN.qm \
		translations/ReverseEngineering_zh-CN.ts \
		ReverseEngineering.qrc \
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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(srcdir)/.. $(all_includes) $(QT_CXXFLAGS)

CLEANFILES = $(BUILT_SOURCES)
