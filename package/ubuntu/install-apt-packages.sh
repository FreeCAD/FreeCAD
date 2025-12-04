#!/bin/bash
set -euo pipefail

# Update package lists quietly
sudo apt-get update -qq

if apt-cache show libvtk9-dev >/dev/null 2>&1; then
  vtk_dev="libvtk9-dev"
else
  vtk_dev="libvtk7-dev"
fi

packages=(
  ccache
  cmake
  doxygen
  graphviz
  imagemagick
  libboost-date-time-dev
  libboost-dev
  libboost-filesystem-dev
  libboost-graph-dev
  libboost-iostreams-dev
  libboost-program-options-dev
  libboost-regex-dev
  libboost-serialization-dev
  libboost-thread-dev
  libcoin-dev
  libeigen3-dev
  libkdtree++-dev
  libmedc-dev
  libocct-data-exchange-dev
  libocct-ocaf-dev
  libocct-visualization-dev
  libopencv-dev
  libproj-dev
  libpcl-dev
  libpyside2-dev
  libqt5opengl5-dev
  libqt5svg5-dev
  libqt5x11extras5-dev
  libshiboken2-dev
  libspnav-dev
  ${vtk_dev}
  libx11-dev
  libxerces-c-dev
  libyaml-cpp-dev
  libzipios++-dev
  netgen
  netgen-headers
  ninja-build
  occt-draw
  pyqt5-dev-tools
  pyside2-tools
  python3-dev
  python3-defusedxml
  python3-git
  python3-lark
  python3-markdown
  python3-matplotlib
  python3-packaging
  python3-pivy
  python3-ply
  python3-pybind11
  python3-pyside2.qtcore
  python3-pyside2.qtgui
  python3-pyside2.qtnetwork
  python3-pyside2.qtsvg
  python3-pyside2.qtwidgets
  qtbase5-dev
  qttools5-dev
  shiboken2
  swig
  xvfb
)

# Install all packages
sudo apt-get install -y --no-install-recommends "${packages[@]}"
