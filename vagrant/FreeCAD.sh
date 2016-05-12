#!/bin/bash
# Vagrant provisioning script to build up FreeCAD based on OCCT 7 and Salome 7.7.1 on Linux Debian/Jessie64
# (c) 2016 Jean-Marie Verdun / vejmarie (vejmarie@ruggedpod.qyshare.com)
# Released under GPL v2.0
# Provided without any warranty
# Warning: compilation time is long quite long

sudo apt-get install -y xfce4 xfce4-goodies
sudo apt-get install -y lightdm
sudo cat /etc/lightdm/lightdm.conf | sed 's/\#autologin-user=/autologin-user=vagrant/' > /tmp/lightdm.conf
sudo cp /tmp/lightdm.conf /etc/lightdm/lightdm.conf
sudo /etc/init.d/lightdm start
# Must add the autlogin script
# Must add a lightdm start / reboot ?
sudo apt-get install -y doxygen                          \
                               libboost1.55-dev                 \
                               libboost-filesystem1.55-dev      \
                               libboost-program-options1.55-dev \
                               libboost-python1.55-dev          \
                               libboost-regex1.55-dev           \
                               libboost-signals1.55-dev         \
                               libboost-system1.55-dev          \
                               libboost-thread1.55-dev          \
                               libcoin80                        \
                               libcoin80-dev                    \
                               libeigen3-dev                    \
                               libpyside-dev                    \
                               libqtcore4                       \
                               libshiboken-dev                  \
                               libxerces-c-dev                  \
                               libxmu-dev                       \
                               libxmu-headers                   \
                               libxmu6                          \
                               libxmuu-dev                      \
                               libxmuu1                         \
                               pyside-tools                     \
                               python-dev                       \
                               python-pyside                    \
                               python-matplotlib                \
                               qt4-dev-tools                    \
                               qt4-qmake                        \
                               shiboken                         \
                               swig
sudo apt-get install -y python-pivy
sudo apt-get install -y git
sudo apt-get install -y cmake
sudo apt-get install -y g++
sudo apt-get install -y libfreetype6-dev
sudo apt-get install -y tcl8.5-dev tk8.5-dev
sudo apt-get install -y libtogl-dev
sudo apt-get install -y libmed-dev
sudo apt-get install -y libmedc-dev
sudo apt-get install -y libhdf5-dev

# Building VTK

wget http://www.vtk.org/files/release/7.0/VTK-7.0.0.tar.gz
gunzip VTK-7.0.0.tar.gz
tar xf VTK-7.0.0.tar
rm VTK-7.0.0.tar
cd VTK-7.0.0
mkdir build
cd build
# cmake .. -DVTK_RENDERING_BACKEND=OpenGL
cmake .. -DVTK_Group_Rendering:BOOL=OFF -DVTK_Group_StandAlone:BOOL=ON -DVTK_RENDERING_BACKEND=None
make -j 2
sudo make install

# Building OCCT

cd ../..
wget "http://git.dev.opencascade.org/gitweb/?p=occt.git;a=snapshot;h=b00770133187b83761e651df50051b2fa3433858;sf=tgz"
mv "index.html?p=occt.git;a=snapshot;h=b00770133187b83761e651df50051b2fa3433858;sf=tgz" occt.tgz
gunzip occt.tgz
tar xf occt.tar
rm occt.tar
cd occt-b007701
grep -v vtkRenderingFreeTypeOpenGL src/TKIVtk/EXTERNLIB >& /tmp/EXTERNLIB
\cp /tmp/EXTERNLIB src/TKIVtk/EXTERNLIB
grep -v vtkRenderingFreeTypeOpenGL src/TKIVtkDraw/EXTERNLIB >& /tmp/EXTERNLIB
\cp /tmp/EXTERNLIB src/TKIVtkDraw/EXTERNLIB
mkdir build
cd build
# cmake .. -DUSE_VTK:BOOL=ON
cmake .. -DUSE_VTK:BOOL=OFF
sudo make -j 2
sudo make install

# Building Netgen

cd ../..
git clone https://github.com/vejmarie/Netgen
cd Netgen/netgen-5.3.1
./configure --with-tcl=/usr/lib/tcl8.5 --with-tk=/usr/lib/tk8.5  --enable-occ  --enable-shared --enable-nglib CXXFLAGS="-DNGLIB_EXPORTS -std=gnu++11"
make -j 2
sudo make install
cd ../..
sudo cp -rf Netgen/netgen-5.3.1 /usr/share/netgen

#building FreeCAD

git clone https://github.com/vejmarie/FreeCAD
cd FreeCAD
git checkout occt7
cd ..
mkdir build
cd build
cmake ../FreeCAD -DBUILD_FEM=1 -DBUILD_FEM_NETGEN=1 -DCMAKE_CXX_FLAGS="-DNETGEN_V5"
make -j 2

