# This package depends on automagic byte compilation
# https://fedoraproject.org/wiki/Changes/No_more_automagic_Python_bytecompilation_phase_2
%global _python_bytecompile_extra 1

# Setup python target for shiboken so the right cmake file is imported.
%global py_suffix %(%{__python3} -c "import sysconfig; print(sysconfig.get_config_var('SOABI'))")

# Maintainers:  keep this list of plugins up to date
# List plugins in %%{_libdir}/%{name}/lib, less '.so' and 'Gui.so', here
%global plugins Drawing Fem FreeCAD Image Import Inspection Mesh MeshPart Part Points QtUnit Raytracing ReverseEngineering Robot Sketcher Start Web PartDesignGui _PartDesign Path PathGui Spreadsheet SpreadsheetGui area DraftUtils DraftUtils libDriver libDriverDAT libDriverSTL libDriverUNV libMEFISTO2 libSMDS libSMESH libSMESHDS libStdMeshers Measure TechDraw TechDrawGui libarea-native Surface SurfaceGui PathSimulator

# Some configuration options for other environments
# rpmbuild --with=occ:  Compile using OpenCASCADE instead of OCE
%global occ %{?_with_occ: 1} %{?!_with_occ: 0}
# rpmbuild --with=bundled_zipios:  use bundled version of zipios++
%global bundled_zipios %{?_with_bundled_zipios: 1} %{?!_with_bundled_zipios: 0}
# rpmbuild --without=bundled_pycxx:  don't use bundled version of pycxx
%global bundled_pycxx %{?_without_bundled_pycxx: 0} %{?!_without_bundled_pycxx: 1}
# rpmbuild --without=bundled_smesh:  don't use bundled version of Salome's Mesh
%global bundled_smesh %{?_without_bundled_smesh: 0} %{?!_without_bundled_smesh: 1}

# See FreeCAD-master/src/3rdParty/salomesmesh/CMakeLists.txt to find this out.
%global bundled_smesh_version 7.7.1.0

# Some plugins go in the Mod folder instead of lib. Deal with those here:
%global mod_plugins Mod/PartDesign
%define name freecad
%define github_name FreeCAD
%define branch master

Name:           %{name}
Epoch:          1
Version:    	0.19
Release:        pre_{{{ git_commit_no }}}
Summary:        A general purpose 3D CAD modeler
Group:          Applications/Engineering

License:        LGPLv2+
URL:            http://www.freecadweb.org/
Source0:        https://github.com/%{github_name}/FreeCAD/archive/%{branch}.tar.gz

# Utilities
BuildRequires:  cmake gcc-c++ gettext dos2unix
BuildRequires:  doxygen swig graphviz
BuildRequires:  gcc-gfortran
BuildRequires:  desktop-file-utils
BuildRequires:  git

# Development Libraries

BuildRequires:  Coin3-devel
BuildRequires:  Inventor-devel
%if %{occ}
BuildRequires:  OpenCASCADE-devel
%else
BuildRequires:  OCE-devel
BuildRequires:  OCE-draw
%endif

BuildRequires:  boost-devel
BuildRequires:  boost-python3-devel
BuildRequires:  eigen3-devel
BuildRequires:  freeimage-devel
BuildRequires:  libXmu-devel
# For appdata
%if 0%{?fedora}
BuildRequires:  libappstream-glib
%endif
BuildRequires:  libglvnd-devel
BuildRequires:  libicu-devel
BuildRequires:  libkdtree++-devel
BuildRequires:  libspnav-devel
BuildRequires:  libusb-devel
BuildRequires:  med-devel
BuildRequires:  mesa-libEGL-devel
BuildRequires:  mesa-libGLU-devel
BuildRequires:  netgen-mesher-devel
BuildRequires:  netgen-mesher-devel-private
BuildRequires:  python3-pivy
BuildRequires:  mesa-libEGL-devel
BuildRequires:  pcl-devel
%if 0%{?fedora} > 29
BuildRequires:  pyside2-tools
%endif
BuildRequires:  python3
BuildRequires:  python3-devel
BuildRequires:  python3-matplotlib
%if ! %{bundled_pycxx}
BuildRequires:  python3-pycxx-devel
%endif
%if 0%{?fedora} > 29
BuildRequires:  python3-pyside2-devel
BuildRequires:  python3-shiboken2-devel
%endif
BuildRequires:  qt5-devel
BuildRequires:  qt5-qtwebkit-devel
%if ! %{bundled_smesh}
BuildRequires:  smesh-devel
%endif
BuildRequires:  tbb-devel
BuildRequires:  vtk-devel
BuildRequires:  xerces-c
BuildRequires:  xerces-c-devel
%if ! %{bundled_zipios}
BuildRequires:  zipios++-devel
%endif
BuildRequires:  zlib-devel

# Packages separated because they are noarch, but not optional so require them
# here.
Requires:       %{name}-data = %{epoch}:%{version}-%{release}
# Obsolete old doc package since it's required for functionality.
Obsoletes:      %{name}-doc < 0.13-5

Requires:       hicolor-icon-theme
Requires:       python3-collada
Requires:       python3-matplotlib
Requires:       python3-pivy
Requires:       python3-pyside2
Requires:	qt5-assistant
%if %{bundled_smesh} 
Provides:       bundled(smesh) = %{bundled_smesh_version}
%endif
%if %{bundled_pycxx} 
Provides:       bundled(python-pycxx)
%endif
Recommends:	python3-pysolar

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


%prep
%autosetup -p1 -n FreeCAD-%{branch}

# Remove bundled pycxx if we're not using it
%if ! %{bundled_pycxx}
rm -rf src/CXX
%endif

