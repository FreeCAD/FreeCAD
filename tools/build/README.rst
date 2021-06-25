======================
Build Container Images
======================

The following will create containers that have all the required dependencies
pre-installed that are needed to build FreeCAD from source.

Containers
==========

Debian
------

The following commands are used to create and run a Debian build environment.

.. code-block:: console

    docker build --file tools/build/Dockerfile.Debian --tag freecad-debian
    docker run --rm --interactive --tty --volume $(pwd):/builds:z freecad-debian


Fedora
------

The following commands are used to create and run a Fedora build environment.

.. code-block:: console

    docker build --file tools/build/Dockerfile.Fedora --tag freecad-fedora
    docker run --rm --interactive --tty --volume $(pwd):/builds:z freecad-fedora


openSUSE
--------

The following commands are used to create and run a openSUSE build environment.

.. code-block:: console

    docker build --file tools/build/Dockerfile.openSUSE --tag freecad-opensuse
    docker run --rm --interactive --tty --volume $(pwd):/builds:z freecad-opensuse


Ubuntu
------

The following commands are used to create and run a Ubuntu build environment.


.. code-block:: console

    docker build --file tools/build/Dockerfile.Ubuntu --tag freecad-ubuntu
    docker run --rm --interactive --tty --volume $(pwd):/builds:z freecad-ubuntu


Build Code
==========

To build the FreeCAD code inside one of the running containers the following
commands should be used

.. code-block:: console

    mkdir freecad-build
    cd freecad-build
    cmake ../freecad-source
    make -j$(nproc --ignore=2)
