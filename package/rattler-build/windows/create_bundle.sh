#!/bin/bash

set -e
set -x 

conda_env="fc_env"

mkdir -p ${conda_env}

cp -a ../.pixi/envs/default/* ${conda_env}

export PATH="${PWD}/${conda_env}/bin:${PATH}"
export CONDA_PREFIX="${PWD}/${conda_env}"

# remove arm binaries that fail to extract unless using latest 7zip
rm $(find ${conda_env} -name \*arm\*.exe)

# delete unnecessary stuff
rm -rf ${conda_env}/include
find ${conda_env} -name \*.a -delete

copy_dir="FreeCAD_Conda_Build"
mkdir -p ${copy_dir}/bin

# Copy Conda's Python and (U)CRT to FreeCAD/bin
cp -a ${conda_env}/DLLs ${copy_dir}/bin/DLLs
cp -a ${conda_env}/Lib ${copy_dir}/bin/Lib
cp -a ${conda_env}/Scripts ${copy_dir}/bin/Scripts
cp -a ${conda_env}/python*.* ${copy_dir}/bin
cp -a ${conda_env}/msvc*.* ${copy_dir}/bin
cp -a ${conda_env}/ucrt*.* ${copy_dir}/bin
# Copy meaningful executables
cp -a ${conda_env}/Library/bin/ccx.exe ${copy_dir}/bin
cp -a ${conda_env}/Library/bin/gmsh.exe ${copy_dir}/bin
cp -a ${conda_env}/Library/bin/dot.exe ${copy_dir}/bin
cp -a ${conda_env}/Library/bin/unflatten.exe ${copy_dir}/bin
cp -a ${conda_env}/Library/mingw-w64/bin/* ${copy_dir}/bin
# copy resources -- perhaps needs reduction
cp -a ${conda_env}/Library/share ${copy_dir}/share
# get all the dependency .dlls
cp -a ${conda_env}/Library/bin/*.dll ${copy_dir}/bin
# Copy FreeCAD build
cp -a ${conda_env}/Library/bin/freecad* ${copy_dir}/bin
cp -a ${conda_env}/Library/bin/FreeCAD* ${copy_dir}/bin
cp -a ${conda_env}/Library/data ${copy_dir}/data
cp -a ${conda_env}/Library/Ext ${copy_dir}/Ext
cp -a ${conda_env}/Library/lib ${copy_dir}/lib
cp -a ${conda_env}/Library/Mod ${copy_dir}/Mod
rm -rf ${conda_env}/bin_tmp

# Apply Patches
mv ${copy_dir}/bin/Lib/ssl.py ssl-orig.py
cp ssl-patch.py ${copy_dir}/bin/Lib/ssl.py

echo '[Paths]' >> ${copy_dir}/bin/qt6.conf
echo 'Prefix = ../lib/qt6' >> ${copy_dir}/bin/qt6.conf

python_version=$(python -c 'import platform; print("py" + platform.python_version_tuple()[0] + platform.python_version_tuple()[1])')
version_name="FreeCAD_${BUILD_TAG}-Windows-$(uname -m)-${python_version}"

echo -e "################"
echo -e "version_name:  ${version_name}"
echo -e "################"

pixi list -e default > ${copy_dir}/packages.txt
sed -i '1s/.*/\nLIST OF PACKAGES:/' ${copy_dir}/packages.txt

mv ${copy_dir} ${version_name}

"${PROGRAMFILES}/7-Zip/7z.exe" a -t7z -mx9 -mmt=${NUMBER_OF_PROCESSORS} ${version_name}.7z ${version_name} -bb

# create hash
sha256sum ${version_name}.7z > ${version_name}.7z-SHA256.txt

if [ "${UPLOAD_RELEASE}" == "true" ]; then
    gh release upload --clobber ${BUILD_TAG} "${version_name}.7z" "${version_name}.7z-SHA256.txt"
fi