%if ! %{bundled_zipios}
rm -rf src/zipios++
#sed -i "s/zipios-config.h/zipios-config.hpp/g" \
#    src/Base/Reader.cpp src/Base/Writer.h
%endif

# Fix encodings
dos2unix -k src/Mod/Test/unittestgui.py \
            ChangeLog.txt \
            data/License.txt

# Removed bundled libraries

%build
rm -rf build && mkdir build && cd build

# Deal with cmake projects that tend to link excessively.
CXXFLAGS='-Wno-error=cast-function-type'; export CXXFLAGS
LDFLAGS='-Wl,--as-needed -Wl,--no-undefined'; export LDFLAGS

%if 0%{?fedora} > 27
%define MEDFILE_INCLUDE_DIRS %{_includedir}/med/
%else
%define MEDFILE_INCLUDE_DIRS %{_includedir}/
%endif

%cmake \
       -DCMAKE_INSTALL_PREFIX=%{_libdir}/%{name} \
       -DCMAKE_INSTALL_DATADIR=%{_datadir}/%{name} \
       -DCMAKE_INSTALL_DOCDIR=%{_docdir}/%{name} \
       -DCMAKE_INSTALL_INCLUDEDIR=%{_includedir} \
       -DRESOURCEDIR=%{_datadir}/%{name} \
       -DFREECAD_USE_EXTERNAL_PIVY=TRUE \
       -DFREECAD_USE_PCL=TRUE \
       -DBUILD_QT5=ON \
       -DSHIBOKEN_INCLUDE_DIR=%{_includedir}/shiboken2 \
       -DSHIBOKEN_LIBRARY=-lshiboken2.%{py_suffix} \
       -DPYTHON_SUFFIX=.%{py_suffix} \
       -DPYSIDE_INCLUDE_DIR=/usr/include/PySide2 \
       -DPYSIDE_LIBRARY=-lpyside2.%{py_suffix} \
       -DPYTHON_EXECUTABLE:FILEPATH=/usr/bin/python3 \
       -DMEDFILE_INCLUDE_DIRS=%{MEDFILE_INCLUDE_DIRS} \
       -DOpenGL_GL_PREFERENCE=GLVND \
       -DCOIN3D_INCLUDE_DIR=%{_includedir}/Coin3 \
       -DCOIN3D_DOC_PATH=%{_datadir}/Coin3/Coin \
       -DFREECAD_USE_EXTERNAL_PIVY=TRUE \
%if %{occ}
       -DUSE_OCC=TRUE \
%endif
%if ! %{bundled_smesh}
       -DFREECAD_USE_EXTERNAL_SMESH=TRUE \
       -DSMESH_FOUND=TRUE \
       -DSMESH_INCLUDE_DIR=%{_includedir}/smesh \
       -DSMESH_DIR=`pwd`/../cMake \
%endif
%if ! %{bundled_zipios}
       -DFREECAD_USE_EXTERNAL_ZIPIOS=TRUE \
%endif
%if ! %{bundled_pycxx}
       -DPYCXX_INCLUDE_DIR=$(pkg-config --variable=includedir PyCXX) \
       -DPYCXX_SOURCE_DIR=$(pkg-config --variable=srcdir PyCXX) \
%endif
       -DPACKAGE_WCREF="%{release} (Git)" \
       -DPACKAGE_WCURL="git://github.com/%{github_name}/FreeCAD.git master" \
       ../

make fc_version
for I in src/Build/Version.h src/Build/Version.h.out; do
	sed -i 's,FCRevision      \"Unknown\",FCRevision      \"%{release} (Git)\",' $I
	sed -i 's,FCRepositoryURL \"Unknown\",FCRepositoryURL \"git://github.com/FreeCAD/FreeCAD.git master\",' $I
done

%{make_build}

%install
cd build
%make_install

# Symlink binaries to /usr/bin
mkdir -p %{buildroot}%{_bindir}
ln -s ../%{_lib}/%{name}/bin/FreeCAD %{buildroot}%{_bindir}/FreeCAD
ln -s ../%{_lib}/%{name}/bin/FreeCADCmd %{buildroot}%{_bindir}/FreeCADCmd

mkdir %{buildroot}%{_metainfodir}/
mv %{buildroot}%{_libdir}/%{name}/share/metainfo/* %{buildroot}%{_metainfodir}/

mkdir %{buildroot}%{_datadir}/applications/
mv %{buildroot}%{_libdir}/%{name}/share/applications/* %{buildroot}%{_datadir}/applications/

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
# Belongs in %%license not %%doc
rm -f %{buildroot}%{_docdir}/freecad/ThirdPartyLibraries.html

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

%check
desktop-file-validate \
    %{buildroot}%{_datadir}/applications/org.freecadweb.FreeCAD.desktop
%{?fedora:appstream-util validate-relax --nonet \
    %{buildroot}/%{_metainfodir}/*.appdata.xml}


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
%license data/License.txt
%doc ChangeLog.txt
%exclude %{_docdir}/%{name}/%{name}.*
%exclude %{_docdir}/%{name}/ThirdPartyLibraries.html
%{_bindir}/*
%{_metainfodir}/*
%dir %{_libdir}/%{name}
%{_libdir}/%{name}/bin/
%{_libdir}/%{name}/%{_lib}/
%{_libdir}/%{name}/Mod/
%{_libdir}/%{name}/Ext/
%{_datadir}/applications/*
%{_datadir}/icons/hicolor/scalable/*
%{_datadir}/pixmaps/*
%{_datadir}/mime/packages/*

%files data
%{_datadir}/%{name}/
%{_docdir}/%{name}/%{name}.q*
