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
FCENV=freecad_dev
# The cmake build directory
HOST=$(uname)

###########################################################################
# Env Checks
###########################################################################
which xcrun
if [ $? != 0 ]; then
  echo "xcrun not found... install XCode command line tools..."
  echo "using: xcode-select --install"
  exit 1
fi

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

  if [[ ${HOST} =~ "Linux" ]]; then
    echo "Linux"
    conda env create -f environment-linux.yml
  elif [[ ${HOST} =~ "Darwin" ]]; then
    echo "OS X"
    conda env create -f environment-osx.yml
  else
    echo "Unknown Host: ${HOST}"
    exit 1
  fi

  conda activate ${FCENV}
  if [ $? != 0 ]; then
    echo "Failed to create conda environment and activate it."
    exit 1
  fi
fi

if [ -z "${CONDA_PREFIX}" ]; then
  echo "Failed to find CONDA_PREFIX variable."
  exit 1
fi

PREFIX="${CONDA_PREFIX}" ./conda/build.sh 
