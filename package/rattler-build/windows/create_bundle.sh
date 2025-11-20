#!/bin/bash

set -e
set -x

conda_env="$(pwd)/../.pixi/envs/default/"

copy_dir="FreeCAD_Windows"
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
mkdir -p ${copy_dir}/doc
cp -a ${conda_env}/Library/doc/{ThirdPartyLibraries.html,LICENSE.html} ${copy_dir}/doc

# delete unnecessary stuff
find ${copy_dir} -name \*.a -delete
find ${copy_dir} -name \*.lib -delete
find ${copy_dir} -name \*arm\*.exe -delete # arm binaries that fail to extract unless using latest 7zip

# Apply Patches
mv ${copy_dir}/bin/Lib/ssl.py .ssl-orig.py
cp ssl-patch.py ${copy_dir}/bin/Lib/ssl.py

echo '[Paths]' >> ${copy_dir}/bin/qt6.conf
echo 'Prefix = ../lib/qt6' >> ${copy_dir}/bin/qt6.conf

# convenient shortcuts to run the binaries
if [ -x /c/ProgramData/chocolatey/tools/shimgen.exe ]; then
    pushd ${copy_dir}
    /c/ProgramData/chocolatey/tools/shimgen.exe -p bin/freecadcmd.exe -i "$(pwd)/../../../WindowsInstaller/icons/FreeCAD.ico" -o "$(pwd)/FreeCADCmd.exe"
    /c/ProgramData/chocolatey/tools/shimgen.exe --gui -p bin/freecad.exe -i "$(pwd)/../../../WindowsInstaller/icons/FreeCAD.ico" -o "$(pwd)/FreeCAD.exe"
    popd
fi

python_version=$("${copy_dir}"/bin/python.exe -c 'import platform; print("py" + platform.python_version_tuple()[0] + platform.python_version_tuple()[1])')
version_name="FreeCAD_${BUILD_TAG}-Windows-$(uname -m)-${python_version}"

echo -e "################"
echo -e "version_name:  ${version_name}"
echo -e "################"

pixi list -e default > ${copy_dir}/packages.txt
sed -i '1s/.*/\nLIST OF PACKAGES:/' ${copy_dir}/packages.txt

mv ${copy_dir} ${version_name}

7z a -t7z -mx9 -mmt=${NUMBER_OF_PROCESSORS} ${version_name}.7z ${version_name} -bb
# create hash
sha256sum ${version_name}.7z > ${version_name}.7z-SHA256.txt

if [ "${MAKE_INSTALLER}" == "true" ]; then
    FILES_FREECAD="$(cygpath -w $(pwd))\\${version_name}"
    nsis_cpdir=$(pwd)/.nsis_tmp
    cp -r "${CONDA_PREFIX}/NSIS" "${nsis_cpdir}"
    # curl -L -o ".nsis-log.zip" http://prdownloads.sourceforge.net/nsis/nsis-3.11-log.zip # we use the log variant of the package already
    # curl -L -o ".nsis-strlen_8192.zip" "http://prdownloads.sourceforge.net/nsis/nsis-3.11-strlen_8192.zip"
    curl -L -o ".NsProcess.7z" "https://nsis.sourceforge.io/mediawiki/images/1/18/NsProcess.zip"
    if [ ! $(echo fc19fc66a5219a233570fafd5daeb0c9b85387b379f6df5ac8898159a57c5944 .NsProcess.7z | sha256sum --check --status) ]; then
        7z x .NsProcess.7z -o"${nsis_cpdir}" -y
        mv "${nsis_cpdir}"/Plugin/nsProcess.dll "${nsis_cpdir}"/Plugins/x86-ansi/nsProcess.dll
        mv "${nsis_cpdir}"/Plugin/nsProcessW.dll "${nsis_cpdir}"/Plugins/x86-unicode/nsProcess.dll
        "${nsis_cpdir}"/makensis.exe -V4 \
            -D"ExeFile=${version_name}-installer.exe" \
            -D"FILES_FREECAD=${FILES_FREECAD}" \
            -X'SetCompressor /FINAL lzma' \
            ../../WindowsInstaller/FreeCAD-installer.nsi
        mv ../../WindowsInstaller/${version_name}-installer.exe .
        sha256sum ${version_name}-installer.exe > ${version_name}-installer.exe-SHA256.txt
    else
        echo "Error: Failed to get NsProcess plugin. Aborting installer creation..."
    fi
    rm -rf "${nsis_cpdir}"
fi

if [ "${UPLOAD_RELEASE}" == "true" ]; then
    gh release upload --clobber ${BUILD_TAG} "${version_name}.7z" "${version_name}.7z-SHA256.txt"
    if [ "${MAKE_INSTALLER}" == "true" ]; then
        gh release upload --clobber ${BUILD_TAG} "${version_name}-installer.exe" "${version_name}-installer.exe-SHA256.txt"
    fi
fi
