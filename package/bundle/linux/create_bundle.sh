#!/bin/bash

set -e
set -x

conda_env="AppDir/usr"

mkdir -p ${conda_env}

cp -a ../../../.pixi/envs/default/* ${conda_env}

echo -e "\nDelete unnecessary stuff"
rm -rf ${conda_env}/include
find ${conda_env} -name \*.a -delete

mv ${conda_env}/bin ${conda_env}/bin_tmp
mkdir ${conda_env}/bin
cp ${conda_env}/bin_tmp/freecad ${conda_env}/bin/
cp ${conda_env}/bin_tmp/freecadcmd ${conda_env}/bin
cp ${conda_env}/bin_tmp/ccx ${conda_env}/bin/
cp ${conda_env}/bin_tmp/python ${conda_env}/bin/
cp ${conda_env}/bin_tmp/pip ${conda_env}/bin/
cp ${conda_env}/bin_tmp/pyside6-rcc ${conda_env}/bin/
cp ${conda_env}/bin_tmp/gmsh ${conda_env}/bin/
cp ${conda_env}/bin_tmp/dot ${conda_env}/bin/
cp ${conda_env}/bin_tmp/unflatten ${conda_env}/bin/
rm -rf ${conda_env}/bin_tmp

sed -i '1s|.*|#!/usr/bin/env python|' ${conda_env}/bin/pip

echo -e "\nCopying Icon and Desktop file"
cp ${conda_env}/share/applications/org.freecad.FreeCAD.desktop AppDir/
sed -i 's/Exec=FreeCAD/Exec=AppRun/g' AppDir/org.freecad.FreeCAD.desktop
cp ${conda_env}/share/icons/hicolor/scalable/apps/org.freecad.FreeCAD.svg AppDir/

# Remove __pycache__ folders and .pyc files
find . -path "*/__pycache__/*" -delete
find . -name "*.pyc" -type f -delete

# reduce size
rm -rf ${conda_env}/conda
rm -rf ${conda_env}/conda-meta
rm -rf ${conda_env}/doc
rm -rf ${conda_env}/etc/conda
rm -rf ${conda_env}/lib/clang
rm -rf ${conda_env}/lib/cmake
rm -rf ${conda_env}/lib/gcc
rm -rf ${conda_env}/lib/libarcher.so.bak
rm -rf ${conda_env}/lib/libasan.*
rm -rf ${conda_env}/lib/libhwasan.*
rm -rf ${conda_env}/lib/liblsan.*
rm -rf ${conda_env}/lib/libtsan.*
rm -rf ${conda_env}/lib/libnode.so.*
rm -rf ${conda_env}/lib/libclang-cpp.so.*
rm -rf ${conda_env}/lib/libclang.so.*
rm -rf ${conda_env}/lib/libLLVM-*.so
rm -rf ${conda_env}/lib/libLLVM.so.*
rm -rf ${conda_env}/lib/node_modules
rm -rf ${conda_env}/lib/objects-Release
rm -rf ${conda_env}/lib/perl5
rm -rf ${conda_env}/lib/python*/site-packages/conda
rm -rf ${conda_env}/lib/python*/site-packages/distlib
rm -rf ${conda_env}/lib/python*/site-packages/pandas/tests
rm -rf ${conda_env}/lib/python*/site-packages/pre_commit
rm -rf ${conda_env}/lib/python*/site-packages/pycparser
rm -rf ${conda_env}/lib/python*/site-packages/pyright*
rm -rf ${conda_env}/lib/python*/site-packages/qtpy/tests
rm -rf ${conda_env}/lib/python*/site-packages/tests
rm -rf ${conda_env}/lib/python*/site-packages/tornado
rm -rf ${conda_env}/lib/python*/site-packages/zstandard
rm -rf ${conda_env}/lib/qt6/bin
rm -rf ${conda_env}/libexec/gcc
rm -rf ${conda_env}/libexec/git-core
rm -rf ${conda_env}/man
rm -rf ${conda_env}/share/cmake
rm -rf ${conda_env}/share/cmake-*
rm -rf ${conda_env}/share/doc
rm -rf ${conda_env}/share/emacs
rm -rf ${conda_env}/share/git-core
rm -rf ${conda_env}/share/git-gui
rm -rf ${conda_env}/share/gitk
rm -rf ${conda_env}/share/gitweb
rm -rf ${conda_env}/share/gtk-doc
rm -rf ${conda_env}/share/info
rm -rf ${conda_env}/share/man
rm -rf ${conda_env}/share/swig
rm -rf ${conda_env}/share/vim
rm -rf ${conda_env}/aarch64-conda-linux-gnu
rm -rf ${conda_env}/x86_64-conda-linux-gnu

find . -name "*.h" -type f -delete
find . -name "*.cmake" -type f -delete

version_name="FreeCAD_${BUILD_TAG}-Linux-$(uname -m)"

echo -e "\################"
echo -e "version_name:  ${version_name}"
echo -e "################"

pixi list -e default > AppDir/packages.txt
sed -i "1s/.*/\nLIST OF PACKAGES:/" AppDir/packages.txt

curl -LO https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$(uname -m).AppImage
chmod a+x appimagetool-$(uname -m).AppImage

if [ "${UPLOAD_RELEASE}" == "true" ]; then
    case "${BUILD_TAG}" in
        *weekly*)
            GH_UPDATE_TAG="weeklies"
            ;;
        *rc*)
            GH_UPDATE_TAG="${BUILD_TAG}"
            ;;
        *)
            GH_UPDATE_TAG="latest"
            ;;
    esac
fi

echo -e "\nCreate the appimage"
# export GPG_TTY=$(tty)
chmod a+x ./AppDir/AppRun
./appimagetool-$(uname -m).AppImage \
  --comp zstd \
  --mksquashfs-opt -Xcompression-level \
  --mksquashfs-opt 22 \
  -u "gh-releases-zsync|FreeCAD|FreeCAD|${GH_UPDATE_TAG}|FreeCAD*$(uname -m)*.AppImage.zsync" \
  AppDir ${version_name}.AppImage
  # -s --sign-key ${GPG_KEY_ID} \

echo -e "\nCreate hash"
sha256sum ${version_name}.AppImage > ${version_name}.AppImage-SHA256.txt

if [ "${UPLOAD_RELEASE}" == "true" ]; then
    gh release upload --clobber ${BUILD_TAG} "${version_name}.AppImage" "${version_name}.AppImage.zsync" "${version_name}.AppImage-SHA256.txt"
    if [ "${GH_UPDATE_TAG}" == "weeklies" ]; then
        generic_name="FreeCAD_weekly-Linux-$(uname -m)"
        mv "${version_name}.AppImage" "${generic_name}.AppImage"
        mv "${version_name}.AppImage.zsync" "${generic_name}.AppImage.zsync"
        mv "${version_name}.AppImage-SHA256.txt" "${generic_name}.AppImage-SHA256.txt"
        gh release create weeklies --prerelease | true
        gh release upload --clobber weeklies "${generic_name}.AppImage" "${generic_name}.AppImage.zsync" "${generic_name}.AppImage-SHA256.txt"
    fi
fi
