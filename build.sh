#! /bin/sh

# We almost need to specify the switches for the OpenCascade library to make
# configure running successfully. 
# CASROOT=${CASROOT:-/opt/OpenCASCADE6.2.0/ros}
#./configure CXXFLAGS="-fno-strict-aliasing" LDFLAGS="-Wl,-z,defs" --with-occ-include=$CASROOT/inc --with-occ-lib=$CASROOT/Linux/lib
./configure CXXFLAGS="-fno-strict-aliasing -Wno-write-strings" LDFLAGS="-Wl,-z,defs"

