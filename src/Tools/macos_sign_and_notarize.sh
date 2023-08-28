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
CONTAINING_FOLDER="FreeCAD-0.21-RC1-x86" # Must contain FreeCAD.app and nothing else
ARCH="intel_x86" # intel_x86 or arm64
VERSION_MAJOR="0"
VERSION_MINOR="21"
VERSION_PATCH="0"
VERSION_SUFFIX="RC1" # e.g. alpha, beta, RC1, RC2, release


# Sign all library files
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp `find ${CONTAINING_FOLDER}/FreeCAD.app -name "*.dylib"`
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp `find ${CONTAINING_FOLDER}/FreeCAD.app -name "*.so"`

# This list of files is generated from:
# file `find . -type f -perm +111 -print` | grep "Mach-O 64-bit executable"
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/bin/freecadcmd
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/bin/python
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/bin/ccx
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/bin/freecad
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/libexec/QtWebEngineProcess
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/libexec/gstreamer-1.0/gst-hotdoc-plugins-scanner
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/libexec/gstreamer-1.0/gst-plugin-scanner
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/libexec/gstreamer-1.0/gst-ptp-helper
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/libexec/p11-kit/p11-kit-server
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/libexec/p11-kit/p11-kit-remote
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/kproplog
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/krb5kdc
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/gss-server
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/sserver
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/kprop
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/kadmin.local
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/kdb5_util
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/kpropd
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/sim_server
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/kadmind
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/sbin/uuserver
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/lib/pgxs/src/test/regress/pg_regress
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/lib/pgxs/src/test/isolation/isolationtester
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/lib/pgxs/src/test/isolation/pg_isolation_regress
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/lib/gettext/urlget
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/lib/gettext/hostname
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Resources/lib/gettext/cldr-plurals

# Two additional files that must be signed that aren't caught by the above searches:
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/packages.txt
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app/Contents/Library/QuickLook/QuicklookFCStd.qlgenerator/Contents/MacOS/QuicklookFCStd

# Finally, sign the app itself (must be done last)
codesign --options runtime -f -s ${FREECAD_SIGNING_KEY_ID} --timestamp ${CONTAINING_FOLDER}/FreeCAD.app

# Create a disk image from the folder
DMG_NAME="FreeCAD-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX}-mac-${ARCH}"
echo "Creating disk image ${DMG_NAME}"
hdiutil create -srcfolder "${CONTAINING_FOLDER}" "${DMG_NAME}"

# Submit it for notarization (requires that an App Store API Key has been set up in the notarytool)
xcrun notarytool submit --wait --keychain-profile "FreeCAD" ${DMG_NAME}.dmg

# Assuming that notarization succeeded, it's a good practice to staple that notarization to the DMG
xcrun stapler staple ${DMG_NAME}
