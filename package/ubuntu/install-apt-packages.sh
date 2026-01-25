#!/bin/bash
set -euo pipefail

# Add the KDE Neon repository for up-to-date and matching Qt6 and PySide packages
# Ubuntu 24.04 does not have PySide6 packages available
neon_key_url="https://archive.neon.kde.org/public.key"
neon_repo_url="https://archive.neon.kde.org/user"
neon_keyring_path="/usr/share/keyrings/neon-keyring.gpg"
neon_list_path="/etc/apt/sources.list.d/neon-qt.list"

neon_enabled=1

neon_key_tmp="$(mktemp)"
trap 'rm -f "$neon_key_tmp"' EXIT

# Fetch the key without sudo so failures are clearer in CI logs
if ! wget -qO "$neon_key_tmp" --timeout=30 --tries=3 "$neon_key_url"; then
  echo "Warning: failed to download KDE Neon signing key from: $neon_key_url" >&2
  neon_enabled=0
elif ! gpg --batch --quiet --show-keys "$neon_key_tmp" >/dev/null; then
  echo "Warning: downloaded Neon key is not valid OpenPGP data: $neon_key_url" >&2
  neon_enabled=0
elif ! sudo gpg --dearmor -o "$neon_keyring_path" "$neon_key_tmp"; then
  echo "Warning: failed to install Neon keyring to: $neon_keyring_path" >&2
  neon_enabled=0
fi

if [[ "$neon_enabled" == "1" ]]; then
  if ! echo "deb [signed-by=$neon_keyring_path] $neon_repo_url noble main" | sudo tee "$neon_list_path" >/dev/null; then
    echo "Warning: failed to add Neon apt source: $neon_repo_url" >&2
    neon_enabled=0
  fi
fi

# Update package lists quietly
if ! sudo apt-get update -qq; then
  echo "Warning: apt-get update failed (continuing)." >&2
  neon_enabled=0
  sudo rm -f "$neon_list_path" >/dev/null 2>&1 || true
  sudo apt-get update -qq || true
fi

packages_common=(
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
  qt6-base-dev
  qt6-l10n-tools
  qt6-tools-dev
  qt6-tools-dev-tools
  swig
  xvfb
)

packages_neon=(
  libpyside6-dev
  pyside6-tools
  python3-pyside6.qtcore
  python3-pyside6.qtgui
  python3-pyside6.qtnetwork
  python3-pyside6.qtsvg
  python3-pyside6.qtwidgets
)

# Install packages available from Ubuntu repositories
sudo apt-get install -y --no-install-recommends "${packages_common[@]}"

# Try Neon-only packages (best effort)
if [[ "$neon_enabled" == "1" ]]; then
  sudo apt-get install -y --no-install-recommends "${packages_neon[@]}" || \
    echo "Warning: failed to install Neon packages (continuing)." >&2
else
  echo "Warning: KDE Neon repository not enabled; skipping PySide6 packages." >&2
fi
