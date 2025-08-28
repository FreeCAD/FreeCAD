%bcond external_libkdtree 1
%bcond external_zipios 0
%bcond external_pycxx 0
%bcond external_smesh 0
%bcond external_gsl 1
%bcond tests 1

%global ondselsolver_commit c1f052e
%global gsl_commit 2828399
%global addon_commit 84f08ee

Name:           freecad
Epoch:          1
Version:        1.0.2
Release:        1%{?dist}
Summary:        A general purpose 3D CAD modeler
ExclusiveArch:  %{qt6_qtwebengine_arches}

License:        GPL-2.0-or-later AND LGPL-2.0-or-later AND LGPL-2.1 AND LGPL-2.1-or-later AND GPL-3.0-or-later AND BSD 3-Clause AND BSL-1.0 AND Artistic-2.0 AND MIT AND CC-BY-3.0 AND ASL-2.0 AND zlib
URL:            http://freecadweb.org/
Source0:        https://github.com/FreeCAD/FreeCAD/archive/%{version}%{?pre:_pre}/FreeCAD-%{version}%{?pre:_pre}.tar.gz
Source1:        https://github.com/FreeCAD/OndselSolver/archive/%{ondselsolver_commit}/OndselSolver-%{ondselsolver_commit}.tar.gz
%if %{without external_gsl}
Source2:        https://github.com/microsoft/GSL/archive/%{gsl_commit}/GSL-%{gsl_commit}.tar.gz
%endif
Source3:        https://github.com/FreeCAD/AddonManager/archive/%{addon_commit}/AddonManager-%{addon_commit}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  Coin4-devel
BuildRequires:  cups-devel
BuildRequires:  doxygen
BuildRequires:  eigen3-devel
BuildRequires:  expat-devel
BuildRequires:  fmt-devel
BuildRequires:  freetype-devel
BuildRequires:  gmock-devel
BuildRequires:  gtest-devel
BuildRequires:  hdf5-devel
# Not yet packaged
#BuildRequires:  IfcOpenShell-devel
BuildRequires:  libspnav-devel
BuildRequires:  lz4-devel
BuildRequires:  med-devel
BuildRequires:  netgen-mesher-devel
BuildRequires:  netgen-mesher-devel-private
BuildRequires:  opencascade-devel
BuildRequires:  pyside6-tools
BuildRequires:  python3-devel
# Not yet packaged
#BuildRequires:  python3-IfcOpenShell
BuildRequires:  python3-matplotlib
BuildRequires:  python3-netgen-mesher
BuildRequires:  python3-pivy
BuildRequires:  python3-pybind11
BuildRequires:  python3-pyside6-devel
BuildRequires:  python3-shiboken6-devel
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  swig
BuildRequires:  xerces-c-devel
BuildRequires:  xz-devel
BuildRequires:  yaml-cpp-devel
%if %{with external_gsl}
BuildRequires:  guidelines-support-library-devel
%endif
%if %{with external_libkdtree}
BuildRequires:  libkdtree++-devel
%endif
%if %{with external_pycxx}
BuildRequires:  python3-pycxx-devel
%endif
%if %{with external_smesh}
BuildRequires:  smesh-devel
%endif
%if %{with external_zipios}
BuildRequires:  zipios++-devel
%endif

# For appdata
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

%if %{with tests}
BuildRequires:  xorg-x11-server-Xvfb
BuildRequires:  openscad
%endif

Requires:       hicolor-icon-theme
Requires:       python3-pivy
Requires:       python3-matplotlib
Requires:       python3-collada
Requires:       python3-pyside6

Requires:       %{name}-data = %{epoch}:%{version}-%{release}

Provides:       bundled(smesh) = 5.1.2.2

# These are needed when intsalling, but should be linked as
# private libraries or if public, should have sonames
#global __provides_exclude_from ^%{_libdir}/%{name}/%{_lib}/.*$


%description
FreeCAD is a general purpose Open Source 3D CAD/MCAD/CAx/CAE/PLM modeler, aimed
directly at mechanical engineering and product design but also fits a wider
range of uses in engineering, such as architecture or other engineering
specialties. It is a feature-based parametric modeler with a modular software
architecture which makes it easy to provide additional functionality without
modifying the core system.


%package data
Summary:        Data files for FreeCAD
BuildArch:      noarch

%description data
Data files for FreeCAD


%prep
%autosetup -p1 -n FreeCAD-%{version}
# External libraries which are git submodules
tar xf %{SOURCE1} -C src/3rdParty/OndselSolver --strip-components=1
%if %{without external_gsl}
tar xf %{SOURCE2} -C src/3rdParty/GSL --strip-components=1
%endif
tar xf %{SOURCE3} -C src/Mod/AddonManager --strip-components=1

# Remove bundled libraries which we don't use
%if %{with external_libkdtree}
rm -rf src/3rdParty/libkdtree
%endif
%if %{with external_pycxx}
rm -rf src/CXX
%endif
%if %{with external_smesh}
rm -rf src/3rdParty/salomesmesh
%endif
%if %{with external_zipios}
rm -rf src/zipios++
%endif


