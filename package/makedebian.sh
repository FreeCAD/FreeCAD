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
REV_FILE=./revision.m4
TMP_PATH=/tmp
MAJ=0
MIN=12
ALIAS="Vulcan"

# go to root directory
CUR_DIR=$PWD
verz=`dirname $(readlink -f ${0})`
cd $verz && cd ..

# let's import OLD_REV (if there)
if [ -f ./.last_revision ]; then
	. ./.last_revision
else
	OLD_REV=0
fi

if svn --xml info >/dev/null 2>&1; then
	REV=`svn --xml info | tr -d '\r\n' | sed -e 's/.*<commit.*revision="\([0-9]*\)".*<\/commit>.*/\1/'`
	LCD=`svn --xml info | tr -d '\r\n' | sed -e 's/.*<commit.*<date>\([0-9\-]*\)\T\([0-9\:]*\)\..*<\/date>.*<\/commit>.*/\1 \2/'`
	URL=`svn --xml info | tr -d '\r\n' | sed -e 's/.*<url>\(.*\)<\/url>.*/\1/'`
elif svn --version --quiet >/dev/null 2>&1; then
	REV=`svn info | grep "^Revision:" | cut -d" " -f2`
	LCD=`svn info | grep "^Last Changed Date:" | cut -d" " -f4,5`
	URL=`svn info | grep "^URL:" | cut -d" " -f2`
else
	REV=0
	LCD=""
	URL=""
fi

if [ "x$REV" != "x$OLD_REV" -o ! -r $REV_FILE ]; then
	echo "m4_define([FREECAD_MAJOR], $MAJ)" > $REV_FILE
	echo "m4_define([FREECAD_MINOR], $MIN)" >> $REV_FILE
	echo "m4_define([FREECAD_MICRO], $REV)" >> $REV_FILE

	#echo "#define FCVersionMajor  \"$MAJ\""   >  src/Build/Version.h
	#echo "#define FCVersionMinor  \"$MIN\""   >> src/Build/Version.h
	#echo "#define FCVersionName   \"$ALIAS\"" >> src/Build/Version.h
	#echo "#define FCRevision      \"$REV\""   >> src/Build/Version.h
	#echo "#define FCRepositoryURL \"$URL\""   >> src/Build/Version.h
	#echo "#define FCCurrentDateT  \"$LCD\"\n" >> src/Build/Version.h
	touch src/Build/Version.h.in
fi

echo "OLD_REV=$REV" > ./.last_revision

SRC_DIR=$PWD

# Prepare source tarball and unpack it in build directory
cd $CUR_DIR
make dist
rm -rf $TMP_PATH/freecad-$REV
mkdir $TMP_PATH/freecad-$REV
mv FreeCAD-$MAJ.$MIN.$REV.tar.gz $TMP_PATH/freecad-$REV/freecad_$MAJ.$MIN.$REV.orig.tar.gz
cd $TMP_PATH/freecad-$REV
tar -xzf freecad_$MAJ.$MIN.$REV.orig.tar.gz
mv FreeCAD-$MAJ.$MIN.$REV freecad-$MAJ.$MIN.$REV
cd freecad-$MAJ.$MIN.$REV
rm -rf src/CXX
rm -rf src/zipios++

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

