
# Maintainers:  keep this list of plugins up to date
# List plugins in %%{_libdir}/%{name}/lib, less '.so' and 'Gui.so', here
%global plugins Drawing Fem FreeCAD Image Import Inspection Mesh MeshPart Part Points QtUnit Raytracing ReverseEngineering Robot Sketcher Start Web PartDesignGui _PartDesign Path PathGui Spreadsheet SpreadsheetGui area DraftUtils DraftUtils libDriver libDriverDAT libDriverSTL libDriverUNV libMEFISTO2 libSMDS libSMESH libSMESHDS libStdMeshers Measure TechDraw TechDrawGui libarea-native Surface SurfaceGui PathSimulator

# Some plugins go in the Mod folder instead of lib. Deal with those here:
%global mod_plugins Mod/PartDesign
%define name freecad
%define github_name FreeCAD
%define branch master

Name:           %{name}
Epoch:          1
Version:    	0.19_pre
Release:        {{{ git_commits_no }}}
Summary:        A general purpose 3D CAD modeler
Group:          Applications/Engineering

License:        GPLv2+
URL:            http://sourceforge.net/apps/mediawiki/free-cad/
Source0:        https://github.com/%{github_name}/FreeCAD/archive/%{branch}.tar.gz

BuildRequires:  Coin3
BuildRequires:  Coin3-devel
BuildRequires:  Inventor-devel
BuildRequires:  OCE-devel
BuildRequires:  OCE-draw
BuildRequires:  boost-devel
BuildRequires:  cmake
BuildRequires:  desktop-file-utils
BuildRequires:  dos2unix
BuildRequires:  doxygen
BuildRequires:  eigen3
BuildRequires:  eigen3-devel
BuildRequires:  freeimage-devel
BuildRequires:  gettext
BuildRequires:  git
BuildRequires:  graphviz
BuildRequires:  libicu-devel
BuildRequires:  libspnav
BuildRequires:  libspnav-devel
BuildRequires:  med
BuildRequires:  med-devel
BuildRequires:  mesa-libGLU-devel
BuildRequires:  netgen-mesher-devel
BuildRequires:  netgen-mesher-devel-private
BuildRequires:  pyside-tools
BuildRequires:  python
BuildRequires:  python-matplotlib
BuildRequires:  python-pivy
BuildRequires:  python-pyside
BuildRequires:  python-pyside-devel
BuildRequires:  python2-devel
BuildRequires:  qt-devel
BuildRequires:  qt-webkit-devel
BuildRequires:  shiboken
BuildRequires:  shiboken-devel
BuildRequires:  smesh
BuildRequires:  smesh-devel
BuildRequires:  swig
BuildRequires:  tbb-devel
BuildRequires:  vtk-devel
BuildRequires:  xerces-c
BuildRequires:  xerces-c-devel
BuildRequires:  zlib-devel
%if 0%{?fedora} > 28
BuildRequires:  boost-python2
BuildRequires:  boost-python2-devel
BuildRequires:  boost-python3
BuildRequires:  boost-python3-devel
BuildRequires:  mesa-libEGL-devel
BuildRequires:  python3-matplotlib
%endif

# For appdata
%if 0%{?fedora}
BuildRequires:  libappstream-glib
%endif

# Packages separated because they are noarch, but not optional so require them
# here.
Requires:       %{name}-data = %{epoch}:%{version}-%{release}
# Obsolete old doc package since it's required for functionality.
Obsoletes:      %{name}-doc < 0.13-5

# Needed for plugin support and is not a soname dependency.
%if ! 0%{?rhel} <= 6 && "%{_arch}" != "ppc64"
# python-pivy does not build on EPEL 6 ppc64.
Requires:       python-pivy
%endif
Requires:       hicolor-icon-theme
Requires:       python-matplotlib
Requires:       python-collada
Requires:       python-pyside

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
specialties. It is a feature-based parametric modeler with a modular software
architecture which makes it easy to provide additional functionality without
modifying the core system.


%package data
Summary:        Data files for FreeCAD
BuildArch:      noarch
Requires:       %{name} = %{epoch}:%{version}-%{release}

%description data
Data files for FreeCAD