%build
%cmake \
  -DFREECAD_QT_MAJOR_VERSION=6 \
  -DCMAKE_INSTALL_PREFIX=%{_libdir}/%{name} \
  -DCMAKE_INSTALL_DATADIR=%{_datadir}/%{name} \
  -DCMAKE_INSTALL_DOCDIR=%{_docdir}/%{name} \
  -DCMAKE_INSTALL_INCLUDEDIR=%{_includedir} \
  -DRESOURCEDIR=%{_datadir}/%{name} \
  -DCOIN3D_INCLUDE_DIR=%{_includedir}/Coin4 \
  -DCOIN3D_DOC_PATH=%{_datadir}/Coin4/Coin \
  -DFREECAD_USE_PYBIND11=ON \
  -DFREECAD_USE_EXTERNAL_FMT=TRUE \
  -DFREECAD_USE_EXTERNAL_GTEST=TRUE \
%if %{with external_pycxx}
  -DPYCXX_INCLUDE_DIR=$(pkg-config --variable=includedir PyCXX) \
  -DPYCXX_SOURCE_DIR=$(pkg-config --variable=srcdir PyCXX) \
%endif
%if %{with external_smesh}
  -DFREECAD_USE_EXTERNAL_SMESH=TRUE \
  -DSMESH_INCLUDE_DIR=%{_includedir}/smesh \
%endif
%if %{with external_zipios}
  -DFREECAD_USE_EXTERNAL_ZIPIOS=TRUE \
%endif
%if %{with tests}
  -DENABLE_DEVELOPER_TESTS=ON \
%endif
  -DBUILD_DRAWING=ON

%cmake_build


%install
%cmake_install
# Symlink binaries to /usr/bin
mkdir -p %{buildroot}%{_bindir}
ln -rs %{buildroot}%{_libdir}/freecad/bin/FreeCAD %{buildroot}%{_bindir}
ln -rs %{buildroot}%{_libdir}/freecad/bin/FreeCADCmd %{buildroot}%{_bindir}

