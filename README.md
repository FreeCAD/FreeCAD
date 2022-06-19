![Logo](https://www.freecadweb.org/images/logo.png)

### Your own 3D parametric modeler

[Website](https://www.freecadweb.org) • 
[Documentation](https://wiki.freecadweb.org) •
[Forum](https://forum.freecadweb.org/) •
[Bug tracker](https://www.freecadweb.org/tracker/) •
[Git repository](https://github.com/FreeCAD/FreeCAD)


[![Release](https://img.shields.io/github/release/freecad/freecad.svg)](https://github.com/freecad/freecad/releases/latest) [![Master][freecad-master-status]][gitlab-branch-master] [![Crowdin](https://d322cqt584bo4o.cloudfront.net/freecad/localized.svg)](https://crowdin.com/project/freecad) [![Gitter](https://img.shields.io/gitter/room/freecad/freecad.svg)](https://gitter.im/freecad/freecad?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) [![Language grade: Python](https://img.shields.io/lgtm/grade/python/g/FreeCAD/FreeCAD.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/FreeCAD/FreeCAD/context:python) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/FreeCAD/FreeCAD.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/FreeCAD/FreeCAD/context:cpp) [![Liberapay](https://img.shields.io/liberapay/receives/FreeCAD.svg?logo=liberapay)](https://liberapay.com/FreeCAD)

<img src="https://user-images.githubusercontent.com/1828501/174066870-1692005b-f8d7-43fb-a289-6d2f07f73d7f.png" width="800"/>

Overview
--------

* **Freedom to build what you want**  FreeCAD is an open-source parametric 3D 
modeler made primarily to design real-life objects of any size. 
Parametric modeling allows you to easily modify your design by going back into 
your model history to change its parameters. 

* **Create 3D from 2D and back** FreeCAD lets you to sketch geometry constrained
 2D shapes and use them as a base to build other objects. 
 It contains many components to adjust dimensions or extract design details from 
 3D models to create high quality production-ready drawings.

* **Designed for your needs** FreeCAD is designed to fit a wide range of uses
including product design, mechanical engineering and architecture,
whether you are a hobbyist, programmer, experienced CAD user, student or teacher.

* **Cross platform** FreeCAD runs on Windows, macOS and Linux operating systems.

* **Underlying technology**
    * **OpenCASCADE** A powerful geometry kernel, the most important component of FreeCAD
    * **Coin3D library** Open Inventor-compliant 3D scene representation model
    * **Python** FreeCAD offers a broad Python API
    * **Qt** Graphical user interface built with Qt


Installing
----------

Precompiled packages for stable releases are available for Windows, macOS and Linux on the
[Releases page](https://github.com/FreeCAD/FreeCAD/releases).

On most Linux distributions, FreeCAD is also directly installable from the 
software center application.

For development releases check the [weekly-builds page](https://github.com/FreeCAD/FreeCAD-Bundle/releases/tag/weekly-builds).

Other options are described at the [wiki Download page](https://wiki.freecadweb.org/Download).

Build Status
------------

| Master | 0.20 | Translation |
|:------:|:----:|:-----------:|
|[![Master][freecad-master-status]][gitlab-branch-master]|[![0.20][freecad-0.20-status]][gitlab-branch-0.20]|[![Crowdin](https://d322cqt584bo4o.cloudfront.net/freecad/localized.svg)](https://crowdin.com/project/freecad)|

[freecad-0.20-status]: https://gitlab.com/freecad/FreeCAD-CI/badges/releases/FreeCAD-0-20/pipeline.svg
[freecad-master-status]: https://gitlab.com/freecad/FreeCAD-CI/badges/master/pipeline.svg
[gitlab-branch-0.20]: https://gitlab.com/freecad/FreeCAD-CI/-/commits/releases/FreeCAD-0-20
[gitlab-branch-master]: https://gitlab.com/freecad/FreeCAD-CI/-/commits/master
[travis-builds]: https://travis-ci.org/FreeCAD/FreeCAD/builds

Compiling
---------

Compiling FreeCAD requires installation of several libraries and their 
development files such as OCCT (Open Cascade), Coin and Qt, listed in the 
pages below. Once this is done, FreeCAD can be compiled with 
CMake. On Windows, these libraries are bundled and offered by the 
FreeCAD team in a convenient package. On Linux, they are usually found 
in your distribution's repositories, and on macOS and other platforms, 
you will usually have to compile them yourself.

The pages below contain up-to-date build instructions:

- [Linux](https://wiki.freecadweb.org/Compile_on_Linux)
- [Windows](https://wiki.freecadweb.org/Compile_on_Windows)
- [macOS](https://wiki.freecadweb.org/Compile_on_MacOS)
- [Cygwin](https://wiki.freecadweb.org/Compile_on_Cygwin)
- [MinGW](https://wiki.freecadweb.org/Compile_on_MinGW)


Reporting Issues
---------

To report an issue please:
- First post to forum to verify the issue; 
- Link forum thread to bug tracker ticket and vice-a-versa; 
- Use the most updated stable or development versions of FreeCAD; 
- Post version info from eg. `Help > About FreeCAD > Copy to clipboard`; 
- Post a Step-By-Step explanation on how to recreate the issue; 
- Upload an example file to demonstrate problem. 

For more detail see:
- [Wiki Page about the Bug Tracker](https://wiki.freecadweb.org/Tracker)
- [Bug Tracker](https://tracker.freecadweb.org)
- [Help Forum](https://forum.freecadweb.org/viewforum.php?f=3)


Usage & Getting help
--------------------

The FreeCAD wiki contains documentation on 
general FreeCAD usage, Python scripting, and development. These 
pages might help you get started:

- [Getting started](https://wiki.freecadweb.org/Getting_started)
- [Features list](https://wiki.freecadweb.org/Feature_list)
- [Frequent questions](https://wiki.freecadweb.org/FAQ/en)
- [Workbenches](https://wiki.freecadweb.org/Workbenches)
- [Scripting](https://wiki.freecadweb.org/Power_users_hub)
- [Development](https://wiki.freecadweb.org/Developer_hub)

The [FreeCAD forum](https://forum.freecadweb.org) is also a great place
to find help and solve specific problems you might encounter when
learning to use FreeCAD.


<p>This project receives generous infrastructure support from
  <a href="https://www.digitalocean.com/">
    <img src="https://opensource.nyc3.cdn.digitaloceanspaces.com/attribution/assets/SVG/DO_Logo_horizontal_blue.svg" width="91px">
  </a> and <a href="https://www.kipro-pcb.com/">KiCad Services Corp.</a>
</p>
