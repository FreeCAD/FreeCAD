libPRC
======

PRC file format library and utilities.

This project contains the following libraries and utilities:
 - asymptote: A library for creating PRC files. It is the PRC-generation code from the
 Asymptote project, but builds as a standalone library.
 - libPRC: An alternate library for generating PRC files, currently under development.
 - osgdb_prc: A plugin for OpenSceneGraph that creates a PRC file from a scene graph.
 - prcconvert: A simple OSG utility to create PRC files. Like "osgconv" but doesn't run the
osgUtil::Optimizer.
 - prctopdf: An executable that produces a PDF file with an embedded PRC file.


Dependencies
============

 - zlib (set ZLIB_ROOT in CMake, if necessary)
 - OpenSceneGraph (set OSG_DIR in CMake, if necessary)
 - libHaru (set LIBHARU_ROOT in CMake, if necessary)
 - libPNG (aet PNG_PNG_INCLUDE_DIR and PNG_LIBRARY in CMake, if necessary)
 - Doxygen

OpenSceneGraph is required to build the osgdb_prc export plugin and
the prcconvert executable file conversion tool.

libHaru, libPNG, and zlib are required to build the prctopdf
executable that embeds PRC files in PDF documents.

zlib is required to build the asymptote library.

Doxygen is required to build documentation from the source.
Set LIBPRC_DOCUMENTATION to ON to do this.


Using CMake
===========

You could run cmake-gui (or ccmake) and fill in the necessary
variables so that CMake can find the dependencies.

Optionally you can run cmake from the command line with something
like this:

```
> cmake <path_to_source> \
      -DOSG_DIR=<path_to_osg> \
      -DZLIB_ROOT=<path_to_zlib> \
      -DLIBHARU_ROOT=<path_to_libharu> \
      -DPNG_PNG_INCLUDE_DIR=<png_include_dir> \
      -DPNG_LIBRARY=<png_library>
```
