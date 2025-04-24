# Some configuration options for other environments
# rpmbuild --without=bundled_zipios: don't use bundled version of zipios++
%bcond_without  bundled_zipios
# rpmbuild --with=bundled_pycxx:  use bundled version of pycxx
%bcond_with bundled_pycxx
# rpmbuild --without=bundled_smesh:  don't use bundled version of Salome's Mesh
%bcond_without bundled_smesh

# rpmbuild --without=tests:  esclude tests in %%check
%bcond_without tests
# rpmbuild --without=bundled_gtest:  don't use bundled version of gtest and gmock
%bcond_without bundled_gtest

%global git_name {{{ git_name }}}
%global wcvrev   {{{ git_repo_release_branched }}}
%global wcurl    {{{ git_repo_vcs }}}
%global wcdate   {{{ echo -n `git log -1 --format="%at" | xargs -I{} date -d @{} +"%Y/%m/%d %T"` }}}
Name:           freecad

Epoch:          1
Version:        1.1.0~dev
Release:        %{wcvrev}%{autorelease}

Summary:        A general purpose 3D CAD modeler
Group:          Applications/Engineering
License:        GPL-2.0-or-later
URL:            https://www.freecad.org/

Source0:        {{{ git_repo_pack }}}
#add all submodule as source
Source1:        {{{ git_pack path=$GIT_ROOT/src/3rdParty/OndselSolver/  dir_name="OndselSolver" }}}
Source2:        {{{ git_pack path=$GIT_ROOT/src/3rdParty/GSL/ dir_name="GSL" }}}
Source3:        {{{ git_pack path=$GIT_ROOT/src/Mod/AddonManager/ dir_name="AddonManager" }}}
%if %{with tests}  && %{with bundled_gtest}
Source4:        {{{ git_pack path=$GIT_ROOT/tests/lib/ name=test-lib dir_name="lib" }}}
%endif


%if %{with bundled_smesh}
# See /src/3rdParty/salomesmesh/CMakeLists.txt to find this out.
%global bundled_smesh_version 7.7.1.0
%endif
%if %{with bundled_pycxx}
# See /src/3rdParty/PyCXX/CXX/Version.h to find this out.
%global bundled_pycxx_version 7.1.9
%endif
# See /src/3rdParty/OndselSolver/CMakeLists.txt to find this out.
%global bundled_ondsel_solver_version 1.0.1

# Utilities
BuildRequires:  cmake gcc-c++ gettext doxygen swig graphviz gcc-gfortran desktop-file-utils tbb-devel ninja-build
%if %{with tests}
BuildRequires:  xorg-x11-server-Xvfb
%if %{without bundled_gtest}
BuildRequires: gtest-devel gmock-devel
%endif
%endif
# Development Libraries
BuildRequires:boost-devel Coin4-devel eigen3-devel freeimage-devel fmt-devel libglvnd-devel libicu-devel libkdtree++-devel libspnav-devel libXmu-devel med-devel mesa-libEGL-devel mesa-libGLU-devel netgen-mesher-devel netgen-mesher-devel-private opencascade-devel openmpi-devel pcl-devel python3 python3-devel python3-matplotlib python3-pivy python3-pybind11 python3-pyside6-devel python3-shiboken6-devel pyside6-tools qt6-qttools-static qt6-qtsvg-devel vtk-devel xerces-c-devel yaml-cpp-devel
%if %{without bundled_smesh}
BuildRequires:  smesh-devel
%endif
%if %{without bundled_zipios}
BuildRequires:  zipios++-devel
%endif
%if %{without bundled_pycxx}
BuildRequires:  python3-pycxx-devel
%endif
# For appdata
%if 0%{?fedora}
BuildRequires:  libappstream-glib
%endif

Requires:       hicolor-icon-theme fmt python3-matplotlib python3-pivy python3-collada python3-pyside6
Requires:       qt6-assistant
# Packages separated because they are noarch, but not optional so require them here.
Requires:       %{name}-data = %{epoch}:%{version}-%{release}
# Obsolete old doc package since it's required for functionality.
Obsoletes:      %{name}-doc < 0.22-1

