==========================
FreeCAD build dependencies
==========================

Distributions
=============

The following commands are used to install the packages needed to build FreeCAD from source on the following distributions.


Arch Linux
----------

The following command is used to install the required packages used to compile FreeCAD on Arch Linux.

.. code-block:: console

    sudo sh tools/build/Docker/arch.sh


Debian
------

The following command is used to install the required packages used to compile FreeCAD on Debian.

.. code-block:: console

    sudo sh tools/build/Docker/debian.sh


Fedora
------

The following command is used to install the required packages used to compile FreeCAD on Fedora.

.. code-block:: console

    sudo sh tools/build/Docker/fedora.sh


Manjaro
-------

The following command is used to install the required packages used to compile FreeCAD on Manjaro Linux.

.. code-block:: console

    sudo sh tools/build/Docker/manjaro.sh


Ubuntu
------


The following command is used to install the required packages used to compile FreeCAD on Ubuntu Linux.

.. code-block:: console

    sudo sh tools/build/Docker/ubuntu.sh


Containers
==========

The following will create containers that have all the required dependencies
pre-installed that are needed to build FreeCAD from source.


Arch Linux
-----------

The following commands are used to create and run a Arch Linux build environment.

.. code-block:: console

    docker build --file tools/build/Dockerfile.Arch --tag freecad-arch
    docker run --rm --interactive --tty --volume $(pwd):/builds:z freecad-arch


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


Manjaro
-------

The following commands are used to create and run a Manjaro build environment.

.. code-block:: console

    docker build --file tools/build/Dockerfile.Manjaro --tag freecad-manjaro
    docker run --rm --interactive --tty --volume $(pwd):/builds:z freecad-manjaro


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
