##!/usr/bin/env bash
#
# MacOS Build Script for FreeCAD using Conda.
# This is intended for developer use.
#
# Copyright (c) 2020 by Jeffrey Zampieron. All rights reserved.
#
# License: LGPLv2.1
#
# References:
# - Conda: https://conda.io/projects/conda/en/latest/user-guide/install/index.html
# - Conda Build: https://docs.conda.io/projects/conda-build/en/latest/install-conda-build.html

###########################################################################
# Script wide setup.
###########################################################################
# The Conda environment name
FCENV=freecad_build
# The cmake build directory
HOST=$(uname)

###########################################################################
# Conda Setup
###########################################################################
which conda
if [ $? != 0 ]; then
  echo "Failed to find conda executable. Please install."
  exit 1
fi

conda activate ${FCENV}
if [ $? != 0 ]; then
  echo "Failed to activate conda env: ${FCENV} ... creating"

  conda create \
    --name ${FCENV} \
    --channel conda-forge \
    --channel defaults \
    conda-build \
    conda-forge-ci-setup=3 \
    pip

  conda activate ${FCENV}
  if [ $? != 0 ]; then
    echo "Failed to create conda environment and activate it."
    exit 1
  fi
fi

conda build ./conda --channel conda-forge 