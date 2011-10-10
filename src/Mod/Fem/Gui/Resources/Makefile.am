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
		translations/Fem_es.qm \
		translations/Fem_es.ts \
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
		translations/Fem_pt.qm \
		translations/Fem_pt.ts \
		translations/Fem_ru.qm \
		translations/Fem_ru.ts \
		translations/Fem_se.qm \
		translations/Fem_se.ts \
		translations/Fem_uk.qm \
		translations/Fem_uk.ts \
		translations/Fem_zh.qm \
		translations/Fem_zh.ts \
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

