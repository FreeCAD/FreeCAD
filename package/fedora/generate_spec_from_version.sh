#!/bin/sh
RELEASE_VERSION="$1"
PACKAGE="${2:-"freecad"}"

SOURCE_URL="https://github.com/FreeCAD/FreeCAD-Bundle/releases/download/$RELEASE_VERSION/freecad_source.tar.gz"
COMMIT_DATE=`grep commit_date: freecad_version.txt | sed 's/commit_date: //g'`
REVISION_NUMBER=`grep rev_number:  freecad_version.txt | sed 's/^rev_number: //g'`
COMMIT_HASH=`grep commit_hash: freecad_version.txt | sed 's/^commit_hash: //g'`
# REMOTE_URL=`grep remote_url:  freecad_version.txt | sed 's/^remote_url: //g'`

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

sed \
-e 's@{{{ build_version }}}@'"$RELEASE_VERSION"'@g' \
-e 's@{{{ package_name }}}@'"$PACKAGE"'@g' \
-e 's@{{{ git_wcdate }}}@'"$COMMIT_DATE"'@g' \
-e 's@{{{ git_wcrev }}}@'"$REVISION_NUMBER"'@g' \
-e 's@{{{ git_commit_hash }}}@'"$COMMIT_HASH"'@g' \
-e 's@{{{ git_repo_pack_with_submodules }}}@'"$SOURCE_URL"'@g' \
"$SCRIPT_DIR/freecad.spec.rpkg" > freecad.spec
