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

${conda_env}/bin/dot -c

sed -i '1s|.*|#!/usr/bin/env python|' ${conda_env}/bin/pip

echo -e "\nCopying Icon and Desktop file"
cp ${conda_env}/share/applications/org.freecad.FreeCAD.desktop AppDir/
sed -i 's/Exec=FreeCAD/Exec=AppRun/g' AppDir/org.freecad.FreeCAD.desktop
cp ${conda_env}/share/icons/hicolor/scalable/apps/org.freecad.FreeCAD.svg AppDir/

# Remove __pycache__ folders and .pyc files
find . -path "*/__pycache__/*" -delete
find . -name "*.pyc" -type f -delete

# Remove parts of packages we do not want and we know aren't getting loaded under normal circumstances.
# Do not remove libraries that executables depend from (with the exception of optional plugins as with
# Graphviz or Qt), nor remove conda-meta; the former is handled by and the latter needed by
# force_remove_conda_packages.py.
rm -rf \
    ${conda_env}/conda \
    ${conda_env}/doc \
    ${conda_env}/etc/conda \
    ${conda_env}/lib/clang \
    ${conda_env}/lib/cmake \
    ${conda_env}/lib/gcc \
    ${conda_env}/lib/node_modules \
    ${conda_env}/lib/objects-Release \
    ${conda_env}/lib/perl5 \
    ${conda_env}/lib/pkgconfig \
    \
    ${conda_env}/lib/graphviz/libgvplugin_gd.* \
    ${conda_env}/lib/graphviz/libgvplugin_gdk.* \
    ${conda_env}/lib/graphviz/libgvplugin_pango.* \
    ${conda_env}/lib/graphviz/libgvplugin_rsvg.* \
    ${conda_env}/lib/graphviz/libgvplugin_webp.* \
    ${conda_env}/lib/libharfbuzz-cairo.so* \
    ${conda_env}/lib/libpangocairo* \
    \
    ${conda_env}/lib/libQt6EglFs* \
    ${conda_env}/lib/qt6/mkspecs \
    ${conda_env}/lib/qt6/bin \
    ${conda_env}/lib/qt6/plugins/egldeviceintegrations/ \
    ${conda_env}/lib/qt6/plugins/generic \
    ${conda_env}/lib/qt6/plugins/platforms/libqeglfs.so \
    ${conda_env}/lib/qt6/plugins/platforms/libqlinuxfb.so \
    ${conda_env}/lib/qt6/plugins/platforms/libqminimal.so \
    ${conda_env}/lib/qt6/plugins/platforms/libqminimalegl.so \
    ${conda_env}/lib/qt6/plugins/platforms/libqoffscreen.so \
    ${conda_env}/lib/qt6/plugins/platforms/libqvkkhrdisplay.so \
    ${conda_env}/lib/qt6/plugins/platforms/libqvnc.so \
    ${conda_env}/lib/qt6/plugins/qmllint \
    ${conda_env}/lib/qt6/plugins/qmlls \
    ${conda_env}/lib/qt6/plugins/qmltooling \
    ${conda_env}/lib/qt6/plugins/sqldrivers \
    ${conda_env}/lib/qt6/plugins/wayland-graphics-integration-server \
    ${conda_env}/lib/qt6/sbom \
    \
    ${conda_env}/lib/python*/site-packages/conda \
    ${conda_env}/lib/python*/site-packages/distlib \
    ${conda_env}/lib/python*/site-packages/pandas/tests \
    ${conda_env}/lib/python*/site-packages/pre_commit \
    ${conda_env}/lib/python*/site-packages/pycparser \
    ${conda_env}/lib/python*/site-packages/pyright* \
    ${conda_env}/lib/python*/site-packages/qtpy/tests \
    ${conda_env}/lib/python*/site-packages/tests \
    ${conda_env}/lib/python*/site-packages/tornado \
    ${conda_env}/lib/python*/site-packages/zstandard \
    \
    ${conda_env}/libexec/gcc \
    ${conda_env}/libexec/git-core \
    ${conda_env}/man \
    ${conda_env}/share/aclocal \
    ${conda_env}/share/cmake \
    ${conda_env}/share/cmake-* \
    ${conda_env}/share/cups \
    ${conda_env}/share/doc \
    ${conda_env}/share/emacs \
    ${conda_env}/share/git-core \
    ${conda_env}/share/git-gui \
    ${conda_env}/share/gitk \
    ${conda_env}/share/gitweb \
    ${conda_env}/share/gtk-doc \
    ${conda_env}/share/info \
    ${conda_env}/share/man \
    ${conda_env}/share/swig \
    ${conda_env}/share/vim \
    ${conda_env}/share/wayland \
    ${conda_env}/share/wayland-protocols

