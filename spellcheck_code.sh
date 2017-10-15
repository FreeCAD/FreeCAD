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

# Categorized files/directories that we want codespell to skip 
TRANSLATION_FILES="*.po,*.ts"
THIRDRDPARTY_FILES="./src/3rdParty,./src/zipios++,./src/CXX"
MISC_FILES="./git,./src/Mod/Assembly/App/opendcm,./src/Mod/Cam/App"

# Output what files/directories we will be skipping
echo -e "$BLUE>> Skipping all translation files: $TRANSLATION_FILES $NC\n"
echo -e "$BLUE>> Skipping all 3 party directories: $THIRDPARTY_FILES $NC\n"
echo -e "$BLUE>> Skipping all misc. directories: $MISC_FILES $NC\n"

# consolidate skipped file/directories in to 1 variable
CODESPELL_SKIPS="$TRANSLATION_FILES,$THIRDPARTY_FILES,$MISC_FILES"

# Round up all changed content
CHANGED_FILES=($(git diff --name-only $TRAVIS_COMMIT_RANGE))

# Show File names that changed
echo -e "$BLUE>> Following files were changed in this pull request (commit range: $TRAVIS_COMMIT_RANGE):$NC"
echo -e "$GREEN\n$CHANGED_FILES\n$NC"

# cat all files that changed
TEXT_CONTENT=`cat $(echo "$CHANGED_FILES")`

# Optionally output all the text in the PRs that is subject to be checked. 
echo -e "$BLUE>> Text content that will be checked:$NC"
echo -e "$GREEN\n$TEXT_CONTENT\n$NC"

# Download FreeCAD's codespell whitelist in to ./fc-word-whitelist.txt
echo -e "$BLUE>> Downloading whitelist files:$NC"
curl -Os https://gist.githubusercontent.com/luzpaz/7ac1bf4412b9c1e5acde715ef9cb612c/raw/fc-word-whitelist.txt

# Run codespell and output results to stdout as well as in to a file
echo -e "$BLUE>> Run codespell:$NC"
codespell -d -q 3 -S "$CODESPELL_SKIPS" -I ./fc-word-whitelist.txt | tee codespell_results.txt

CODESPELL_RESULTS=`cat codespell_results.txt`

# Send results back to the PR comment
echo -e "$BLUE>> Sending results in a comment on the Github pull request #$TRAVIS_PULL_REQUEST:$NC"
curl -i -H "Authorization: token $GH_TOKEN" \
    -H "Content-Type: application/json" \
    -X POST -d "{\"body\":\"$CODESPELL_RESULTS\"}" \
    https://api.github.com/repos/FreeCAD/FreeCAD/issues/$TRAVIS_PULL_REQUEST/comments

# Clean up
# rm ./fc-word-whitelist.txt codespell_results.txt

exit 0;