# Install through %%doc and %%license
rm -rf %{buildroot}%{_defaultdocdir}/%{name}/
# Need to figure out if FreeCAD can install correctly at some point.
mv %{buildroot}%{_libdir}/%{name}/share/* %{buildroot}%{_datadir}/
rmdir %{buildroot}%{_libdir}/%{name}/share

# Remove files from external libraries which don't need to be installed
rm -f %{buildroot}%{_libdir}/%{name}/include/E57Format/E57Export.h
rmdir %{buildroot}%{_libdir}/%{name}/include/E57Format/
rmdir %{buildroot}%{_libdir}/%{name}/include/
rm -f %{buildroot}%{_includedir}/OndselSolver/*
rmdir %{buildroot}%{_includedir}/OndselSolver
rmdir %{buildroot}%{_includedir}
rm -f %{buildroot}%{_datadir}/pkgconfig/OndselSolver.pc
rmdir %{buildroot}%{_datadir}/pkgconfig/
# Bytecompile Python modules
%py_byte_compile %{__python3} %{buildroot}%{_libdir}/%{name}


%check
desktop-file-validate %{buildroot}%{_datadir}/applications/org.freecad.FreeCAD.desktop
appstream-util validate-relax --nonet %{buildroot}%{_metainfodir}/org.freecad.FreeCAD.metainfo.xml

%if %{with tests}
pushd %_vpath_builddir
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/AddonManager/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Assembly/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/BIM/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/CAM/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Draft/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Drawing/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Fem/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Help/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Idf/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Import/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Inspection/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Material/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Measure/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Mesh/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/MeshPart/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/OpenSCAD/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Part/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Points/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/ReverseEngineering/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Robot/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Show/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Sketcher/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Spreadsheet/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Start/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Surface/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/TechDraw/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Test/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Tux/"
pp="${pp-}${pp+:}%{buildroot}%{_libdir}/freecad/Mod/Web/"

# Passsing
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestAddonManagerApp
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestAssemblyWorkbench
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestArch
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestCAMApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestDraft
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestFemApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestMaterialsApp
# Passing
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  MeshTestsApp
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestPartApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestPartDesignApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestSketcherApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestSpreadsheet
# Passing
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestSurfaceApp
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestTechDrawApp
# Passing
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  BaseTests
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  UnitTests
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  Document
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  Metadata
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  StringHasher
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  UnicodeTests
PYTHONPATH=$PYTHONPATH:"${pp-}" bin/FreeCADCmd -t  TestPythonSyntax

PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestAddonManagerApp
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestAssemblyWorkbench
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestArch
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestCAMApp
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestDraft
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestFemApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestMaterialsApp
# Passing
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  MeshTestsApp
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestPartApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestPartDesignApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestSketcherApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestSpreadsheet
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestSurfaceApp
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestTechDrawApp
# Passing
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  BaseTests
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  UnitTests
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  Document
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  Metadata
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  StringHasher
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  UnicodeTests
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestPythonSyntax
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestAddonManagerGui
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestArchGui
# Passing
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestDraftGui
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestFemGui
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestImportGui
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestMaterialsGui
# Passing
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestOpenSCADGui
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestPartGui
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestPartDesignGui
# Passing
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestSketcherGui
# Failing
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  TestTechDrawGui
#PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  Workbench
# Passing
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  Menu
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  Menu.MenuDeleteCases
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  Menu.MenuCreateCases
PYTHONPATH=$PYTHONPATH:"${pp-}" xvfb-run bin/FreeCAD -t  GuiDocument
popd

skip="${skip-}${skip+}PropertyExpressionEngineTest.executeCrossPropertyReference|"
skip="${skip-}${skip+}ExpressionParserTest.functionPARSEQUANT|"
skip="${skip-}${skip+}ExpressionParserTest.isTokenAConstant|"
skip="${skip-}${skip+}DocumentObjectTest.getSubObjectList|"
skip="${skip-}${skip+}TestModel.TestInstallation|"
skip="${skip-}${skip+}TestModel.TestModelLoad|"
skip="${skip-}${skip+}TestModel.TestModelByPath|"
skip="${skip-}${skip+}TestMaterial.TestInstallation|"
skip="${skip-}${skip+}TestMaterial.TestMaterialsWithModel|"
skip="${skip-}${skip+}TestMaterial.TestMaterialByPath|"
skip="${skip-}${skip+}TestMaterial.TestAddPhysicalModel|"
skip="${skip-}${skip+}TestMaterial.TestAddAppearanceModel|"
skip="${skip-}${skip+}TestMaterial.TestCalculiXSteel|"
skip="${skip-}${skip+}TestMaterial.TestColumns|"
skip="${skip-}${skip+}TestMaterialFilter.TestFilters|"
skip="${skip-}${skip+}TestMaterialCards.TestCopy|"
skip="${skip-}${skip+}TestMaterialCards.TestColumns|"
skip="${skip-}${skip+}MeasureDistance.testCircleCircle|"
skip="${skip-}${skip+}ImporterTest.Test3MF|"
skip="${skip-}${skip+}WireJoinerTest.setOutline|"
skip="${skip-}${skip+}FeaturePartMakeElementRefineTest.makeElementRefineBoxes|"
skip="${skip-}${skip+}PropertyTopoShapeTest.testPropertyPartShapeTopoShape|"
skip="${skip-}${skip+}PropertyTopoShapeTest.testPropertyPartShapeTopoDSShape|"
skip="${skip-}${skip+}PropertyTopoShapeTest.testPropertyPartShapeGetPyObject|"
skip="${skip-}${skip+}PropertyTopoShapeTest.testShapeHistory|"
skip="${skip-}${skip+}PropertyTopoShapeTest.testPropertyShapeHistory|"
skip="${skip-}${skip+}PropertyTopoShapeTest.testPropertyShapeCache|"
skip="${skip-}${skip+}PropertyTopoShapeTest.testPropertyShapeCachePyObj|"
skip="${skip-}${skip+}PartFeaturesTest.testRuledSurface|"
skip="${skip-}${skip+}PartFeaturesTest.testLoft|"
skip="${skip-}${skip+}PartFeaturesTest.testSweep|"
skip="${skip-}${skip+}PartFeaturesTest.testThickness|"
skip="${skip-}${skip+}PartFeaturesTest.testRefine|"
skip="${skip-}${skip+}PartFeaturesTest.testReverse|"
skip="${skip-}${skip+}FeaturePartTest.testGetElementName|"
skip="${skip-}${skip+}FeaturePartTest.create|"
skip="${skip-}${skip+}FeaturePartTest.getElementHistory|"
skip="${skip-}${skip+}FeaturePartTest.getRelatedElements|"
skip="${skip-}${skip+}FeaturePartTest.getElementFromSource|"
skip="${skip-}${skip+}FeaturePartTest.getSubObject|"
skip="${skip-}${skip+}FeaturePartTest.getElementTypes|"
skip="${skip-}${skip+}FeaturePartTest.getComplexElementTypes|"
skip="${skip-}${skip+}GeometryTest.testTrimBSpline|"
skip="${skip-}${skip+}FuzzyBooleanTest.testLoadedCorrectly|"
skip="${skip-}${skip+}FuzzyBooleanTest.testDefaultFuzzy|"
skip="${skip-}${skip+}FuzzyBooleanTest.testGoodFuzzy|"
skip="${skip-}${skip+}FuzzyBooleanTest.testFailsTooSmallFuzzy|"
skip="${skip-}${skip+}FuzzyBooleanTest.testCompletelyFailsTooBigFuzzy|"
skip="${skip-}${skip+}FeatureRevolutionTest.testExecute|"
skip="${skip-}${skip+}FeatureRevolutionTest.testExecuteBase|"
skip="${skip-}${skip+}FeatureRevolutionTest.testAxis|"
skip="${skip-}${skip+}FeatureRevolutionTest.testAxisLink|"
skip="${skip-}${skip+}FeatureRevolutionTest.testSymmetric|"
skip="${skip-}${skip+}FeatureRevolutionTest.testAngle|"
skip="${skip-}${skip+}FeatureRevolutionTest.testMustExecute|"
skip="${skip-}${skip+}FeatureRevolutionTest.testGetProviderName|"
skip="${skip-}${skip+}FeaturePartFuseTest.testIntersecting|"
skip="${skip-}${skip+}FeaturePartFuseTest.testCompound|"
skip="${skip-}${skip+}FeaturePartFuseTest.testRecursiveCompound|"
skip="${skip-}${skip+}FeaturePartFuseTest.testNonIntersecting|"
skip="${skip-}${skip+}FeaturePartFuseTest.testTouching|"
skip="${skip-}${skip+}FeaturePartFuseTest.testAlmostTouching|"
skip="${skip-}${skip+}FeaturePartFuseTest.testBarelyIntersecting|"
skip="${skip-}${skip+}FeaturePartFuseTest.testMustExecute|"
skip="${skip-}${skip+}FeaturePartFuseTest.testGetProviderName|"
skip="${skip-}${skip+}FeaturePartFuseTest.testRefine|"
skip="${skip-}${skip+}FeaturePartCutTest.testIntersecting|"
skip="${skip-}${skip+}FeaturePartCutTest.testNonIntersecting|"
skip="${skip-}${skip+}FeaturePartCutTest.testTouching|"
skip="${skip-}${skip+}FeaturePartCutTest.testAlmostTouching|"
skip="${skip-}${skip+}FeaturePartCutTest.testBarelyIntersecting|"
skip="${skip-}${skip+}FeaturePartCutTest.testMustExecute|"
skip="${skip-}${skip+}FeaturePartCutTest.testGetProviderName|"
skip="${skip-}${skip+}FeaturePartCutTest.testMapping|"
skip="${skip-}${skip+}FeaturePartCommonTest.testIntersecting|"
skip="${skip-}${skip+}FeaturePartCommonTest.testNonIntersecting|"
skip="${skip-}${skip+}FeaturePartCommonTest.testTouching|"
skip="${skip-}${skip+}FeaturePartCommonTest.testAlmostTouching|"
skip="${skip-}${skip+}FeaturePartCommonTest.testBarelyIntersecting|"
skip="${skip-}${skip+}FeaturePartCommonTest.testMustExecute|"
skip="${skip-}${skip+}FeaturePartCommonTest.testGetProviderName|"
skip="${skip-}${skip+}FeaturePartCommonTest.testHistory|"
skip="${skip-}${skip+}FeaturePartCommonTest.testMapping|"
skip="${skip-}${skip+}FeatureOffsetTest.testOffset3D|"
skip="${skip-}${skip+}FeatureOffsetTest.testOffset3DWithExistingElementMap|"
skip="${skip-}${skip+}FeatureOffsetTest.testOffset2D|"
skip="${skip-}${skip+}FeatureMirroringTest.testXMirror|"
skip="${skip-}${skip+}FeatureMirroringTest.testYMirrorWithExistingElementMap|"
skip="${skip-}${skip+}FeatureFilletTest.testOtherEdges|"
skip="${skip-}${skip+}FeatureFilletTest.testMostEdges|"
skip="${skip-}${skip+}FeatureFilletTest.testMustExecute|"
skip="${skip-}${skip+}FeatureFilletTest.testGetProviderName|"
skip="${skip-}${skip+}FeatureExtrusionTest.testMustExecute|"
skip="${skip-}${skip+}FeatureExtrusionTest.testGetProviderName|"
skip="${skip-}${skip+}FeatureExtrusionTest.testFetchAxisLink|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExtrudeShape|"
skip="${skip-}${skip+}FeatureExtrusionTest.testComputeFinalParameters|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteSimple|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteSimpleRev|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteSolid|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteReverse|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteSymmetric|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteAngled|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteAngledRev|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteEdge|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteDir|"
skip="${skip-}${skip+}FeatureExtrusionTest.testExecuteFaceMaker|"
skip="${skip-}${skip+}FeatureExtrusionTest.testFaceWithHoles|"
skip="${skip-}${skip+}FeatureCompoundTest.testIntersecting|"
skip="${skip-}${skip+}FeatureCompoundTest.testNonIntersecting|"
skip="${skip-}${skip+}FeatureChamferTest.testOther|"
skip="${skip-}${skip+}FeatureChamferTest.testMost|"
skip="${skip-}${skip+}FeatureChamferTest.testMustExecute|"
skip="${skip-}${skip+}FeatureChamferTest.testGetProviderName|"
skip="${skip-}${skip+}AttachExtensionTest.testPlanePlane|"
skip="${skip-}${skip+}AttachExtensionTest.testAttacherEngineType|"
skip="${skip-}${skip+}AttachExtensionTest.testAttacherTypeEngine|"
skip="${skip-}${skip+}AttacherTest.TestSetReferences|"
skip="${skip-}${skip+}AttacherTest.TestSuggestMapModes|"
skip="${skip-}${skip+}AttacherTest.TestGetShapeType|"
skip="${skip-}${skip+}AttacherTest.TestGetInertialPropsOfShape|"
skip="${skip-}${skip+}AttacherTest.TestGetRefObjects|"
skip="${skip-}${skip+}AttacherTest.TestCalculateAttachedPlacement|"
skip="${skip-}${skip+}AttacherTest.TestAllStringModesValid|"
skip="${skip-}${skip+}AttacherTest.TestAllModesBoundaries|"
skip="${skip-}${skip+}ShapeBinderTest.shapeBinderExists|"
skip="${skip-}${skip+}ShapeBinderTest.subShapeBinderExists|"
skip="${skip-}${skip+}DatumPlaneTest.attachDatumPlane|"
skip="${skip-}${skip+}SketchObjectTest.createSketchObject|"
skip="${skip-}${skip+}SketchObjectTest.testGeoIdFromShapeTypeEdge|"
skip="${skip-}${skip+}SketchObjectTest.testGeoIdFromShapeTypeVertex|"
skip="${skip-}${skip+}SketchObjectTest.testGeoIdFromShapeTypeExternalEdge|"
skip="${skip-}${skip+}SketchObjectTest.testGeoIdFromShapeTypeHAxis|"
skip="${skip-}${skip+}SketchObjectTest.testGeoIdFromShapeTypeVAxis|"
skip="${skip-}${skip+}SketchObjectTest.testGeoIdFromShapeTypeRootPoint|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionNoUnits1|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionNoUnits2|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionWithUnits1|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionWithUnits2|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionWithUnits3|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionWithUnits4|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionApplyAndReverse1|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionApplyAndReverse2|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionSimple|"
skip="${skip-}${skip+}SketchObjectTest.testReverseAngleConstraintToSupplementaryExpressionApplyAndReverse|"
skip="${skip-}${skip+}SketchObjectTest.testGetElementName|"
skip="${skip-}${skip+}ReaderTest.charStreamBase64Encoded|"
skip="${skip-}${skip+}QuantitySpinBox_Tests_run"
%ctest -E "${skip-}"
%endif

%files
%license %{_vpath_builddir}%{_defaultdocdir}/freecad/ThirdPartyLibraries.html  LICENSE
%doc README.md
%{_bindir}/FreeCAD
%{_bindir}/FreeCADCmd
%dir %{_libdir}/%{name}
%dir %{_libdir}/%{name}/bin/
%{_libdir}/%{name}/bin/FreeCAD
%{_libdir}/%{name}/bin/FreeCADCmd
%{_libdir}/%{name}/bin/freecad-thumbnailer
%{_libdir}/%{name}/Ext/
%{_libdir}/%{name}/%{_lib}/
%{_libdir}/%{name}/Mod/
%{python3_sitelib}/freecad/
%{_datadir}/applications/org.freecad.FreeCAD.desktop
%{_datadir}/icons/hicolor/*/apps/org.freecad.FreeCAD.png
%{_datadir}/icons/hicolor/scalable/apps/org.freecad.FreeCAD.svg
%{_datadir}/icons/hicolor/scalable/mimetypes/application-x-extension-fcstd.svg
%{_datadir}/pixmaps/freecad.svg
%{_datadir}/mime/packages/org.freecad.FreeCAD.xml
%{_datadir}/thumbnailers/FreeCAD.thumbnailer
%{_metainfodir}/org.freecad.FreeCAD.metainfo.xml

