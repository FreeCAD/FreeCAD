#BUILT_SOURCES=\
#		Draft_rc.py

# rules for Qt Resource Compiler:
#%_rc.py: Resources/%.qrc
#	$(PYRCC4) -name $(*F) $< -o $(@F)

# Change data dir from default ($(prefix)/share) to $(prefix)
datadir = $(prefix)/Mod/Draft
data_DATA = \
		Draft.py \
		DraftTools.py \
		DraftGui.py \
		DraftSnap.py \
		DraftTrackers.py \
		DraftVecUtils.py \
		DraftGeomUtils.py \
		WorkingPlane.py \
		importOCA.py \
		importDXF.py \
		importSVG.py \
		importAirfoilDAT.py \
		Init.py \
		InitGui.py \
		macros.py \
		Draft_rc.py

nobase_data_DATA = \
		draftlibs/dxfColorMap.py \
		draftlibs/dxfImportObjects.py \
		draftlibs/dxfLibrary.py \
		draftlibs/dxfReader.py \
		draftlibs/__init__.py

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(data_DATA) $(nobase_data_DATA) \
		CMakeLists.txt \
		draft.dox \
		Resources/Draft.qrc \
		Resources/icons/Draft_2DShapeView.svg \
		Resources/icons/Draft_AddPoint.svg \
		Resources/icons/Draft_AddToGroup.svg \
		Resources/icons/Draft_Apply.svg \
		Resources/icons/Draft_Arc.svg \
		Resources/icons/Draft_Array.svg \
		Resources/icons/Draft_BSpline.svg \
		Resources/icons/Draft_Circle.svg \
		Resources/icons/Draft_Clone.svg \
		Resources/icons/Draft_Construction.svg \
		Resources/icons/Draft_Cursor.svg \
		Resources/icons/Draft_DelPoint.svg \
		Resources/icons/Draft_Dimension.svg \
		Resources/icons/Draft_Dot.svg \
		Resources/icons/Draft_Downgrade.svg \
		Resources/icons/Draft_Draft2Sketch.svg \
		Resources/icons/Draft_Draft.svg \
		Resources/icons/Draft_Drawing.svg \
		Resources/icons/Draft_Edit.svg \
		Resources/icons/Draft_Finish.svg \
		Resources/icons/Draft_Line.svg \
		Resources/icons/Draft_Lock.svg \
		Resources/icons/Draft_Macro.svg \
		Resources/icons/Draft_Move.svg \
		Resources/icons/Draft_Offset.svg \
		Resources/icons/Draft_Point.svg \
		Resources/icons/Draft_Polygon.svg \
		Resources/icons/Draft_Rectangle.svg \
		Resources/icons/Draft_Rotate.svg \
		Resources/icons/Draft_Scale.svg \
		Resources/icons/Draft_SelectGroup.svg \
		Resources/icons/Draft_SelectPlane.svg \
		Resources/icons/Draft_SwitchMode.svg \
		Resources/icons/Draft_Text.svg \
		Resources/icons/Draft_Trimex.svg \
		Resources/icons/Draft_Upgrade.svg \
		Resources/icons/Draft_Wipe.svg \
		Resources/icons/Draft_Wire.svg \
		Resources/icons/Draft_WireToBSpline.svg \
		Resources/icons/preferences-draft.svg \
		Resources/icons/Snap_Angle.svg \
		Resources/icons/Snap_Center.svg \
		Resources/icons/Snap_Endpoint.svg \
		Resources/icons/Snap_Extension.svg \
		Resources/icons/Snap_Grid.svg \
		Resources/icons/Snap_Intersection.svg \
		Resources/icons/Snap_Lock.svg \
		Resources/icons/Snap_Midpoint.svg \
		Resources/icons/Snap_Near.svg \
		Resources/icons/Snap_Ortho.svg \
		Resources/icons/Snap_Parallel.svg \
		Resources/icons/Snap_Perpendicular.svg \
		Resources/patterns/concrete.svg \
		Resources/patterns/cross.svg \
		Resources/patterns/line.svg \
		Resources/patterns/simple.svg \
		Resources/patterns/square.svg \
		Resources/translations/Draft_af.qm \
		Resources/translations/Draft_af.ts \
		Resources/translations/Draft_de.qm \
		Resources/translations/Draft_de.ts \
		Resources/translations/Draft_es-ES.qm \
		Resources/translations/Draft_es-ES.ts \
		Resources/translations/Draft_fi.qm \
		Resources/translations/Draft_fi.ts \
		Resources/translations/Draft_fr.qm \
		Resources/translations/Draft_fr.ts \
		Resources/translations/Draft_hr.qm \
		Resources/translations/Draft_hr.ts \
		Resources/translations/Draft_hu.qm \
		Resources/translations/Draft_hu.ts \
		Resources/translations/Draft_it.qm \
		Resources/translations/Draft_it.ts \
		Resources/translations/Draft_ja.qm \
		Resources/translations/Draft_ja.ts \
		Resources/translations/Draft_nl.qm \
		Resources/translations/Draft_nl.ts \
		Resources/translations/Draft_no.qm \
		Resources/translations/Draft_no.ts \
		Resources/translations/Draft_pl.qm \
		Resources/translations/Draft_pl.ts \
		Resources/translations/Draft_pt-BR.qm \
		Resources/translations/Draft_pt-BR.ts \
		Resources/translations/Draft_ru.qm \
		Resources/translations/Draft_ru.ts \
		Resources/translations/Draft_sv-SE.qm \
		Resources/translations/Draft_sv-SE.ts \
		Resources/translations/Draft_uk.qm \
		Resources/translations/Draft_uk.ts \
		Resources/translations/Draft_zh-CN.qm \
		Resources/translations/Draft_zh-CN.ts \
		Resources/translations/Draft_zh-TW.qm \
		Resources/translations/Draft_zh-TW.ts \
		Resources/translations/Draft_cs.qm \
		Resources/translations/Draft_cs.ts \
		Resources/translations/Draft_sk.qm \
		Resources/translations/Draft_sk.ts \
		Resources/translations/Draft_tr.qm \
		Resources/translations/Draft_tr.ts \
		Resources/translations/Draft_ro.qm \
		Resources/translations/Draft_ro.ts \
		Resources/ui/userprefs-base.ui \
		Resources/ui/userprefs-import.ui

