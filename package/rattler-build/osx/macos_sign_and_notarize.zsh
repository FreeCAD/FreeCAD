#!/bin/zsh

# SPDX-License-Identifier: LGPL-2.1-or-later

# Default values
SIGNING_KEY_ID="${FREECAD_SIGNING_KEY_ID}"
KEYCHAIN_PROFILE="FreeCAD"
CONTAINING_FOLDER="."
APP_NAME="FreeCAD.app"
VOLUME_NAME="FreeCAD"
DMG_NAME="FreeCAD-macOS-$(uname -m).dmg"
DMG_SETTINGS="dmg_settings.py"

# Function to display usage information
function usage {
    echo "Usage: $0 [-k|--key-id <signing_key_id>] [-p|--keychain-profile <keychain_profile>]"
    echo "                [-d|--dir <containing_folder>] [-n|--app-name <app_name.app>]"
    echo "                [-v|--volume-name <volume_name>] [-o|--output <image_name.dmg>]"
    echo "                [-s|--dmg-settings <dmg_settings.py>]"
    echo
    echo "This script signs and notarizes a FreeCAD.app bundle. It expects that the bundle is in a folder"
    echo "by itself (that folder will be used as the basis for the created disk image file, so anything"
    echo "else in it will become part of the image). That folder should be located in the same folder as"
    echo "this script."
    echo
    echo "If <signing_key_id> is not passed it defaults to env variable FREECAD_SIGNING_KEY_ID, it should"
    echo "be a Developer ID Application certificate that has been installed into the login keychain."
    echo "For a list of available keys see the output of"
    echo "    security find-identity -p basic -v"
    echo "For instructions on how to configure the credentials for the tool for use before running this"
    echo "script see the documentation for"
    echo "    xcrun notarytool store-credentials"

    exit 1
}

# Parse command line arguments
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -k|--key-id)
            SIGNING_KEY_ID="$2"
            shift 2
            ;;
        -p|--keychain-profile)
            KEYCHAIN_PROFILE="$2"
            shift 2
            ;;
        -d|--dir)
            CONTAINING_FOLDER="$2"
            shift 2
            ;;
        -n|--app-name)
            APP_NAME="$2"
            shift 2
            ;;
        -v|--volume-name)
            VOLUME_NAME="$2"
            shift 2
            ;;
        -o|--output)
            DMG_NAME="$2"
            shift 2
            ;;
        -s|--dmg-settings)
            DMG_SETTINGS="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Unknown parameter passed: $1"
            usage
            ;;
    esac
done

# Check if SIGNING_KEY_ID is set
if [ -z "$SIGNING_KEY_ID" ]; then
    echo "Error: Signing key ID is required."
    usage
fi

# Check for dmgbuild executable
if ! command -v dmgbuild &> /dev/null; then
    echo "Error: dmgbuild not installed. Please install it for example using pip:"
    echo 'pip3 install "dmgbuild[badge_icons]>=1.6.0,<1.7.0"'
    exit 1
fi

function run_codesign {
    echo "Signing $1"
    /usr/bin/codesign --options runtime -f -s ${SIGNING_KEY_ID} --timestamp --entitlements entitlements.plist "$1"
}

IFS=$'\n'
dylibs=($(/usr/bin/find "${CONTAINING_FOLDER}/${APP_NAME}" -name "*.dylib"))
shared_objects=($(/usr/bin/find "${CONTAINING_FOLDER}/${APP_NAME}" -name "*.so"))
bundles=($(/usr/bin/find "${CONTAINING_FOLDER}/${APP_NAME}" -name "*.bundle"))
executables=($(/usr/bin/find "${CONTAINING_FOLDER}/${APP_NAME}" -type f -perm +111 -exec file {} + | grep "Mach-O 64-bit executable" | sed 's/:.*//g'))
IFS=$' \t\n' # The default

signed_files=("${dylibs[@]}" "${shared_objects[@]}" "${bundles[@]}" "${executables[@]}")

# This list of files is generated from:
# file `find . -type f -perm +111 -print` | grep "Mach-O 64-bit executable" | sed 's/:.*//g'
for exe in ${signed_files}; do
    run_codesign "${exe}"
done

# Two additional files that must be signed that aren't caught by the above searches:
run_codesign "${CONTAINING_FOLDER}/${APP_NAME}/Contents/packages.txt"
run_codesign "${CONTAINING_FOLDER}/${APP_NAME}/Contents/Library/QuickLook/QuicklookFCStd.qlgenerator/Contents/MacOS/QuicklookFCStd"

# Finally, sign the app itself (must be done last)
run_codesign "${CONTAINING_FOLDER}/${APP_NAME}"

# Create a disk image from the folder
echo "Creating disk image ${DMG_NAME}"
dmgbuild -s ${DMG_SETTINGS} -Dcontaining_folder="${CONTAINING_FOLDER}" -Dapp_name="${APP_NAME}" "${VOLUME_NAME}" "${DMG_NAME}"

# Submit it for notarization (requires that an App Store API Key has been set up in the notarytool)
time xcrun notarytool submit --wait --keychain-profile "${KEYCHAIN_PROFILE}" "${DMG_NAME}"

# Assuming that notarization succeeded, it's a good practice to staple that notarization to the DMG
xcrun stapler staple "${DMG_NAME}"
