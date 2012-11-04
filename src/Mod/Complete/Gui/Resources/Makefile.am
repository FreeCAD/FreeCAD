noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_Complete.cpp 

nodist_libResources_la_SOURCES=\
		qrc_Complete.cpp 

EXTRA_DIST = \
		translations/Complete_af.qm \
		translations/Complete_de.qm \
		translations/Complete_es-ES.qm \
		translations/Complete_fi.qm \
		translations/Complete_fr.qm \
		translations/Complete_hr.qm \
		translations/Complete_it.qm \
		translations/Complete_nl.qm \
		translations/Complete_no.qm \
		translations/Complete_pl.qm \
		translations/Complete_pt-BR.qm \
		translations/Complete_ru.qm \
		translations/Complete_sv-SE.qm \
		translations/Complete_uk.qm \
		translations/Complete_zh-CN.qm \
		translations/Complete_zh-TW.qm \
		translations/Complete_cs.qm \
		translations/Complete_tr.qm \
		translations/Complete_ro.qm \
		translations/Complete_sk.qm \
		translations/Complete_ja.qm \
		translations/Complete_hu.qm \
		translations/Complete_af.ts \
		translations/Complete_de.ts \
		translations/Complete_es-ES.ts \
		translations/Complete_fi.ts \
		translations/Complete_fr.ts \
		translations/Complete_hr.ts \
		translations/Complete_it.ts \
		translations/Complete_nl.ts \
		translations/Complete_no.ts \
		translations/Complete_pl.ts \
		translations/Complete_pt-BR.ts \
		translations/Complete_ru.ts \
		translations/Complete_sv-SE.ts \
		translations/Complete_uk.ts \
		translations/Complete_zh-CN.ts \
		translations/Complete_zh-TW.ts \
		translations/Complete_cs.ts \
		translations/Complete_tr.ts \
		translations/Complete_ro.ts \
		translations/Complete_sk.ts \
		translations/Complete_ja.ts \
		translations/Complete_hu.ts \
		Complete.qrc \
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
