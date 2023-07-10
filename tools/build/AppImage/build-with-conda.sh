#!/bin/sh

# ***************************************************************************
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
# *                      and FreeCAD maintainers (looooo, adrianinsaval )   *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

# adapted from https://github.com/FreeCAD/FreeCAD-Bundle/blob/master/conda/linux/create_bundle.sh

# This script produces a FreeCAD AppImage locally using conda repositories. It
# is an all-in-one script that fetches FreeCAD and all dependencies from conda
# repo (it also extracts a couple of XDG files from the FreeCAD source). Everything
# comes from conda repositories, therefore, you need to make sure they contain the
# FreeCAD package you want to build. An easy way to check is by searching for
# available FreeCAD packages:
#
#     conda search "freecad=0.21.0" -c adrianinsaval/label/dev -c freecad -c conda-forge
#


# To use this script, you will need:
#
#     1) Mambaforge from https://github.com/conda-forge/miniforge#mambaforge
#     if you choose to not install the conda environment when installing Mambaforge,
#     you will need to do it before running this script, by running the following
#     in your terminal. This will make the "conda" and "mamba" commands available:
#     eval "$(/home/yorik/Mambaforge/bin/conda shell.zsh hook)"
#     (replace the Mambaforge path by yours and zsh by your shell name)
#
#     2) appimagetool from https://appimage.github.io/appimagetool/
#     place it in your exec path so it can be found by this script
#
#     3) a gpg key to sign the package
#
#     4) verify or change the config values here below
#
#     5) If you are on Fedora, there are lines to uncomment below in the script
#
# When done, just run this script and you should get an appimage with the
# corresponding sha hash in the current folder.
#
# To cleanup after build: Just delete the "AppDir" folder (not done automatically since
# next builds can reuse the downloaded packages).

# config

# make sure you have a gpg key for this email
gpg_key="yorik@freecad.org"
# the FreeCAD version we're looking for
target_version="0.21.0"
# make sure target_python matches the one FreeCAD is built with! Check with
# conda search "freecad=0.21.0" -c adrianinsaval/label/dev -c freecad -c conda-forge
target_python="3.10"

# end config

# export MAMBA_NO_BANNER=1

# building needed files
mkdir -p AppDir
cat > AppDir/AppRun <<EOF
#!/bin/bash
HERE="\$(dirname "\$(readlink -f "\${0}")")"
export PREFIX=\${HERE}/usr
export LD_LIBRARY_PATH=\${HERE}/usr/lib\${LD_LIBRARY_PATH:+':'}\$LD_LIBRARY_PATH
export PYTHONHOME=\${HERE}/usr
export PATH_TO_FREECAD_LIBDIR=\${HERE}/usr/lib
# export QT_QPA_PLATFORM_PLUGIN_PATH=\${HERE}/usr/plugins
# export QT_XKB_CONFIG_ROOT=\${HERE}/usr/lib
export FONTCONFIG_FILE=/etc/fonts/fonts.conf
export FONTCONFIG_PATH=/etc/fonts
# export QTWEBENGINEPROCESS_PATH=\${HERE}/usr/libexec/QtWebEngineProcess

# Show packages info if DEBUG env variable is set
if [ "\$DEBUG" = 1 ]; then
    cat \${HERE}/packages.txt
fi

# SSL
# https://forum.freecadweb.org/viewtopic.php?f=4&t=34873&start=20#p327416
export SSL_CERT_FILE=\$PREFIX/ssl/cacert.pem
# https://github.com/FreeCAD/FreeCAD-AppImage/pull/20
export GIT_SSL_CAINFO=\$HERE/usr/ssl/cacert.pem
# https://github.com/FreeCAD/FreeCAD-Bundle/issues/92#issuecomment-1086829486
export QTWEBENGINE_DISABLE_SANDBOX=1
# Support for launching other applications (from /usr/bin)
# https://github.com/FreeCAD/FreeCAD-AppImage/issues/30
if [ ! -z "\$1" ] && [ -e "\$HERE/usr/bin/\$1" ] ; then
    MAIN="\$HERE/usr/bin/\$1" ; shift
else
    MAIN="\$HERE/usr/bin/freecad"
fi

\${MAIN} "\$@"
EOF
chmod +x AppDir/AppRun

# building package name
target_date=$(date +"%Y-%m-%d")
arch=$(uname -m)
package_name="FreeCAD_${target_version}-${target_date}-conda-Linux-${arch}-py$(echo ${target_python} | sed 's/\.//g')"
conda_env="AppDir/usr"

# dependencies
echo "\nCreating the environment"
packages="freecad=${target_version} occt vtk python=${target_python} \
blas=*=openblas numpy matplotlib-base scipy sympy pandas \
six pyyaml pycollada lxml xlutils olefile requests blinker \
opencv qt.py nine docutils calculix opencamlib ifcopenshell \
appimage-updater-bridge"

