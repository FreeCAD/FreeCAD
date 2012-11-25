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
		icons/actions/drawing-orthoviews.svg \
		icons/actions/drawing-openbrowser.svg \
		icons/actions/drawing-annotation.svg \
		icons/actions/drawing-clip.svg \
		icons/Page.svg \
		icons/Pages.svg \
		icons/View.svg \
		translations/Drawing_af.qm \
		translations/Drawing_de.qm \
		translations/Drawing_es-ES.qm \
		translations/Drawing_fi.qm \
		translations/Drawing_fr.qm \
		translations/Drawing_hr.qm \
		translations/Drawing_it.qm \
		translations/Drawing_nl.qm \
		translations/Drawing_no.qm \
		translations/Drawing_pl.qm \
		translations/Drawing_pt-BR.qm \
		translations/Drawing_ru.qm \
		translations/Drawing_sv-SE.qm \
		translations/Drawing_uk.qm \
		translations/Drawing_zh-CN.qm \
		translations/Drawing_zh-TW.qm \
		translations/Drawing_cs.qm \
		translations/Drawing_tr.qm \
		translations/Drawing_ro.qm \
		translations/Drawing_sk.qm \
		translations/Drawing_ja.qm \
		translations/Drawing_hu.qm \
		translations/Drawing_af.ts \
		translations/Drawing_de.ts \
		translations/Drawing_es-ES.ts \
		translations/Drawing_fi.ts \
		translations/Drawing_fr.ts \
		translations/Drawing_hr.ts \
		translations/Drawing_it.ts \
		translations/Drawing_nl.ts \
		translations/Drawing_no.ts \
		translations/Drawing_pl.ts \
		translations/Drawing_pt-BR.ts \
		translations/Drawing_ru.ts \
		translations/Drawing_sv-SE.ts \
		translations/Drawing_uk.ts \
		translations/Drawing_zh-CN.ts \
		translations/Drawing_zh-TW.ts \
		translations/Drawing_cs.ts \
		translations/Drawing_tr.ts \
		translations/Drawing_ro.ts \
		translations/Drawing_sk.ts \
		translations/Drawing_ja.ts \
		translations/Drawing_hu.ts \
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
