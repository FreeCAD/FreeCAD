# Contents

<a name="contents"></a>

- [Building FreeCAD on Mac OS 10.15.x -- Catalina](#build-freecad-macos-catalina)
- [Directions](#directions)
  - [Install Xcode Command line tools](#install-xcode-cli-tools)
  - [Install Conda](#install-conda)
  - [Run the shell script](#run-the-shell-script)
- [Building FreeCAD on macOS using homebrew packages with & without formula file](#homebrew-build-fc-on-macos)
- [Requirements](#homebrew-requirements)
  - [Install required FreeCAD dependencies](#homebrew-install-required-deps)
- [Limitations of using freecad formula file](#homebrew-limits-of-formula-file)
- [Directions, Installing FreeCAD using brew packages without formula file](#homebrew-install-no-form-file)
  - [Expanded Directions](#homebrew-expanded-directions)
    - [Boost v1.75 fix](#homebrew-boost-175-fix)
- [Errors, Issues, & Possible Solutions](#errors-issues-solutions)

<!-- SEE: https://stackoverflow.com/a/7335259/708807 for explanation of using `name` attribute for in page linking within a MD doc -->

# Building FreeCAD on Mac OS 10.15.x -- Catalina #

<a name="build-freecad-macos-catalina"></a>

General notes on how the tooling works:

This setup uses [conda](https://docs.conda.io) for dependency management.
Conda is able to pull the deps from a repository called conda-forge and
setup an isolated build environment. Not quite as isolated as docker, but
it is a good option for Mac and is what the FreeCAD CI system uses.

Once the dependencies are installed into a conda environment, then the
build uses the standard `cmake` configuration process to configure the build
and `ninja` to invoke the actual compilation against whatever host compiler
(gcc, clang, etc.) was installed by conda's `cxx-compiler` package for
that architecture.

All of this, and some sanity checks, are in a unified shell script. See below.

# Directions

<a name="directions"></a>

## Install XCode Command line tools ##

<a name="install-xcode-cli-tools"></a>

Run `xcode-select --install` and click through.

## Install Conda ##

<a name="install-conda"></a>

Refer to [MiniConda Docs](https://docs.conda.io/en/latest/miniconda.html).

## Run the shell script ##

<a name="run-the-shell-script"></a>

Run the `./build_unix_dev_conda.sh` and go get coffee. Builds take
an hour+ on a 2012 Retina MacBook.

Output binaries will be in the `./build/bin/FreeCAD` *and*
`${CONDA_PREFIX}/bin/FreeCAD` directories.

You can code/build/test using the cmake configuration folder `./build` in
the standard way *from within the freecad_dev conda environment*.

---

# Building FreeCAD on macOS using homebrew with & without a formula file

<a name="homebrew-build-fc-on-macos"></a>

> The below procedure provides an alternative way to install FreeCAD from the git source on macOS without having to use conda, but rather relies on [**mac homebrew**][lnk1] to manage dependencies.

## Requirements

<a name="homebrew-requirements"></a>

- macOS, running High Sierra _10.13_ or later
- homebrew installed and working
- All required dependencies to build FreeCAD installed using `brew install`

There is an official [**homebrew tap**][lnk2] that provides a list of formulas along with FreeCAD to setup all the dependencies to build FreeCAD from source on macOS, and also provides prebuilt bottles to install FreeCAD from a package rather than building from source.

> 💡 The below steps will build a FreeCAD binary that will launch FreeCAD from a command line interface, and will **NOT** build the **FreeCAD.app** bundle, ie. a double clickable app icon that can be launched from a Finder window.

### Install required FreeCAD dependencies

<a name="homebrew-install-required-deps"></a>

- Setup homebrew to use the official [**freecad-homebrew**][lnk2] tap.

```shell
brew tap FreeCAD/freecad
```

- Install FreeCAD dependencies provided by the tap

```shell
brew install --only-dependencies freecad
```

> The above step will install FreeCAD dependencies provided by the tap, and if a _bottle_ is provided by the tap homebrew will install the bottled version of the dep rather than building from source, unless the `install` command explicitly uses a _flag_ to build from source.

After all the dependencies have been installed, it should be possible to install FreeCAD from the provided bottle.

```shell
brew install freecad/freecad/freecad
```

> As of writing this, there are bottles provided for macOS Catalina and Big Sur
> > If running a different version of macOS then building FreeCAD from source will be required.

To explicitly build FreeCAD from source using the formula file provided by the tap

```shell
brew install freecad/freecad/freecad --build-from-source --HEAD --verbose
```

The above command will grab the latest git source of FreeCAD and output the build process to the terminal.

> NOTE: On a MacBookPro 2013 late model it takes ~60 minutes to build FreeCAD from source.

After the _make_ and _make install_ process completes it should be possible to launch FreeCAD from any directory using a terminal with the below commands,

```shell
FreeCAD
FreeCADCmd
```

- `FreeCAD` will launch a GUI version of FreeCAD
- `FreeCADCmd` will launch **only** a command line version of FreeCAD

## Limitations of using the FreeCAD formula file

<a name="homebrew-limits-of-formula-file"></a>

If FreeCAD is installed via the bottle then one will have to wait for a new bottle to be generated to install a later version of FreeCAD.  However, if FreeCAD is built from source, then FreeCAD will have all the updates up to the time the build process was started.

If any of the dependencies FreeCAD relies on is updated FreeCAD will likely require a rebuild.  Mac homebrew does provide a feature to pin packages at specific versions to prevent them from updating, and also allows setting of an environment variable to prevent homebrew from automatically checking for updates (which can slow things down). All that said, FreeCAD can be built using all the dependencies provided by Mac homebrew, but not using the formula file: instead cloning the source to an arbitrary path on a local filesystem. This provides a couple of advantages:

- If `brew cleanup` is run and FreeCAD was installed using the above-provided command, all source tarballs or bottles that were _checked out_ or downloaded during the install process will be deleted from the system. If a reinstall or upgrade is later required then homebrew will have to refetch the bottles, or reclone the git source again.
- Mac homebrew provides a method, _install flag_, for keeping the source regardless if the build succeeds or fails. The options are limited, however, and performing a standard `git clone` outside of homebrew is **much** preferred. 
- Cloning the FreeCAD source allows passing **any** cmake flags not provided by the formula file
  - Allowing the use of other build systems such as _ninja_
  - Allowing the use of alternate compilers, e.g. _ccache_
  - Pulling in subsequent updates are quicker because the `git clone` of the FreeCAD source will remain on the local filesystem even if a `brew cleanup` is run
  - Subsequent recompiles should not take 60 minutes if using a caching strategy such as _ccache_.

## Directions, Installing FreeCAD using brew packages without a formula file

<a name="homebrew-install-no-form-file"></a>

> ⚠️ The below directions assume macOS High Sierra or later is being used, homebrew is setup properly, and all dependencies were installed successfully.

**TL;DR**

- Clone the FreeCAD source, pass cmake args/flags within source dir, run make, and make install, then profit 💰

### Expanded Directions

<a name="homebrew-expanded-directions"></a>

- Clone the FreeCAD source from GitHub

```shell
git clone https://github.com/freecad/freecad
cd ./freecad
git fetch
```

> The above _fetch_ cmd will take some time to fetch the commit history for the repo, but if a shallow clone is performed then FreeCAD will not show to correct build number in the About dialog [**learn more**][lnk3].

Advanced users may alter the process below to build in a different location, use a different compiler, etc. but these instructions represent a procedure that works successfully for this author.

Set the path / environment variables for specifying the compilers to use

```
export CC="/usr/local/opt/llvm/bin/clang"
export CXX="/usr/local/opt/llvm/bin/clang++"
```

- Linking the brew-provided install of python 3 will be required in order for cmake to find the proper python and python libraries.

```shell
brew link python@3.9
```

#### Boost v1.75 fix

<a name="homebrew-boost-175-fix"></a>

- Due to recent changes in boost v1.75, building FreeCAD will fail with the below linking error message (for a more exhaustive error message, [**learn more**][lnk4])

To work around the linking issue until the [**PR**][lnk5] is merged install boost will the patches applied within the PR.

```shell
ld: library not found for -licudata
```

```shell
git checkout -b mybuild

cmake \
-DCMAKE_C_FLAGS_RELEASE=-DNDEBUG \
-DCMAKE_CXX_FLAGS_RELEASE=-DNDEBUG \
-DCMAKE_INSTALL_PREFIX=/opt/beta/freecad \
-DCMAKE_INSTALL_LIBDIR=lib \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_FIND_FRAMEWORK=LAST
-DCMAKE_VERBOSE_MAKEFILE=ON \
-Wno-dev \
-DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/macOSX10.14.sdk \
-std=c++14 \
-DCMAKE_CXX_STANDARD=14 \
-DBUILD_ENABLE_CXX_STD:STRING=C++14 \
-Wno-deprecated-declarations \
-DUSE_PYTHON3=1 -DPYTHON_EXECUTABLE=/usr/local/bin/python3 \
-DBUILD_FEM_NETGEN=1 \
-DBUILD_FEM=1 \
-DBUILD_TECHDRAW=0 \
-DFREECAD_USE_EXTERNAL_KDL=ON \
-DFREECAD_CREATE_MAC_APP=OFF \
-DCMAKE_PREFIX_PATH="/usr/local/opt/qt/lib/cmake;/usr/local/opt/nglib/Contents/Resources;/usr/local/opt/vtk@8.2/lib/cmake;/usr/local;" .
```

After the configuration completes run the below commands to start the build & install process

```shell
make
make install
```

> 💡 Author's note: The above cmake build flags are the ones I've had good luck with, but that's not to say other ones can be added or removed. And for reasons unknown to me the above build process takes ~ twice along than using `brew install --build-from-source`

If everything goes well FreeCAD should be able to launch from a terminal

## Errors and Issues + possible solutions

<a name="errors-issues-solutions"></a>

Some common pitfalls are listed in this section.

---

<details>
<summary><strong>error:</strong> no member named </summary>

```shell
[ 18%] Building CXX object src/Gui/CMakeFiles/FreeCADGui.dir/DlgProjectInformationImp.cpp.o
cd /opt/code/github/public/forks/freecad/build/src/Gui && /usr/local/bin/ccache /usr/local/opt/llvm/bin/clang++ -DBOOST_ALL_NO_LIB -DBOOST_FILESYSTEM_DYN_LINK -DBOOST_PP_VARIADICS=1 -DBOOST_PROGRAM_OPTIONS_DYN_LINK -DBOOST_REGEX_DYN_LINK -DBOOST_SYSTEM_DYN_LINK -DBOOST_THREAD_DYN_LINK -DBUILD_ADDONMGR -DCMAKE_BUILD_TYPE=\"Release\" -DFreeCADGui_EXPORTS -DGL_SILENCE_DEPRECATION -DHAVE_CONFIG_H -DHAVE_FREEIMAGE -DHAVE_PYSIDE2 -DHAVE_RAPIDJSON -DHAVE_SHIBOKEN2 -DHAVE_TBB -DNDEBUG -DOCC_CONVERT_SIGNALS -DPYSIDE_QML_SUPPORT=1 -DQT_CORE_LIB -DQT_GUI_LIB -DQT_NETWORK_LIB -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_PRINTSUPPORT_LIB -DQT_SVG_LIB -DQT_UITOOLS_LIB -DQT_WIDGETS_LIB -DQT_XML_LIB -D_OCC64 -I/opt/code/github/public/forks/freecad/build -I/opt/code/github/public/forks/freecad/build/src -I/opt/code/github/public/forks/freecad/src -I/opt/code/github/public/forks/freecad/src/Gui -I/opt/code/github/public/forks/freecad/src/Gui/Quarter -I/opt/code/github/public/forks/freecad/build/src/Gui -I/opt/code/github/public/forks/freecad/src/Gui/.. -I/opt/code/github/public/forks/freecad/build/src/Gui/.. -I/opt/code/github/public/forks/freecad/build/src/Gui/Language -I/opt/code/github/public/forks/freecad/build/src/Gui/propertyeditor -I/opt/code/github/public/forks/freecad/build/src/Gui/TaskView -I/opt/code/github/public/forks/freecad/build/src/Gui/Quarter -I/opt/code/github/public/forks/freecad/build/src/Gui/DAGView -I/usr/local/include/eigen3 -I/usr/local/include/PySide2/QtCore -I/usr/local/include/PySide2/QtGui -I/usr/local/include/PySide2/QtWidgets -isystem /usr/local/include -isystem /usr/local/Frameworks/Python.framework/Versions/3.9/include/python3.9 -iframework /usr/local/opt/qt/lib -isystem /usr/local/opt/qt/lib/QtCore.framework/Headers -isystem /usr/local/opt/qt/./mkspecs/macx-clang -isystem /usr/local/opt/qt/lib/QtWidgets.framework/Headers -isystem /usr/local/opt/qt/lib/QtGui.framework/Headers -isystem /Library/Developer/CommandLineTools/SDKs/macOSX10.14.sdk/System/Library/Frameworks/OpenGL.framework/Headers -isystem /usr/local/opt/qt/lib/QtOpenGL.framework/Headers -isystem /usr/local/opt/qt/lib/QtPrintSupport.framework/Headers -isystem /usr/local/opt/qt/lib/QtSvg.framework/Headers -isystem /usr/local/opt/qt/lib/QtNetwork.framework/Headers -isystem /usr/local/opt/qt/include -isystem /usr/local/opt/qt/include/QtUiTools -isystem /usr/local/include/shiboken2 -isystem /usr/local/include/PySide2 -isystem /usr/local/opt/qt/lib/QtXml.framework/Headers -Wall -Wextra -Wpedantic -Wno-write-strings  -Wno-undefined-var-template -DNDEBUG -isysroot /Library/Developer/CommandLineTools/SDKs/macOSX10.14.sdk -fPIC -I/usr/local/Cellar/open-mpi/4.0.5/include -fPIC -std=gnu++14 -o CMakeFiles/FreeCADGui.dir/DlgProjectInformationImp.cpp.o -c /opt/code/github/public/forks/freecad/src/Gui/DlgProjectInformationImp.cpp
/opt/code/github/public/forks/freecad/src/Gui/DlgProjectInformationImp.cpp:56:9: error: no member named 'lineEditProgramVersion' in 'Gui::Dialog::Ui_DlgProjectInformation'
    ui->lineEditProgramVersion->setText(QString::fromUtf8(doc->getProgramVersion()));
    ~~  ^
1 error generated.
make[2]: *** [src/Gui/CMakeFiles/FreeCADGui.dir/DlgProjectInformationImp.cpp.o] Error 1
make[1]: *** [src/Gui/CMakeFiles/FreeCADGui.dir/all] Error 2
make: *** [all] Error 2
```

</details>

FreeCAD may fail to build if creating a build directory within the _src_ directory and running `cmake ..` within the newly created _build_ directory.  As it currently stands, `cmake` needs run within the _src_ directory or the this error message will likely appear during the build process.

---

<details>
<summary> ⚠️ <strong>warning:</strong> specified path differs in case from file name on disk</summary>

```
/opt/code/github/public/forks/FreeCAD/src/Gui/moc_DlgParameterFind.cpp:10:10: warning: non-portable path to file '"../../../FreeCAD/src/Gui/DlgParameterFind.h"'; specified path differs in case from file name on disk [-Wnonportable-include-path]
#include "../../../freecad/src/Gui/DlgParameterFind.h"
         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         "../../../FreeCAD/src/Gui/DlgParameterFind.h"
1 warning generated.
```

</details>

On macOS most filesystems are case **insensitive**, whereas most GNU+Linux distros use _case sensitive_ file systems. So if FreeCAD source is cloned within a `FreeCAD` directory the build process on macOS may look for a `freecad` that is _case sensitive_ however the file system isn't case sensitive, thus the compiler will provide the above warning message.

One way to resolve such error message is to rename `FreeCAD` to `freecad`

```shell
mv FreeCAD freecadd;
mv freecadd freecad;
```

---

<!-- links -->

[lnk1]: <http://brew.sh>
[lnk2]: <https://github.com/FreeCAD/homebrew-freecad>
[lnk3]: <https://forum.freecad.org/viewtopic.php?f=4&t=51981#p446796>
[lnk4]: <https://gist.github.com/ipatch/6116824ab1f2a99b526cb07e43317b91#gistcomment-3577066>
[lnk5]: <https://github.com/Homebrew/homebrew-core/pull/67615>
