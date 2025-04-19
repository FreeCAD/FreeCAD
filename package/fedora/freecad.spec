
Name:           freecad

Epoch:          2
Version:        {{{ git_repo_version  lead=1.1 follow="~"}}}
Release:        disable_test_%autorelease

Summary:        A general purpose 3D CAD modeler
Group:          Applications/Engineering

License:        GPL-2.0-or-later
URL:            https://www.freecad.org/
VCS:            {{{ git_repo_vcs }}}

Source0:        {{{git_repo_pack_with_submodules}}}






# This package depends on automagic byte compilation
# https://fedoraproject.org/wiki/Changes/No_more_automagic_Python_bytecompilation_phase_3
%global py_bytecompile 1

# Setup python target for shiboken so the right cmake file is imported.
%global py_suffix %(%{__python3} -c "import sysconfig; print(sysconfig.get_config_var('SOABI'))")


# Maintainers:  keep this list of plugins up to date
# List plugins in %%{_libdir}/%%{name}/lib, less '.so' and 'Gui.so', here
%global plugins AssemblyApp AssemblyGui CAMSimulator DraftUtils Fem FreeCAD Import Inspection MatGui Materials Measure Mesh MeshPart Part PartDesignGui Path PathApp PathSimulator Points QtUnitGui ReverseEngineering Robot Sketcher Spreadsheet Start Surface TechDraw Web _PartDesign area flatmesh libDriver libDriverDAT libDriverSTL libDriverUNV libE57Format libMEFISTO2 libOndselSolver libSMDS libSMESH libSMESHDS libStdMeshers libarea-native


# Some configuVCS:            {{{ git_repo_vcs }}}ration options for other environments
# rpmbuild --with=bundled_zipios: use bundled version of zipios++
%bcond_with  bundled_zipios
# rpmbuild --with=bundled_pycxx:  use bundled version of pycxx
%bcond_with bundled_pycxx
# rpmbuild --without=bundled_smesh:  don't use bundled version of Salome's Mesh
%bcond_without bundled_smesh
# rpmbuild --with=tests:  include  tests in build
%bcond_with tests
%if %{with tests}
%global plugins %{plugins} libgmock libgmock_main  libgtest libgtest_main
%endif

#path that contain main FreeCAD sources for cmake
%global _vpath_srcdir  %_builddir/{{{ git_repo_name  }}}
#use absolute path for cmake macro
%global _vpath_builddir  %_builddir/%_vpath_builddir


# See FreeCAD-main/src/3rdParty/salomesmesh/CMakeLists.txt to find this out.
%global bundled_smesh_version 7.7.1.0

%global mod_plugins Mod/PartDesign

# Utilities
BuildRequires:  cmake gcc-c++ gettext
BuildRequires:  doxygen swig graphviz
BuildRequires:  gcc-gfortran
BuildRequires:  desktop-file-utils
BuildRequires:  git

BuildRequires:  tbb-devel

# Development Libraries
BuildRequires:  freeimage-devel
BuildRequires:  libXmu-devel
BuildRequires:  mesa-libEGL-devel
BuildRequires:  mesa-libGLU-devel
BuildRequires:  opencascade-devel
BuildRequires:  Coin4-devel
BuildRequires:  python3-devel
BuildRequires:  python3-matplotlib
BuildRequires:  python3-pivy
BuildRequires:  boost-devel
BuildRequires:  eigen3-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  qt6-qttools-static

BuildRequires:  fmt-devel

BuildRequires:  xerces-c
BuildRequires:  xerces-c-devel
BuildRequires:  libspnav-devel

BuildRequires:  python3-shiboken6-devel
BuildRequires:  python3-pyside6-devel
BuildRequires:  pyside6-tools
%if %{without bundled_smesh}
BuildRequires:  smesh-devel
%endif
BuildRequires:  netgen-mesher-devel
BuildRequires:  netgen-mesher-devel-private
%if %{without bundled_zipios}
BuildRequires:  zipios++-devel
%endif

%if %{without bundled_pycxx}
BuildRequires:  python3-pycxx-devel
%endif
BuildRequires:  python3-pybind11
BuildRequires:  libicu-devel
BuildRequires:  vtk-devel
BuildRequires:  openmpi-devel
BuildRequires:  med-devel
BuildRequires:  libkdtree++-devel

BuildRequires:  pcl-devel
BuildRequires:  python3
BuildRequires:  libglvnd-devel
BuildRequires:  yaml-cpp-devel
#BuildRequires:  zlib-devel

# For appdata
%if 0%{?fedora}
BuildRequires:  libappstream-glib
%endif

# Packages separated because they are noarch, but not optional so require them
# here.
Requires:       %{name}-data = %{epoch}:%{version}-%{release}
# Obsolete old doc package since it's required for functionality.
Obsoletes:      %{name}-doc < 0.22-1

Requires:       hicolor-icon-theme

Requires:       fmt

Requires:       python3-pivy
Requires:       python3-matplotlib
Requires:       python3-collada
Requires:       python3-pyside6
Requires:       qt6-assistant


%if %{with bundled_smesh}
Provides:       bundled(smesh) = %{bundled_smesh_version}
%endif
%if %{with bundled_pycxx}
Provides:       bundled(python-pycxx)
%endif
Recommends:     python3-pysolar

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


