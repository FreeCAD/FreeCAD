#!/usr/bin/env bash

FEEDSTOCK_ROOT=$(cd "$(dirname "$0")"; pwd;)
SOURCE_DIR=${FEEDSTOCK_ROOT}/../..
echo ${SOURCE_DIR}

docker info

config=$(cat <<CONDARC

channels:
 - freecad
 - conda-forge
 - defaults

show_channel_urls: true

CONDARC
)

HOST_USER_ID=$(id -u)


if hash docker-machine 2> /dev/null && docker-machine active > /dev/null; then
    HOST_USER_ID=$(docker-machine ssh $(docker-machine active) id -u)
fi

rm -f "$FEEDSTOCK_ROOT/build_artefacts/conda-forge-build-done"

cat << EOF | docker run -i \
                        -v "${SOURCE_DIR}":/source \
                        -e HOST_USER_ID="${HOST_USER_ID}" \
                        -a stdin -a stdout -a stderr \
                        condaforge/linux-anvil \
                        bash || exit 1

set -x
export PYTHONUNBUFFERED=1

echo "$config" > ~/.condarc

conda clean --lock

conda install --yes --quiet conda-build

/usr/bin/sudo -n yum install -y libXt-devel libXmu-devel libXi-devel mesa-libGLU-devel
conda build /source/package/conda
EOF