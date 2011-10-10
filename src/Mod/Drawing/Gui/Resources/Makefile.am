noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_Drawing.cpp 

nodist_libResources_la_SOURCES=\
		qrc_Drawing.cpp 

EXTRA_DIST = \
		icons/actions/document-new.png \
		icons/actions/document-new.svg \
		icons/actions/drawing-landscape-A0.svg \
		icons/actions/drawing-landscape-A1.svg \
		icons/actions/drawing-landscape-A2.svg \
		icons/actions/drawing-landscape.svg \
		icons/actions/drawing-view.svg \
		icons/actions/drawing-landscape-A3.svg \
		icons/actions/drawing-landscape-A4.svg \
		icons/actions/drawing-landscape-new.svg \
		icons/actions/drawing-portrait-A4.svg \
		icons/Page.svg \
		icons/Pages.svg \
		icons/View.svg \
		translations/Drawing_af.qm \
		translations/Drawing_de.qm \
		translations/Drawing_es.qm \
		translations/Drawing_fi.qm \
		translations/Drawing_fr.qm \
		translations/Drawing_hr.qm \
		translations/Drawing_it.qm \
		translations/Drawing_nl.qm \
		translations/Drawing_no.qm \
		translations/Drawing_pt.qm \
		translations/Drawing_ru.qm \
		translations/Drawing_se.qm \
		translations/Drawing_uk.qm \
		translations/Drawing_zh.qm \
		translations/Drawing_af.ts \
		translations/Drawing_de.ts \
		translations/Drawing_es.ts \
		translations/Drawing_fi.ts \
		translations/Drawing_fr.ts \
		translations/Drawing_hr.ts \
		translations/Drawing_it.ts \
		translations/Drawing_nl.ts \
		translations/Drawing_no.ts \
		translations/Drawing_pt.ts \
		translations/Drawing_ru.ts \
		translations/Drawing_se.ts \
		translations/Drawing_uk.ts \
		translations/Drawing_zh.ts \
		Drawing.qrc \
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
