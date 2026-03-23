# deb from pivy-source

- download latest release
 https://github.com/looooo/pivy/releases
- `py2dsc <pivy_archive>`
- add additional dependencies in pivy_<version>.debian.tar.xz:
debian/control
```
Build-Depends: debhelper (>= 9), python-support, python-all-dev,
libsoqt4-dev, libcoin80-dev, libsimage-dev, swig
.
.
.
Depends: ${shlibs:Depends}, ${misc:Depends}, ${python:Depends}, python-qt4-gl
```
- `cd deb_dist/pivy/pivy_<version>`
- `dpkg-buildpackage -rfakeroot -uc -us`


# build with 14.04 trusty

- sudo docker run -i -t -v ~/projects/:/projects  --name ubuntu_1404 ubuntu:trusty
- sudo apt-get update
- sudo apt-get install libcoin80-dev libsimage-dev libsoqt-dev swig debhelper python-support python3-all-dev python-stdeb# python-all-dev
- build pivy (previous step)


# update version
http://manpages.ubuntu.com/manpages/xenial/man1/uupdate.1.html