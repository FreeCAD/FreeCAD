#!/bin/zsh

# SPDX-License-Identifier: LGPL-2.1-or-later

# This script signs and notarizes a FreeCAD.app bundle. It expects that the bundle is in a folder
# by itself (that folder will be used as the basis for the created disk image file, so anything
# else in it will become part of the image). That folder should be located in the same folder as
# this script.

# An environment variable called FREECAD_SIGNING_KEY_ID must exist that contains the ID of a
# Developer ID Application certificate that has been installed into the login keychain. See the
# output of
#   security find-identity -p basic -v
# for a list of available keys, and the documentation for
#   xcrun notarytool store-credentials
# for instructions on how to configure the credentials for the tool for use before running this
# script.

# CONFIGURATION OPTIONS
CONTAINING_FOLDER="FreeCAD_0.21.1_arm64" # Must contain FreeCAD.app and nothing else
ARCH="arm64" # intel_x86 or arm64
VERSION_MAJOR="0"
VERSION_MINOR="21"
VERSION_PATCH="1"
VERSION_SUFFIX="" # e.g. alpha, beta, RC1, RC2, release

function run_codesign {
    echo "Signing $1"
    codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp --entitlements entitlements.plist $1
}

function run_codesign_simple {
    echo "Signing (simple) $1"
    codesign -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp $1
}

function run_codesign_extension {
    local target="$1"
    local entitlements_file="$2"
    echo "Signing extension $target with entitlements $entitlements_file"
    codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp --entitlements "$entitlements_file" "$target"
}

IFS=$'\n'
dylibs=($(find ${CONTAINING_FOLDER}/FreeCAD.app -name "*.dylib"))
shared_objects=($(find ${CONTAINING_FOLDER}/FreeCAD.app -name "*.so"))
executables=($(file `find . -type f -perm +111 -print` | grep "Mach-O 64-bit executable" | sed 's/:.*//g'))
IFS=$' \t\n' # The default

# Sign Qt libraries first (they may have framework-style names that need special handling)
echo "Signing Qt libraries..."
qt_libs=($(find ${CONTAINING_FOLDER}/FreeCAD.app/Contents/lib -name "Qt*" -type f))
for qt_lib in ${qt_libs[@]}; do
    if file "$qt_lib" | grep -q "Mach-O"; then
        run_codesign_simple "${qt_lib}"
    fi
done

# Sign Qt platform plugins
echo "Signing Qt platform plugins..."
qt_plugins=($(find ${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns -name "*.dylib" -type f 2>/dev/null))
for qt_plugin in ${qt_plugins[@]}; do
    if file "$qt_plugin" | grep -q "Mach-O"; then
        run_codesign_simple "${qt_plugin}"
    fi
done

signed_files=("${dylibs[@]}" "${shared_objects[@]}" "${executables[@]}")

# This list of files is generated from:
# file `find . -type f -perm +111 -print` | grep "Mach-O 64-bit executable" | sed 's/:.*//g'
for exe in ${signed_files}; do
    # Skip Qt libraries and platform plugins as they were already signed above
    if [[ "$exe" != */Contents/lib/Qt* ]] && [[ "$exe" != */Contents/PlugIns/*.dylib ]]; then
        run_codesign "${exe}"
    fi
done

# Additional files that must be signed that aren't caught by the above searches:
run_codesign "${CONTAINING_FOLDER}/FreeCAD.app/Contents/packages.txt"

# Sign legacy QuickLook generator (for backward compatibility with older macOS)
if [ -f "${CONTAINING_FOLDER}/FreeCAD.app/Contents/Library/QuickLook/QuicklookFCStd.qlgenerator/Contents/MacOS/QuicklookFCStd" ]; then
    run_codesign "${CONTAINING_FOLDER}/FreeCAD.app/Contents/Library/QuickLook/QuicklookFCStd.qlgenerator/Contents/MacOS/QuicklookFCStd"
fi

# Sign new Swift QuickLook extensions (macOS 15.0+)
if [ -d "${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns" ]; then
    # Find the entitlements files
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PREVIEW_ENTITLEMENTS="$SCRIPT_DIR/../MacAppBundle/QuickLook/modern/PreviewExtension.entitlements"
    THUMBNAIL_ENTITLEMENTS="$SCRIPT_DIR/../MacAppBundle/QuickLook/modern/ThumbnailExtension.entitlements"

    # Sign individual executables within .appex bundles first
    if [ -f "${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns/FreeCADThumbnailExtension.appex/Contents/MacOS/FreeCADThumbnailExtension" ]; then
        run_codesign "${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns/FreeCADThumbnailExtension.appex/Contents/MacOS/FreeCADThumbnailExtension"
    fi
    if [ -f "${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns/FreeCADPreviewExtension.appex/Contents/MacOS/FreeCADPreviewExtension" ]; then
        run_codesign "${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns/FreeCADPreviewExtension.appex/Contents/MacOS/FreeCADPreviewExtension"
    fi

    # Then sign the .appex bundles themselves with extension-specific entitlements
    if [ -d "${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns/FreeCADThumbnailExtension.appex" ] && [ -f "$THUMBNAIL_ENTITLEMENTS" ]; then
        run_codesign_extension "${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns/FreeCADThumbnailExtension.appex" "$THUMBNAIL_ENTITLEMENTS"
    fi
    if [ -d "${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns/FreeCADPreviewExtension.appex" ] && [ -f "$PREVIEW_ENTITLEMENTS" ]; then
        run_codesign_extension "${CONTAINING_FOLDER}/FreeCAD.app/Contents/PlugIns/FreeCADPreviewExtension.appex" "$PREVIEW_ENTITLEMENTS"
    fi
fi

# Finally, sign the app itself (must be done last)
run_codesign "${CONTAINING_FOLDER}/FreeCAD.app"

# Create a disk image from the folder
DMG_NAME="FreeCAD-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-mac-${ARCH}.dmg"
echo "Creating disk image ${DMG_NAME}"
pip3 install "dmgbuild[badge_icons]>=1.6.0,<1.7.0"
dmgbuild -s dmg_settings.py -Dcontaining_folder="${CONTAINING_FOLDER}" "FreeCAD" "${DMG_NAME}.dmg"

# Submit it for notarization (requires that an App Store API Key has been set up in the notarytool)
xcrun notarytool submit --wait --keychain-profile "FreeCAD" ${DMG_NAME}

# Assuming that notarization succeeded, it's a good practice to staple that notarization to the DMG
xcrun stapler staple ${DMG_NAME}
