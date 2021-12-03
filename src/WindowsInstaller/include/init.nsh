/*
init.nsh

Initialization functions
*/

#--------------------------------
# User initialization

Var FCLangName

Function InitUser

  # Get FreeCAD language
  
  ReadRegStr $FCLangName SHELL_CONTEXT "${APP_REGKEY_SETUP}" "FreeCAD Language"
  
  ${If} $FCLangName != ""
    StrCpy $LangName $FCLangName
  ${EndIf}
  
FunctionEnd

#--------------------------------
# visible installer sections

Section "!${APP_NAME}" SecCore
 SectionIn RO
SectionEnd

Section "$(SecFileAssocTitle)" SecFileAssoc
 StrCpy $CreateFileAssociations "true" 
SectionEnd

Section "$(SecDesktopTitle)" SecDesktop
 StrCpy $CreateDesktopIcon "true"
SectionEnd

# Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecCore} "$(SecCoreDescription)"
!insertmacro MUI_DESCRIPTION_TEXT ${SecFileAssoc} "$(SecFileAssocDescription)"
!insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} "$(SecDesktopDescription)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END


# .onInit must be here after the section definition because we have to set
# the selection states of the dictionary sections
Function .onInit

  ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  ${if} $R0 == "5.0" # 2000
  ${orif} $R0 == "5.1" # XP
  ${orif} $R0 == "5.2" # 2003
  ${orif} $R0 == "6.0" # Vista
    MessageBox MB_OK|MB_ICONSTOP "${APP_NAME} ${APP_VERSION} requires Windows 7 or newer." /SD IDOK
    Quit
  ${endif}
  
  # check if it is a 64bit system
  ${if} ${RunningX64}
   SetRegView 64
   !define LIBRARY_X64
  ${endif}
  
  # Check that FreeCAD is not currently running
  ${nsProcess::FindProcess} ${BIN_FREECAD} $R0
  # if running result is '0', if not running it is '603'
  ${if} $R0 == "0"
   MessageBox MB_OK|MB_ICONSTOP "$(UnInstallRunning)" /SD IDOK
   Abort
  ${endif}
  # plugin must be unloaded
  ${nsProcess::Unload}
  
  # initialize the multi-uder installer UI
  !insertmacro MULTIUSER_INIT
  
  # check if this FreeCAD version is already installed
  ${if} $MultiUser.Privileges == "Admin"
  ${orif} $MultiUser.Privileges == "Power"
   ReadRegStr $0 HKLM "${APP_UNINST_KEY}" "DisplayIcon"
  ${else}
   ReadRegStr $0 HKCU "${APP_UNINST_KEY}" "DisplayIcon"
   # handle also the case that FreeCAD is already installed in HKLM
   ${if} $0 == ""
    ReadRegStr $0 HKLM "${APP_UNINST_KEY}" "DisplayIcon"
   ${endif}
  ${endif}
  ${if} $0 != ""
   # check if the uninstaller was acidentally deleted
   # if so, don't bother the user if they really want to install a new FreeCAD over an existing one
   # because they won't have a chance to deny this
   StrCpy $4 $0 -16 # remove '\bin\FreeCAD.exe'
   # (for FileCheck the variables $0 and $1 cannot be used)
   !insertmacro FileCheck $5 "Uninstall-${APP_NAME}.exe" "$4" # macro from Utils.nsh
   ${if} $5 == "False"
    Goto ForceInstallation
   ${endif}
   # installing over an existing installation of the same FreeCAD release is not necessary
   # if the users does this, they most probably have a problem with FreeCAD that can better be solved
   # by reinstalling FreeCAD
   # for beta and other test releases over-installing can even cause errors
   MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION "$(AlreadyInstalled)" /SD IDNO IDYES ForceInstallation 
   Abort
   ForceInstallation:
  ${endif}
  
  # check if there is an existing FreeCAD installation of the same FreeCAD series
  # we usually don't release more than 10 versions so with 20 we are safe to check if a newer version is installed
  IntOp $4 ${APP_VERSION_REVISION} + 20
  ${for} $5 0 $4
   ${if} $MultiUser.Privileges == "Admin"
   ${orif} $MultiUser.Privileges == "Power"
    ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}${APP_VERSION_MAJOR}${APP_VERSION_MINOR}$5" "DisplayVersion"
    # also check for an emergency release
    ${if} $0 == ""
     ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}${APP_VERSION_MAJOR}${APP_VERSION_MINOR}$51" "DisplayVersion"
    ${endif}
   ${else}
    ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}${APP_VERSION_MAJOR}${APP_VERSION_MINOR}$5" "DisplayVersion"
    # also check for an emergency release
    ${if} $0 == ""
     ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}${APP_VERSION_MAJOR}${APP_VERSION_MINOR}$51" "DisplayVersion"
    ${endif}
   ${endif}
   ${if} $0 != ""
    StrCpy $R5 $0 # store the read version number
    StrCpy $OldVersionNumber "${APP_VERSION_MAJOR}${APP_VERSION_MINOR}$5"
    # we don't stop here because we want the latest installed version
   ${endif} 
  ${next}
  
  # NSIS cannot handle numbers with leading zero, thus cut it off before comparing
  StrCpy $1 $OldVersionNumber "" 1
  StrCpy $2 ${APP_SERIES_KEY} "" 1
  ${if} $1 > $2
   # store the version number and reformat it temporarily for the error message
   StrCpy $R0 $OldVersionNumber
   StrCpy $OldVersionNumber $R5
   MessageBox MB_OK|MB_ICONSTOP "$(NewerInstalled)" /SD IDOK
   StrCpy $OldVersionNumber $R0
   Abort
  ${endif}

  # this can be reset to "true" in section SecDesktop
  StrCpy $CreateDesktopIcon "false"
  StrCpy $CreateFileAssociations "false"
 
  ${IfNot} ${Silent}
    # Show banner while installer is intializating 
    Banner::show /NOUNLOAD "Checking system"
    Banner::destroy
  ${EndIf}