mamba create -p ${conda_env} ${packages} \
  --copy -c adrianinsaval/label/dev \
  -c freecad -c conda-forge -y

echo "\n################"
echo "package_name:  ${package_name}"
echo "################"

echo "\nInstalling freecad.appimage_updater"
mamba run -p ${conda_env} pip install https://github.com/looooo/freecad.appimage_updater/archive/master.zip

echo "\nUninstalling some unneeded packages"
conda uninstall -p ${conda_env} libclang --force -y

mamba list -p ${conda_env} > AppDir/packages.txt
sed -i "1s/.*/\nLIST OF PACKAGES:/" AppDir/packages.txt

echo "\nDeleting unnecessary stuff"
rm -rf ${conda_env}/include
find ${conda_env} -name \*.a -delete
mv ${conda_env}/bin ${conda_env}/bin_tmp
mkdir ${conda_env}/bin
cp ${conda_env}/bin_tmp/freecad ${conda_env}/bin/
cp ${conda_env}/bin_tmp/freecadcmd ${conda_env}/bin/
cp ${conda_env}/bin_tmp/ccx ${conda_env}/bin/
cp ${conda_env}/bin_tmp/python ${conda_env}/bin/
cp ${conda_env}/bin_tmp/pip ${conda_env}/bin/
cp ${conda_env}/bin_tmp/pyside2-rcc ${conda_env}/bin/
cp ${conda_env}/bin_tmp/assistant ${conda_env}/bin/
sed -i '1s|.*|#!/usr/bin/env python|' ${conda_env}/bin/pip
rm -rf ${conda_env}/bin_tmp

echo "\nCreating qt config"
echo "[Paths]\nPrefix = ./../" > qt.conf
cp qt.conf ${conda_env}/bin/
cp qt.conf ${conda_env}/libexec/
rm qt.conf

echo "\nCopying icons and .desktop file"
mkdir -p ${conda_env}/share/icons/hicolor/scalable/apps/
cp ../../../src/Gui/Icons/freecad.svg ${conda_env}/share/icons/hicolor/scalable/apps/org.freecadweb.FreeCAD.svg
cp ${conda_env}/share/icons/hicolor/scalable/apps/org.freecadweb.FreeCAD.svg AppDir
mkdir -p ${conda_env}/share/icons/hicolor/64x64/apps/
cp ../../../src/Gui/Icons/freecad-icon-64.png ${conda_env}/share/icons/hicolor/64x64/apps/org.freecadweb.FreeCAD.png
cp ${conda_env}/share/icons/hicolor/64x64/apps/org.freecadweb.FreeCAD.png AppDir
mkdir -p ${conda_env}/share/applications/
cp ../../../src/XDGData/org.freecadweb.FreeCAD.desktop ${conda_env}/share/applications/
sed -i "s/Exec\=FreeCAD\ \%F/Exec=AppRun/g" ${conda_env}/share/applications/org.freecadweb.FreeCAD.desktop
cp ${conda_env}/share/applications/org.freecadweb.FreeCAD.desktop AppDir
cp ../../../src/XDGData/org.freecadweb.FreeCAD.appdata.xml.in ${conda_env}/share/metainfo/org.freecadweb.FreeCAD.appdata.xml
sed -i "s/@PACKAGE_VERSION@/${target_version}/g" ${conda_env}/share/metainfo/org.freecadweb.FreeCAD.appdata.xml
sed -i "s/@APPDATA_RELEASE_DATE@/${target_date}/g" ${conda_env}/share/metainfo/org.freecadweb.FreeCAD.appdata.xml

echo "\nCleaning"

# Remove __pycache__ folders and .pyc files
find . -path "*/__pycache__/*" -delete
find . -name "*.pyc" -type f -delete

# reduce size
rm -rf ${conda_env}/conda-meta/
rm -rf ${conda_env}/doc/global/
rm -rf ${conda_env}/share/gtk-doc/
rm -rf ${conda_env}/lib/cmake/

# remove unnecessary development files
find . -name "*.h" -type f -delete
find . -name "*.cmake" -type f -delete

# The following two lines must be uncommented if using this on Fedora 28 and up
# echo "\nAdd libnsl"
# cp ../../libc6/lib/$ARCH-linux-gnu/libnsl* ${conda_env}/lib/

echo "\nCreating the appimage"
chmod a+x ./AppDir/AppRun
appimagetool-${arch}.AppImage --sign --sign-key ${gpg_key} AppDir ${package_name}.AppImage

echo "\nCreating hash"
shasum -a 256 ${package_name}.AppImage > ${package_name}.AppImage-SHA256.txt

echo "\nAll done! You can delete the AppDir folder"
