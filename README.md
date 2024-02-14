<a href="https://freecad.org"><img src="https://www.freecad.org/svg/icon-freecad.svg" height="100px" width="100px"></a>

### Your own 3D parametric modeler

[Website](https://www.freecad.org) • 
[Documentation](https://wiki.freecad.org) •
[Forum](https://forum.freecad.org/) •
[Bug tracker](https://github.com/FreeCAD/FreeCAD/issues) •
[Git repository](https://github.com/FreeCAD/FreeCAD) •
[Blog](https://blog.freecad.org)


[![Release](https://img.shields.io/github/release/freecad/freecad.svg)](https://github.com/freecad/freecad/releases/latest) [![Crowdin](https://d322cqt584bo4o.cloudfront.net/freecad/localized.svg)](https://crowdin.com/project/freecad) [![Liberapay](https://img.shields.io/liberapay/receives/FreeCAD.svg?logo=liberapay)](https://liberapay.com/FreeCAD)

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
      
* **Features**
    * **Full Parametric Model** All objects are natively parametric meaning that the shape is based
      on properties or depend on other objects
    * **Modular Architechture** this allows plugin extensions that add functionality to the core app.
      It allows access to any part of the software with the built in Python interpeter
    * **Import/Export to standard formats** the tool lets you easily use files from other cad software with
      suppor for files formats such as STEP, IGES, OBJ, STL, DXF, SVG, DAE, IFC, OFF, NASTRAN, and VRML. The
      software also has its own file format called FCStd.
    * **Sketcher** There is a sketcher with a built in constraint solver which gives the user the ability to
      make geometry constrained 2D shapes.
    * **Technical Drawing Module** This has options for detailed views, cross section views, dimensioning,
      and other types of views as well. This is very helpful to have since it allows the user to create
      2D views of 3D models that have already been made.
    * **Architecture Module** This module allows a BIM like workflow with IFC compatibility which helps the
      user create models for professional architechture purposes.
    * **Path Module** This path module is one that is dedicated to mechanical machining for computer aided
      manufacturing. It lets you output display and adjust the G Codes which is very helpful since they are
      used to control the target machine.
    * **Integrated Spreadsheet and Expression Parser** this is used to create models that are bases on formulas
      and also organize the model data in a central location
    * **multi-platform** unlike many other cad software, FreeCad is multi-platform which means that it will
      behave the same on macos, windows and Linux.
    * **full GUI application** FreeCad features a gui based on the Qt framework with a 3D viewer sourced
      from Open Inventor. This makes rendering accessible and fast.
    * **Command Line support** FreeCad can be run without an interface using command line controls which means
      it can be used in a server enviroment.
    * **Workbench Concept** Tools are grouped in workbenches which is a good feature since you only need to
      display the tools that you need for a certain task.

Installing
----------

Precompiled packages for stable releases are available for Windows, macOS and Linux on the
[Releases page](https://github.com/FreeCAD/FreeCAD/releases).

On most Linux distributions, FreeCAD is also directly installable from the 
software center application.

For development releases check the [weekly-builds page](https://github.com/FreeCAD/FreeCAD-Bundle/releases/tag/weekly-builds).

Other options are described at the [wiki Download page](https://wiki.freecad.org/Download).

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

- [Linux](https://wiki.freecad.org/Compile_on_Linux)
- [Windows](https://wiki.freecad.org/Compile_on_Windows)
- [macOS](https://wiki.freecad.org/Compile_on_MacOS)
- [Cygwin](https://wiki.freecad.org/Compile_on_Cygwin)
- [MinGW](https://wiki.freecad.org/Compile_on_MinGW)


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

- [Bug Tracker](https://github.com/FreeCAD/FreeCAD/issues)
- [Reporting Issues and Requesting Features](https://github.com/FreeCAD/FreeCAD/issues/new/choose)
- [Contributing](https://github.com/FreeCAD/FreeCAD/blob/master/CONTRIBUTING.md)
- [Help Forum](https://forum.freecad.org/viewforum.php?f=3)
- [Developers Handbook](https://freecad.github.io/DevelopersHandbook/)

The [FPA](https://fpa.freecad.org) offers developers the opportunity
to apply for a grant to work on projects of their choosing. Check
[jobs and funding](https://blog.freecad.org/jobs/) to know more.


Usage & Getting help
--------------------

The FreeCAD wiki contains documentation on 
general FreeCAD usage, Python scripting, and development. These 
pages might help you get started:

- [Getting started](https://wiki.freecad.org/Getting_started)
- [Features list](https://wiki.freecad.org/Feature_list)
- [Frequent questions](https://wiki.freecad.org/FAQ/en)
- [Workbenches](https://wiki.freecad.org/Workbenches)
- [Scripting](https://wiki.freecad.org/Power_users_hub)
- [Development](https://wiki.freecad.org/Developer_hub)

The [FreeCAD forum](https://forum.freecad.org) is also a great place
to find help and solve specific problems you might encounter when
learning to use FreeCAD.


<p>This project receives generous infrastructure support from
  <a href="https://www.digitalocean.com/">
    <img src="https://opensource.nyc3.cdn.digitaloceanspaces.com/attribution/assets/SVG/DO_Logo_horizontal_blue.svg" width="91px">
  </a> and <a href="https://www.kipro-pcb.com/">KiCad Services Corp.</a>
</p>
