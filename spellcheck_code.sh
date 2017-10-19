#!/bin/bash
# Script that runs codespell (a typo finder) called from FreeCAD TravisCI to test incoming PRs
# Inspired from https://blog.eleven-labs.com/en/how-to-check-the-spelling-of-your-docs-from-travis-ci/
# Tips:
# 1. use "exit 0;" to not interrupt build process
# 2. Needs a repo_public token as an ENV variable that is hidden
# 3. requires most up to date codespell: pip install --user --upgrade git+https://github.com/lucasdemarchi/codespell.git


# For colorizing output. When using them, we need to terminate with $NC to reset back to default 
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;36m'
NC='\033[0m' # No Color

# Categorized FreeCAD files/dirs that we want codespell to skip 
TRANSLATION_FILES="*.po,*.ts"
THIRDPARTY_FILES="./src/3rdParty,./src/zipios++,./src/CXX"
MISC_FILES="./git,./src/Mod/Assembly/App/opendcm,./src/Mod/Cam/App"

# Output what FreeCAD files/dirs we will be skipping
echo -e "$BLUE>> Skipping all translation files: $TRANSLATION_FILES $NC\n"
echo -e "$BLUE>> Skipping all 3 party directories: $THIRDPARTY_FILES $NC\n"
echo -e "$BLUE>> Skipping all misc. directories: $MISC_FILES $NC\n"

# consolidate skipped file/dirs all in to 1 variable
CODESPELL_SKIPS="\"${TRANSLATION_FILES},${THIRDPARTY_FILES},${MISC_FILES}\""

# Round up all changed content
CHANGED_FILES=($(git diff --name-only $TRAVIS_COMMIT_RANGE))


# Show File names that changed
echo -e "$BLUE>> Following files were changed in this pull request (commit range: $TRAVIS_COMMIT_RANGE):$NC"
echo -e "$GREEN\n$CHANGED_FILES\n$NC"


# cat all files that changed
# TEXT_CONTENT=`cat $(echo "$CHANGED_FILES")`

# Optionally output all the text in the PRs that is subject to be checked. 
# echo -e "$BLUE>> Text content that will be checked:$NC"
# echo -e "$GREEN\n$TEXT_CONTENT\n$NC"

# Download FreeCAD's codespell whitelist in to ./fc-word-whitelist.txt
echo -e "$BLUE>> Downloading FreeCAD whitelist $NC"
curl -Os https://gist.githubusercontent.com/luzpaz/7ac1bf4412b9c1e5acde715ef9cb612c/raw/fc-word-whitelist.txt

# Run codespell and output results to stdout as well as in to a file
echo -e "$BLUE>> Run codespell -d -q 3 -S "$CODESPELL_SKIPS" -I ./fc-word-whitelist.txt:$NC"
# we need to sort the diffed file names in to a space delimited list so that we can pass it as parameters to codespell
# hence the bash sorcery below 
mapfile -t files < <(git diff --name-only $TRAVIS_COMMIT_RANGE); codespell -d -q 3 -S "$CODESPELL_SKIPS" -I ./fc-word-whitelist.txt  "${files[@]}"

# exit 0;