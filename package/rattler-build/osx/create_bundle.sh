#!/bin/bash

set -e
set -x

conda_env="FreeCAD.app/Contents/Resources"

mkdir -p ${conda_env}

cp -a ../.pixi/envs/default/* ${conda_env}

export PATH="${PWD}/${conda_env}/bin:${PATH}"
export CONDA_PREFIX="${PWD}/${conda_env}"

# delete unnecessary stuff
rm -rf ${conda_env}/include
find ${conda_env} -name \*.a -delete

mv ${conda_env}/bin ${conda_env}/bin_tmp
mkdir ${conda_env}/bin
cp ${conda_env}/bin_tmp/freecad ${conda_env}/bin/
cp ${conda_env}/bin_tmp/freecadcmd ${conda_env}/bin
cp ${conda_env}/bin_tmp/ccx ${conda_env}/bin/
cp ${conda_env}/bin_tmp/python ${conda_env}/bin/
cp ${conda_env}/bin_tmp/pip ${conda_env}/bin/
cp ${conda_env}/bin_tmp/pyside6-rcc ${conda_env}/bin/
cp ${conda_env}/bin_tmp/gmsh ${conda_env}/bin/
cp ${conda_env}/bin_tmp/dot ${conda_env}/bin/
cp ${conda_env}/bin_tmp/unflatten ${conda_env}/bin/
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

# build and install the launcher
cmake -B build launcher
cmake --build build
mkdir -p FreeCAD.app/Contents/MacOS
cp build/FreeCAD FreeCAD.app/Contents/MacOS/FreeCAD

python_version=$(python -c 'import platform; print("py" + platform.python_version_tuple()[0] + platform.python_version_tuple()[1])')
version_name="FreeCAD_${BUILD_TAG}-macOS-$(uname -m)-${python_version}"
application_menu_name="FreeCAD_${BUILD_TAG}"

echo -e "\################"
echo -e "version_name:  ${version_name}"
echo -e "################"

cp Info.plist.template ${conda_env}/../Info.plist
sed -i "s/FREECAD_VERSION/${version_name}/" ${conda_env}/../Info.plist
sed -i "s/APPLICATION_MENU_NAME/${application_menu_name}/" ${conda_env}/../Info.plist

pixi list -e default > FreeCAD.app/Contents/packages.txt
sed -i '1s/.*/\nLIST OF PACKAGES:/' FreeCAD.app/Contents/packages.txt

# copy the plugin into its final location
cp -a ${conda_env}/Library ${conda_env}/..
rm -rf ${conda_env}/Library

if [[ "${SIGN_RELEASE}" == "true" ]]; then
    # create the signed dmg
    ./macos_sign_and_notarize.zsh -p "FreeCAD" -k ${SIGNING_KEY_ID} -o "${version_name}.dmg"
else
    # create the dmg
    dmgbuild -s dmg_settings.py "FreeCAD" "${version_name}.dmg"
fi

# create hash
sha256sum ${version_name}.dmg > ${version_name}.dmg-SHA256.txt

if [[ "${UPLOAD_RELEASE}" == "true" ]]; then
    gh release upload --clobber ${BUILD_TAG} "${version_name}.dmg" "${version_name}.dmg-SHA256.txt"
fi