FunctionEnd

# this function is called at first after starting the uninstaller
Function un.onInit

  !insertmacro MULTIUSER_UNINIT

  # Check that FreeCAD is not currently running
  ${nsProcess::FindProcess} ${BIN_FREECAD} $R0
  # if running result is '0', if not running it is '603'
  ${if} $R0 == "0"
   MessageBox MB_OK|MB_ICONSTOP "$(UnInstallRunning)" /SD IDOK
   Abort
  ${endif}
  # plugin must be unloaded
  ${nsProcess::Unload}
  
  # check if it is a 64bit system
  ${if} ${RunningX64}
   SetRegView 64
  ${endif}

  # set registry root key
  ${if} $MultiUser.Privileges == "Admin"
  ${orif} $MultiUser.Privileges == "Power"
    SetShellVarContext all
  ${else}
   SetShellVarContext current
  ${endif}

  # Ascertain whether the user has sufficient privileges to uninstall.
  # abort when FreeCAD was installed with admin permissions but the user doesn't have administrator privileges
  ReadRegStr $0 HKLM "${APP_UNINST_KEY}" "DisplayVersion"
  ${if} $0 != ""
  ${andif} $MultiUser.Privileges != "Admin"
  ${andif} $MultiUser.Privileges != "Power"
   MessageBox MB_OK|MB_ICONSTOP "$(UnNotAdminLabel)" /SD IDOK
   Abort
  ${endif}
  # warning when FreeCAD couldn't be found in the registry
  ${if} $0 == "" # check in HKCU
   ReadRegStr $0 HKCU "${APP_UNINST_KEY}" "DisplayVersion"
   ${if} $0 == ""
     MessageBox MB_OK|MB_ICONEXCLAMATION "$(UnNotInRegistryLabel)" /SD IDOK
   ${endif}
  ${endif}
  
  # Macro to investigate name of FreeCAD's preferences folders to be able remove them
  !insertmacro UnAppPreSuff $AppPre $AppSuff # macro from Utils.nsh

  # question message if the user really wants to uninstall FreeCAD
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "$(UnReallyRemoveLabel)" /SD IDYES IDYES +2 # continue if yes
  Abort

FunctionEnd
