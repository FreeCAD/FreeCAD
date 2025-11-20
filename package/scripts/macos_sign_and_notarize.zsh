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
    echo 'Error: dmgbuild not installed. Please install it'
    echo '- using pixi:'
    echo 'pixi g install dmgbuild --with pyobjc-framework-Quartz'
    echo '- using pip:'
    echo 'pip3 install dmgbuild pyobjc-framework-Quartz'
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

ID_FILE="${DMG_NAME}.notarization_id"

# Submit it for notarization (requires that an App Store API Key has been set up in the notarytool)
# This is a *very slow* process, and occasionally the GitHub runners lose the internet connection for a short time
# during the run. So in order to be fault-tolerant, this script polls, instead of using --wait
submit_notarization_request() {
  if [[ -s "${ID_FILE}" ]]; then
    cat "${ID_FILE}"
    return
  fi
  local out
  if ! out=$(xcrun notarytool submit --keychain-profile "${KEYCHAIN_PROFILE}" \
                --output-format json --no-progress "${DMG_NAME}" 2>&1); then
      print -r -- "$out" >&2
      return 1
  fi
  # We asked for JSON output so we had something stable, but of course parsing JSON with ZSH is ugly, so a quick bit of
  # Python does it instead...
  local id
  id=$(print -r -- "$out" |
    /usr/bin/python3 -c 'import sys, json; print(json.load(sys.stdin).get("id",""))'
  )
  [[ -n "$id" ]] || { print -r -- "Could not parse submission id" >&2; return 1; }
  print -r -- "$id" > "${ID_FILE}"
  print -r -- "$id"  # ID is a string here, not an integer, so I can't just return it
}

wait_for_notarization_result() {
  local id="$1" attempt=0
  while :; do
    if xcrun notarytool wait "$id" --keychain-profile "${KEYCHAIN_PROFILE}" \
          --timeout 10m --no-progress >/dev/null; then
      return 0
    fi

    (( attempt++ ))
    # If the failure was transient (timeout/HTTP/connection) just retry, but make sure to check to see if the problem
    # was actually that the signing failed before retrying.
    local tmp_json
    tmp_json=$(mktemp)
    trap 'rm -f "$tmp_json"' EXIT INT TERM

    xcrun notarytool info "$id" --keychain-profile "${KEYCHAIN_PROFILE}" --output-format json 2>/dev/null > "$tmp_json"
    /usr/bin/python3 - "$tmp_json" <<'PY'
import sys, json
try:
    with open(sys.argv[1]) as f:
        s = (json.load(f).get("status") or "").lower()
    if s in ("invalid", "rejected"):
        sys.exit(2)
    else:
        sys.exit(0)
except Exception:
    sys.exit(1)
PY
    rc=$?

    rm -f "$tmp_json"

    if [[ $rc == 2 ]]; then
      print -r -- "Notarization was not accepted by Apple:" >&2
      xcrun notarytool log "$id" --keychain-profile "${KEYCHAIN_PROFILE}" >&2
      return 3
    fi

    if [[ $attempt -gt 120 ]]; then
      print -r -- "🏳️ Notarization is taking too long, bailing out. 🏳️" >&2
      return 4
    fi
    sleep $(( (attempt<6?2**attempt:60) + RANDOM%5 ))  # Increasing timeout plus jitter for multi-run safety
  done
}

if ! id="$(submit_notarization_request)"; then
  print -r -- "❌ Failed to submit notarization request" >&2
  exit 1
fi
if [[ -z "$id" ]]; then
  print -r -- "❌ Submission succeeded but no ID was returned" >&2
  exit 1
fi
print "Notarization submission ID: $id"

if wait_for_notarization_result "$id"; then
  print "✅ Notarization succeeded. Stapling..."
  xcrun stapler staple "${DMG_NAME}"
  print "Stapled: ${DMG_NAME}"
  rm -f "${ID_FILE}"
else
  rc=$?
  print "❌ Notarization failed (code $rc)." >&2
  exit "$rc"
fi