%if %{with bundled_smesh}
Provides:       bundled(smesh) = %{bundled_smesh_version}
%endif
%if %{with bundled_pycxx}
Provides:       bundled(python3-pycxx) = %{bundled_pycxx_version}
%endif

Provides:       bundled(libondselsolver) = %{bundled_ondsel_solver_version}

Recommends:     python3-pysolar
Recommends:     IfcOpenShell-python3


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
Requires:       %{name} = %{epoch}:%{version}-%{release}

%description data
    Data files for FreeCAD


%package libondselsolver-devel
Summary:        Development file for OndselSolver
BuildArch:      noarch
Requires:       %{name} = %{epoch}:%{version}-%{release}

%description libondselsolver-devel
    Development file for OndselSolver


#path that contain main FreeCAD sources for cmake
%global _vpath_srcdir  %_builddir/%{git_name}
%global tests_resultdir %{_datadir}/%{name}/%{_arch}/tests_result
%prep
    rm -rf %{git_name}

    # extract submodule archive and move in correct path
    %setup -T -a 1 -q -c -D -n %{git_name}/src/3rdParty/ #OndselSolver
    %setup -T -a 2 -q -c -D -n %{git_name}/src/3rdParty/ #GSL
    %setup -T -a 3 -q -c -D -n %{git_name}/src/Mod/ #AddonManager
%if %{with tests}  && %{with bundled_gtest}
    %setup -T -a 4 -q -c -D -n %{git_name}/tests/ #lib
%endif
    %setup -T -b 0 -q -D -n %{git_name}

%build
    cd %_builddir
    # Deal with cmake projects that tend to link excessively.
    LDFLAGS='-Wl,--as-needed -Wl,--no-undefined'; export LDFLAGS

    %cmake \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=%{_libdir}/%{name} \
        -DCMAKE_INSTALL_DATADIR=%{_datadir}/%{name} \
        -DCMAKE_INSTALL_DOCDIR=%{_docdir}/%{name} \
        -DCMAKE_INSTALL_INCLUDEDIR=%{_includedir} \
        -DCMAKE_INSTALL_DATAROOTDIR=%{_datadir} \
        -DRESOURCEDIR=%{_datadir}/%{name} \
        -DSITE_PACKAGE_DIR=%{python3_sitelib}/%{name} \
        -DFREECAD_USE_EXTERNAL_PIVY=TRUE \
        -DFREECAD_USE_EXTERNAL_FMT=TRUE \
        -DFREECAD_USE_PCL:BOOL=OFF \
        -DFREECAD_QT_VERSION:STRING=6 \
        -DOpenGL_GL_PREFERENCE=GLVND \
        -DUSE_OCC=TRUE \
    %if %{without bundled_pycxx}
        -DPYCXX_INCLUDE_DIR=$(pkg-config --variable=includedir PyCXX) \
        -DPYCXX_SOURCE_DIR=$(pkg-config --variable=srcdir PyCXX) \
    %endif
    %if %{without bundled_smesh}
        -DFREECAD_USE_EXTERNAL_SMESH=TRUE \
    %endif
    %if %{without bundled_zipios}
        -DFREECAD_USE_EXTERNAL_ZIPIOS=TRUE \
    %endif
    %if %{with tests}
        -DENABLE_DEVELOPER_TESTS=TRUE \
        -DINSTAL_GTEST=FALSE \
    %else
        -DENABLE_DEVELOPER_TESTS=FALSE \
    %endif
        -DONDSELSOLVER_BUILD_EXE=TRUE \
        -DBUILD_GUI=TRUE \
        -G=Ninja

    #adapt the script src/Tools/SubWCRev.py to extract the info during srpm generation
    #and store it in a place that can be retrived at build time
    #maybe in a string(json) that can be passed as cmake variable;
    sed -i -e 's|"$WCREV$"|"%{wcvrev}"|g' %_vpath_builddir/src/Build/Version.h.in
    sed -i -e 's|"$WCURL$"|"%{wcurl}"|g'  %_vpath_builddir/src/Build/Version.h.in
    sed -i -e 's|"$WDATE$"|"%{wcdate}"|g'  %_vpath_builddir/src/Build/Version.h.in

    %cmake_build

