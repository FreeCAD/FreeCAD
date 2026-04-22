#!/bin/bash

set -e
set -x

conda_env="FreeCAD.app/Contents/Resources"

mkdir -p ${conda_env}

cp -a ../.pixi/envs/default/* ${conda_env}

# delete unnecessary stuff
rm -rf ${conda_env}/include
find ${conda_env} -name \*.a -delete

# Build release bin/ with only the binaries needed at runtime.
mv ${conda_env}/bin ${conda_env}/bin_tmp
mkdir ${conda_env}/bin
while IFS= read -r entry; do
    [ -z "$entry" ] && continue
    [ -f "${conda_env}/bin_tmp/${entry}" ] && cp "${conda_env}/bin_tmp/${entry}" "${conda_env}/bin/"
done < ../../../src/MacAppBundle/bundle_bin_entries.txt
rm -rf ${conda_env}/bin_tmp

sed -i '1s|.*|#!/usr/bin/env python|' ${conda_env}/bin/pip

# copy resources
cp resources/* ${conda_env}

# Remove __pycache__ folders and .pyc files
find . -path "*/__pycache__/*" -delete
find . -name "*.pyc" -type f -delete

# fix problematic rpaths and reexport_dylibs for signing
# see https://github.com/FreeCAD/FreeCAD/issues/10144#issuecomment-1836686775
# and https://github.com/FreeCAD/FreeCAD-Bundle/pull/203
# and https://github.com/FreeCAD/FreeCAD-Bundle/issues/375
python ../scripts/fix_macos_lib_paths.py ${conda_env}/lib -r

# Move installed Contents files into place, including the launcher, Info.plist,
# and ad-hoc signed QuickLook App Extensions.
#
# CI's real signing pass happens below via macos_sign_and_notarize.zsh.
mv ${conda_env}/Contents/* FreeCAD.app/Contents/
rmdir ${conda_env}/Contents

# Read the dmg filename back from the Info.plist that build.sh + cmake just
# created (CFBundleVersion is set from FREECAD_BUNDLE_VERSION).
version_name=$(/usr/libexec/PlistBuddy -c "Print :CFBundleVersion" FreeCAD.app/Contents/Info.plist)

echo -e "\################"
echo -e "version_name:  ${version_name}"
echo -e "################"

pixi list -e default > FreeCAD.app/Contents/packages.txt
sed -i '1s/.*/\nLIST OF PACKAGES:/' FreeCAD.app/Contents/packages.txt

if [[ "${MACOS_SIGN_RELEASE}" == "true" ]]; then
    # create the signed dmg
    ../../scripts/macos_sign_and_notarize.zsh -p "FreeCAD" -k ${MACOS_SIGNING_KEY_ID} -o "${version_name}.dmg"
else
    # Ad-hoc sign the outer bundle. Inner .appex / .qlgenerator signing is
    # already handled at cmake-build time by the sign_modern_extensions and
    # sign_legacy_generator targets, and those signatures survived the
    # install → cp -a → mv chain into FreeCAD.app/Contents/.
    echo "Ad-hoc signing app bundle..."
    codesign --force --sign - FreeCAD.app/Contents/packages.txt
    codesign --force --sign - FreeCAD.app

    # create the dmg
    dmgbuild -s dmg_settings.py "FreeCAD" "${version_name}.dmg"
fi

# create hash
sha256sum ${version_name}.dmg > ${version_name}.dmg-SHA256.txt

if [[ "${UPLOAD_RELEASE}" == "true" ]]; then
    for attempt in 1 2 3 4 5; do
        if gh release upload --clobber ${BUILD_TAG} "${version_name}.dmg" "${version_name}.dmg-SHA256.txt"; then
            break
        fi
        if [[ $attempt -eq 5 ]]; then
            echo "Failed to upload release after 5 attempts" >&2
            exit 1
        fi
        echo "Upload attempt $attempt failed, retrying in $((attempt * 10))s..."
        sleep $((attempt * 10))
    done
fi
