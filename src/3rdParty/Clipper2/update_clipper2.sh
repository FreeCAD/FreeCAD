#!/bin/bash

set -e

# Manual update of the Clipper2 source

if [ $# -ne 1 ]; 
    then echo "Usage: update_clipper2.sh <git hash/tag>"
    exit 1
fi

if [ ! -f $PWD/pixi.toml ];
    then echo "This script should be run from the FreeCAD source root dir"
    exit 1
fi

readonly Clipper2_FC_SOURCE=$PWD/src/3rdParty/Clipper2/
find $Clipper2_FC_SOURCE ! -name 'update_clipper2.sh' -type f -exec rm -f {} +

rm -rf /tmp/Clipper2
git clone https://github.com/AngusJohnson/Clipper2 /tmp/Clipper2
pushd /tmp/Clipper2
git checkout $1

# Partial copy of the source
cp ./CPP/CMakeLists.txt $Clipper2_FC_SOURCE
cp ./LICENSE $Clipper2_FC_SOURCE
cp -r ./CPP/Clipper2Lib $Clipper2_FC_SOURCE
cp ./CPP/*in $Clipper2_FC_SOURCE

popd