%prep
%autosetup -n FreeCAD-%{branch}

# Fix encodings
dos2unix -k src/Mod/Test/unittestgui.py \
            ChangeLog.txt \
            data/License.txt

# Removed bundled libraries

%build
rm -rf build && mkdir build && cd build

# Deal with cmake projects that tend to link excessively.
#LDFLAGS='-Wl,--as-needed'; export LDFLAGS

%if 0%{?fedora} > 27
%define MEDFILE_INCLUDE_DIRS %{_includedir}/med/
%else
%define MEDFILE_INCLUDE_DIRS %{_includedir}/
%endif

%cmake -DCMAKE_INSTALL_PREFIX=%{_libdir}/%{name} \
       -DCMAKE_INSTALL_DATADIR=%{_datadir}/%{name} \
       -DCMAKE_INSTALL_DOCDIR=%{_docdir}/%{name} \
       -DCMAKE_INSTALL_INCLUDEDIR=%{_includedir} \
       -DRESOURCEDIR=%{_datadir}/%{name} \
       -DFREECAD_USE_EXTERNAL_PIVY=TRUE \
       -DMEDFILE_INCLUDE_DIRS=%{MEDFILE_INCLUDE_DIRS} \
       ../

sed -i 's,FCRevision      \"Unknown\",FCRevision      \"%{release} (Git)\",' src/Build/Version.h
sed -i 's,FCRepositoryURL \"Unknown\",FCRepositoryURL \"git://github.com/FreeCAD/FreeCAD.git master\",' src/Build/Version.h

make %{?_smp_mflags}

%install
cd build
%make_install

# Symlink binaries to /usr/bin
mkdir -p %{buildroot}%{_bindir}
pushd %{buildroot}%{_bindir}
ln -s ../%{_lib}/%{name}/bin/FreeCAD .
ln -s ../%{_lib}/%{name}/bin/FreeCADCmd .
popd

mkdir %{buildroot}%{_datadir}/appdata/
mv %{buildroot}%{_libdir}/%{name}/share/metainfo/* %{buildroot}%{_datadir}/appdata/

mkdir %{buildroot}%{_datadir}/applications/
mv %{buildroot}%{_libdir}/%{name}/share/applications/* %{buildroot}%{_datadir}/applications/

mkdir -p %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/
mv %{buildroot}%{_libdir}/%{name}/share/icons/hicolor/scalable/apps/* %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/

mkdir -p %{buildroot}%{_datadir}/mime/packages/
mv %{buildroot}%{_libdir}/%{name}/share/mime/packages/* %{buildroot}%{_datadir}/mime/packages/

pushd %{buildroot}%{_libdir}/%{name}/share/
rmdir metainfo/
rmdir applications/
rmdir -p mime/packages/
rmdir -p icons/hicolor/scalable/apps/
popd

# Bug maintainers to keep %%{plugins} macro up to date.
#
# Make sure there are no plugins that need to be added to plugins macro
new_plugins=`ls %{buildroot}%{_libdir}/%{name}/lib | sed -e  '%{plugin_regexp}'`
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
    if [ -z "`ls %{buildroot}%{_libdir}/%{name}/lib/$p*.so`" ]; then
        set +x
        echo -e "\n\n\n**** ERROR:\n" \
             "\nExtra entry in %%{plugins} macro with no matching plugin:" \
             "'$p'.\n\nPlease remove from %%{plugins} macro at top of" \
             "\nspecfile and rebuild.\n****\n" 1>&2
        exit 1
    fi
done

%check
%{?fedora:appstream-util validate-relax --nonet \
    %{buildroot}/%{_datadir}/appdata/*.appdata.xml}


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
%dir %{_libdir}/%{name}
%{_libdir}/%{name}/bin/
%{_libdir}/%{name}/lib/
%{_libdir}/%{name}/Mod/
%{_libdir}/%{name}/Ext/
%{_datadir}/applications/*
%{_datadir}/icons/hicolor/scalable/apps/*
%{_datadir}/appdata/*
%{_datadir}/mime/packages/*

%files data
%{_datadir}/%{name}/
%{_docdir}/%{name}/%{name}.q*
