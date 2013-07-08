#!/bin/sh
# (c) 2009 Werner Mayer  GPL

# This script creates a new source tarball and extracts it in the build
# directory. The source directory and tarball are renamed to  Debian-
# conform names.
# Then the debian directory is created from a diff file and prepared for
# the new build. For this the following steps are executed:
#
# 1. cd freecad-X.Y.xyz
# 2. patch -p1 < .../debian/diff/freecad_hardy.diff
# 3. dch -v X.Y.xyz-1hardy1 "New release for Ubuntu 8.04 (Hardy Heron)"
# 4. debuild
#
# Example how to work with sed can be found here: 
# http://www.grymoire.com/Unix/Sed.html

# global settings
TMP_PATH=/tmp
MAJ=0
MIN=13

# go to root directory
CUR_DIR=$PWD
verz=`dirname $(readlink -f ${0})`
cd $verz && cd ..

# http://blog.marcingil.com/2011/11/creating-build-numbers-using-git-commits/
if git log -1 >/dev/null 2>&1; then
	REV=`git rev-list HEAD | wc -l | sed -e 's/ *//g' | xargs -n1 printf %04d`
else
	REV=0
fi

SRC_DIR=$PWD

# Prepare source tarball and unpack it in build directory
cd $CUR_DIR
make dist-git
cd $verz && cd ..
rm -rf $TMP_PATH/freecad-$REV
mkdir $TMP_PATH/freecad-$REV
mv freecad-$MAJ.$MIN.$REV.tar.gz $TMP_PATH/freecad-$REV/freecad_$MAJ.$MIN.$REV.orig.tar.gz
cd $TMP_PATH/freecad-$REV
tar -xzf freecad_$MAJ.$MIN.$REV.orig.tar.gz
cd freecad-$MAJ.$MIN.$REV
#rm -rf src/CXX
#rm -rf src/zipios++

# Prepare debian folder and set the revision number in debian/changelog
# for package versioning
LSB_RELS=`lsb_release -r | cut -f2`
LSB_DESC=`lsb_release -d | cut -f2`
LSB_CODE=`lsb_release -c | cut -f2`
patch -p1 < $SRC_DIR/package/debian/diff/freecad_$LSB_CODE.diff
DEBEMAIL="wmayer@users.sourceforge.net"
export DEBEMAIL
dch -v $MAJ.$MIN.$REV-1"$LSB_CODE"1 "New release for $LSB_DESC ($LSB_CODE)"
debuild
cd ..
gunzip freecad_$MAJ.$MIN.$REV-1"$LSB_CODE"1.diff.gz
mv freecad_$MAJ.$MIN.$REV-1"$LSB_CODE"1.diff $SRC_DIR/package/debian/freecad_$LSB_CODE.diff


exit 0

