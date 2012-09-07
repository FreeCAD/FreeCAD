noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_PartDesign.cpp 

nodist_libResources_la_SOURCES=\
		qrc_PartDesign.cpp 

EXTRA_DIST = \
		translations/PartDesign_af.qm \
		translations/PartDesign_af.ts \
		translations/PartDesign_de.qm \
		translations/PartDesign_de.ts \
		translations/PartDesign_es.qm \
		translations/PartDesign_es.ts \
		translations/PartDesign_fi.qm \
		translations/PartDesign_fi.ts \
		translations/PartDesign_fr.qm \
		translations/PartDesign_fr.ts \
		translations/PartDesign_hr.qm \
		translations/PartDesign_hr.ts \
		translations/PartDesign_it.qm \
		translations/PartDesign_it.ts \
		translations/PartDesign_nl.qm \
		translations/PartDesign_nl.ts \
		translations/PartDesign_no.qm \
		translations/PartDesign_no.ts \
		translations/PartDesign_pl.qm \
		translations/PartDesign_pl.ts \
		translations/PartDesign_pt.qm \
		translations/PartDesign_pt.ts \
		translations/PartDesign_ru.qm \
		translations/PartDesign_ru.ts \
		translations/PartDesign_se.qm \
		translations/PartDesign_se.ts \
		translations/PartDesign_uk.qm \
		translations/PartDesign_uk.ts \
		translations/PartDesign_zh.qm \
		translations/PartDesign_zh.ts \
		icons/PartDesign_Groove.svg \
		icons/PartDesign_Pad.svg \
		icons/PartDesign_Pocket.svg \
		icons/PartDesign_Revolution.svg \
		icons/PartDesign_Mirrored.svg \
		icons/PartDesign_LinearPattern.svg \
		icons/PartDesign_PolarPattern.svg \
		icons/PartDesign_Scaled.svg \
		icons/PartDesign_MultiTransform.svg \
		PartDesign.qrc


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
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(srcdir)/.. \
		$(all_includes) $(QT_CXXFLAGS) -I$(OCC_INC)

CLEANFILES = $(BUILT_SOURCES)
