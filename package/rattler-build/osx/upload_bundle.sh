source .bundle-vars

# create hash
sha256sum ${version_name}.dmg > ${version_name}.dmg-SHA256.txt

gh release upload --clobber ${BUILD_TAG} "${version_name}.dmg" "${version_name}.dmg-SHA256.txt"
