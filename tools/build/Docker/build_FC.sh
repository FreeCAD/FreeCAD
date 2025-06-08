#!/bin/bash
#
# This script will launch the build of FreeCAD.
#

p_build_type="Debug"

#------------------------------------------------------------------------------
# FUNCTION: usage
#------------------------------------------------------------------------------
usage()
{
    cat >&2 <<EOF

Usage: $(basename $0)

Build FreeCAD either in Debug (default) or Release mode

General options:
    -d|--debug: Launch a debug build. This is the default.
    -r|--release: Launch a release build.
    -h|--help: This message

SYNOPSIS:

Build FreeCAD in Debug mode
?> $0
?> $0 --debug

Build FreeCAD in Rebug mode
?> $0 --release

EOF
}


#==============================================================================
#
# MAIN PROGRAM SECTION.
#
#==============================================================================
options=$(getopt --alternative --name $(basename $0) --options "hdr" --longoptions help,debug,release -- $0 "$@")
if [ $? -ne 0 ]; then
    usage
    exit 1
fi
eval set -- "$options"
while true; do
    case "$1" in
    -d|--debug)   p_build_type="Debug" ;;
    -r|--release) p_build_type="Release" ;;
    -h|--help)    usage; exit 0 ;;
    --) shift; break ;;
    esac
    shift
done

git config --global --add safe.directory ${FREECAD_SOURCE_DIR}

set -e

# NOTE: The PYTHON_LIBRARY is dependant on the base image used
#   in the docker file.
cmake \
    -D PYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.13.so.1.0 \
    -D PYTHON_INCLUDE_DIR=/usr/include/python3.13/ \
    -D PYTHON_EXECUTABLE=/usr/bin/python3 \
    -D FREECAD_USE_OCC_VARIANT="Official Version" \
    -D BUILD_QT6=ON \
    -D BUILD_FEM=ON \
    -D BUILD_SANDBOX=OFF \
    -D BUILD_DESIGNER_PLUGIN=ON \
    -D FREECAD_QT_MAJOR_VERSION=6 \
    -D FREECAD_QT_VERSION=6 \
    -D ENABLE_DEVELOPER_TESTS=Off \
    -D Boost_USE_DEBUG_RUNTIME=FALSE \
    -D FREECAD_USE_PCL=Off \
    -D CMAKE_BUILD_TYPE=$p_build_type \
    -S ${FREECAD_SOURCE_DIR} \
    -B ${FREECAD_BUILD_DIR}

pushd ${FREECAD_BUILD_DIR}
make -j $(nproc --ignore=1)
