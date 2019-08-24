# This package depends on automagic byte compilation
# https://fedoraproject.org/wiki/Changes/No_more_automagic_Python_bytecompilation_phase_2
%global _python_bytecompile_extra 1

# Setup python target for shiboken so the right cmake file is imported.
%global py_suffix %(%{__python3} -c "import sysconfig; print(sysconfig.get_config_var('SOABI'))")

# Maintainers:  keep this list of plugins up to date
# List plugins in %%{_libdir}/freecad/lib, less '.so' and 'Gui.so', here
%global plugins Complete DraftUtils Drawing Fem FreeCAD Image Import Inspection Mesh MeshPart Part PartDesign Path Points QtUnit Raytracing ReverseEngineering Robot Sketcher Spreadsheet Start Web PartDesignGui _PartDesign Spreadsheet SpreadsheetGui area

# Some configuration options for other environments
# rpmbuild --with=occ:  Compile using OpenCASCADE instead of OCE
%global occ %{?_with_occ: 1} %{?!_with_occ: 0}
# rpmbuild --with=bundled_zipios:  use bundled version of zipios++
%global bundled_zipios %{?_with_bundled_zipios: 1} %{?!_with_bundled_zipios: 0}
# rpmbuild --without=bundled_pycxx:  don't use bundled version of pycxx
%global bundled_pycxx %{?_with_bundled_pycxx: 0} %{?!_with_bundled_pycxx: 1}
# rpmbuild --without=bundled_smesh:  don't use bundled version of Salome's Mesh
%global bundled_smesh %{?_with_bundled_smesh: 0} %{?!_with_bundled_smesh: 1}

%define name freecad
%define github_name FreeCAD
%define branch master

Name:           %{name}
Epoch:          1
Version:        0.19_pre
Release:        {{{ git_commits_no }}}
Summary:        A general purpose 3D CAD modeler
Group:          Applications/Engineering

License:        GPLv2+
URL:            http://freecadweb.org/
Source0:        https://github.com/%{github_name}/FreeCAD/archive/%{branch}.tar.gz


# Utilities
BuildRequires:  cmake gcc-c++ gettext dos2unix
BuildRequires:  doxygen swig graphviz
BuildRequires:  gcc-gfortran
BuildRequires:  desktop-file-utils
%ifnarch ppc64
BuildRequires:  tbb-devel
%endif
# Development Libraries
BuildRequires:  freeimage-devel
BuildRequires:  libXmu-devel
BuildRequires:  mesa-libGLU-devel
%if %{occ}
BuildRequires:  OpenCASCADE-devel
%else
BuildRequires:  OCE-devel
%endif
BuildRequires:  Coin3-devel
BuildRequires:  python3-devel
BuildRequires:  python3-matplotlib
BuildRequires:  boost-devel boost-python3-devel
BuildRequires:  eigen3-devel
BuildRequires:  qt-devel qt-webkit-devel
BuildRequires:  SoQt-devel
BuildRequires:  xerces-c xerces-c-devel
BuildRequires:  libspnav-devel
BuildRequires:  shiboken-python3-devel
BuildRequires:  python-pyside-devel pyside-tools
%if ! %{bundled_smesh}
BuildRequires:  smesh-devel
%endif
BuildRequires:  netgen-mesher-devel
%if ! %{bundled_zipios}
BuildRequires:  zipios++-devel
%endif
%if ! %{bundled_pycxx}
BuildRequires:  python3-pycxx-devel
%endif
BuildRequires:  libicu-devel
BuildRequires:  vtk-devel
#BuildRequires:  openmpi-devel
BuildRequires:  med-devel
BuildRequires:  libkdtree++-devel

# For appdata
%if 0%{?fedora}
BuildRequires:  libappstream-glib
%endif

Requires:       %{name}-data = %{epoch}:%{version}-%{release}

Provides:       bundled(smesh) = 5.1.2.2
Provides:       bundled(python-pycxx)

Requires:       python3-pivy
Requires:       python3-matplotlib
Requires:       python3-collada
Requires:       python3-pyside
Requires:       qt-assistant


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
#rm -rf src/3rdParty


%build
mkdir build && pushd build


# Deal with cmake projects that tend to link excessively.
CXXFLAGS='-Wno-error=cast-function-type'; export CXXFLAGS
LDFLAGS='-Wl,--as-needed -Wl,--no-undefined'; export LDFLAGS

%cmake -DCMAKE_INSTALL_PREFIX=%{_libdir}/%{name} \
       -DCMAKE_INSTALL_DATADIR=%{_datadir}/%{name} \
       -DCMAKE_INSTALL_DOCDIR=%{_docdir}/%{name} \
       -DCMAKE_INSTALL_INCLUDEDIR=%{_includedir} \
       -DRESOURCEDIR=%{_datadir}/%{name} \
       -DPYTHON_EXECUTABLE=%{__python3} \
