RELEASE_VERSION="$1"

SOURCE_URL="https://github.com/FreeCAD/FreeCAD-Bundle/releases/download/$RELEASE_VERSION/freecad_source.tar.gz"
curl -O --location --retry 3 "https://github.com/FreeCAD/FreeCAD-Bundle/releases/download/$RELEASE_VERSION/freecad_version.txt"
curl -O --location --retry 3 $SOURCE_URL
curl -O --location --retry 3 "https://raw.githubusercontent.com/filippor/FreeCAD/refs/heads/copr/package/fedora/freecad.spec.rpkg"
COMMIT_DATE=`grep commit_date: freecad_version.txt | sed 's/commit_date: //g'`
REVISION_NUMBER=`grep rev_number:  freecad_version.txt | sed 's/^rev_number: //g'`
COMMIT_HASH=`grep commit_hash: freecad_version.txt | sed 's/^commit_hash: //g'`
# REMOTE_URL=`grep remote_url:  freecad_version.txt | sed 's/^remote_url: //g'`

PREP_MACRO="rm -rf %{git_name}\n   %setup -T -a 0 -q -c -D -n %{git_name}"



sed \
-e 's@{{{ git_name }}}@FreeCAD@g' \
-e 's@{{{ build_version }}}@'"$RELEASE_VERSION"'@g' \
-e 's@{{{ package_name }}}@'"$COPR_PACKAGE"'@g' \
-e 's@{{{ git_wcdate }}}@'"$COMMIT_DATE"'@g' \
-e 's@{{{ git_wcrev }}}@'"$REVISION_NUMBER"'@g' \
-e 's@{{{ git_commit_hash }}}@'"$COMMIT_HASH"'@g' \
-e 's@{{{ git_repo_pack_with_submodules }}}@'"$SOURCE_URL"'@g' \
-e 's@{{{ git_repo_setup_macro }}}@'"$PREP_MACRO"'@g' \
freecad.spec.rpkg > freecad.spec

rm freecad.spec.rpkg
