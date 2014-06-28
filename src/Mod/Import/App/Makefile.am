
lib_LTLIBRARIES=libImport.la Import.la

BUILT_SOURCES=\
	StepShapePy.cpp \
	StepShapePy.h

libImport_la_SOURCES=\
		AppImportPy.cpp \
		ImportOCAF.cpp \
		ImportOCAF.h \
		PreCompiled.cpp \
		PreCompiled.h \
		StepShape.cpp \
		StepShape.h \
		StepShapePyImp.cpp 



# the library search path.
libImport_la_LDFLAGS = -L../../../Base -L../../../App -L../../Part/App $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libImport_la_CPPFLAGS = -DAppPartExport=

libImport_la_LIBADD   = \
		-lTKIGES \
		-lTKSTEP \
		-lFreeCADBase \
		-lFreeCADApp \
		-lPart

#--------------------------------------------------------------------------------------
# Loader of libImport

Import_la_SOURCES=\
		AppImport.cpp

# the library search path.
Import_la_LDFLAGS = $(libImport_la_LDFLAGS) -module -avoid-version
Import_la_CPPFLAGS = $(libImport_la_CPPFLAGS)

Import_la_LIBADD   = \
		$(libImport_la_LIBADD) \
		-lImport

Import_la_DEPENDENCIES = libImport.la

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src \
		$(all_includes) -I$(OCC_INC) -I../../../

libdir = $(prefix)/Mod/Import

%.cpp: %.xml $(top_srcdir)/src/Tools/generateTemplates/templateClassPyExport.py
	$(PYTHON) $(top_srcdir)/src/Tools/generate.py --outputPath $(@D) $<

# We need this softlink for Python to load
install-data-local:
		cd $(DESTDIR)$(libdir) && \
		rm -f Import.so && \
		$(LN_S) libImport.so Import.so

EXTRA_DIST = \
    	SCL/__init__.py \
    	SCL/AggregationDataTypes.py \
    	SCL/Aufspannung.stp \
    	SCL/BaseType.py \
    	SCL/Builtin.py \
    	SCL/ConstructedDataTypes.py \
    	SCL/essa_par.py \
    	SCL/gasket1.p21 \
    	SCL/Model.py \
    	SCL/Part21.py \
    	SCL/Rules.py \
    	SCL/SCLBase.py \
    	SCL/SimpleDataTypes.py \
    	SCL/SimpleReader.py \
    	SCL/TypeChecker.py \
    	SCL/Utils.py \
		FeatureImportIges.cpp \
		FeatureImportIges.h \
		FeatureImportStep.cpp \
		FeatureImportStep.h \
    	ap203_configuration_controlled_3d_design_of_mechanical_parts_and_assemblies_mim_lf.py \
    	automotive_design.py \
    	CMakeLists.txt \
    	config_control_design.py \
    	ifc2x3.py \
    	ifc4.py \
    	StepShapePy.xml 