%if 0%{?fedora} < 30
       -DPYTHON_SUFFIX=.%{py_suffix} \
%endif
       -DOpenGL_GL_PREFERENCE=GLVND \
       -DCOIN3D_INCLUDE_DIR=%{_includedir}/Coin3 \
       -DCOIN3D_DOC_PATH=%{_datadir}/Coin3/Coin \
       -DFREECAD_USE_EXTERNAL_PIVY=TRUE \
%if %{occ}
       -DUSE_OCC=TRUE \
%endif
%if ! %{bundled_smesh}
       -DFREECAD_USE_EXTERNAL_SMESH=TRUE \
       -DSMESH_INCLUDE_DIR=%{_includedir}/smesh \
%endif
%if ! %{bundled_zipios}
       -DFREECAD_USE_EXTERNAL_ZIPIOS=TRUE \
%endif
%if ! %{bundled_pycxx}
       -DPYCXX_INCLUDE_DIR=$(pkg-config --variable=includedir PyCXX) \
       -DPYCXX_SOURCE_DIR=$(pkg-config --variable=srcdir PyCXX) \
%endif
       -DMEDFILE_INCLUDE_DIRS=%{_includedir}/med \
       ../

sed -i 's,FCRevision      \"Unknown\",FCRevision      \"%{release} (Git)\",' src/Build/Version.h
sed -i 's,FCRepositoryURL \"Unknown\",FCRepositoryURL \"git://github.com/FreeCAD/FreeCAD.git master\",' src/Build/Version.h

make %{?_smp_mflags}


%install
pushd build
%make_install
popd

# Symlink binaries to /usr/bin
mkdir -p %{buildroot}%{_bindir}
ln -rs %{buildroot}%{_libdir}/freecad/bin/FreeCAD %{buildroot}%{_bindir}
ln -rs %{buildroot}%{_libdir}/freecad/bin/FreeCADCmd %{buildroot}%{_bindir}
#popd

# Fix problems with unittestgui.py
#chmod +x %{buildroot}%{_libdir}/%{name}/Mod/Test/unittestgui.py

# Move mis-installed files to the right location
# Need to figure out if FreeCAD can install correctly at some point.
mkdir -p %{buildroot}%{_datadir}
mv %{buildroot}%{_libdir}/%{name}/share/* \
   %{buildroot}%{_datadir}

#sed -i 's,@lib@,%{_lib},g' %{buildroot}%{_datadir}/applications/%{name}.desktop

# Install desktop icon
#install -pD -m 0644 src/Gui/Icons/%{name}.svg \
#    %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/%{name}.svg

# Remove obsolete Start_Page.html
rm -f %{buildroot}%{_docdir}/%{name}/Start_Page.html

# Install appdata file
#mkdir -p %{buildroot}%{_datadir}/appdata
#install -pm 0644 %{SOURCE103} %{buildroot}%{_datadir}/appdata/

# Belongs in %%license not %%doc
rm -f %{buildroot}%{_docdir}/freecad/ThirdPartyLibraries.html


%check
desktop-file-validate \
    %{buildroot}%{_datadir}/applications/org.freecadweb.FreeCAD.desktop
%{?fedora:appstream-util validate-relax --nonet \
    %{buildroot}%{_metainfodir}/*.appdata.xml}


%if 0%{?rhel} && 0%{?rhel} <= 7
%post
/usr/bin/update-desktop-database &> /dev/null || :
/usr/bin/update-mime-database %{_datadir}/mime &> /dev/null || :

%postun
/usr/bin/update-desktop-database &> /dev/null || :
/usr/bin/update-mime-database %{_datadir}/mime &> /dev/null || :
%endif


%files
%license data/License.txt src/Doc/ThirdPartyLibraries.html
%doc ChangeLog.txt README.md
%exclude %{_docdir}/freecad/freecad.*
%{_bindir}/*
%{_metainfodir}/*.appdata.xml
%{_datadir}/applications/*.desktop
%{_datadir}/icons/hicolor/scalable/apps/*.svg
%{_datadir}/mime/packages/*.xml
%dir %{_libdir}/%{name}
%{_libdir}/%{name}/bin/
%{_libdir}/%{name}/lib/
%{_libdir}/%{name}/Ext/
%{_libdir}/%{name}/Mod/

%files data
%{_datadir}/%{name}/
%{_docdir}/%{name}/freecad.q*


%changelog
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
