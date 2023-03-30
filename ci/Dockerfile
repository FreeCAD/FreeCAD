FROM ubuntu:20.04
MAINTAINER Przemo Firszt
# This is the docker image definition used to build FreeCAD. It's currently accessible on:
# https://hub.docker.com/repository/docker/freecadci/runner
# on under name freecadci/runner when using docker

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y
RUN apt-get update -y && apt-get install -y gnupg2
RUN echo "deb http://ppa.launchpad.net/freecad-maintainers/freecad-daily/ubuntu focal main" >> /etc/apt/sources.list.d/freecad-daily.list
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 83193AA3B52FF6FCF10A1BBF005EAE8119BB5BCA
RUN apt-get update -y

# those 3 are for debugging purposes only. Not required to build FreeCAD
RUN apt-get install -y 			\
	vim				\
	nano				\
	bash

# Main set of FreeCAD dependencies. To be verified.
RUN apt-get install -y				\
	ccache					\
	cmake					\
	debhelper				\
	dh-exec					\
	dh-python				\
	doxygen					\
	git					\
	graphviz				\
	libboost-date-time-dev			\
	libboost-dev				\
	libboost-filesystem-dev			\
	libboost-filesystem1.71-dev		\
	libboost-graph-dev			\
	libboost-iostreams-dev			\
	libboost-program-options-dev		\
	libboost-program-options1.71-dev	\
	libboost-python1.71-dev			\
	libboost-regex-dev			\
	libboost-regex1.71-dev			\
	libboost-serialization-dev		\
	libboost-system1.71-dev			\
	libboost-thread-dev			\
	libboost-thread1.71-dev			\
	libboost1.71-dev			\
	libcoin-dev				\
	libdouble-conversion-dev		\
	libeigen3-dev				\
	libglew-dev				\
	libgts-bin				\
	libgts-dev				\
	libkdtree++-dev				\
	liblz4-dev				\
	libmedc-dev				\
	libmetis-dev				\
	libnglib-dev				\
	libocct-data-exchange-dev		\
	libocct-ocaf-dev			\
	libocct-visualization-dev		\
	libopencv-dev				\
	libproj-dev				\
	libpyside2-dev				\
	libqt5opengl5				\
	libqt5opengl5-dev			\
	libqt5svg5-dev				\
	libqt5webkit5				\
	libqt5webkit5-dev			\
	libqt5x11extras5-dev			\
	libqt5xmlpatterns5-dev			\
	libshiboken2-dev			\
	libspnav-dev				\
	libvtk7-dev				\
	libvtk7.1p				\
	libvtk7.1p-qt				\
	libx11-dev				\
	libxerces-c-dev				\
	libzipios++-dev				\
	lsb-release				\
	nastran					\
	netgen					\
	netgen-headers				\
	occt-draw				\
	pybind11-dev				\
	pyqt5-dev-tools				\
	pyside2-tools				\
	python3-dev				\
	python3-matplotlib			\
	python3-pivy				\
	python3-ply				\
	python3-pyside2.*			\
	python3-pyside2.qtcore			\
	python3-pyside2.qtgui			\
	python3-pyside2.qtsvg			\
	python3-pyside2.qtuitools		\
	python3-pyside2.qtwidgets		\
	python3-pyside2.qtxml			\
	python3-requests			\
	python3-yaml				\
	qt5-default				\
	qt5-qmake				\
	qtbase5-dev				\
	qttools5-dev				\
	qtwebengine5-dev			\
	swig

RUN apt-get update -y --fix-missing

# Clean
RUN apt-get clean \
    && rm /var/lib/apt/lists/* \
          /usr/share/doc/* \
          /usr/share/locale/* \
          /usr/share/man/* \
          /usr/share/info/* -fR


