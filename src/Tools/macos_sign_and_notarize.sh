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

IFS=$'\n'
dylibs=($(find ${CONTAINING_FOLDER}/FreeCAD.app -name "*.dylib"))
shared_objects=($(find ${CONTAINING_FOLDER}/FreeCAD.app -name "*.so"))
executables=($(file `find . -type f -perm +111 -print` | grep "Mach-O 64-bit executable" | sed 's/:.*//g'))
IFS=$' \t\n' # The default

signed_files=("${dylibs[@]}" "${shared_objects[@]}" "${executables[@]}")

# This list of files is generated from:
# file `find . -type f -perm +111 -print` | grep "Mach-O 64-bit executable" | sed 's/:.*//g'
for exe in ${signed_files}; do
    run_codesign "${exe}"
done

# Two additional files that must be signed that aren't caught by the above searches:
run_codesign "${CONTAINING_FOLDER}/FreeCAD.app/Contents/packages.txt"
run_codesign "${CONTAINING_FOLDER}/FreeCAD.app/Contents/Library/QuickLook/QuicklookFCStd.qlgenerator/Contents/MacOS/QuicklookFCStd"

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
