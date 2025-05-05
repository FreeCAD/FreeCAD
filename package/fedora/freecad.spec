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

#ex: "FreeCAD"
%global git_name {{{ git_name }}}
#ex "freecad-git"
%global package_name {{{ package_name }}}
#ex:"41384 +36 (Git Shallow)"
%global wcvrev   {{{ git_wcrev }}}
#ex: "2025/04/25 10:15:00"
%global wcdate   {{{ git_wcdate }}}
#ex1: "weekly-build"
#ex2: "1.0.0"
%global commit_id {{{ git_commit_hash }}}
%global build_version  {{{ build_version }}}

#ex1: weekly_build
#ex2: "1.0.0"
%global clean_version %{lua:
local ver = rpm.expand("%{build_version}")
    ver = ver:gsub("-","_")
    print(ver)
}
#ex: "20250424.41384+36.Git.1.fc42"
%global release_ver  %{lua:
    local date = rpm.expand("%{wcdate}")
        :match("^[^%s]+")  --remove time
    local vcsrev = rpm.expand("%{wcvrev}")
        :gsub("%s+", "")
        :gsub("%(", ".")
        :gsub("%)", "")
    local commit = rpm.expand("%{commit_id}")
        :sub(1, 7)
    local rev = (date.."."..vcsrev.."."..commit)
        :gsub("-", "")
        :gsub(":", "_")
        :gsub("%s+", ".")
        :gsub("/", "")
    print(rev)
}


Name:           %{package_name}
Epoch:          1
Version:        %clean_version
Release:        %{release_ver}.%{autorelease}

Summary:        A general purpose 3D CAD modeler
Group:          Applications/Engineering
License:        GPL-2.0-or-later
URL:            https://www.freecad.org/

Source0:        {{{ git_repo_pack_with_submodules }}}


# Setup python target for shiboken so the right cmake file is imported.
%global py_suffix %(%{__python3} -c "import sysconfig; print(sysconfig.get_config_var('SOABI'))")

# Maintainers:  keep this list of plugins up to date
# List plugins in %%{_libdir}/%{name}/lib, less '.so' and 'Gui.so', here
%global plugins AssemblyApp AssemblyGui CAMSimulator DraftUtils Fem FreeCAD Import Inspection MatGui Materials Measure Mesh MeshPart Part PartDesignGui Path PathApp PathSimulator Points QtUnitGui ReverseEngineering Robot Sketcher Spreadsheet Start Surface TechDraw Web _PartDesign area flatmesh libDriver libDriverDAT libDriverSTL libDriverUNV libE57Format libMEFISTO2 libOndselSolver libSMDS libSMESH libSMESHDS libStdMeshers libarea-native
%if %{with tests}
 %global plugins %{plugins} libgmock libgmock_main  libgtest libgtest_main
%endif


# See FreeCAD-main/src/3rdParty/salomesmesh/CMakeLists.txt to find this out.
%global bundled_smesh_version 7.7.1.0

# Utilities
BuildRequires:  cmake gcc-c++ gettext doxygen swig graphviz gcc-gfortran desktop-file-utils git tbb-devel
%if %{with tests}
BuildRequires:  xorg-x11-server-Xvfb
%if %{without bundled_gtest}
BuildRequires: gtest-devel gmock-devel
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

# Packages separated because they are noarch, but not optional so require them
# here.
Requires:       %{name}-data = %{epoch}:%{version}-%{release}
# Obsolete old doc package since it's required for functionality.
Obsoletes:      %{name}-doc < 0.22-1

Requires:       hicolor-icon-theme fmt python3-matplotlib python3-pivy python3-collada python3-pyside6 qt6-assistant

%if %{with bundled_smesh}
Provides:       bundled(smesh) = %{bundled_smesh_version}
%endif
%if %{with bundled_pycxx}
Provides:       bundled(python-pycxx)
%endif
Recommends:     python3-pysolar



# plugins and private shared libs in %%{_libdir}/freecad/lib are private;
# prevent private capabilities being advertised in Provides/Requires
%define plugin_regexp /^\\\(libFreeCAD.*%(for i in %{plugins}; do echo -n "\\\|$i\\\|$iGui"; done)\\\)\\\(\\\|Gui\\\)\\.so/d
%{?filter_setup:
%filter_provides_in %{_libdir}/%{name}/lib
%filter_from_requires %{plugin_regexp}
%filter_from_provides %{plugin_regexp}
%filter_provides_in %{_libdir}/%{name}/Mod
%filter_requires_in %{_libdir}/%{name}/Mod
%filter_setup
}

