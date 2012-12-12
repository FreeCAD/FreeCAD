noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_Sketcher.cpp

nodist_libResources_la_SOURCES=\
		qrc_Sketcher.cpp

EXTRA_DIST = \
icons/Constraint_PointOnObject.svg \
		icons/small/Constraint_TangentToStart_sm.xpm \
		icons/small/Constraint_Length_sm.xpm \
		icons/small/Constraint_Concentric_sm.xpm \
		icons/small/Constraint_TangentToEnd_sm.xpm \
		icons/small/Constraint_PointOnMidPoint_sm.xpm \
		icons/small/Constraint_PointOnStart_sm.xpm \
		icons/small/Constraint_Vertical_sm.xpm \
		icons/small/Constraint_PointOnEnd_sm.xpm \
		icons/small/Constraint_PointOnObject_sm.xpm \
		icons/small/Constraint_PointToObject_sm.xpm \
		icons/small/Constraint_InternalAngle_sm.xpm \
		icons/small/Constraint_Radius_sm.xpm \
		icons/small/Constraint_Horizontal_sm.xpm \
		icons/small/Constraint_ExternalAngle_sm.xpm \
		icons/small/Constraint_Parallel_sm.xpm \
		icons/small/Constraint_Perpendicular_sm.xpm \
		icons/small/Constraint_PointOnPoint_sm.xpm \
		icons/small/Constraint_Tangent_sm.xpm \
		icons/small/Constraint_EqualLength_sm.xpm \
		icons/small/Constraint_HorizontalDistance_sm.xpm \
		icons/small/Constraint_VerticalDistance_sm.xpm \
		icons/small/Constraint_Symmetric_sm.xpm \
		icons/Constraint_PointOnObject.svg \
		icons/Constraint_TangentToStart.svg \
		icons/Constraint_Length.svg \
		icons/Constraint_Concentric.svg \
		icons/Constraint_TangentToEnd.svg \
		icons/Constraint_PointOnMidPoint.svg \
		icons/Sketcher_ConstrainDistance.svg \
		icons/Sketcher_ConstrainParallel.svg \
		icons/Constraint_PointOnStart.svg \
		icons/Constraint_Vertical.svg \
		icons/Constraint_PointOnEnd.svg \
		icons/Constraint_PointToObject.svg \
		icons/Constraint_InternalAngle.svg \
		icons/Constraint_Radius.svg \
		icons/Constraint_Horizontal.svg \
		icons/Constraint_ExternalAngle.svg \
		icons/Constraint_Parallel.svg \
		icons/Constraint_Perpendicular.svg \
		icons/Constraint_PointOnPoint.svg \
		icons/Constraint_Tangent.svg \
		icons/Constraint_EqualLength.svg \
		icons/Constraint_HorizontalDistance.svg \
		icons/Constraint_VerticalDistance.svg \
		icons/Constraint_Symmetric.svg \
		icons/Sketcher_AlterConstruction.svg \
		icons/Sketcher_AlterFillet.svg \
		icons/Sketcher_ConstrainCoincident.svg \
		icons/Sketcher_ConstrainHorizontal.svg \
		icons/Sketcher_ConstrainLock.svg \
		icons/Sketcher_ConstrainVertical.svg \
		icons/Sketcher_CreateArc.svg \
		icons/Sketcher_CreateCircle.svg \
		icons/Sketcher_CreateLine.svg \
		icons/Sketcher_CreatePoint.svg \
		icons/Sketcher_CreatePolyline.svg \
		icons/Sketcher_CreateRectangle.svg \
		icons/Sketcher_CreateFillet.svg \
		icons/Sketcher_CreateText.svg \
		icons/Sketcher_DraftLine.svg \
		icons/Sketcher_Trimming.svg \
		icons/Sketcher_External.svg \
		icons/Sketcher_LeaveSketch.svg \
		icons/Sketcher_MapSketch.svg \
		icons/Sketcher_NewSketch.svg \
		icons/Sketcher_ViewSketch.svg \
		icons/Sketcher_Sketch.svg \
		translations/Sketcher_af.qm \
		translations/Sketcher_af.ts \
		translations/Sketcher_de.qm \
		translations/Sketcher_de.ts \
		translations/Sketcher_es-ES.qm \
		translations/Sketcher_es-ES.ts \
		translations/Sketcher_fi.qm \
		translations/Sketcher_fi.ts \
		translations/Sketcher_fr.qm \
		translations/Sketcher_fr.ts \
		translations/Sketcher_hr.qm \
		translations/Sketcher_hr.ts \
		translations/Sketcher_it.qm \
		translations/Sketcher_it.ts \
		translations/Sketcher_nl.qm \
		translations/Sketcher_nl.ts \
		translations/Sketcher_no.qm \
		translations/Sketcher_no.ts \
		translations/Sketcher_pl.qm \
		translations/Sketcher_pl.ts \
		translations/Sketcher_pt-BR.qm \
		translations/Sketcher_pt-BR.ts \
		translations/Sketcher_ru.qm \
		translations/Sketcher_ru.ts \
		translations/Sketcher_sv-SE.qm \
		translations/Sketcher_sv-SE.ts \
		translations/Sketcher_uk.qm \
		translations/Sketcher_uk.ts \
		translations/Sketcher_zh-CN.qm \
		translations/Sketcher_zh-CN.ts \
		translations/Sketcher_zh-TW.qm \
		translations/Sketcher_zh-TW.ts \
		translations/Sketcher_cs.qm \
		translations/Sketcher_cs.ts \
		translations/Sketcher_sk.qm \
		translations/Sketcher_sk.ts \
		translations/Sketcher_hu.qm \
		translations/Sketcher_hu.ts \
		translations/Sketcher_ro.qm \
		translations/Sketcher_ro.ts \
		translations/Sketcher_tr.qm \
		translations/Sketcher_tr.ts \
		translations/Sketcher_ja.qm \
		translations/Sketcher_ja.ts \
		Sketcher.qrc


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
