/*

Settings for FreeCAD installer

These typically need to be modified for each FreeCAD release

*/

# Make the installer as small as possible
# comment this for testing builds since it will reduce the time to create an installer
# a lot - for the cost of a much greater file size.
# So assure it is active for release builds!
SetCompressor /SOLID lzma

#--------------------------------
# File locations
# !!! you may need to adjust them to the folders in your Windows system !!!
!define FILES_FREECAD "${__FILEDIR__}\FreeCAD"
!define FILES_THUMBS "${__FILEDIR__}\thumbnail"

# msvc redistributables location is required for LibPack builds but not conda
# when using a LibPack build set the redistributables directory location here
# or with /D command line argument to makensis.exe
#!define FILES_DEPS "${__FILEDIR__}\MSVCRedist"

#--------------------------------
# Version number

!define COPYRIGHT_YEAR 2025

#!define APP_VERSION_MAJOR 1
#!define APP_VERSION_MINOR 0
#!define APP_VERSION_PATCH 0

# get version info from freecadcmd
!system "${FILES_FREECAD}\bin\freecadcmd.exe --safe-mode -c $\"print(f'!define APP_VERSION_MAJOR \$\"{App.Version()[0]}\$\"')$\">${__FILEDIR__}\version.nsh" = 0
!system "${FILES_FREECAD}\bin\freecadcmd.exe --safe-mode -c $\"print(f'!define APP_VERSION_MINOR \$\"{App.Version()[1]}\$\"')$\">>${__FILEDIR__}\version.nsh" = 0
!system "${FILES_FREECAD}\bin\freecadcmd.exe --safe-mode -c $\"print(f'!define APP_VERSION_PATCH \$\"{App.Version()[2]}\$\"')$\">>${__FILEDIR__}\version.nsh" = 0
!system "${FILES_FREECAD}\bin\freecadcmd.exe --safe-mode -c $\"print(f'!define APP_VERSION_REVISION \$\"{App.Version()[3].split()[0]}\$\"')$\">>${__FILEDIR__}\version.nsh" = 0
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

!define ExeFile "${APP_NAME}_${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}${APP_VERSION_EMERGENCY}-conda-Windows-x86_64-installer-${APP_VERSION_BUILD}.exe"

#--------------------------------
# installer bit type - FreeCAD is only provided as 64bit build
!define MULTIUSER_USE_PROGRAMFILES64
