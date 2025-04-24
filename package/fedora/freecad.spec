Name:           freecad

Epoch:          1
Version:        {{{ git_repo_version  lead=1.1 follow=0~pre }}}.{{{ echo $GIT_BRANCH }}}
Release:        %autorelease

Summary:        A general purpose 3D CAD modeler
Group:          Applications/Engineering

License:        GPL-2.0-or-later
URL:            https://www.freecad.org/
VCS:            {{{ git_repo_vcs }}}

Source0:        {{{git_repo_pack_with_submodules}}}



# Maintainers:  keep this list of plugins up to date
# List plugins in %%{_libdir}/%%{name}/lib, less '.so' and 'Gui.so', here
%global plugins AssemblyApp AssemblyGui CAMSimulator DraftUtils Fem FreeCAD Import Inspection MatGui Materials Measure Mesh MeshPart Part PartDesignGui Path PathApp PathSimulator Points QtUnitGui ReverseEngineering Robot Sketcher Spreadsheet Start Surface TechDraw Web _PartDesign area flatmesh libDriver  libDriverDAT libDriverSTL libDriverUNV libE57Format libMEFISTO2 libSMDS libSMESH libSMESHDS libStdMeshers libarea-native

%global exported_libs "libOndselSolver"
# See /src/3rdParty/salomesmesh/CMakeLists.txt to find this out.
%global bundled_smesh_version 7.7.1.0
# See /src/3rdParty/PyCXX/CXX/Version.h to find this out.
%global bundled_pycxx_version 7.1.9
# See /src/3rdParty/OndselSolver/CMakeLists.txt to find this out.
%global bundled_ondsel_solver_version 1.0.1

# Some configuration options for other environments
# rpmbuild --without=bundled_zipios: don't use bundled version of zipios++
%bcond_without  bundled_zipios
# rpmbuild --with=bundled_pycxx:  use bundled version of pycxx
%bcond_with bundled_pycxx
# rpmbuild --without=bundled_smesh:  don't use bundled version of Salome's Mesh
%bcond_without bundled_smesh

# rpmbuild --with=tests:  include  tests in build
%bcond_with tests
%if %{with tests}
%global plugins %{plugins} libgmock libgmock_main  libgtest libgtest_main
%global enabled_tests MeshTestsApp TestSurfaceApp BaseTests UnitTests Metadata StringHasher UnicodeTests TestPythonSyntax
%global enabled_gui_tests MeshTestsApp TestSurfaceApp BaseTests UnitTests Metadata StringHasher UnicodeTests TestPythonSyntax TestDraftGui TestFemGui TestSketcherGui Menu Menu.MenuDeleteCases Menu.MenuCreateCases GuiDocument  TestAddonManagerGui TestOpenSCADGui
%endif


# Utilities
BuildRequires:  cmake gcc-c++ gettext doxygen swig graphviz gcc-gfortran desktop-file-utils tbb-devel
%if %{with tests}
BuildRequires:  xorg-x11-server-Xvfb
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

Provides:       bundled(libOndselSolver) = %{bundled_ondsel_solver_version}

Recommends:     python3-pysolar

%description
    FreeCAD is a general purpose Open Source 3D CAD/MCAD/CAx/CAE/PLM modeler, aimed
    directly at mechanical engineering and product design but also fits a wider
    range of uses in engineering, such as architecture or other engineering
    specialties. It is a feature-based parametric modeler with a modular software
    architecture which makes it easy to provide additional functionality without
    modifying the core system.

%changelog
    {{{ git_repo_changelog }}}
    * Sun Apr 20 2025 Filippo Rossoni Clean Up and use rpkg macro to build on copr
    * Mon Mar 10 2025 Leif-Jöran Olsson <info@friprogramvarusyndikatet.se> - 1.1.0-1
    - Adding support for building with Qt6 and PySide6 for Fedora 40+

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



# plugins and private shared libs in %%{_libdir}/freecad/lib are private;
# prevent private capabilities being advertised in Provides/Requires
%define plugin_regexp /^\\\(libFreeCAD.*%(for i in %{plugins}; do echo -n "\\\|$i\\\|$iGui"; done)\\\)\\\(\\\|Gui\\\)\\.so/d
%{?filter_setup:
    %filter_provides_in %{_libdir}/%{name}/lib
    %filter_from_requires %{plugin_regexp}
    %filter_from_provides %{plugin_regexp}

    %filter_provides_in %{_libdir}/%{name}/Mod
    %filter_requires_in %{_libdir}/%{name}/Mod
%filter_setup}


