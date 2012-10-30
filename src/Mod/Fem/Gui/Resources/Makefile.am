noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_Fem.cpp 

nodist_libResources_la_SOURCES=\
		qrc_Fem.cpp 

EXTRA_DIST = \
		icons/Fem_FemMesh.svg \
		translations/Fem_af.qm \
		translations/Fem_af.ts \
		translations/Fem_de.qm \
		translations/Fem_de.ts \
		translations/Fem_es-ES.qm \
		translations/Fem_es-ES.ts \
		translations/Fem_fi.qm \
		translations/Fem_fi.ts \
		translations/Fem_fr.qm \
		translations/Fem_fr.ts \
		translations/Fem_hr.qm \
		translations/Fem_hr.ts \
		translations/Fem_it.qm \
		translations/Fem_it.ts \
		translations/Fem_nl.qm \
		translations/Fem_nl.ts \
		translations/Fem_no.qm \
		translations/Fem_no.ts \
		translations/Fem_pl.qm \
		translations/Fem_pl.ts \
		translations/Fem_pt-BR.qm \
		translations/Fem_pt-BR.ts \
		translations/Fem_ru.qm \
		translations/Fem_ru.ts \
		translations/Fem_sv-SE.qm \
		translations/Fem_sv-SE.ts \
		translations/Fem_uk.qm \
		translations/Fem_uk.ts \
		translations/Fem_zh-CN.qm \
		translations/Fem_zh-CN.ts \
		translations/Fem_zh-TW.qm \
		translations/Fem_zh-TW.ts \
		translations/Fem_ro.qm \
		translations/Fem_ro.ts \
		translations/Fem_cs.qm \
		translations/Fem_cs.ts \
		translations/Fem_sk.qm \
		translations/Fem_sk.ts \
		translations/Fem_tr.qm \
		translations/Fem_tr.ts \
		translations/Fem_hu.qm \
		translations/Fem_hu.ts \
		translations/Fem_ja.qm \
		translations/Fem_ja.ts \
		Fem.qrc \
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

