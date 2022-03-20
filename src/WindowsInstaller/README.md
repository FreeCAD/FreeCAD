# FreeCADInstProj
A Windows installer for FreeCAD

To build the installer you can do the following:
1. Get the latest zip-file of the whole installer source code "FC-standard-installer.zip" from</br>
   https://github.com/donovaly/FreeCADInstProj/releases
2. Extract it to e.g. the path "C:\FreeCAD\Installer"
3. Open the file Settings.nsh with a text editor</br>
   (the editor jEdit (jedit.org) can be recommended to edit NSIS files)</br>
   and adapt there the following paths to the ones on your PC, e.g.:</br>
   !define FILES_FREECAD "C:\FreeCAD\Installer\FreeCAD"</br>
   !define FILES_DEPS "C:\FreeCAD\Installer\MSVCRedist"
4. Specify in Settings.nsh if it should be an installer for 32bit by commenting the line</br>
   !define MULTIUSER_USE_PROGRAMFILES64
5. Install the latest version 3.x of NSIS (https://nsis.sourceforge.io/Download)
6. Download these special release files of NSIS that support large strings:</br>
   https://nsis.sourceforge.io/Special_Builds#Large_strings</br>
   and copy the containing files into the corresponding NSIS installations folders
7. Download these special release files of NSIS that support logging:</br>
   https://nsis.sourceforge.io/Special_Builds#Advanced_logging</br>
   and copy the containing files into the corresponding NSIS installations folders
8. Copy the file ~\nsprocess\Include\nsProcess.nsh to the folder</br>
   \Include of NSIS's installation folder.</br>
   Copy the file ~\nsprocess\Plugins\x86-unicode\nsProcess.dll to the folder</br>
   \Plugins\x86-unicode of NSIS's installation folder.</br>
   (You can alternatively get nsProcess from https://nsis.sourceforge.io/NsProcess_plugin)
9. Copy all FreeCAD files to the folder "~\FreeCAD"
   e.g. "C:\FreeCAD\Installer\FreeCAD"
10. If you use a version of FreeCAD that was compiled using another MSVC version than MSVC 2019,
   copy its distributable DLLs to the folder FILES_DEPS (see step 3).
11. Right-click on the file FreeCAD-installer.nsi and choose "Compile NSIS script"
   to compile the installer.
12. The folder ~\MSVCRedist contains already all MSVC 2019 x64 redistributable DLLs necessary
   for FreeCAD 0.20dev. If another MSVC version was used to compile FreeCAD, replace the DLLs by
   the ones of the used MSVC. (They are usually available in the folder
   C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Redist\MSVC)

For test builds of the installer you can turn off the compression. This speeds up
the build time for the installer a lot but increases its file size. The compression
is turned off by uncommenting the line
SetCompressor /SOLID lzma
in the file Settings.nsh.
