#!/bin/sh
dnf install --assumeyes gcc git cmake doxygen swig gettext \
    desktop-file-utils libXmu-devel freeimage-devel mesa-libGLU-devel \
    python3 python3-devel python3-pyside2-devel pyside2-tools \
    boost-devel tbb-devel eigen3-devel qt-devel qt-webkit-devel \
    qt5-qtxmlpatterns qt5-qttools-static qt5-qtxmlpatterns-devel \
    qt5-qtsvg-devel ode-devel xerces-c xerces-c-devel opencv-devel smesh-devel \
    Coin3 Coin3-devel SoQt-devel freetype freetype-devel vtk med med-devel \
    libspnav-devel python3-pivy opencascade-devel pcl-devel openmpi-devel
