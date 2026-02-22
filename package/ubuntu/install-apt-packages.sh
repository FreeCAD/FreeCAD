#!/bin/bash
# SPDX-FileNotice: Part of the FreeCAD project.

set -euo pipefail

# Add the KDE Neon repository for up-to-date and matching Qt6 and PySide packages
# Ubuntu 24.04 does not have PySide6 packages available
sudo wget -qO- http://archive.neon.kde.org/public.key | sudo gpg --dearmor -o /usr/share/keyrings/neon-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/neon-keyring.gpg] http://archive.neon.kde.org/user noble main" | sudo tee /etc/apt/sources.list.d/neon-qt.list

# Update package lists quietly
sudo apt-get update -qq

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
  libpyside6-dev
  libqt6opengl6-dev
  libqt6svg6-dev
  libspnav-dev
  libvtk9-dev
  libx11-dev
  libxerces-c-dev
  libyaml-cpp-dev
  libzipios++-dev
  netgen
  netgen-headers
  ninja-build
  occt-draw
  pyside6-tools
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
  python3-pyside6.qtcore
  python3-pyside6.qtgui
  python3-pyside6.qtnetwork
  python3-pyside6.qtsvg
  python3-pyside6.qtwidgets
  qt6-base-dev
  qt6-l10n-tools
  qt6-tools-dev
  qt6-tools-dev-tools
  swig
  xvfb
)

# Install all packages
sudo apt-get install -y --no-install-recommends "${packages[@]}"