%files data
%license LICENSE
%{_datadir}/%{name}/


%changelog
* Sun Aug 10 2025 Sandro Mani <manisandro@gmail.com> - 1:1.0.2-1
- Update to 1.0.2
- Build against external guidelines-support-library
- List other licenses
- Filter providers

* Sat Aug 02 2025 Sandro Mani <manisandro@gmail.com> - 1:1.0.1-1
- Update to 1.0.1

* Mon Nov 25 2024 Sandro Mani <manisandro@gmail.com> - 1:1.0.0-1
- Update to 1.0.0

* Wed Jul 19 2023 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.20.2-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_39_Mass_Rebuild

* Mon Feb 20 2023 Jonathan Wakely <jwakely@redhat.com> - 1:0.20.2-4
- Rebuilt for Boost 1.81

* Thu Jan 19 2023 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.20.2-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_38_Mass_Rebuild

* Sun Jan 15 2023 Orion Poplawski <orion@nwra.com> - 1:0.20.2-2
- Rebuild for vtk 9.2.5

* Fri Jan 13 2023 Richard Shaw <hobbes1069@gmail.com> - 1:0.20.2-1
- Update to 0.20.2.

* Tue Aug 30 2022 Richard Shaw <hobbes1069@gmail.com> - 1:0.20.1-1.1
- Rebuild for retagged upstream source, fixes rhbz#2121671.
- Readd Python 3.11 patches that did not make it into the current release.

