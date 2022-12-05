/*
declaration.nsh

Configuration and variables of FreeCAD installer
*/

#--------------------------------
# File locations

!define FILES_LICENSE "license.rtf"

#--------------------------------
# Names and version

!define APP_NAME "FreeCAD"
!define APP_VERSION_NUMBER "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_REVISION}.${APP_VERSION_BUILD}"
# For the proposed install folder we use the scheme "FreeCAD 0.18" 
# however for the Registry, we need the scheme "FreeCAD 0.18.x" in order
# to check if it is exactly this version (to support side-by-side installations)
!define APP_SERIES_NAME "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}"
!define APP_SERIES_KEY "${APP_VERSION_MAJOR}${APP_VERSION_MINOR}${APP_VERSION_REVISION}${APP_VERSION_EMERGENCY}"
!define APP_SERIES_KEY2 "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_REVISION}${APP_EMERGENCY_DOT}${APP_VERSION_EMERGENCY}"
!define APP_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\${APP_NAME}.exe"
!define APP_DIR "${APP_NAME} ${APP_SERIES_NAME}"
# Fixme: FC should use different preferences folder for every release
!define APP_DIR_USERDATA ${APP_NAME}
#!define APP_DIR_USERDATA "${APP_NAME}${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}"
!define APP_INFO "${APP_NAME} - Your Own 3D Parametric Modeler"
!define APP_WEBPAGE "https://www.freecad.org/"
!define APP_WEBPAGE_INFO "${APP_NAME} Website"
!define APP_WIKI "https://wiki.freecad.org/Main_Page"
!define APP_WIKI_INFO "${APP_NAME} Wiki"
!define APP_COPYRIGHT "${APP_NAME} is Copyright © 2001-${COPYRIGHT_YEAR} by the ${APP_NAME} Team"

!define APP_RUN "bin\${APP_NAME}.exe"
!define BIN_FREECAD "${APP_NAME}.exe"

!define APP_REGKEY "SOFTWARE\${APP_NAME}${APP_SERIES_KEY}" # like "FreeCAD0180"
!define APP_REGKEY_SETUP "${APP_REGKEY}\Setup"
!define APP_REGKEY_SETTINGS "${APP_REGKEY}\Settings"

!define APP_REGNAME_DOC "${APP_NAME}.Document"

!define APP_EXT ".FCStd"
!define APP_EXT1 ".FCStd1"
!define APP_MIME_TYPE "application/x-zip-compressed"

!define APP_EXT_BAK ".FCBak"
!define APP_EXT_MACRO ".FCMacro"
!define APP_EXT_MAT ".FCMat"
!define APP_EXT_SCRIPT ".FCScript"

!define APP_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SETUP_UNINSTALLER_KEY}"

#--------------------------------
# Setup settings

!define SETUP_EXE ${ExeFile}

!define SETUP_ICON "icons\FreeCAD.ico"
!define SETUP_HEADERIMAGE "graphics\header.bmp"
!define SETUP_WIZARDIMAGE "graphics\orange.bmp"
!define SETUP_UNINSTALLER "Uninstall-${APP_NAME}.exe"
!define SETUP_UNINSTALLER_KEY "${APP_NAME}${APP_SERIES_KEY}"

#--------------------------------
# Variables that are shared between multiple files

Var APPDATemp
Var AppPre
var AppSubfolder
Var AppSuff
Var CreateDesktopIcon
Var CreateFileAssociations
Var OldVersionNumber
Var Pointer
Var Search
Var StartmenuFolder
Var String
Var UserList
Var LangName
