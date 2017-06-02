#!/usr/bin/env bash

# copied from a conda-forge feedstock...

FEEDSTOCK_ROOT=$(cd "$(dirname "$0")"; pwd;)
RECIPE_ROOT=$FEEDSTOCK_ROOT
SOURCE_ROOT=$RECIPE_ROOT/../..

docker info

config=$(cat <<CONDARC

channels:
 - freecad
 - conda-forge
 - defaults

conda-build:
 root-dir: /feedstock_root/build_artefacts

show_channel_urls: true

CONDARC
)

cat << EOF | docker run -i \
                        -v "${SOURCE_ROOT}":/source \
                        -a stdin -a stdout -a stderr \
                        condaforge/linux-anvil \
                        bash || exit $?

export BINSTAR_TOKEN=${BINSTAR_TOKEN}
export PYTHONUNBUFFERED=1

echo "$config" > ~/.condarc
# A lock sometimes occurs with incomplete builds. The lock file is stored in build_artefacts.
conda clean --lock

conda install --yes --quiet conda-forge-build-setup
source run_conda_forge_build_setup

yum install -y libXt-devel libXmu-devel libXi-devel mesa-libGLU-devel rsync


# Embarking on 3 case(s).

    set -x
    export CONDA_PY=36
    set +x
    conda build /source/package/conda --quiet || exit 1
EOF
