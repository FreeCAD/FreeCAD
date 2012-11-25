
lib_LTLIBRARIES=libRaytracingGui.la RaytracingGui.la

BUILT_SOURCES=\
		ui_DlgSettingsRay.h \
		moc_DlgSettingsRayImp.cpp \
		FreeCADpov.h \
		qrc_Raytracing.cpp 

libRaytracingGui_la_SOURCES=\
		AppRaytracingGuiPy.cpp \
		Command.cpp \
		DlgSettingsRayImp.cpp \
		DlgSettingsRayImp.h \
		PovrayHighlighter.cpp \
		PovrayHighlighter.h \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libRaytracingGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../../Part/App -L../App -L$(OCC_LIB) \
		$(QT_LIBS) $(sim_ac_coin_ldflags) $(sim_ac_coin_libs) $(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		$(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libRaytracingGui_la_CPPFLAGS = -DAppPartExport= -DAppRaytracingGuiExport= -DFeatureRayExportPov=

libRaytracingGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lTKernel \
		-lTKG2d \
		-lTKG3d \
		-lTKMath \
		-lTKBool \
		-lTKBRep \
		-lTKGeomAlgo \
		-lTKGeomBase \
		-lPart \
		-lRaytracing

#--------------------------------------------------------------------------------------
# Loader of libRaytracingGui

RaytracingGui_la_SOURCES=\
		AppRaytracingGui.cpp

# the library search path.
RaytracingGui_la_LDFLAGS = $(libRaytracingGui_la_LDFLAGS) -module -avoid-version
RaytracingGui_la_CPPFLAGS = $(libRaytracingGui_la_CPPFLAGS)

RaytracingGui_la_LIBADD   = \
		$(libRaytracingGui_la_LIBADD) \
		-lRaytracingGui

RaytracingGui_la_DEPENDENCIES = libRaytracingGui.la

#--------------------------------------------------------------------------------------

# rule for Qt MetaObject Compiler:
moc_%.cpp: %.h
	$(QT_MOC) $< -o $(@F)

# rule for Qt MetaObject Compiler:
%.moc: %.h
	$(QT_MOC) $< -o $(@F)

# rules for Qt User Interface Compiler:
ui_%.h: %.ui
	$(QT_UIC) $< -o $(@F)

# rules for Qt Resource Compiler:
qrc_%.cpp: Resources/%.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)

FreeCADpov.h: FreeCADpov
	$(PYTHON) $(top_srcdir)/src/Tools/PythonToCPP.py $< $@

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS) -I$(OCC_INC) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)


libdir = $(prefix)/Mod/Raytracing

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		Resources/icons/preferences-raytracing.svg \
		Resources/icons/Raytrace_Camera.svg \
		Resources/icons/Raytrace_Export.svg \
		Resources/icons/Raytrace_ExportProject.svg \
		Resources/icons/Raytrace_New.svg \
		Resources/icons/Raytrace_Part.svg \
		Resources/icons/Raytrace_NewPartSegment.svg \
		DlgSettingsRay.ui \
		FreeCADpov \
		Resources/Raytracing.qrc \
		Resources/translations/Raytracing_af.qm \
		Resources/translations/Raytracing_af.ts \
		Resources/translations/Raytracing_de.qm \
		Resources/translations/Raytracing_de.ts \
		Resources/translations/Raytracing_es-ES.qm \
		Resources/translations/Raytracing_es-ES.ts \
		Resources/translations/Raytracing_fi.qm \
		Resources/translations/Raytracing_fi.ts \
		Resources/translations/Raytracing_fr.qm \
		Resources/translations/Raytracing_fr.ts \
		Resources/translations/Raytracing_hr.qm \
		Resources/translations/Raytracing_hr.ts \
		Resources/translations/Raytracing_it.qm \
		Resources/translations/Raytracing_it.ts \
		Resources/translations/Raytracing_nl.qm \
		Resources/translations/Raytracing_nl.ts \
		Resources/translations/Raytracing_no.qm \
		Resources/translations/Raytracing_no.ts \
		Resources/translations/Raytracing_pl.qm \
		Resources/translations/Raytracing_pl.ts \
		Resources/translations/Raytracing_pt-BR.qm \
		Resources/translations/Raytracing_pt-BR.ts \
		Resources/translations/Raytracing_ru.qm \
		Resources/translations/Raytracing_ru.ts \
		Resources/translations/Raytracing_sv-SE.qm \
		Resources/translations/Raytracing_sv-SE.ts \
		Resources/translations/Raytracing_uk.qm \
		Resources/translations/Raytracing_uk.ts \
		Resources/translations/Raytracing_hu.qm \
		Resources/translations/Raytracing_hu.ts \
		Resources/translations/Raytracing_sk.qm \
		Resources/translations/Raytracing_sk.ts \
		Resources/translations/Raytracing_cs.qm \
		Resources/translations/Raytracing_cs.ts \
		Resources/translations/Raytracing_ro.qm \
		Resources/translations/Raytracing_ro.ts \
		Resources/translations/Raytracing_tr.qm \
		Resources/translations/Raytracing_tr.ts \
		Resources/translations/Raytracing_ja.qm \
		Resources/translations/Raytracing_ja.ts \
		Resources/translations/Raytracing_zh-TW.qm \
		Resources/translations/Raytracing_zh-TW.ts \
		Resources/translations/Raytracing_zh-CN.qm \
		Resources/translations/Raytracing_zh-CN.ts \
		CMakeLists.txt
