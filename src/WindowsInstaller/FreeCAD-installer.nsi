/*
FreeCAD Installer for Windows
Author: Uwe Stöhr
Compatible with NSIS 3.x
*/

# Do a Cyclic Redundancy Check to make sure the installer
# was not corrupted by the download.
CRCCheck on

# make it a Unicode installer
Unicode true

# enable support for high DPI resolution
ManifestDPIAware true

# installer settings like version numbers
!include settings.nsh

# declarations of FreeCAD's registry keys
!include include\declarations.nsh

# Multi-User settings
!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "${APP_REGKEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME ""

!define MULTIUSER_INSTALLMODE_INSTDIR "${APP_DIR}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY "${APP_REGKEY}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME ""

!define MULTIUSER_INSTALLMODE_FUNCTION InitUser
!define MULTIUSER_MUI

# included NSIS files
!include InstallOptions.nsh
!include LangFile.nsh
!include Library.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include MultiUser.nsh
!include Sections.nsh
!include WinVer.nsh
!include x64.nsh

# load the nsPprocess plugin
!include nsProcess.nsh

# Set of various macros and functions
!include include\utils.nsh

# set up the installer pages
!include include\gui.nsh

# sets the install sections and checks the system on starting the un/installer
!include include\init.nsh

# install FreeCAD and needed third-party programs like Python etc.
!include setup\install.nsh

# uninstall FreeCAD and all programs that were installed together with FreeCAD
!include setup\uninstall.nsh

# configure FreeCAD (set start menu and write registry entries)
!include setup\configure.nsh

#--------------------------------
# Output file

Outfile "${SETUP_EXE}"

# sign the installer executable
!finalize 'signing.bat ${SETUP_EXE}'
