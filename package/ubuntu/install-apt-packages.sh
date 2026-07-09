#!/bin/bash
set -euox pipefail

# Update package lists quietly
sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends wget gpg ca-certificates

# Add the KDE Neon repository for up-to-date and matching Qt6 and PySide packages
# Ubuntu 24.04 does not have PySide6 packages available
KEY=$(wget --retry-connrefused --waitretry=3 --tries=5 -qO- https://archive.neon.kde.org/public.key)
if [ -z "$KEY" ]; then
  echo "Failed to download KDE Neon GPG key" >&2
  exit 1
fi
echo "$KEY" | sudo gpg --yes --dearmor -o /usr/share/keyrings/neon-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/neon-keyring.gpg] http://archive.neon.kde.org/user noble main" | sudo tee /etc/apt/sources.list.d/neon-qt.list

sudo apt-get update -qq

packages=(
  build-essential
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
  libeigen3-dev
  libexpat1-dev
  libgtest-dev
  libgmock-dev
  libfmt-dev
  libkdtree++-dev
  libmedc-dev
  libocct-data-exchange-dev
  libocct-ocaf-dev
  libocct-visualization-dev
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
  nlohmann-json3-dev
  occt-draw
  pyside6-tools
  python3-cxx-dev
  python3-av
  python-is-python3
  python3-dev
  python3-defusedxml
  python3-git
  python3-lark
  python3-markdown
  python3-matplotlib
  python3-packaging
  python3-pip
  python3-ply
  python3-pybind11
  python3-pyside6.qtcore
  python3-pyside6.qtgui
  python3-pyside6.qtnetwork
  python3-pyside6.qtsvg
  python3-pyside6.qtsvgwidgets
  python3-pyside6.qtwidgets
  qt6-base-dev
  qt6-l10n-tools
  qt6-tools-dev
  qt6-tools-dev-tools
  qt6-webengine-dev
  swig
  xvfb
)

# Install all packages
sudo apt-get install -y --no-install-recommends "${packages[@]}"
