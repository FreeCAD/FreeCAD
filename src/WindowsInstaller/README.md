# FreeCADInstProj
A Windows installer for FreeCAD

To build the installer you can do the following:
1. Get the latest zip-file of the whole installer source code "FC-standard-installer.zip"
   from https://github.com/donovaly/FreeCADInstProj/releases
2. Extract it to e.g. the path "C:\FreeCAD\Installer"
3. Open the file Settings.nsh with a text editor
   (the editor jEdit (jedit.org) can be recommended to edit NSIS files)
   and adapt there the following paths to the ones on your PC, e.g.:</br>
   !define FILES_FREECAD "C:\FreeCAD\Installer\FreeCAD"</br>
   !define FILES_DEPS "C:\FreeCAD\Installer\MSVCRedist"
4. Specify in Settings.nsh if it should be an installer for 32bit by commenting the line </br>
  !define MULTIUSER_USE_PROGRAMFILES64
5. Install the latest version 3.x of NSIS (https://nsis.sourceforge.io/Download)
6. Copy the file ~\nsprocess\Include\nsProcess.nsh to the folder</br>
   \Include of NSIS's installation folder.</br>
   Copy the file ~\nsprocess\Plugins\x86-unicode\nsProcess.dll to the folder</br>
   \Plugins\x86-unicode of NSIS's installation folder.
   (You can alternatively get nsProcess from https://nsis.sourceforge.io/NsProcess_plugin)
7. Copy all FreeCAD files to the folder "~\FreeCAD"
   e.g. "C:\FreeCAD\Installer\FreeCAD"
8. If you use compiled FreeCAD using another MSVC version than MSVC 2015, copy
   its distributable DLLs to the folder FILES_DEPS (see step 2).
9. Right-click on the file FreeCAD-installer.nsi and choose "Compile NSIS script"
   to compile the installer
10. The folder ~\MSVCRedist contains already all MSVC 2015 redistributable DLLs necessary
   for FreeCAD. If another MSVC version was used to compile FreeCAD, replace the DLLs by
   the ones of the used MSVC. (They are usually available in the folder
   C:\Program Files (x86)\MSVC2xxx\VC\redist)

For test builds of the installer you can turn off the compression. This speeds up
the build time for the installer a lot but increases its file size. The compression
is disabled by uncommenting the line
SetCompressor /SOLID lzma
in the file Settings.nsh.
