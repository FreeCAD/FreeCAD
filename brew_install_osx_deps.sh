#!/usr/bin/env bash

BREW_LIST=(ninja \
  doxygen \
  hdf5@1.10 \
  boost \
  xerces-c \
  opencascade \
  vtk \
  swig \
  qt5 \
  freecad/freecad/coin \
  boost-python3 \
  pcl \
  pyside \
  libspnav \
  pybind11 \
)

for pkg in ${BREW_LIST[@]}; do
  echo "Brew Installing Pkg: ${pkg}"
  brew install ${pkg}
  if [ $? != 0 ]; then
    echo "Failed to install ${pkg}"
    exit 1
  fi
done