#!/bin/sh
# SPDX-FileNotice: Part of the FreeCAD project.

apt-get install --no-install-recommends --yes build-essential cmake doxygen git \
libtool lsb-release \
python3 swig \
libboost-dev libboost-date-time-dev libboost-filesystem-dev libboost-graph-dev libboost-iostreams-dev \
libboost-program-options-dev libboost-python-dev libboost-regex-dev libboost-serialization-dev libboost-thread-dev \
libcoin-dev \
libeigen3-dev libgts-bin libgts-dev libkdtree++-dev libmedc-dev libopencv-dev libproj-dev libvtk9-dev \
libx11-dev libxerces-c-dev libyaml-cpp-dev libzipios++-dev libpcl-dev python3-yaml \
\
pybind11-dev qt6-base-dev qt6-svg-dev qt6-tools-dev qt6-webengine-dev libpyside6-dev libshiboken6-dev pyside6-tools \
pyqt6-dev-tools python3-dev python3-matplotlib python3-packaging python3-pivy python3-ply python3-pyside6.qtcore \
python3-pyside6.qtgui python3-pyside6.qtnetwork python3-pyside6.qtsvg python3-pyside6.qtwebchannel \
python3-pyside6.qtwebenginecore python3-pyside6.qtwebenginequick python3-pyside6.qtwebenginewidgets python3-pyside6.qtwidgets \
\
libocct-data-exchange-dev libocct-draw-dev libocct-foundation-dev libocct-modeling-algorithms-dev libocct-modeling-data-dev \
libocct-ocaf-dev libocct-visualization-dev occt-draw \
\
libsimage-dev doxygen libcoin-doc libspnav-dev checkinstall
