source .bundle-vars

# create hash
sha256sum ${version_name}.7z > ${version_name}.7z-SHA256.txt

gh release upload --clobber ${BUILD_TAG} "${version_name}.7z" "${version_name}.7z-SHA256.txt"
if [ -n "${INSTALLER_PATH}" ]; then
    sha256sum ${INSTALLER_PATH} > ${INSTALLER_PATH}-SHA256.txt
    gh release upload --clobber ${BUILD_TAG} "${INSTALLER_PATH}" "${INSTALLER_PATH}-SHA256.txt"
fi
