##!/usr/bin/env bash

HOST=$(uname)

mkdir -p build
cd build

if [[ ${FEATURE_DEBUG} = 1 ]]; then
      BUILD_TYPE="Debug"
else
      BUILD_TYPE="Release"
fi

declare -a CMAKE_PLATFORM_FLAGS

if [[ ${HOST} =~ "Linux" ]]; then
  PREFIX=/usr/local
  echo "adding hacks for linux"
  # temporary workaround for vtk-cmake setup
  # should be applied @vtk-feedstock
  sed -i 's#/home/conda/feedstock_root/build_artifacts/vtk_.*_build_env/x86_64-conda_cos6-linux-gnu/sysroot/usr/lib.*;##g' ${PREFIX}/lib/cmake/vtk-8.2/Modules/vtkhdf5.cmake
  # temporary workaround for qt-cmake:
  sed -i 's|_qt5gui_find_extra_libs(EGL.*)|_qt5gui_find_extra_libs(EGL "EGL" "" "")|g' $PREFIX/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake
  sed -i 's|_qt5gui_find_extra_libs(OPENGL.*)|_qt5gui_find_extra_libs(OPENGL "GL" "" "")|g' $PREFIX/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake
  CMAKE_PLATFORM_FLAGS+=(-DCMAKE_TOOLCHAIN_FILE="${RECIPE_DIR}/cross-linux.cmake")
  CMAKE_PLATFORM_FLAGS+=(-DPYTHON_EXECUTABLE:FILEPATH=$PREFIX/bin/python)
fi


if [[ ${HOST} =~ "Darwin" ]]; then
  # add hacks for osx here!
  echo "adding hacks for osx"
  PREFIX=/usr/local
  #ln -s /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk

  # install space-mouse
  #curl -o /tmp/3dFW.dmg -L 'https://download.3dconnexion.com/drivers/mac/10-6-6_360DF97D-ED08-4ccf-A55E-0BF905E58476/3DxWareMac_v10-6-6_r3234.dmg'
  #hdiutil attach -readonly /tmp/3dFW.dmg
  #sudo installer -package /Volumes/3Dconnexion\ Software/Install\ 3Dconnexion\ software.pkg -target /
  #diskutil eject /Volumes/3Dconnexion\ Software
  #CMAKE_PLATFORM_FLAGS+=(-D3DCONNEXIONCLIENT_FRAMEWORK:FILEPATH="/Library/Frameworks/3DconnexionClient.framework")
  CMAKE_PLATFORM_FLAGS+=(-DFREECAD_USE_3DCONNEXION:BOOL=OFF)
  CMAKE_PLATFORM_FLAGS+=(-DFREECAD_CREATE_MAC_APP:BOOL=ON)
  CMAKE_PLATFORM_FLAGS+=(-DPYTHON_EXECUTABLE=/usr/local/bin/python3)
  CMAKE_PLATFORM_FLAGS+=(-DHOMEBREW_PREFIX=${PREFIX})
  CMAKE_PLATFORM_FLAGS+=(-DHDF5_DIR:PATH=/usr/local/opt/hdf5@1.10)
  CMAKE_PLATFORM_FLAGS+=(-DQt5Core_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5Core)
  CMAKE_PLATFORM_FLAGS+=(-DQt5Network_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5Network)
  CMAKE_PLATFORM_FLAGS+=(-DQt5Xml_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5Xml)
  CMAKE_PLATFORM_FLAGS+=(-DQt5XmlPatterns_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5XmlPatterns)
  CMAKE_PLATFORM_FLAGS+=(-DQt5Widgets_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5Widgets)
  CMAKE_PLATFORM_FLAGS+=(-DQt5PrintSupport_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5PrintSupport)
  CMAKE_PLATFORM_FLAGS+=(-DQt5OpenGL_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5OpenGL)
  CMAKE_PLATFORM_FLAGS+=(-DQt5Svg_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5Svg)
  CMAKE_PLATFORM_FLAGS+=(-DQt5UiTools_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5UiTools)
  CMAKE_PLATFORM_FLAGS+=(-DQt5Concurrent_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5Concurrent)
  CMAKE_PLATFORM_FLAGS+=(-DQt5WebEngineWidgets_DIR=/usr/local/Cellar/qt5/5.15.0/lib/cmake/Qt5WebEngineWidgets)
fi

cmake -G "Ninja" \
      -D BUILD_WITH_CONDA:BOOL=ON \
      -D CMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -D CMAKE_INSTALL_PREFIX:FILEPATH=$PREFIX \
      -D CMAKE_PREFIX_PATH:FILEPATH=$PREFIX \
      -D CMAKE_LIBRARY_PATH:FILEPATH=$PREFIX/lib \
      -D CMAKE_INSTALL_LIBDIR:FILEPATH=$PREFIX/lib \
      -D CMAKE_INCLUDE_PATH:FILEPATH=$PREFIX/include \
      -D BUILD_QT5:BOOL=ON \
      -D FREECAD_USE_OCC_VARIANT="Official Version" \
      -D OCC_INCLUDE_DIR:FILEPATH=$PREFIX/include \
      -D USE_BOOST_PYTHON:BOOL=OFF \
      -D FREECAD_USE_PYBIND11:BOOL=ON \
      -D BUILD_ENABLE_CXX11:BOOL=ON \
      -D SMESH_INCLUDE_DIR:FILEPATH=$PREFIX/include/smesh \
      -D FREECAD_USE_EXTERNAL_SMESH=ON \
      -D BUILD_FLAT_MESH:BOOL=ON \
      -D BUILD_FEM_NETGEN:BOOL=ON \
      -D BUILD_PLOT:BOOL=OFF \
      -D BUILD_SHIP:BOOL=OFF \
      -D OCCT_CMAKE_FALLBACK:BOOL=OFF \
      -D FREECAD_USE_QT_DIALOG:BOOL=ON \
      -D BUILD_DYNAMIC_LINK_PYTHON:BOOL=OFF \
      -D Boost_NO_BOOST_CMAKE:BOOL=ON \
      -D FREECAD_USE_PCL:BOOL=ON \
      -D INSTALL_TO_SITEPACKAGES:BOOL=ON \
      ${CMAKE_PLATFORM_FLAGS[@]} \
      ..

if [ $? != 0 ]; then
  echo "Failed to configure with cmake."
  exit 1
fi

echo "FREECAD_USE_3DCONNEXION=${FREECAD_USE_3DCONNEXION}"

ninja install

if [ $? != 0 ]; then
  echo "Failed to compile with ninja"
  exit 1
fi
rm -r ${PREFIX}/share/doc/FreeCAD     # smaller size of package!
mv ${PREFIX}/bin/FreeCAD ${PREFIX}/bin/freecad
mv ${PREFIX}/bin/FreeCADCmd ${PREFIX}/bin/freecadcmd
