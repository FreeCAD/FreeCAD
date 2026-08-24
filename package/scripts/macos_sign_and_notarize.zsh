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

# A failure here is fatal: catch it as early as possible to hopefully be able to provide some kind
# of useful diagnostic information.
function run_codesign {
    echo "Signing $1"
    if ! /usr/bin/codesign --options runtime -f -s ${SIGNING_KEY_ID} --timestamp --entitlements entitlements.plist "$1"; then
        print -r -- "❌ codesign failed for $1" >&2
        exit 1
    fi
}

function run_codesign_extension {
    local target="$1"
    local entitlements_file="$2"
    echo "Signing extension $target with entitlements $entitlements_file"
    if ! /usr/bin/codesign --options runtime -f -s ${SIGNING_KEY_ID} --timestamp --entitlements "$entitlements_file" "$target"; then
        print -r -- "❌ codesign failed for extension $target" >&2
        exit 1
    fi
}

IFS=$'\n'
dylibs=($(/usr/bin/find "${CONTAINING_FOLDER}/${APP_NAME}" -name "*.dylib"))
shared_objects=($(/usr/bin/find "${CONTAINING_FOLDER}/${APP_NAME}" -name "*.so"))
bundles=($(/usr/bin/find "${CONTAINING_FOLDER}/${APP_NAME}" -name "*.bundle"))
executables=($(/usr/bin/find "${CONTAINING_FOLDER}/${APP_NAME}" -type f -perm +111 -exec file {} + | grep "Mach-O 64-bit executable" | grep -v " (for architecture " | sed 's/:.*//g'))
IFS=$' \t\n' # The default

typeset -U signed_files
signed_files=("${dylibs[@]}" "${shared_objects[@]}" "${bundles[@]}" "${executables[@]}")

# Not caught by the searches above:
run_codesign "${CONTAINING_FOLDER}/${APP_NAME}/Contents/packages.txt"

MAIN_EXECUTABLE="${CONTAINING_FOLDER}/${APP_NAME}/Contents/MacOS/${APP_NAME:r}"

