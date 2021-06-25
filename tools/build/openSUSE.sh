#!/bin/sh
zypper --non-interactive install --no-recommends -t pattern devel_C_C++ \
    devel_qt5 basesystem devel_basis minimal_base

zypper --non-interactive install git gcc cmake gcc-c++ python3-devel \
    libboost_headers1_66_0-devel libboost_filesystem1_66_0-devel \
    libboost_program_options1_66_0-devel libboost_regex1_66_0-devel \
    libboost_signals1_66_0-devel libboost_system1_66_0-devel \
    libboost_thread1_66_0-devel libboost_mpi1_66_0-devel libxerces-c-devel \
    zlib-devel occt occt-devel vtk-devel libmed-devel eigen3-devel swig \
    Coin-devel libqt5-qtbase-devel libqt5-qtsvg-devel libqt5-qttools-devel \
    libqt5-qtxmlpatterns-devel python3-pyside2-devel python3-matplotlib \
    python3-pivy python3-GitPython libboost_python-py3-1_66_0-devel doxygen \
    python3-vtk-openmpi2 openmpi2-devel boost-gnu-openmpi2-hpc-devel\
    vtk-openmpi2-devel openmpi2-gnu-hpc-devel boost_1_71_0-gnu-openmpi2-hpc-python3
    python3-mpi4py glew-devel \
    libspnav-devel libshiboken-devel python3-pyside-shiboken graphviz