#path that contain main FreeCAD sources for cmake
%global _vpath_srcdir  %_builddir/{{{ git_repo_name  }}}
#use absolute path for cmake macro
%global _vpath_builddir  %_builddir/%_vpath_builddir

%prep
    {{{ git_repo_setup_macro }}}


%build
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
        -DPACKAGE_WCREV="{{{ git_repo_release_branched }}}" \
        -DPACKAGE_WCURL="{{{ git_repo_vcs }}}"\
    %if %{with tests}
        -DENABLE_DEVELOPER_TESTS=TRUE \
    %else
        -DENABLE_DEVELOPER_TESTS=FALSE \
    %endif
        -DBUILD_GUI=TRUE \

    sed -i -e 's|"$WCREV$"|"{{{ git_repo_release_branched }}}"|g' %_vpath_builddir/src/Build/Version.h.in
    sed -i -e 's|"$WCURL$"|"{{{ git_repo_vcs }}}"|g'              %_vpath_builddir/src/Build/Version.h.in

    %cmake_build

%install
    cd %_vpath_builddir
    %cmake_install

    # Symlink binaries to /usr/bin
    mkdir -p %{buildroot}%{_bindir}
    ln -s ../%{_lib}/%{name}/bin/FreeCAD %{buildroot}%{_bindir}/FreeCAD
    ln -s ../%{_lib}/%{name}/bin/FreeCADCmd %{buildroot}%{_bindir}/FreeCADCmd

    # Remove header from external library that's erroneously installed
    rm -f %{buildroot}%{_libdir}/%{name}/include/E57Format/E57Export.h


%check
    desktop-file-validate %{buildroot}%{_datadir}/applications/org.freecad.FreeCAD.desktop
    %{?fedora:appstream-util validate-relax --nonet %{buildroot}%{_metainfodir}/*.metainfo.xml}



%if %{with tests}
    for t in %{enabled_tests}; do
        %{buildroot}/%{_libdir}/%{name}/bin/FreeCADCmd -t $t
    done
    for t in %{enabled_gui_tests}; do
        xvfb-run %{buildroot}/%{_libdir}/%{name}/bin/FreeCAD -t $t
    done

    #ctest are failing
    #%%ctest
%endif

    # Bug maintainers to keep %%{plugins} macro up to date.
    #
    # Make sure there are no plugins that need to be added to plugins macro
    %define exported_libs_regexp /^\\\(%(for i in %{exported_libs}; do echo -n "\\\|$i"; done)\\\)\\.so/d
    new_plugins=`ls %{buildroot}%{_libdir}/%{name}/%{_lib} | sed -e  '%{plugin_regexp}' | sed -e '%{exported_libs_regexp}'`

    if [ -n "$new_plugins" ]; then
        echo -e "\n\n\n**** ERROR:\n" \
            "\nPlugins not caught by regexps:" \
            "\n" $new_plugins \
            "\n\nPlugins in %{_libdir}/%{name}/lib do not exist in" \
            "\nspecfile %%{plugins} or %%{exported_libs_regexp} macro." \
            "\nPlease add these to %%{plugins} or %%{exported_libs_regexp}" \
            "\nmacro at top of specfile" \
            "\nand rebuild.\n****\n" 1>&2
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
    # Make sure there are no entries in the exported_libs_regexp macro that don't match plugins
    for d in %{exported_libs}; do
        if [ -z "`ls %{buildroot}%{_libdir}/%{name}/%{_lib}/$d*.so`" ]; then
            set +x
            echo -e "\n\n\n**** ERROR:\n" \
                "\nExtra entry in %%{exported_libs} macro with no matching lib:" \
                "'$d'.\n\nPlease remove from %%{exported_libs} macro at top of" \
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
    %{python3_sitelib}/%{name}/*

%files data
    %{_datadir}/%{name}/
    %{_docdir}/%{name}/LICENSE.html
    %{_docdir}/%{name}/ThirdPartyLibraries.html

%files libondselsolver-devel
    %{_datadir}/pkgconfig/OndselSolver.pc
    %{_includedir}/OndselSolver/*