# Remove linker forwarding .so files
rm -rf \
    ${conda_env}/lib/libncurses.so \
    ${conda_env}/lib/libncursesw.so

# Remove libstdc++ & libgcc_s to use the host ones, as shipping those may clash with
# the host version and prevent things like Mesa from loading
rm -rf ${conda_env}/lib/libgcc_s* ${conda_env}/lib/libstdc++*

# Remove all uneeded and especially unwanted dependencies.
# Unwanted deps mostly consists of Mesa, libdrm, libva, and associated CPU/GPU-specific code
# that has to be provided by the host system anyway. Bundling them can and will cause
# problems if the versions of transitive dependencies between bundled and host start
# diverging, as with https://github.com/FreeCAD/FreeCAD/issues/32061.
# Unneeded deps are all compile tooling, unused paths such as libpq (pulled in by Qt
# Database), and formats that will never reasonably get used in FreeCAD such as Theora.
compile_tools_libs=(
    # Compilers, linkers & tools to run them
    compilers
    'libllvm*'
    'libclang*'
    libsanitizer
    gcc_impl_linux
    llvm-openmp
    ccache
    ninja
    mold
    clangxx
    cmake
    # Python binding, typing & debugging
    debugpy
    swig
    pybind11
    pyright
    # Documentation
    doxygen
    docutils
    # Revision management
    git
    pre-commit
    # Conda itself
    conda-devenv
    conda
    libmamba
    libmambapy
    conda-libmamba-solver
    # Tools
    gtest
    nodejs
    sed
)
database_connectors=(
    'mysql-*'
    libpq
)
hw_accel_libs=(
    libdrm
    'mesa-*'
    libva
    '*openvino*'
    libvpl
    intel-media-driver
)
format_libs=(
    librsvg  # Referenced by an unused ffmpeg video decode backend
    sdl3  # Referenced by an unused ffmpeg video output backend
    sdl2  # Ditto
    libraw  # Referenced by freeimage itself used by OCC, but who imports RAWs in CAD?
    libtheora  # Referenced by VTK but format never took off, erased by WebM/VP{8,9}
    libsndfile  # Referenced by pulseaudio functions only reached in its CLI utils that we remove
)
gtk=(  # Pulled by graphviz but only the SVG output path is used
    '*gtk3*'
    cairo
    gdk-pixbuf
    epoxy
    # Do not actually remove pango: it is necessary for text sizing on the graphviz SVG output.
    # Instead, its Cairo-specific plugin, pangocairo, is manually removed above.
)
./force_remove_conda_packages.py \
    -e "${conda_env}" \
    -s symbol_subst.yaml \
    --keep-needed-libs 'lib(drm|GL|GLU|GLX|stdc\+\+|gcc_s)\.so.*' \
    '*-devel-*' \
    '*-devel' \
    "${compile_tools_libs[@]}" \
    "${database_connectors[@]}" \
    "${hw_accel_libs[@]}" \
    "${format_libs[@]}" \
    "${gtk[@]}" \
    '*-xvfb-*' \
    tk \
    libcurl