%install
    cd %_builddir
    %cmake_install

    # Symlink binaries to /usr/bin
    mkdir -p %{buildroot}%{_bindir}
    ln -s ../%{_lib}/%{name}/bin/FreeCAD %{buildroot}%{_bindir}/FreeCAD
    ln -s ../%{_lib}/%{name}/bin/FreeCADCmd %{buildroot}%{_bindir}/FreeCADCmd

    # Remove header from external library that's erroneously installed
    rm -f %{buildroot}%{_libdir}/%{name}/include/E57Format/E57Export.h
    rm -rf %{buildroot}%{_includedir}/gmock
    rm -rf %{buildroot}%{_includedir}/gtest


%check
    cd %_builddir
    desktop-file-validate %{buildroot}%{_datadir}/applications/org.freecad.FreeCAD.desktop
    %{?fedora:appstream-util validate-relax --nonet %{buildroot}%{_metainfodir}/*.metainfo.xml}

%if %{with tests}
    mkdir -p %{buildroot}%tests_resultdir
    pushd %_vpath_builddir
    ./tests/Tests_run &> %{buildroot}%tests_resultdir/Tests_run.result              || echo "**** Failed Test_run ****"
    tail -n 50 %{buildroot}%tests_resultdir/Tests_run.result
    ./bin/FreeCADCmd -t 0  &> %{buildroot}%tests_resultdir/FreeCADCmd_test.result   || echo "**** Failed FreeCADCmd -t 0 ****"
    tail -n 50 %{buildroot}%tests_resultdir/FreeCADCmd_test.result
    xvfb-run ./bin/FreeCAD -t 0 &> %{buildroot}%tests_resultdir/FreeCAD_test.result || echo "**** Failed FreeCAD -t 0 ****"
    tail -n 50 %{buildroot}%tests_resultdir/FreeCAD_test.result
    popd
    %ctest &> %{buildroot}%tests_resultdir/ctest.result                             || echo "Failed ctest"
    tail -n 50 %{buildroot}%tests_resultdir/ctest.result
%endif

%post
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
    /usr/bin/update-desktop-database &> /dev/null || :
    /usr/bin/update-mime-database %{_datadir}/mime &> /dev/null || :

%postun
    if [ $1 -eq 0 ] ; then
        /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
        /usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
    fi
    /usr/bin/update-desktop-database &> /dev/null || :
    /usr/bin/update-mime-database %{_datadir}/mime &> /dev/null || :

%posttrans
    /usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor/scalable/apps &>/dev/null || :


%files
    %{_bindir}/*
    %{_metainfodir}/*
    %dir %{_libdir}/%{name}
    %{_libdir}/%{name}/bin/
    %{_libdir}/%{name}/%{_lib}/
    %{_libdir}/%{name}/Ext/
    %{_libdir}/%{name}/Mod/
    %{_datadir}/applications/*
    %{_datadir}/icons/hicolor/*
    %{_datadir}/pixmaps/*
    %{_datadir}/mime/packages/*
    %{_datadir}/thumbnailers/*
    %{python3_sitelib}/%{name}/*
%if %{with tests}
    %tests_resultdir/*
%endif

%files data
    %{_datadir}/%{name}/
    %{_docdir}/%{name}/LICENSE.html
    %{_docdir}/%{name}/ThirdPartyLibraries.html

%files libondselsolver-devel
    %{_datadir}/pkgconfig/OndselSolver.pc
    %{_includedir}/OndselSolver/*


%changelog
    %autochangelog
