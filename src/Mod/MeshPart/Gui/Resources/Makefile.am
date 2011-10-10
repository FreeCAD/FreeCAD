noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_MeshPart.cpp 

nodist_libResources_la_SOURCES=\
		qrc_MeshPart.cpp 

EXTRA_DIST = \
		icons/actions/MeshFace.svg \
		translations/MeshPart_af.qm \
		translations/MeshPart_af.ts \
		translations/MeshPart_de.qm \
		translations/MeshPart_de.ts \
		translations/MeshPart_es.qm \
		translations/MeshPart_es.ts \
		translations/MeshPart_fi.qm \
		translations/MeshPart_fi.ts \
		translations/MeshPart_fr.qm \
		translations/MeshPart_fr.ts \
		translations/MeshPart_hr.qm \
		translations/MeshPart_hr.ts \
		translations/MeshPart_it.qm \
		translations/MeshPart_it.ts \
		translations/MeshPart_nl.qm \
		translations/MeshPart_nl.ts \
		translations/MeshPart_no.qm \
		translations/MeshPart_no.ts \
		translations/MeshPart_pt.qm \
		translations/MeshPart_pt.ts \
		translations/MeshPart_ru.qm \
		translations/MeshPart_ru.ts \
		translations/MeshPart_se.qm \
		translations/MeshPart_se.ts \
		translations/MeshPart_uk.qm \
		translations/MeshPart_uk.ts \
		translations/MeshPart_zh.qm \
		translations/MeshPart_zh.ts \
		MeshPart.qrc


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
