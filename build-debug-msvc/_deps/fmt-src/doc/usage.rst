*****
Usage
*****

To use the {fmt} library, add :file:`fmt/core.h`, :file:`fmt/format.h`,
:file:`fmt/format-inl.h`, :file:`src/format.cc` and optionally other headers
from a `release archive <https://github.com/fmtlib/fmt/releases/latest>`_ or
the `Git repository <https://github.com/fmtlib/fmt>`_ to your project.
Alternatively, you can :ref:`build the library with CMake <building>`.

.. _building:

Building the Library
====================

The included `CMake build script`__ can be used to build the fmt
library on a wide range of platforms. CMake is freely available for
download from https://www.cmake.org/download/.

__ https://github.com/fmtlib/fmt/blob/master/CMakeLists.txt

CMake works by generating native makefiles or project files that can
be used in the compiler environment of your choice. The typical
workflow starts with::

  mkdir build          # Create a directory to hold the build output.
  cd build
  cmake ..  # Generate native build scripts.

where :file:`{<path/to/fmt>}` is a path to the ``fmt`` repository.

If you are on a \*nix system, you should now see a Makefile in the
current directory. Now you can build the library by running :command:`make`.

Once the library has been built you can invoke :command:`make test` to run
the tests.

You can control generation of the make ``test`` target with the ``FMT_TEST``
CMake option. This can be useful if you include fmt as a subdirectory in
your project but don't want to add fmt's tests to your ``test`` target.

If you use Windows and have Visual Studio installed, a :file:`FMT.sln`
file and several :file:`.vcproj` files will be created. You can then build them
using Visual Studio or msbuild.

On Mac OS X with Xcode installed, an :file:`.xcodeproj` file will be generated.

To build a `shared library`__ set the ``BUILD_SHARED_LIBS`` CMake variable to
``TRUE``::

  cmake -DBUILD_SHARED_LIBS=TRUE ...

__ https://en.wikipedia.org/wiki/Library_%28computing%29#Shared_libraries


To build a `static library` with position independent code (required if the main
consumer of the fmt library is a shared library i.e. a Python extension) set the
``CMAKE_POSITION_INDEPENDENT_CODE`` CMake variable to ``TRUE``::

  cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ...


Installing the Library
======================

After building the library you can install it on a Unix-like system by running
:command:`sudo make install`.

Usage with CMake
================

You can add the ``fmt`` library directory into your project and include it in
your ``CMakeLists.txt`` file::

   add_subdirectory(fmt)

or

::

   add_subdirectory(fmt EXCLUDE_FROM_ALL)

to exclude it from ``make``, ``make all``, or ``cmake --build .``.

You can detect and use an installed version of {fmt} as follows::

   find_package(fmt)
   target_link_libraries(<your-target> fmt::fmt)

Setting up your target to use a header-only version of ``fmt`` is equally easy::

   target_link_libraries(<your-target> PRIVATE fmt::fmt-header-only)

Usage with build2
=================

You can use `build2 <https://build2.org>`_, a dependency manager and a
build-system combined, to use ``fmt``.

Currently this package is available in these package repositories:

- **https://cppget.org/fmt/** for released and published versions.
- `The git repository with the sources of the build2 package of fmt <https://github.com/build2-packaging/fmt.git>`_
  for unreleased or custom revisions of ``fmt``.

**Usage:**

- ``build2`` package name: ``fmt``
- Library target name : ``lib{fmt}``

For example, to make your ``build2`` project depend on ``fmt``:

- Add one of the repositories to your configurations, or in your
  ``repositories.manifest``, if not already there::

    :
    role: prerequisite
    location: https://pkg.cppget.org/1/stable

- Add this package as a dependency to your ``./manifest`` file
  (example for ``v7.0.x``)::

    depends: fmt ~7.0.0

- Import the target and use it as a prerequisite to your own target
  using `fmt` in the appropriate ``buildfile``::

    import fmt = fmt%lib{fmt}
    lib{mylib} : cxx{**} ... $fmt

Then build your project as usual with `b` or `bdep update`.

For ``build2`` newcomers or to get more details and use cases, you can read the
``build2``
`toolchain introduction <https://build2.org/build2-toolchain/doc/build2-toolchain-intro.xhtml>`_.

Building the Documentation
==========================

To build the documentation you need the following software installed on your
system:

* `Python <https://www.python.org/>`_ with pip and virtualenv
* `Doxygen <http://www.stack.nl/~dimitri/doxygen/>`_
* `Less <http://lesscss.org/>`_ with ``less-plugin-clean-css``.
  Ubuntu doesn't package the ``clean-css`` plugin so you should use ``npm``
  instead of ``apt`` to install both ``less`` and the plugin::

    sudo npm install -g less less-plugin-clean-css.

First generate makefiles or project files using CMake as described in
the previous section. Then compile the ``doc`` target/project, for example::

  make doc

This will generate the HTML documentation in ``doc/html``.

Conda
=====

fmt can be installed on Linux, macOS and Windows with
`Conda <https://docs.conda.io/en/latest/>`__, using its
`conda-forge <https://conda-forge.org>`__
`package <https://github.com/conda-forge/fmt-feedstock>`__, as follows::

  conda install -c conda-forge fmt

Vcpkg
=====

You can download and install fmt using the `vcpkg
<https://github.com/Microsoft/vcpkg>`__ dependency manager::

  git clone https://github.com/Microsoft/vcpkg.git
  cd vcpkg
  ./bootstrap-vcpkg.sh
  ./vcpkg integrate install
  ./vcpkg install fmt

The fmt port in vcpkg is kept up to date by Microsoft team members and community
contributors. If the version is out of date, please `create an issue or pull
request <https://github.com/Microsoft/vcpkg>`__ on the vcpkg repository.

LHelper
=======

You can download and install fmt using
`lhelper <https://github.com/franko/lhelper>`__ dependency manager::

  lhelper activate <some-environment>
  lhelper install fmt

All the recipes for lhelper are kept in the
`lhelper's recipe <https://github.com/franko/lhelper-recipes>`__ repository.

Android NDK
===========

fmt provides `Android.mk file`__ that can be used to build the library
with `Android NDK <https://developer.android.com/tools/sdk/ndk/index.html>`_.
For an example of using fmt with Android NDK, see the
`android-ndk-example <https://github.com/fmtlib/android-ndk-example>`_
repository.

__ https://github.com/fmtlib/fmt/blob/master/support/Android.mk

Homebrew
========

fmt can be installed on OS X using `Homebrew <https://brew.sh/>`_::

  brew install fmt
