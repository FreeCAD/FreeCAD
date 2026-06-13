#!/bin/bash
set -euo pipefail

# Add the KDE Neon repository for up-to-date and matching Qt6 and PySide packages
# Ubuntu 24.04 does not have PySide6 packages available
KEY=$(wget --retry-connrefused --waitretry=3 --tries=5 -qO- https://archive.neon.kde.org/public.key)
if [ -z "$KEY" ]; then
  echo "Failed to download KDE Neon GPG key" >&2
  exit 1
fi
echo "$KEY" | sudo gpg --dearmor -o /usr/share/keyrings/neon-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/neon-keyring.gpg] http://archive.neon.kde.org/user noble main" | sudo tee /etc/apt/sources.list.d/neon-qt.list

# Update package lists with retry logic for transient failures
update_package_lists() {
  local max_attempts=10
  local attempt=1
  local wait_time=5  # Exponential, doubles on every attempt

  while [ $attempt -le $max_attempts ]; do
    echo "apt update attempt $attempt of $max_attempts" >&2

    if sudo apt-get update -qq; then
      echo "apt update succeeded" >&2
      return 0
    fi

    if [ $attempt -lt $max_attempts ]; then
      echo "apt update failed, waiting ${wait_time}s before retry" >&2
      sleep "$wait_time"
      wait_time=$((wait_time * 2))
    fi

    attempt=$((attempt + 1))
  done

  echo "apt update failed after $max_attempts attempts" >&2
  echo "Available repositories:" >&2
  ls -la /etc/apt/sources.list.d/ >&2 || true
  echo "apt configuration" >&2
  apt-config dump | grep -E "^APT|^Acquire" >&2 || true
  exit 1
}

update_package_lists

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
  libgtest-dev
  libgmock-dev
  libfmt-dev
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
  python3-cxx-dev
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
  python3-pyside6.qtsvgwidgets
  python3-pyside6.qtwidgets
  qt6-base-dev
  qt6-l10n-tools
  qt6-tools-dev
  qt6-tools-dev-tools
  swig
  xvfb
)

# Install packages with retry logic for transient failures
install_packages() {
  local max_attempts=10
  local attempt=1
  local wait_time=5  # Exponential, doubles on every attempt

  while [ $attempt -le $max_attempts ]; do
    echo "Package installation attempt $attempt of $max_attempts" >&2
    echo "Total packages to install: ${#packages[@]}" >&2

    if sudo apt-get install -y --no-install-recommends "${packages[@]}"; then
      echo "Package installation succeeded" >&2
      return 0
    fi

    if [ $attempt -lt $max_attempts ]; then
      echo "Package installation failed, waiting ${wait_time}s before retry" >&2
      sleep "$wait_time"
      wait_time=$((wait_time * 2))
    fi

    attempt=$((attempt + 1))
  done

  echo "Package installation failed after $max_attempts attempts" >&2
  echo "Diagnostic information" >&2
  echo "Failed package list" >&2
  printf '%s\n' "${packages[@]}" >&2
  echo "Available repositories" >&2
  ls -la /etc/apt/sources.list.d/ >&2 || true
  echo "apt cache state" >&2
  apt-cache stats 2>&1 | head -20 >&2 || true
  exit 1
}

install_packages
