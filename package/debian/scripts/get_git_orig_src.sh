#!/bin/bash

# The script creates a tar.xz tarball from git-repository of freecad-project
# ./get_orig_src.sh commitID   -   creates a tarball of specified commit
# ./get_orig_src.sh   - creates a tarball of the latest version
# Packages, that needs to be installed to use the script:
# atool, git-core

set -e

git clone git://free-cad.git.sourceforge.net/gitroot/free-cad/free-cad git_temp_packaging

cd git_temp_packaging

if [ $1 ]
then
    echo 'Checking out the revision ' $1
    git checkout -b newvers $1
else
    echo 'Using the latest revision'
fi 

GIT_CMT_COUNT=$(git rev-list HEAD | wc -l)

DEB_VER=0.13.$GIT_CMT_COUNT-dfsg
FOLDER_NAME=freecad-$DEB_VER
TARBALL_NAME=freecad_$DEB_VER.orig.tar.xz

echo $DEB_VER
echo $FOLDER_NAME
echo $TARBALL_NAME

python src/Tools/SubWCRev.py

cd ..

rm -fr $FOLDER_NAME

mv git_temp_packaging $FOLDER_NAME
rm -rf $FOLDER_NAME/.git 
rm -rf $FOLDER_NAME/src/3rdParty/CxImage
rm -rf $FOLDER_NAME/src/3rdParty/Pivy
rm -rf $FOLDER_NAME/src/3rdParty/Pivy-0.5

tar Jcvf $TARBALL_NAME $FOLDER_NAME

rm -fr $FOLDER_NAME
