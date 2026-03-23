# Coin. Open Inventor API implementation

[![Packaging status](https://repology.org/badge/tiny-repos/coin3d.svg)](https://repology.org/project/coin3d/versions)

## Introduction

Coin is an OpenGL-based, 3D graphics library that has its roots in the
Open Inventor 2.1 API, which Coin still is compatible with.

<p align="center">
  <img src="docs/Coin3DElements.svg" width="480">
</p>

If you are not familiar with Open Inventor, it is a scene graph based,
retained mode, rendering and model manipulation, C++ class library,
originally designed by SGI.  It quickly became the de facto standard
graphics library for 3D visualization and visual simulation software
in the scientific and engineering community after its release.  It
also became the basis for the VRML1 file format standard.  Several
books exists on the subject of Open Inventor, most notably The
Inventor Mentor, and The Inventor Toolmaker, both highly recommended
for those who want to learn how to use Open Inventor.

Coin is based on the API of this library, but was developed from
scratch independently before SGI Open Inventor became open source.  It
does not share any code with SGI Open Inventor, other than through
random coincidences guided by the Open Inventor API design.  Coin
reached the goal of Open Inventor 2.1 compatibility in the autumn of
the year 2000, and has since then been extended with a huge set of
additional features, ranging from 3D sound support to GLSL shader
support, additional file formats like VRML97, and a large number of
internal changes for keeping up with the newer, more optimized OpenGL
rendering techniques that were not available in the earlier days.

Another term you might find when reading about Coin is "Coin3D", which
is the term we use on the larger group of libraries that all fall
under the same license as Coin.  Coin is the core of Coin3D.


## Source Code and Binary Compatibility

Coin is one of three Open Inventor implementations.  All
implementations are source code compatible across the Open Inventor
2.1 API.  Source code compatibility means you can write software for a
given API and at build time choose to use any implementation of that
API.  Which one should not matter - the software should function the
same way with all of them.

The Open Inventor implementations are not Application Binary Interface
compatible with each other.  This means it will not work to first
build a piece of software against one implementation of an API and
then later replace the run-time library with another implementation of
the API.  Some libraries have ABI compatibility in this way (OpenGL
and Mesa, Motif and Lesstif) but those libraries are C libraries, and
do not have to worry about different memory footprints for objects and
different entry orders in the virtual function tables amongst other
things.

Coin is binary compatible with itself.  Each release of the Coin
library has a version number that consists of three digits.  They are
called "major", "minor", and "micro" version numbers respectively.
Coin 1.2.3 has major 1, minor 2 and micro 3.

* All releases with the same major and minor version number have the
  exact same API and ABI.  Such releases are called patch level
  releases, and only consist of bugfixes, documentation updates and
  updates to the packaging.

* All releases with the same major number are upwards binary
  compatible.  Such releases are called minor releases, and are
  releases made to add new extensions to the library API.

  Upwards compatibility means that applications linked with one
  version of Coin will not have to be rebuilt if you or the end-user
  of your application install a newer version of Coin with the same
  major number and a minor number that is greater than the library the
  software was initially linked with.  Let's say an application you
  have distributed was based on Coin 3.0.1 and then Coin 3.1.0 is
  released, your users can safely upgrade their Coin library without
  needing a new version of your application.

  This also works the other way for most platforms, as long as the
  software does not actually use any of the extensions that have been
  introduced after the release of the library you downgrade to.  This
  can be a bit tricky to get right (sometimes you might reference new
  functionality in an indirect way), so if you really need to have
  backwards compatibility like this, the best thing is to link with an
  early version of Coin in the particular major number series
  (e.g. Coin 3.0.0) and then release the software with the newest
  version of Coin (e.g. the future Coin 3.1.2).  Then, the end-user
  can safely downgrade the Coin-library on his side without any extra
  hassle.  The Coin API documentation will clearly state for which
  minor release an extension was introduced, so you can stay away from
  those functions if it is important, and so you can document with
  your software what the lowest acceptable Coin library version is.

* Releases with different major numbers are not compatible with each
  other.  They are called major releases, and break compatibility with
  the other major releases of the library on purpose.  The purpose of
  major releases is to clean up the library API (as for any evolving
  project, this has to be done once in a while) and change the
  fundamentals where they can be improved upon.

  Run-time libraries with different major versions can safely coexist
  on a system at once, so installing Coin 3.0.0 does not mean that you
  must scrap all the Coin 3.\*-based applications you may have.  On
  Windows, the DLLs are named differently for each major release, so
  there will be no mixups between coin3.dll and coin2.dll.  On UNIX
  systems, the application will at load-time look for a shared library
  named libCoin.so.#, where # will be indicative of the library ABI
  version.  Different major releases will have different # numbers, so
  the files do not conflict.

  However, there can only be one Software Development Kit for Coin
  available at once.  A Software Development Kit constitutes the
  header files and linktime libraries.  The declarations in the Coin
  1.0 header files will conflict with the declarations needed for
  producing code for Coin 2.0, and the same with both of those versus
  Coin 3.0.  When you install the Coin 3.x SDK, it will overwrite the
  headers for Coin 2.x unless you take special measures to prevent it.
  You can therefore only develop software against the version of Coin
  you installed most recently.  To circumvent this limitation, you
  need to do some trickery with include and libdir paths.  This should
  not be too difficult for a seasoned software developer.


## Historical Notes

Coin started out, back in 1995, as a scene graph rendering library for
VRML 1.0 scenes.  It was originally based on SGI's Qv library for parsing
files in the VRML 1.0 format.  After years of extending this humble
beginning with new functionality like VRML1 and VRML2 rendering and
export, the library was in late 1997 in dire need of a fundamental
redesign.

On the surface, the API looked quite like Open Inventor already.  The
concepts used by Open Inventor are also often mentioned as good design
methodologies in many software engineering books, and some of our
developers had some experience with the library in advance and
found it incredibly convenient.  At the same time as we were
contemplating a rewrite, the Free Software Movement got some great
buzz going, and we saw the golden opportunity to homestead our library
as the Free Software alternative to Open Inventor.  We therefore
decided to go for the rewrite, and after a short period coined the
name Coin.

As luck would not have it, as soon as we went to beta status with Coin
for SIGGRAPH 2000, SGI also decided to release their Open Inventor as
Free Software.  It soon became apparent though, that SGI Open Inventor
was released to mainly be kept in maintenance mode.  This made us
confident that continuing the Coin development would still be well
worth it.

The development of Coin was in the beginning primarily done on Linux
and IRIX systems, but is now mostly developed under Linux, Windows
with Cygwin, and Mac OS X systems.

Many people have contributed through the years to the success of Coin,
be it in the form of patches, problem reports, or other kinds of
feedback to the core Coin developer team.  The file THANKS tries to
credit all those helpful souls.  Our apologies to those who have been
forgotten.

## Latest release

Coin has historically been released quite infrequently.  To try to
improve on this, we decided in 2007 to switch from feature-based
release cycles to date-based release cycles, aiming for a new release
every six months.  What we haven't switched is the decision to let
the version number be decided based on ABI compatibility with earlier
Coin versions.

See the file NEWS for the summary of changes, the file RELNOTES for a
more verbose description of the more significant updates, and the file
ChangeLog for the detailed source code update list.

See the file INSTALL for installation instructions, and all the other
README.* files for platform-specific notes.

In 2019 a new major version was released, 4.0.0 which included some additional
API changes to improve conformance to Open Inventor 2.1 API.

## License and trademarks

BSD License (c) Kongsberg Oil & Gas Technologies AS

OpenGL and Open Inventor are trademarks of SGI Inc. 