%description
    FreeCAD is a general purpose Open Source 3D CAD/MCAD/CAx/CAE/PLM modeler, aimed
    directly at mechanical engineering and product design but also fits a wider
    range of uses in engineering, such as architecture or other engineering
    specialities. It is a feature-based parametric modeler with a modular software
    architecture which makes it easy to provide additional functionality without
    modifying the core system.


%package data
Summary:        Data files for FreeCAD
BuildArch:      noarch
Requires:       %{name} = %{epoch}:%{version}-%{release}

%description data
    Data files for FreeCAD


#path that contain main FreeCAD sources for cmake
%global _vpath_srcdir  %_builddir/%{git_name}
%global tests_resultdir %{_datadir}/%{name}/tests_result/%{_arch}
%prep
    {{{ git_repo_setup_macro }}}

    # Remove bundled pycxx if we're not using it
    %if %{without bundled_pycxx}
    rm -rf src/CXX
    %endif

    %if %{without bundled_zipios}
    rm -rf src/zipios++
    #sed -i "s/zipios-config.h/zipios-config.hpp/g" \
    #    src/Base/Reader.cpp src/Base/Writer.h
    %endif

# Removed bundled libraries

%build
     cd %_builddir

    # Deal with cmake projects that tend to link excessively.
    LDFLAGS='-Wl,--as-needed -Wl,--no-undefined'; export LDFLAGS

    %define MEDFILE_INCLUDE_DIRS %{_includedir}/med/

     %cmake \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_INSTALL_PREFIX=%{_libdir}/%{name} \
        -DCMAKE_INSTALL_DATADIR=%{_datadir}/%{name} \
        -DCMAKE_INSTALL_DOCDIR=%{_docdir}/%{name} \
        -DCMAKE_INSTALL_INCLUDEDIR=%{_includedir} \
        -DCMAKE_INSTALL_DATAROOTDIR=%{_datadir} \
        -DRESOURCEDIR=%{_datadir}/%{name} \
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
    %endif
        -DONDSELSOLVER_BUILD_EXE=TRUE \
        -DBUILD_GUI=TRUE

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

    rm -rf %{buildroot}%{_datadir}/pkgconfig/OndselSolver.pc
    rm -rf %{buildroot}%{_includedir}/OndselSolver/*

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
#    xvfb-run ./bin/FreeCAD -t 0 &> %{buildroot}%tests_resultdir/FreeCAD_test.result || echo "**** Failed FreeCAD -t 0 ****"
#    tail -n 50 %{buildroot}%tests_resultdir/FreeCAD_test.result
    popd
    %ctest &> %{buildroot}%tests_resultdir/ctest.result                             || echo "Failed ctest"
    tail -n 50 %{buildroot}%tests_resultdir/ctest.result
%endif

    # Bug maintainers to keep %%{plugins} macro up to date.
    #
    # Make sure there are no plugins that need to be added to plugins macro
    new_plugins=`ls %{buildroot}%{_libdir}/%{name}/%{_lib} | sed -e  '%{plugin_regexp}'`
    if [ -n "$new_plugins" ]; then
        echo -e "\n\n\n**** ERROR:\n" \
            "\nPlugins not caught by regexp:  " $new_plugins \
            "\n\nPlugins in %{_libdir}/%{name}/lib do not exist in" \
            "\nspecfile %%{plugins} macro.  Please add these to" \
            "\n%%{plugins} macro at top of specfile and rebuild.\n****\n" 1>&2
        exit 1
    fi
    # Make sure there are no entries in the plugins macro that don't match plugins
    for p in %{plugins}; do
        if [ -z "`ls %{buildroot}%{_libdir}/%{name}/%{_lib}/$p*.so`" ]; then
            set +x
            echo -e "\n\n\n**** ERROR:\n" \
                "\nExtra entry in %%{plugins} macro with no matching plugin:" \
                "'$p'.\n\nPlease remove from %%{plugins} macro at top of" \
                "\nspecfile and rebuild.\n****\n" 1>&2
            exit 1
        fi
    done


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
    #find a way to configure in cmake with %%name to avoid conflict with different package name
    %{python3_sitelib}/freecad/*
%if %{with tests}
    %tests_resultdir/*
%endif


%files data
    %{_datadir}/%{name}/
    %{_docdir}/%{name}/LICENSE.html
    %{_docdir}/%{name}/ThirdPartyLibraries.html

%changelog
    * Mon Mar 10 2025 Leif-Jöran Olsson <info@friprogramvarusyndikatet.se> - 1.1.0-1
    - Adding support for building with Qt6 and PySide6 for Fedora 40+
