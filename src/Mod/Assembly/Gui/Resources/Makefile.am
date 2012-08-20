noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_Assembly.cpp 

nodist_libResources_la_SOURCES=\
		qrc_Assembly.cpp 

EXTRA_DIST = \
		icons/actions/Axle_constraint.svg \
		translations/Assembly_af.qm \
		translations/Assembly_de.qm \
		translations/Assembly_es.qm \
		translations/Assembly_fi.qm \
		translations/Assembly_fr.qm \
		translations/Assembly_hr.qm \
		translations/Assembly_hu.qm \
		translations/Assembly_it.qm \
		translations/Assembly_ja.qm \
		translations/Assembly_nl.qm \
		translations/Assembly_no.qm \
		translations/Assembly_pl.qm \
		translations/Assembly_pt.qm \
		translations/Assembly_ru.qm \
		translations/Assembly_se.qm \
		translations/Assembly_uk.qm \
		translations/Assembly_zh.qm \
		translations/Assembly_af.ts \
		translations/Assembly_de.ts \
		translations/Assembly_es.ts \
		translations/Assembly_fi.ts \
		translations/Assembly_fr.ts \
		translations/Assembly_hr.ts \
		translations/Assembly_hu.ts \
		translations/Assembly_ja.ts \
		translations/Assembly_it.ts \
		translations/Assembly_nl.ts \
		translations/Assembly_no.ts \
		translations/Assembly_pl.ts \
		translations/Assembly_pt.ts \
		translations/Assembly_ru.ts \
		translations/Assembly_se.ts \
		translations/Assembly_uk.ts \
		translations/Assembly_zh.ts \
		Assembly.qrc \
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
