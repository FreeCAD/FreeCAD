# Building FreeCAD on Mac OS 10.15.x -- Catalina #

## Obtain Dependencies ##

The [FreeCAD Wiki](https://wiki.freecadweb.org/Third_Party_Libraries) has
a list of required deps. They are reproduced here for developers.

| Library Name | Version    | Link to Obtain         |
|:------------:|:----------:|:----------------------:|
| Python       | >= 3.4     | http://www.python.org/     |
| Boost        | >= 1.33    | http://www.boost.org/      |
| OpenCASCADE  | >= 6.7     | http://www.opencascade.org |
| Qt           | >= 5.6     | https://www.qt.io/         |
| Shiboken2    | same as Qt | https://wiki.qt.io/Qt_for_Python/Shiboken |
| PySide2      | same as Qt | https://wiki.qt.io/Qt_for_Python/Shiboken |
| Coin3D       | >= 3.x     | https://github.com/coin3d/coin |
| *SoQt        | >= 1.2     |	https://github.com/coin3d/soqt |
| Quarter      | >= 1.0     | https://github.com/coin3d/quarter |
| Pivy         |	>= 0.6.5 	| https://github.com/coin3d/pivy/ |
| FreeType     |	>= XXX 	  | XXX |
| PyCXX        |	>= XXX    |	XXX |
| KDL          |	>= XXX 	  | XXX |
| Point Cloud Library |	>= XXX |	XXX |
| Salome SMESH |	>= XXX |	XXX |
| VTK |	>= 6.0 |	XXX |
| Ply |	>= 3.11 |	https://www.dabeaz.com/ply/ |
| Xerces-C++ |	>= 3.0 |	https://xerces.apache.org/xerces-c/ |
| Eigen3 |	>= 3.0 |	http://eigen.tuxfamily.org/index.php?title=Main_Page |
| Zipios++ |	>= 0.1.5 |	https://snapwebsites.org/project/zipios, https://github.com/Zipios/Zipios |
| Zlib |	>= 1.0 |	http://www.zlib.net/, https://github.com/madler/zlib |
| libarea |	>= 0.0.20140514-1 |	https://github.com/danielfalck/libarea |

`*`: Deprecated

The easiest way on Mac to get the Dependencies is homebrew.

### Building MED ###

Note that the tests don't compile on GCC 10 or GCC 7. Kind of concerning.
Also we end up w/ MED using HDF5 @ 1.10 and VTK using 1.12... does this work?
```
mkdir build && \
pushd build && \
cmake -G Ninja \
  -DMEDFILE_BUILD_TESTS=OFF \
  -DHDF5_ROOT_DIR=/usr/local/opt/hdf5@1.10 ../ \
  && ninja \
  && ninja install
```

## Run CMake ##

```
mkdir freecad-build
cd freecad-build
cmake ../freecad-source \
  -DBUILD_QT5=ON \
  -DPYTHON_EXECUTABLE=/usr/local/bin/python3 \
  -DHOMEBREW_PREFIX=/usr/local
make -j$(nproc --ignore=2)
```