rm -rf ${conda_env}/conda-meta
rm -rf ${conda_env}/doc
rm -rf ${conda_env}/etc/conda
rm -rf ${conda_env}/lib/clang
rm -rf ${conda_env}/lib/cmake
rm -rf ${conda_env}/lib/gcc
rm -rf ${conda_env}/lib/libarcher.so.bak
rm -rf ${conda_env}/lib/libasan.*
rm -rf ${conda_env}/lib/libhwasan.*
rm -rf ${conda_env}/lib/libgmock*.*
rm -rf ${conda_env}/lib/libgtest.*
rm -rf ${conda_env}/lib/liblsan.*
rm -rf ${conda_env}/lib/libtsan.*
rm -rf ${conda_env}/lib/libubsan.*
rm -rf ${conda_env}/lib/libnode.so.*
rm -rf ${conda_env}/lib/libclang-cpp.so.*
rm -rf ${conda_env}/lib/libclang.so.*
rm -rf ${conda_env}/lib/libLLVM-*.so
rm -rf ${conda_env}/lib/libLLVM.so.*
rm -rf ${conda_env}/lib/libdrm*.so*
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

find ${conda_env} \( \
    -name "*.a" -o \
    -name "*.h" -o \
    -name "*.prl" -o \
    -name "*.cmake" \
    \) -type f -delete

version_name="FreeCAD_${BUILD_TAG}-Linux-$(uname -m)"

echo -e "\################"
echo -e "version_name:  ${version_name}"
echo -e "################"

pixi list -e default > AppDir/packages.txt
sed -i "1s/.*/\nLIST OF PACKAGES:/" AppDir/packages.txt

echo "Copying resources and compiling translations for the AppRun script..."
cp ../../../src/Gui/Icons/launch-error.svg AppDir/
for po in ./AppDir/locale/*/AppRun.po; do
    LC_MESSAGES_DIR="$(dirname "${po}")/LC_MESSAGES"
    mkdir -p "${LC_MESSAGES_DIR}"
    msgfmt -o "${LC_MESSAGES_DIR}/AppRun.mo" "${po}"
done

echo "Running FreeCAD command-line smoke test..."
if ! "${conda_env}/bin/freecadcmd" --safe-mode --version; then
    echo "FreeCAD command-line smoke test failed; the Linux bundle cannot start."
    exit 1
fi

echo "Running FreeCAD bundled Pivy smoke test..."
if ! "${conda_env}/bin/freecadcmd" --safe-mode --console "import pivy; from pivy import coin; print(pivy.__file__); print(coin.SoDB.getVersion())"; then
    echo "FreeCAD bundled Pivy smoke test failed; the Linux bundle cannot import the bundled Coin/Pivy runtime."
    exit 1
fi

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
export VERSION="${BUILD_TAG}"
./appimagetool-$(uname -m).AppImage \
  --comp zstd \
  --mksquashfs-opt -Xcompression-level \
  --mksquashfs-opt 22 \
  -u "gh-releases-zsync|FreeCAD|FreeCAD|${GH_UPDATE_TAG}|FreeCAD*$(uname -m)*.AppImage.zsync" \
  AppDir ${version_name}.AppImage
  # -s --sign-key ${GPG_KEY_ID} \

if [ "${UPLOAD_RELEASE}" == "true" ]; then
    gh release upload --clobber ${BUILD_TAG} "${version_name}.AppImage" "${version_name}.AppImage.zsync"
    if [ "${GH_UPDATE_TAG}" == "weeklies" ]; then
        generic_name="FreeCAD_weekly-Linux-$(uname -m)"
        mv "${version_name}.AppImage" "${generic_name}.AppImage"
        mv "${version_name}.AppImage.zsync" "${generic_name}.AppImage.zsync"
        sed -i "s/URL: ${version_name}/URL: ${generic_name}/g" "${generic_name}.AppImage.zsync"
        gh release create weeklies --prerelease | true
        gh release upload --clobber weeklies "${generic_name}.AppImage" "${generic_name}.AppImage.zsync"
    fi
fi
