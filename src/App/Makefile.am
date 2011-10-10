
lib_LTLIBRARIES=libFreeCADApp.la

BUILT_SOURCES=\
		ComplexGeoDataPy.cpp \
		DocumentObjectGroupPy.cpp \
		DocumentObjectPy.cpp \
		DocumentPy.cpp \
		FeaturePythonPy.cpp \
		MaterialPy.cpp \
		PropertyContainerPy.cpp \
		InitScript.h \
		TestScript.h

libFreeCADApp_la_BUILT=\
		ComplexGeoDataPy.h \
		DocumentObjectGroupPy.h \
		DocumentObjectPy.h \
		DocumentPy.h \
		FeaturePythonPy.h \
		MaterialPy.h \
		PropertyContainerPy.h

libFreeCADApp_la_SOURCES=\
		Annotation.cpp \
		Annotation.h \
		Application.cpp \
		ApplicationPy.cpp \
		ColorModel.cpp \
		ComplexGeoData.cpp \
		ComplexGeoDataPyImp.cpp \
		Document.cpp \
		DocumentObject.cpp \
		DocumentObjectFileIncluded.cpp \
		DocumentObjectGroup.cpp \
		DocumentObjectGroupPyImp.cpp \
		DocumentObjectPyImp.cpp \
		DocumentObserver.cpp \
		DocumentObserverPython.cpp \
		DocumentPyImp.cpp \
		DynamicProperty.cpp \
		FeaturePython.cpp \
		FeaturePythonPyImp.cpp \
		FeatureTest.cpp \
		GeoFeature.cpp \
		InventorObject.cpp \
		VRMLObject.cpp \
		Material.cpp \
		MaterialPyImp.cpp \
		MeasureDistance.cpp \
		Placement.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		Property.cpp \
		PropertyFile.cpp \
		PropertyGeo.cpp \
		PropertyContainer.cpp \
		PropertyContainerPyImp.cpp \
		PropertyLinks.cpp \
		PropertyPythonObject.cpp \
		PropertyStandard.cpp \
		PropertyUnits.cpp \
		Transactions.cpp

includedir = @includedir@/App

nodist_include_HEADERS=\
		$(libFreeCADApp_la_BUILT)

include_HEADERS=\
		Application.h \
		ColorModel.h \
		ComplexGeoData.h \
		Document.h \
		DocumentObject.h \
		DocumentObjectFileIncluded.h \
		DocumentObjectGroup.h \
		DocumentObserver.h \
		DocumentObserverPython.h \
		DynamicProperty.h \
		FeaturePython.h \
		FeatureTest.h \
		GeoFeature.h \
		InventorObject.h \
		VRMLObject.h \
		Material.h \
		MeasureDistance.h \
		Placement.h \
		Property.h \
		PropertyFile.h \
		PropertyGeo.h \
		PropertyContainer.h \
		PropertyLinks.h \
		PropertyPythonObject.h \
		PropertyStandard.h \
		PropertyUnits.h \
		Transactions.h

%Script.h: FreeCAD%.py
	$(PYTHON) $(top_srcdir)/src/Tools/PythonToCPP.py $< $@

%.cpp: %.xml $(top_srcdir)/src/Tools/generateTemplates/templateClassPyExport.py
	$(PYTHON) $(top_srcdir)/src/Tools/generate.py --outputPath $(@D) $<

# the library search path.
libFreeCADApp_la_LDFLAGS = -L../Base $(QT4_CORE_LIBS) $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libFreeCADApp_la_CPPFLAGS = -DRESOURCEDIR=\"$(datadir)\" -DDOCDIR=\"$(docdir)\"

libFreeCADApp_la_LIBADD   = \
		@BOOST_FILESYSTEM_LIB@ \
		@BOOST_PROGOPTIONS_LIB@ \
		@BOOST_SIGNALS_LIB@ \
		@BOOST_SYSTEM_LIB@ \
		@BOOST_REGEX_LIB@ \
		@ZIPIOS_LIB@ \
		-lFreeCADBase \
		-l@PYTHON_LIB@ \
		-lxerces-c

# set the include path found by configure
AM_CXXFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src $(all_includes) $(QT4_CORE_CXXFLAGS)

CLEANFILES = $(BUILT_SOURCES) $(libFreeCADApp_la_BUILT)

EXTRA_DIST = \
		ComplexGeoDataPy.xml \
		DocumentObjectGroupPy.xml \
		DocumentObjectPy.xml \
		DocumentPy.xml \
		FeaturePythonPy.xml \
		MaterialPy.xml \
		PropertyContainerPy.xml \
		core-app.dox \
		CMakeLists.txt \
		FreeCADInit.py \
		FreeCADTest.py



