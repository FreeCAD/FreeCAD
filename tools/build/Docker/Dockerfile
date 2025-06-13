FROM ubuntu:plucky

ENV FREECAD_VERSION="FreeCAD-1-0"

LABEL MAINTAINER="xxxx@example.com"
LABEL DESCRIPTION="A docker image to build FreeCAD locally"
LABEL VERSION="$FREECAD_VERSION"

SHELL ["/bin/bash", "-c"]

WORKDIR /tmp

ENV DEBIAN_FRONTEND=noninteractive
ENV LOCALE="C.UTF-8"

# Build tools, and misc supporting tools
RUN apt update && \
    apt install -y build-essential cmake libtool lsb-release git \
    # Python3
    python3 swig \
    # Boost libraries
    libboost-dev \
    libboost-date-time-dev \
    libboost-filesystem-dev \
    libboost-graph-dev \
    libboost-iostreams-dev \
    libboost-program-options-dev \
    libboost-python-dev \
    libboost-regex-dev \
    libboost-serialization-dev \
    libboost-thread-dev \
    # Coin libraries
    libcoin-dev libcoin-doc libcoin-runtime \
    # Misc libraries
    libeigen3-dev \
    libgts-bin \
    libgts-dev \
    libkdtree++-dev \
    libmedc-dev \
    libopencv-dev \
    libproj-dev \
    libvtk9-dev \
    libx11-dev \
    libxerces-c-dev \
    libyaml-cpp-dev \
    libzipios++-dev \
    # Python 3 and Qt6
    libpyside6-py3-6.8 libqt6opengl6-dev \
    libshiboken2-dev \
    python3-pyside6.qtcore python3-pyside6.qtgui python3-pyside6.qtnetwork \
    python3-pyside6.qtsvg python3-pyside6.qtwebchannel \
    python3-pyside6.qtwebenginequick python3-pyside6.qtwebenginecore \
    python3-pyside6.qtwebenginewidgets python3-pyside6.qtwidgets \
    python3-pyside6.qtuitools libpyside6-dev \
    libqt6opengl6-dev libqt6svg6-dev libshiboken2-dev \
    qt6-base-dev qt6-tools-dev \
    python3-dev python3-matplotlib python3-packaging python3-pivy \
    python3-pybind11 python3-ply \
    qt6-webengine-dev qt6-l10n-tools qt6-tools-dev-tools \
    # OpenCascade
    libocct*-dev occt-draw \
    # Optional packges
    checkinstall doxygen graphviz libsimage-dev libspnav-dev \
    # To fix compilation problems
    libhdf5-openmpi-dev python3-pip

# Known python dependencies
# RUN python3 -m pip install --break-system-packages ifcopenshell

# DEBUG C++ with gdb
RUN apt install -y gdb
# libcanberra-gtk-module libcanberra-gtk3-module
# greenlet dependency can not be compiled
# RUN python3 -m pip install --break-system-packages gdbgui
ENV FREECAD_GDB_PORT=5000
EXPOSE 5000

# DEBUG Python with winpdb
RUN apt install -y wxpython-tools
RUN python3 -m pip install --break-system-packages winpdb-reborn
ENV FREECAD_WINPDB_PORT=51000
ENV FREECAD_WINPDB_PWD=1234
EXPOSE 51000

# These environment variable are set here to be used
#   by the different container's scripts
ENV FREECAD_CONFIG_DIR="/root/.local/FreeCAD"
ENV FREECAD_BUILD_DIR="/mnt/build"
ENV FREECAD_SOURCE_DIR="/mnt/source"

# Add the build & debug scripts
ADD build_FC.sh /root/build_FC.sh

WORKDIR /root