# This list of files is generated from:
# file `find . -type f -perm +111 -print` | grep "Mach-O 64-bit executable" | sed 's/:.*//g'
for exe in ${signed_files}; do
    # Skip .appex executables as they will be signed separately with their bundles
    if [[ "$exe" != */Contents/PlugIns/*.appex/* && "$exe" != "$MAIN_EXECUTABLE" ]]; then
        run_codesign "${exe}"
    fi
done

# Sign legacy QuickLook generator if present (not built for macOS 15.0+)
if [ -f "${CONTAINING_FOLDER}/${APP_NAME}/Contents/Library/QuickLook/QuicklookFCStd.qlgenerator/Contents/MacOS/QuicklookFCStd" ]; then
    run_codesign "${CONTAINING_FOLDER}/${APP_NAME}/Contents/Library/QuickLook/QuicklookFCStd.qlgenerator/Contents/MacOS/QuicklookFCStd"
fi

# Sign new Swift QuickLook extensions (macOS 15.0+) with their specific entitlements
# These must be signed before the app itself to avoid overriding the extension signatures
if [ -d "${CONTAINING_FOLDER}/${APP_NAME}/Contents/PlugIns" ]; then
    # Find the entitlements files relative to script location
    # Script is in package/scripts/, entitlements are in src/MacAppBundle/QuickLook/modern/
    SCRIPT_DIR="${0:A:h}"  # zsh equivalent of dirname with full path resolution
    PREVIEW_ENTITLEMENTS="${SCRIPT_DIR}/../../src/MacAppBundle/QuickLook/modern/PreviewExtension.entitlements"
    THUMBNAIL_ENTITLEMENTS="${SCRIPT_DIR}/../../src/MacAppBundle/QuickLook/modern/ThumbnailExtension.entitlements"

    # Sign individual executables within .appex bundles first
    if [ -f "${CONTAINING_FOLDER}/${APP_NAME}/Contents/PlugIns/FreeCADThumbnailExtension.appex/Contents/MacOS/FreeCADThumbnailExtension" ]; then
        run_codesign "${CONTAINING_FOLDER}/${APP_NAME}/Contents/PlugIns/FreeCADThumbnailExtension.appex/Contents/MacOS/FreeCADThumbnailExtension"
    fi
    if [ -f "${CONTAINING_FOLDER}/${APP_NAME}/Contents/PlugIns/FreeCADPreviewExtension.appex/Contents/MacOS/FreeCADPreviewExtension" ]; then
        run_codesign "${CONTAINING_FOLDER}/${APP_NAME}/Contents/PlugIns/FreeCADPreviewExtension.appex/Contents/MacOS/FreeCADPreviewExtension"
    fi

    # Then sign the .appex bundles themselves with extension-specific entitlements
    if [ -d "${CONTAINING_FOLDER}/${APP_NAME}/Contents/PlugIns/FreeCADThumbnailExtension.appex" ] && [ -f "$THUMBNAIL_ENTITLEMENTS" ]; then
        run_codesign_extension "${CONTAINING_FOLDER}/${APP_NAME}/Contents/PlugIns/FreeCADThumbnailExtension.appex" "$THUMBNAIL_ENTITLEMENTS"
    fi
    if [ -d "${CONTAINING_FOLDER}/${APP_NAME}/Contents/PlugIns/FreeCADPreviewExtension.appex" ] && [ -f "$PREVIEW_ENTITLEMENTS" ]; then
        run_codesign_extension "${CONTAINING_FOLDER}/${APP_NAME}/Contents/PlugIns/FreeCADPreviewExtension.appex" "$PREVIEW_ENTITLEMENTS"
    fi
fi

# Finally, sign the app itself (must be done last)
run_codesign "${CONTAINING_FOLDER}/${APP_NAME}"

# Create a disk image from the folder
echo "Creating disk image ${DMG_NAME}"
dmgbuild -s ${DMG_SETTINGS} -Dcontaining_folder="${CONTAINING_FOLDER}" -Dapp_name="${APP_NAME}" "${VOLUME_NAME}" "${DMG_NAME}"

ID_FILE="${DMG_NAME}.notarization_id"

# Total wall-clock budget for Apple to finish processing a submission. Individual "notarytool wait"
# calls have their own shorter timeout and are retried until this budget is exhausted.
NOTARIZATION_TIMEOUT_SECONDS="${NOTARIZATION_TIMEOUT_SECONDS:-3600}"

# How long to keep retrying the staple *after* Apple has confirmed the submission was Accepted. A
# ticket for an accepted submission is normally published within seconds, so a few minutes is
# already generous, there's no benefit to keep hammering on this for hours.
STAPLE_MAX_ATTEMPTS="${STAPLE_MAX_ATTEMPTS:-20}"

# Set to 1 to ship a notarized-but-unstapled disk image instead of failing. An unstapled image still
# passes Gatekeeper, but only if the user is online the first time they open it, so this is a
# reasonable trade for weekly builds and a bad one for tagged releases.
ALLOW_UNSTAPLED="${ALLOW_UNSTAPLED:-0}"

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

# Reports the submission status ("Accepted", "Invalid", "Rejected", "In Progress") on stdout, or
# nothing at all if the query itself failed, which the caller treats as "ask again later".
notarization_status() {
  local id="$1" out
  out=$(xcrun notarytool info "$id" --keychain-profile "${KEYCHAIN_PROFILE}" \
          --output-format json 2>/dev/null) || return 1
  print -r -- "$out" |
    /usr/bin/python3 -c 'import sys, json; print(json.load(sys.stdin).get("status") or "")' 2>/dev/null
}

# The exit code of "notarytool wait" is NOT a success indicator: it returns 0 as soon as the
# submission reaches *any* terminal state, and Invalid and Rejected are terminal states. So I guess
# Apple thinks that's a success?! OK, so we manually check, because we don't have a choice.
wait_for_notarization_result() {
  local id="$1"
  local deadline=$(( $(date +%s) + NOTARIZATION_TIMEOUT_SECONDS ))
  local attempt=0
  local submission_status

  while :; do
    # Failure here isn't real failure: it means "timed out" or "the network is borked", etc.
    # The status query below is what actually decides whether we are done.
    xcrun notarytool wait "$id" --keychain-profile "${KEYCHAIN_PROFILE}" \
      --timeout 10m --no-progress >/dev/null 2>&1

    submission_status="$(notarization_status "$id")"
    print -r -- "Notarization status: ${submission_status:-unavailable}"

    case "${submission_status:l}" in
      accepted)
        return 0
        ;;
      invalid|rejected)
        print -r -- "❌ Apple did not accept the submission (status: ${submission_status})." >&2
        print -r -- "--- begin notarytool log ---" >&2
        xcrun notarytool log "$id" --keychain-profile "${KEYCHAIN_PROFILE}" >&2
        print -r -- "--- end notarytool log ---" >&2
        return 3
        ;;
    esac

    (( attempt++ ))
    if (( $(date +%s) >= deadline )); then
      print -r -- "🏳️ Notarization did not finish within ${NOTARIZATION_TIMEOUT_SECONDS}s, bailing out. 🏳️" >&2
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

# We are getting occasional (well, really somewhat frequent) failures of the "staple" action
# from Apple's server. I don't know if this is because of a flaky GitHub connection, or flaky
# Apple server, or what, but we should be quite flexible in detecting and handling these failures.
# *Sometimes*, but not always, the staple action will emit failure text, but still return 0, so
# we have to check for both the bad exit code, as well as a few different possible error strings.
#
# This retries only for as long as a transient fault could last in real life. It is deliberately
# NOT a multi-hour loop: this function is only ever reached once Apple has confirmed the submissio
# was "Accepted", and a ticket for an accepted submission does not take hours to appear. A persistent
# "Record not found" here means no ticket exists, so we should bail out, because it's never going to
# appear.
staple_with_retry() {
  local target="$1"
  local attempt=0
  local out rc
  while :; do
    (( attempt++ ))
    out=$(xcrun stapler staple "${target}" 2>&1)
    rc=$?
    print -r -- "$out"

    local looks_failed=0
    if [[ $rc -ne 0 ]]; then
      looks_failed=1
    elif print -r -- "$out" | grep -E -q -e 'The staple and validate action failed' \
                                          -e 'Error 6[0-9]' \
                                          -e 'Record not found' \
                                          -e "CloudKit's response is inconsistent" \
                                          -e 'Could not validate ticket'; then
      looks_failed=1
    fi

    if [[ $looks_failed -eq 0 ]]; then
      if xcrun stapler validate "${target}" >/dev/null 2>&1; then
        return 0
      fi
      print -r -- "Staple appeared to succeed but stapler validate failed." >&2
    fi

    if (( attempt >= STAPLE_MAX_ATTEMPTS )); then
      print -r -- "Failed to staple after ${attempt} attempts" >&2
      return 1
    fi

    print -r -- "Staple attempt ${attempt} failed, retrying..."
    sleep $(( (attempt<6?2**attempt:30) + RANDOM%5 ))  # Increasing timeout plus jitter for multi-run safety
  done
}

wait_for_notarization_result "$id"
rc=$?
if (( rc != 0 )); then
  print "❌ Notarization failed (code $rc)." >&2
  # Drop the cached id so that a re-run submits afresh instead of re-querying a dead submission.
  rm -f "${ID_FILE}"
  exit "$rc"
fi

print "✅ Apple accepted the submission. Stapling..."
if staple_with_retry "${DMG_NAME}"; then
  print "Stapled: ${DMG_NAME}"
  rm -f "${ID_FILE}"
  exit 0
fi

# Reaching here means Apple accepted the submission but never published a ticket we can attach, which
# is a fault on Apple's side rather than anything wrong with the disk image.
print "❌ Failed to staple ${DMG_NAME} (submission ${id} was Accepted, but no ticket was issued)." >&2
# The cached id describes the disk image built by *this* run. A re-run rebuilds the image from
# scratch, and a ticket for the previous image can never be stapled to the new one, so the id has to
# go or the retry is guaranteed to fail the same way.
rm -f "${ID_FILE}"
if [[ "${ALLOW_UNSTAPLED}" == "1" ]]; then
  print "⚠️  Shipping an unstapled disk image. It is notarized, so Gatekeeper will accept it, but only" >&2
  print "⚠️  for users who are online the first time they open it." >&2
  exit 0
fi
exit 1
