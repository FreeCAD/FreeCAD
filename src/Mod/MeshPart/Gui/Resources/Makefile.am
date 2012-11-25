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
		translations/MeshPart_es-ES.qm \
		translations/MeshPart_es-ES.ts \
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
		translations/MeshPart_pl.qm \
		translations/MeshPart_pl.ts \
		translations/MeshPart_pt-BR.qm \
		translations/MeshPart_pt-BR.ts \
		translations/MeshPart_ru.qm \
		translations/MeshPart_ru.ts \
		translations/MeshPart_sv-SE.qm \
		translations/MeshPart_sv-SE.ts \
		translations/MeshPart_uk.qm \
		translations/MeshPart_uk.ts \
		translations/MeshPart_zh-CN.qm \
		translations/MeshPart_zh-CN.ts \
		translations/MeshPart_zh-TW.qm \
		translations/MeshPart_zh-TW.ts \
		translations/MeshPart_hu.qm \
		translations/MeshPart_hu.ts \
		translations/MeshPart_ja.qm \
		translations/MeshPart_ja.ts \
		translations/MeshPart_cs.qm \
		translations/MeshPart_cs.ts \
		translations/MeshPart_sk.qm \
		translations/MeshPart_sk.ts \
		translations/MeshPart_ro.qm \
		translations/MeshPart_ro.ts \
		translations/MeshPart_tr.qm \
		translations/MeshPart_tr.ts \
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
