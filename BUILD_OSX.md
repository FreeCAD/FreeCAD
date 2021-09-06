# Building FreeCAD on Mac OS 10.15.x -- Catalina #

General notes on how the tooling works:

This setup uses [conda](https://docs.conda.io) for dependency management.
Conda is able to pull the deps from a repository called conda-forge and
setup an isolated build environment. Not quite as isolated as docker, but
it is a good option for Mac and its what the FreeCAD CI system uses.

Once the dependencies are installed into a conda environment, then the
build uses the standard `cmake` configuration process to configure the build
and `ninja` to invoke the actual compilation against whatever host compiler
(gcc, clang, etc.) was installed by conda's `cxx-compiler` package for
that architecture.

All of this, and some sanity checks, are in a unified shell script. See below.

# Directions #

## Install XCode Command line tools ##

Run `xcode-select --install` and click through.

## Install Conda ##

Refer to [MiniConda Docs](https://docs.conda.io/en/latest/miniconda.html).

## Run the shell script ##

Run the `./build_unix_dev_conda.sh` and go get coffee. Builds take
an hour+ on a 2012 Retina MacBook.

Output binaries will be in the `./build/bin/FreeCAD` *and*
`${CONDA_PREFIX}/bin/FreeCAD` directories.

You can code/build/test using the cmake configuration folder `./build` in
the standard way *from within the freecad_dev conda environment*.

