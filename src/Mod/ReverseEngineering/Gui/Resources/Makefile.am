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
		translations/ReverseEngineering_es.qm \
		translations/ReverseEngineering_es.ts \
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
		translations/ReverseEngineering_pt.qm \
		translations/ReverseEngineering_pt.ts \
		translations/ReverseEngineering_ru.qm \
		translations/ReverseEngineering_ru.ts \
		translations/ReverseEngineering_se.qm \
		translations/ReverseEngineering_se.ts \
		translations/ReverseEngineering_uk.qm \
		translations/ReverseEngineering_uk.ts \
		translations/ReverseEngineering_zh.qm \
		translations/ReverseEngineering_zh.ts \
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
