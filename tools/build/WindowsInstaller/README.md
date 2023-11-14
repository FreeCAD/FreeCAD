# Creating a Windows installer for FreeCAD

These are instructions for building an NSIS-based installer for FreeCAD. They were designed for FreeCAD 0.21 and later,
and presume that you have cloned a copy of FreeCAD's source code, and therefore have the directory *src/WindowsInstaller*.

## Install NSIS
To set up your system for building an NSIS installer:
1. Install the latest version 3.x of NSIS (https://nsis.sourceforge.io/Download)
2. Download these special release files of NSIS that support large strings:</br>
   https://nsis.sourceforge.io/Special_Builds#Large_strings</br>
   and copy the contained files into the corresponding NSIS installations folders
3. Download these special release files of NSIS that support logging:</br>
   https://nsis.sourceforge.io/Special_Builds#Advanced_logging</br>
   and copy the contained files into the corresponding NSIS installations folders
4. Download and install the nsProcess plugin from https://nsis.sourceforge.io/NsProcess_plugin -- you will need the version that supports Unicode, so make sure to follow the appropriate instructions on their site to install that one (as of this writing it involves manually copying and renaming the plugin DLL file).

## Build the installer
Next, update the installer settings for the current version of FreeCAD. Starting from the *src/WindowsInstaller* folder in the FreeCAD source tree:
1. Set the appropriate version strings for the release you are creating. These are used to construct the filename of the installer, among other things. If you have to upload a new version of the installer for the exact same release of FreeCAD, increment `APP_VERSION BUILD` as needed.
```
!define APP_VERSION_MAJOR 0
!define APP_VERSION_MINOR 21
!define APP_VERSION_REVISION 0
!define APP_VERSION_EMERGENCY "RC1"
!define APP_VERSION_BUILD 1
```
2. Within the folder *src/WindowsInstaller*, create a new folder called MSVCRedist and copy the following files from your MSVC installation into it:
```
vcruntime140.dll
concrt140.dll
msvcp140.dll
vcamp140.dll
vccorlib140.dll
vcomp140.dll
```    
3. Open the file *Settings.nsh* with a text editor (both jEdit and Visual Studio Code are good editors for NSIS files). Edit the following paths to correspond to your system: `FILES_FREECAD` corresponds to your installation directory (e.g. `CMAKE_INSTALL_PREFIX` if you self-compiled) and `FILES_DEPS` is the folder you created with the MSVC redistributable files in it.
```
!define FILES_FREECAD "C:\FreeCAD\Installer\FreeCAD"
!define FILES_DEPS "C:\FreeCAD\Installer\MSVCRedist"
```
4. Ensure the FreeCAD files are in place. Here you have two options:
   * If you are working from an already-compiled version of FreeCAD provided to you by an outside source: in this case, simply ensure that `FILES_FREECAD` is set to the directory containing those files.
   * If you compiled FreeCAD on your own as described [here](https://wiki.freecad.org/Compile_on_Windows) (and using the Install option outlined there). Then:
       * Open the file *Settings.nsh* as described in step 3. above and set there</br>
        `!define FILES_FREECAD` to the folder you specified as `CMAKE_INSTALL_PREFIX`
       * Copy into that folder the file *Delete.bat* that is part of the installer
       * open a command line in Windows and change to the folder
       * run the comamand</br>
        `Delete.bat`
       * (These steps assure that the installer only contains files users need. Moreover it assures that the
       overall files size is below 2 GB and we can use the most compact compression for the installer.)
5. Right-click on the file *FreeCAD-installer.nsi* and choose **Compile NSIS script**
   to compile the installer.


NOTE: For test builds of the installer you can turn off the compression. This speeds up
the build time for the installer a lot but increases its file size. The compression
is turned off by uncommenting the line</br>
`SetCompressor /SOLID lzma`</br>
in the file *Settings.nsh*.
