#!/bin/sh
RELEASE_VERSION="$1"

SOURCE_URL="https://github.com/FreeCAD/FreeCAD-Bundle/releases/download/$RELEASE_VERSION/freecad_source.tar.gz"
curl -O --location --retry 3 "https://github.com/FreeCAD/FreeCAD-Bundle/releases/download/$RELEASE_VERSION/freecad_version.txt"
curl -O --location --retry 3 $SOURCE_URL
curl -O --location --retry 3 "https://raw.githubusercontent.com/filippor/FreeCAD/refs/heads/copr/package/fedora/freecad.spec.rpkg"
curl -O --location --retry 3 "https://raw.githubusercontent.com/filippor/FreeCAD/refs/heads/copr/package/fedora/generate_spec_from_version.sh"

sh generate_spec_from_version.sh "$RELEASE_VERSION" "$COPR_PACKAGE"

