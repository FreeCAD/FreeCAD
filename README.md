<h1 align="center">
    <img
        width="200"
        alt="FreeCAD Logo"
        src="https://www.freecadweb.org/images/logo.png">
    </div>
</h1>

<h3 align="center">Your own 3D parametric modeler</h3>

![screenshot](http://www.freecadweb.org/wiki/images/thumb/7/72/Freecad016_screenshot1.jpg/800px-Freecad016_screenshot1.jpg)

FreeCAD is a general purpose feature-based, parametric 3D modeler for 
CAD, MCAD, CAx, CAE and PLM, aimed directly at mechanical engineering 
and product design but also fits a wider range of uses in engineering, 
such as architecture or other engineering specialties. It is 100% Open 
Source (LGPL2+ license) and extremely modular, allowing for very 
advanced extension and customization.

FreeCAD is based on OpenCASCADE, a powerful geometry kernel, features an 
Open Inventor-compliant 3D scene representation model provided by the 
Coin 3D library, and a broad Python API. The interface is built with Qt. 
FreeCAD runs exactly the same way on Windows, Mac OSX, BSD and Linux 
platforms.

- [Home page](http://www.freecadweb.org)
- [Documentation wiki](http://www.freecadweb.org/wiki/)
- [Forum](http://forum.freecadweb.org/)
- [Bug tracker](http://www.freecadweb.org/tracker/)
- [Git repository](https://github.com/FreeCAD/FreeCAD)

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

[![Gitter](https://img.shields.io/gitter/room/freecad/freecad.svg)](https://gitter.im/freecad/freecad?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

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
