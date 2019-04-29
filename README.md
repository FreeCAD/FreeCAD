![Logo](https://www.freecadweb.org/images/logo.png)

### Your own 3D parametric modeler

[Website](https://www.freecadweb.org) • 
[Docs](https://www.freecadweb.org/wiki/) •
[Forum](https://forum.freecadweb.org/) •
[Bug tracker](https://www.freecadweb.org/tracker/)

[![Gitter](https://img.shields.io/gitter/room/freecad/freecad.svg)](https://gitter.im/freecad/freecad?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)


[![Release](https://img.shields.io/github/release/freecad/freecad.svg)](https://github.com/freecad/freecad/releases/latest) [![Master][freecad-master-status]][travis-branches] [![Crowdin](https://d322cqt584bo4o.cloudfront.net/freecad/localized.svg)](https://crowdin.com/project/freecad)


![screenshot](http://www.freecadweb.org/wiki/images/thumb/7/72/Freecad016_screenshot1.jpg/800px-Freecad016_screenshot1.jpg)

Overview
--------

* **Freedom to build what you want**  FreeCAD is an open-source parametric 3D 
modeler made primarily to design real-life objects of any size. 
Parametric modeling allows you to easily modify your design by going back into 
your model history and changing its parameters. 

* **Create 3D from 2D & back** FreeCAD allows you to sketch geometry constrained
 2D shapes and use them as a base to build other objects. 
 It contains many components to adjust dimensions or extract design details from 
 3D models to create high quality production ready drawings.

* **Designed for your needs** FreeCAD is designed to fit a wide range of uses
 including product design, mechanical engineering and architecture. 
 Whether you are a hobbyist, a programmer, an experienced CAD user, 
 a student or a teacher, you will feel right at home with FreeCAD.

* **Cross platform** FreeCAD runs on Windows, Mac and Linux

* **Underlying technology**
    * **Based on OpenCASCADE** A powerful geometry kernel
    * **Coin3D library** Open Inventor-compliant 3D scene representation model
    * **Python** FreeCAD offers a broad Python API
    * **Qt** Graphical User Interface built with Qt


Installing
----------

Precompiled (installable) packages are available for Windows and Mac on the
[Releases page](https://github.com/FreeCAD/FreeCAD/releases).

On most Linux distributions, FreeCAD is directly installable from the 
software center application.

Other options are described at the [wiki Download page](http://www.freecadweb.org/wiki/Download).

Build Status <img src="https://blog.travis-ci.com/images/travis-mascot-200px.png" height="30"/>
------------

| Master | 0.18 | Translation |
|:------:|:----:|:-----------:|
|[![Master][freecad-master-status]][travis-branches]|[![0.18][freecad-0.18-status]][travis-branches]|[![Crowdin](https://d322cqt584bo4o.cloudfront.net/freecad/localized.svg)](https://crowdin.com/project/freecad)|

[freecad-0.18-status]: https://travis-ci.org/FreeCAD/FreeCAD.svg?branch=releases/FreeCAD-0-18
[freecad-master-status]: https://travis-ci.org/FreeCAD/FreeCAD.svg?branch=master
[travis-branches]: https://travis-ci.org/FreeCAD/FreeCAD/branches
[travis-builds]: https://travis-ci.org/FreeCAD/FreeCAD/builds

Compiling
---------

Compiling FreeCAD requires installation of several libraries and their 
development files such as OpenCASCADe, Coin and Qt, listed in the 
pages below. Once this is done, FreeCAD can be simply compiled with 
cMake. On Windows, these libraries are bundled and offered by the 
FreeCAD team in a convenient package. On Linux, they are usually found 
in your distribution's repositories, and on Mac OSX and other platforms 
you will usually need to compile them yourself.

The pages below contain up-to-date build instructions:

- [Linux](http://www.freecadweb.org/wiki/CompileOnUnix)
- [Windows](http://www.freecadweb.org/wiki/CompileOnWindows)
- [Mac OSX](http://www.freecadweb.org/wiki/CompileOnMac)
- [Cygwin](http://www.freecadweb.org/wiki/CompileOnCygwin)
- [MinGW](http://www.freecadweb.org/wiki/CompileOnMinGW)

Usage & Getting help
--------------------

The FreeCAD wiki contains documentation on 
general FreeCAD usage, Python scripting, and development. These 
pages might help you get started:

- [Getting started](http://www.freecadweb.org/wiki/Getting_started)
- [Features list](http://www.freecadweb.org/wiki/Feature_list)
- [Frequent questions](http://www.freecadweb.org/wiki/FAQ)
- [Workbenches](http://www.freecadweb.org/wiki/Workbench_Concept)
- [Scripting](http://www.freecadweb.org/wiki/Power_users_hub)
- [Development](http://www.freecadweb.org/wiki/Developer_hub)

The [FreeCAD forum](http://forum.freecadweb.org) is also a great place
to find help and solve specific problems you might encounter when
learning to use FreeCAD.