%prep
{{{ git_repo_setup_macro }}}

%build
cd %_vpath_srcdir
# Deal with cmake projects that tend to link excessively.
CXXFLAGS='-Wno-error=cast-function-type'; export CXXFLAGS
LDFLAGS='-Wl,--as-needed -Wl,--no-undefined'; export LDFLAGS

%cmake \
       -DCMAKE_INSTALL_PREFIX=%{_libdir}/%{name} \
       -DCMAKE_INSTALL_DATADIR=%{_datadir}/%{name} \
       -DCMAKE_INSTALL_DOCDIR=%{_docdir}/%{name} \
       -DCMAKE_INSTALL_INCLUDEDIR=%{_includedir} \
       -DRESOURCEDIR=%{_datadir}/%{name} \
       -DFREECAD_USE_EXTERNAL_PIVY=TRUE \
       -DFREECAD_USE_EXTERNAL_FMT=TRUE \
       -DFREECAD_USE_PCL:BOOL=OFF \
       -DFREECAD_QT_VERSION:STRING=6 \
       -DPYTHON_SUFFIX=.%{py_suffix} \
       -DOpenGL_GL_PREFERENCE=GLVND \
       -DUSE_OCC=TRUE \
%if %{without bundled_smesh}
       -DFREECAD_USE_EXTERNAL_SMESH=TRUE \
%endif
%if %{without bundled_zipios}
       -DFREECAD_USE_EXTERNAL_ZIPIOS=TRUE \
%endif
       -DPACKAGE_WCREF="{{{ git_repo_release_branched }}}" \
       -DPACKAGE_WCURL="{{{ git_repo_vcs }}}"\
%if %{with tests}
       -DENABLE_DEVELOPER_TESTS=TRUE \
%else
       -DENABLE_DEVELOPER_TESTS=FALSE \
%endif
       -DBUILD_GUI=TRUE \

%cmake_build

%install
cd %_vpath_builddir
%cmake_install

# Symlink binaries to /usr/bin
mkdir -p %{buildroot}%{_bindir}
ln -s ../%{_lib}/%{name}/bin/FreeCAD %{buildroot}%{_bindir}/FreeCAD
ln -s ../%{_lib}/%{name}/bin/FreeCADCmd %{buildroot}%{_bindir}/FreeCADCmd

mkdir %{buildroot}%{_metainfodir}/
mv %{buildroot}%{_libdir}/%{name}/share/metainfo/* %{buildroot}%{_metainfodir}/

mkdir %{buildroot}%{_datadir}/applications/
mv %{buildroot}%{_libdir}/%{name}/share/applications/* %{buildroot}%{_datadir}/applications/


mkdir -p %{buildroot}%{_datadir}/thumbnailers/
mv %{buildroot}%{_libdir}/%{name}/share/thumbnailers/* %{buildroot}%{_datadir}/thumbnailers/

mkdir -p %{buildroot}%{_datadir}/icons/hicolor/scalable/
mv %{buildroot}%{_libdir}/%{name}/share/icons/hicolor/scalable/* %{buildroot}%{_datadir}/icons/hicolor/scalable/

mkdir -p %{buildroot}%{_datadir}/pixmaps/
mv %{buildroot}%{_libdir}/%{name}/share/pixmaps/* %{buildroot}%{_datadir}/pixmaps/

mkdir -p %{buildroot}%{_datadir}/mime/packages/
mv %{buildroot}%{_libdir}/%{name}/share/mime/packages/* %{buildroot}%{_datadir}/mime/packages/

pushd %{buildroot}%{_libdir}/%{name}/share/
rmdir metainfo/
rmdir applications/
rm -rf mime
rm -rf icons
popd

# Remove obsolete Start_Page.html
rm -f %{buildroot}%{_docdir}/%{name}/Start_Page.html

# Remove header from external library that's erroneously installed
rm -f %{buildroot}%{_libdir}/%{name}/include/E57Format/E57Export.h

rm -rf %{buildroot}%{_includedir}/OndselSolver/*
rm -f %{buildroot}%{_libdir}/%{name}/share/pkgconfig/OndselSolver.pc

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

# Bytecompile Python modules
%py_byte_compile %{__python3} %{buildroot}%{_libdir}/%{name}


%check
desktop-file-validate \
    %{buildroot}%{_datadir}/applications/org.freecad.FreeCAD.desktop
%{?fedora:appstream-util validate-relax --nonet \
    %{buildroot}%{_metainfodir}/*.metainfo.xml}

%if %{with tests}
%ctest
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
%{_datadir}/icons/hicolor/scalable/*
%{_datadir}/pixmaps/*
%{_datadir}/mime/packages/*
%{_datadir}/thumbnailers/*
%{_libdir}/../lib/python*/site-packages/%{name}/*

%files data
%{_datadir}/%{name}/
%{_docdir}/%{name}/LICENSE.html
%{_docdir}/%{name}/ThirdPartyLibraries.html

%changelog
* Mon Mar 10 2025 Leif-Jöran Olsson <info@friprogramvarusyndikatet.se> - 1.1.0-1
- Adding support for building with Qt6 and PySide6 for Fedora 40+
