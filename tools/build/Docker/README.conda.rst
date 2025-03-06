========================
FreeCAD build with Conda
========================

Container
=========

Build and run the `freecad-conda` container with the following commands:

.. code-block:: console

    docker build --file tools/build/Docker/Dockerfile.Conda --tag freecad-conda .
    docker run --rm --interactive --tty --volume $(pwd):/builds:z freecad-conda

Build Code
==========

Within the `freecad-conda` container, install the build dependencies and build
FreeCAD using the following commands:

.. code-block:: console

    conda/setup-environment.sh
    conda activate freecad
    cmake --preset conda-linux-debug
    cmake --build build/debug
