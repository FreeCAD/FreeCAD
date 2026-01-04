/*

Settings for FreeCAD installer

These typically need to be modified for each FreeCAD release

*/

# Make the installer as small as possible
# Using /SOLID is usually better for file size but it can't be used if the original size is
# more than 2GB, if building with /SOLID fails try disabling it
# comment this or use /DFC_TEST_BUILD command line option for testing builds since it will reduce
# the time to create an installer a lot at the cost of a much greater file size.
# So assure it is active for release builds!
!ifndef FC_TEST_BUILD
    SetCompressor /SOLID lzma
!endif

#--------------------------------
# File locations
# !!! you may need to adjust them to the folders in your Windows system !!!
# can be specified with /D command line argument to makensis.exe
!ifndef FILES_FREECAD
    !define FILES_FREECAD "${__FILEDIR__}\FreeCAD"
!endif
!ifndef FILES_THUMBS
    !define FILES_THUMBS "${__FILEDIR__}\thumbnail"
!endif

# msvc redistributables location is required for LibPack builds but not conda
# when using a LibPack build set the redistributables directory location here
# or with /D command line argument to makensis.exe
#!define FILES_DEPS "${__FILEDIR__}\MSVCRedist"

#--------------------------------
# get version info from freecadcmd
!system "${FILES_FREECAD}\bin\freecadcmd.exe --safe-mode $\"${__FILEDIR__}\write_version_nsh.py$\"" = 0
!include "${__FILEDIR__}\version.nsh"
!delfile "${__FILEDIR__}\version.nsh"

!define APP_VERSION_EMERGENCY "" # use "1" for an emergency release of FreeCAD otherwise ""
	# alternatively you can use APP_VERSION_EMERGENCY for a custom suffix of the version number
!define APP_EMERGENCY_DOT "" # use "." for an emergency release of FreeCAD otherwise ""
!define APP_VERSION_BUILD 1 # Start with 1 for the installer releases of each version

!define APP_VERSION "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}${APP_EMERGENCY_DOT}${APP_VERSION_EMERGENCY}" # Version to display

#--------------------------------
# Installer file name
# Typical names for the release are "FreeCAD-020-Installer-1.exe" etc.

!ifndef ExeFile
    !define ExeFile "${APP_NAME}_${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}${APP_VERSION_EMERGENCY}-Windows-x86_64-installer-${APP_VERSION_BUILD}.exe"
!endif

#--------------------------------
# installer bit type - FreeCAD is only provided as 64bit build
!define MULTIUSER_USE_PROGRAMFILES64
