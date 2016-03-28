FROM ubuntu:14.04
RUN apt-get update
RUN apt-get install -yqq software-properties-common
RUN add-apt-repository ppa:freecad-maintainers/freecad-daily
RUN apt-get update
RUN apt-get install -yqq build-essential python python2.7-dev subversion cmake libtool autotools-dev automake bison flex gfortran git
RUN apt-get install -yqq libCoin80-dev libCoin80-doc libsoqt4-dev libqt4-dev qt4-dev-tools libsoqt4-dev python-qt4 libqtwebkit-dev
RUN apt-get install -yqq liboce-foundation-dev liboce-modeling-dev liboce-ocaf-dev liboce-visualization-dev oce-draw
RUN apt-get install -yqq libode-dev libeigen2-dev libeigen3-dev libsimage-dev libxerces-c2-dev
RUN apt-get install -yqq libpyside-dev pyside-tools libshiboken-dev doxygen python-pivy
RUN apt-get install -yqq libboost1.55-all-dev
RUN apt-get install -yqq libmedc-dev libvtk6-dev libproj-dev
RUN apt-get install -yqq libxerces-c-dev
RUN ln -s /usr/lib/x86_64-linux-gnu/libxerces-c.so /usr/lib/libxerces-c.so
