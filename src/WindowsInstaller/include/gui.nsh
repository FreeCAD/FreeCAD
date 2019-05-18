/*

gui.nsh

Installer user interface settings

*/

#--------------------------------
# General

Name "${APP_NAME} ${APP_VERSION}"
BrandingText " "

#--------------------------------
# Interface settings

!define MUI_ABORTWARNING
!define MUI_ICON "${SETUP_ICON}"
!define MUI_UNICON "${SETUP_ICON}"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${SETUP_HEADERIMAGE}"
!define MUI_HEADERIMAGE_RIGHT
!define MUI_WELCOMEFINISHPAGE_BITMAP "${SETUP_WIZARDIMAGE}"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${SETUP_WIZARDIMAGE}"

#--------------------------------
# Pages

# Installer

# Welcome page
!define MUI_WELCOMEPAGE_TEXT $(TEXT_WELCOME)
!insertmacro MUI_PAGE_WELCOME
# Show the license.
!define MUI_LICENSEPAGE_BUTTON $(^NextBtn)
!define MUI_LICENSEPAGE_TEXT_BOTTOM " "
!insertmacro MUI_PAGE_LICENSE "${FILES_LICENSE}"

# Decision if it should be installed as admin or not
!insertmacro MULTIUSER_PAGE_INSTALLMODE

# Specify the installation directory.
!insertmacro MUI_PAGE_DIRECTORY

# Define which components to install.
!insertmacro MUI_PAGE_COMPONENTS

# Specify where to install program shortcuts.
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${APP_DIR}"
!insertmacro MUI_PAGE_STARTMENU ${APP_NAME} $StartmenuFolder

# Watch the components being installed.
!insertmacro MUI_PAGE_INSTFILES

# The option to run FreeCAD from the finish page is currently disabled because
# it may run with Administrator priviledges, therefore causing a different
# user directory to be used. This could be fixed by creating a separate
# process without UAC elevation.
#!define MUI_FINISHPAGE_RUN_TEXT "$(FinishPageRun)"
#!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_RUN}"

!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION StartFreeCAD
!define MUI_FINISHPAGE_SHOWREADME_TEXT $(FinishPageRun)
!define MUI_FINISHPAGE_LINK $(TEXT_FINISH_WEBSITE)
!define MUI_FINISHPAGE_LINK_LOCATION "https://freecadweb.org/"
#!define MUI_PAGE_CUSTOMFUNCTION_SHOW CheckDesktopShortcut
!insertmacro MUI_PAGE_FINISH

# Uninstaller

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_UNPAGE_FINISH

#--------------------------------
# Installer Languages

!include lang\TranslatedLanguages.nsh

#--------------------------------
# Version information

VIProductVersion "${APP_VERSION_NUMBER}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${APP_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${APP_DIR}.${APP_VERSION_REVISION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${APP_INFO}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${APP_VERSION}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "${APP_COPYRIGHT}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${APP_NAME} Team"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" ""
