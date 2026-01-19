source .bundle-vars

# create hash
sha256sum ${version_name}.AppImage > ${version_name}.AppImage-SHA256.txt

gh release upload --clobber ${BUILD_TAG} "${version_name}.AppImage" "${version_name}.AppImage.zsync" "${version_name}.AppImage-SHA256.txt"
if [ "${GH_UPDATE_TAG}" == "weeklies" ]; then
    generic_name="FreeCAD_weekly-Linux-$(uname -m)"
    mv "${version_name}.AppImage" "${generic_name}.AppImage"
    mv "${version_name}.AppImage.zsync" "${generic_name}.AppImage.zsync"
    mv "${version_name}.AppImage-SHA256.txt" "${generic_name}.AppImage-SHA256.txt"
    gh release create weeklies --prerelease | true
    gh release upload --clobber weeklies "${generic_name}.AppImage" "${generic_name}.AppImage.zsync" "${generic_name}.AppImage-SHA256.txt"
fi
