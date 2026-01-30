#!/usr/bin/bash 

mkdir build
cd build
cmake ../
make -j$(nproc --ignore=2)