* Tue Aug 09 2022 Richard Shaw <hobbes1069@gmail.com> - 1:0.20.1-1
- Update to 0.20.1.

* Thu Jul 21 2022 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.20-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_37_Mass_Rebuild

* Wed Jun 29 2022 Richard Shaw <hobbes1069@gmail.com> - 1:0.20-1
- Update to 0.20.

* Fri Jun 24 2022 Jonathan Wakely <jwakely@redhat.com> -1:0.19.4-4
- Remove obsolete boost-python3-devel build dependency (#2100748)

* Wed May 11 2022 Richard Shaw <hobbes1069@gmail.com> - 1:0.19.4-3
- Add patch to provide std::unique_ptr, fixes #2084307.

* Wed May 04 2022 Thomas Rodgers <trodgers@redhat.com> - 1:0.19.4-2
- Rebuilt for Boost 1.78

* Tue Mar 01 2022 Richard Shaw <hobbes1069@gmail.com> - 1:0.19.4-1
- Update to 0.19.4.

* Sat Jan 29 2022 Richard Shaw <hobbes1069@gmail.com> - 1:0.19.3-1
- Update to 0.19.3.

* Thu Jan 20 2022 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.19.2-7
- Rebuilt for https://fedoraproject.org/wiki/Fedora_36_Mass_Rebuild

* Thu Nov 25 2021 Orion Poplawski <orion@nwra.com> - 1:0.19.2-6
- Rebuild for vtk 9.1.0

* Thu Aug 19 2021 Richard Shaw <hobbes1069@gmail.com> - 1:0.19.2-5
- Add patch from upstream for better vtk9 compatibility.

* Fri Aug 06 2021 Jonathan Wakely <jwakely@redhat.com> - 1:0.19.2-4
- Rebuilt for Boost 1.76

* Wed Jul 21 2021 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.19.2-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_35_Mass_Rebuild

* Fri Jun 04 2021 Python Maint <python-maint@redhat.com> - 1:0.19.2-2
- Rebuilt for Python 3.10

* Wed May 05 2021 Richard Shaw <hobbes1069@gmail.com> - 1:0.19.2-1
- Update to 0.19.2.

* Tue Mar 30 2021 Jonathan Wakely <jwakely@redhat.com> - 1:0.19-0.8.20210130git4db83a41ca
- Rebuilt for removed libstdc++ symbol (#1937698)

* Sun Feb 21 2021 Richard Shaw <hobbes1069@gmail.com> - 1:0.19-0.8.20210221git110860fa47
- Update to 110860fa4700dabf263918f80afcc75982b7dc37.

* Sun Jan 31 2021 Orion Poplawski <orion@nwra.com> - 1:0.19-0.7.20210130git4db83a41ca
- Rebuild for VTK 9

* Sat Jan 30 2021 Richard Shaw <hobbes1069@gmail.com> - 1:0.19-0.6.20210130git4db83a41ca
- Update to 0.19pre, git 4db83a41ca5800a0238a3030c81e33950c3070a3.

* Tue Jan 26 2021 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.19-0.5.20201125gita50ae33557
- Rebuilt for https://fedoraproject.org/wiki/Fedora_34_Mass_Rebuild

* Fri Jan 22 2021 Jonathan Wakely <jwakely@redhat.com> - 1:0.19-0.4.20201125gita50ae33557
- Rebuilt for Boost 1.75

* Wed Nov 25 2020 Richard Shaw <hobbes1069@gmail.com> - 1:0.19-0.3.20201125gita50ae33557
- Rebuild with OCC 7.5.0.

* Wed Nov 25 2020 Richard Shaw <hobbes1069@gmail.com> - 1:0.19-0.2.20201125gita50ae33557
- Update to latest git checkout, properly fixes ambiguous reference in
  Part/Sketcher.

* Wed Nov 25 2020 Richard Shaw <hobbes1069@gmail.com> - 1:0.19-0.1.20201124git6bd39e8a90
- Update to 0.19 pre-release.

* Mon Nov 23 2020 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.4-13
- Rebuild for OpenCascade 7.5.0.

* Sat Aug 01 2020 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.18.4-12
- Second attempt - Rebuilt for
  https://fedoraproject.org/wiki/Fedora_33_Mass_Rebuild

* Mon Jul 27 2020 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.18.4-11
- Rebuilt for https://fedoraproject.org/wiki/Fedora_33_Mass_Rebuild

* Sat Jun 20 2020 Miro Hrončok <mhroncok@redhat.com> - 1:0.18.4-10
- Bytecompile Python modules

* Wed Jun 03 2020 Scott Talbert <swt@techie.net> - 1:0.18.4-9
- Fix build with unbundled pycxx

* Tue May 26 2020 Miro Hrončok <mhroncok@redhat.com> - 1:0.18.4-8
- Rebuilt for Python 3.9

* Tue May 05 2020 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.4-7
- Rebuild for Pyside2 5.14.

* Tue Jan 28 2020 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.18.4-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_32_Mass_Rebuild

* Thu Jan 16 2020 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.4-5
- Move < f32 back to Coin3.

* Thu Jan 09 2020 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.4-2
- Rebuild for Qt/PySide 5.13.2.

* Tue Nov 05 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.4-1
- Update to 0.18.4.

* Mon Nov 04 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.3-7
- Fix python3-pyside2 requires and other specfile cleanup.

* Mon Oct 28 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.3-6
- Rebuild for downgraded PySide2 so the version matches with Qt5.

* Thu Oct 10 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.3-5.1
- Rebuild for Coin4 and python-pyside2 on rawhide (f32).
- Rebuild for python-pyside2 only for others.

* Mon Aug 19 2019 Miro Hrončok <mhroncok@redhat.com> - 1:0.18.3-4
- Rebuilt for Python 3.8

* Sat Jul 27 2019 Ivan Mironov <mironov.ivan@gmail.com> - 1:0.18.3-3
- Build C++ code with usual CXXFLAGS (including -O2)

* Thu Jul 25 2019 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.18.3-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_31_Mass_Rebuild

* Thu Jul 18 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.3-1
- Update to 0.18.3.

* Mon May 20 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.2-3
- Bump release so NVER is higher on f31 than f30 & f29.

* Sun May 19 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.2-2
- Remove more python2 dependencies and fix shiboken building with python2.

* Sun May 12 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18.2-1
- Update to 0.18.2.
- Hopefully fix python3 issues.

* Sun Mar 24 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18-2
- Rebuild to require python3 pivy and collada.

* Wed Mar 13 2019 Richard Shaw <hobbes1069@gmail.com> - 1:0.18-1
- Update to 0.18.

* Thu Jan 31 2019 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.17-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_30_Mass_Rebuild

* Fri Jul 13 2018 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.17-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_29_Mass_Rebuild

* Tue Apr 10 2018 Richard Shaw <hobbes1069@gmail.com> - 1:0.17-1
- Update to 0.17 release.

* Sat Mar 31 2018 Richard Shaw <hobbes1069@gmail.com> - 1:0.17-0.1
- Update to 0.17 prerelease.

* Wed Mar 07 2018 Adam Williamson <awilliam@redhat.com> - 1:0.16-12
- Rebuild to fix GCC 8 mis-compilation
  See https://da.gd/YJVwk ("GCC 8 ABI change on x86_64")

* Wed Feb 07 2018 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.16-11
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Wed Aug 23 2017 Richard Shaw <hobbes1069@gmail.com> - 1:0.16-10
- Add qt-assistant so that help works properly.

* Wed Aug 02 2017 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.16-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Binutils_Mass_Rebuild

* Wed Jul 26 2017 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.16-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Fri Jul 21 2017 Kalev Lember <klember@redhat.com> - 1:0.16-7
- Rebuilt for Boost 1.64

* Thu May 11 2017 Richard Shaw <hobbes1069@gmail.com> - 1:0.16-6
- Rebuild for OCE 0.18.1.

* Tue Feb 07 2017 Kalev Lember <klember@redhat.com> - 1:0.16-5
- Rebuilt for Boost 1.63

* Wed Dec 28 2016 Rich Mattes <richmattes@gmail.com> - 1:0.16-4
- Rebuild for eigen3-3.3.1

* Mon Sep 26 2016 Dominik Mierzejewski <rpm@greysector.net> - 1:0.16-3
- rebuilt for matplotlib-2.0.0

* Tue May 17 2016 Jonathan Wakely <jwakely@redhat.com> - 1:0.16-2
- Rebuilt for linker errors in boost (#1331983)

* Wed Apr 13 2016 Richard Shaw <hobbes1069@gmail.com> - 1:0.16-1
- Update to latest upstream release.

* Wed Apr  6 2016 Richard Shaw <hobbes1069@gmail.com> - 1:0.16-0.1
- Update to 0.16 prerelease.

* Wed Feb 03 2016 Fedora Release Engineering <releng@fedoraproject.org> - 1:0.15-12
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Thu Jan 28 2016 Jonathan Wakely <jwakely@redhat.com> 0.15-11
- Patched and rebuilt for Boost 1.60

* Mon Jan  4 2016 Richard Shaw <hobbes1069@gmail.com> - 1:0.15-10
- Fix appdata license, fixes BZ#1294623.

* Thu Aug 27 2015 Jonathan Wakely <jwakely@redhat.com> - 1:0.15-9
- Rebuilt for Boost 1.59

* Wed Jul 29 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1:0.15-8
- Rebuilt for https://fedoraproject.org/wiki/Changes/F23Boost159

* Wed Jul 22 2015 David Tardon <dtardon@redhat.com> - 1:0.15-7
- rebuild for Boost 1.58

* Wed Jun 17 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1:0.15-6
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Thu May 28 2015 Richard Shaw <hobbes1069@gmail.com> - 1:0.15-5
- Fix version reporting in the About dialog (BZ#1192841).

* Tue May 19 2015 Richard Shaw <hobbes1069@gmail.com> - 1:0.15-4
- Bump Epoch to downgrade to 0.14 for Fedora 21 and below due to Coin2/Coin3
  library mismatch between Freecad & python-pivy (BZ#1221713).

* Fri Apr 10 2015 Richard Shaw <hobbes1069@gmail.com> - 0.15-1
- Update to latest upstream release.

* Tue Jan 27 2015 Petr Machata <pmachata@redhat.com> - 0.14-6
- Rebuild for boost 1.57.0

* Tue Jan  6 2015 Richard Shaw <hobbes1069@gmail.com> - 0.14-5
- Fix bug introduced by PythonSnap patch, fixes BZ#1178672.

* Thu Sep 18 2014 Richard Shaw <hobbes1069@gmail.com> - 0.14-4
- Patch PythonSnap, fixes BZ#1143814.

* Sat Aug 16 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.14-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Mon Aug  4 2014 Richard Shaw <hobbes1069@gmail.com> - 0.14-2
- Add python-pyside as requirement as it is not currently being pulled in as a
  automatic dependency by rpmbuild.

* Wed Jul 16 2014 Richard Shaw <hobbes1069@gmail.com> - 0.14-1
- Update to latest upstream release.

* Mon Jun 23 2014 John Morris <john@zultron.com> - 0.13-10
- Add Requires: qt-assistant for bz #1112045

* Thu Jun 19 2014 Richard Shaw <hobbes1069@gmail.com> - 0.13-9
- Fix obsoletes of old documentation subpackage.
- Add conditional so EPEL 6 ppc64 does not require python-pivy which does not
  build on that platform.

* Sat Jun 07 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 0.13-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Thu May 29 2014 Richard Shaw <hobbes1069@gmail.com> - 0.13-7
- Update OCE patch with bad conditional which caused undefined symbols.

* Fri May 23 2014 Richard Shaw <hobbes1069@gmail.com> - 0.13-6
- Fix duplicate documentation.
- Correct license tag to GPLv2+.

* Mon May 19 2014 Richard Shaw <hobbes1069@gmail.com> - 0.13-5
- Move noarch data into it's own subpackage.
- Fix cmake conditionals to work for epel7.

* Thu Oct 10 2013 Richard Shaw <hobbes1069@gmail.com> - 0.13-4
- Rebuild for OCE 0.13.

* Mon Jul 15 2013 Richard Shaw <hobbes1069@gmail.com> - 0.13-3
- Rebuild for updated OCE.

* Mon Apr 29 2013 Nicolas Chauvet <kwizart@gmail.com> - 0.13-2
- https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Mon Feb 18 2013 Richard Shaw <hobbes1069@gmail.com> - 0.13-1
- Update to latest upstream release.

* Sat Oct 20 2012 John Morris <john@zultron.com> - 0.12-9
- Use cmake28 package on el6
- Remove COIN3D_DOC_PATH cmake def (one less warning during build)
- Add PyQt as requirement.
- Add libicu-devel as build requirement.

* Wed Sep 26 2012 Richard Shaw <hobbes1069@gmail.com> - 0.12-8
- Rebuild for boost 1.50.

* Thu Jul 05 2012 Richard Shaw <hobbes1069@gmail.com> - 0.12-7
- Remove BuildRequires: tbb-devel and gts-devel
- Add missing license files to %%doc.
- Add missing requirement for hicolor-icon-theme.
- Fix excessive linking issue.
- Other minor spec updates.

* Mon Jun 25 2012  <john@zultron.com> - 0.12-6
- Filter out automatically generated Provides/Requires of private libraries
- freecad.desktop not passing 'desktop-file-validate'; fixed
- Remove BuildRequires: tbb-devel and gts-devel
- Update license tag to GPLv3+ only.
- Add missing license files to %%doc.
- Add missing build requirement for hicolor-icon-theme.
- Fix excessive linking issue.
- Other minor spec updates.

* Mon Jun 25 2012  <john@zultron.com> - 0.12-5
- New patch to unbundle PyCXX
- Add conditional build options for OpenCASCADE, bundled Zipios++,
  bundled PyCXX, bundled smesh

* Tue Jun 19 2012 Richard Shaw <hobbes1069@gmail.com> - 0.12-4
- Add linker flag to stop excessive linking.

* Thu May 31 2012 Richard Shaw <hobbes1069@gmail.com> - 0.12-3
- Add patch for GCC 4.7 on Fedora 17.

* Thu Nov 10 2011 Richard Shaw <hobbes1069@gmail.com> - 0.12-2
- Initial release.
