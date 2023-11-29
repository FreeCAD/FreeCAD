/*

uninstall.nsh

Uninstall 

*/

Var FileAssociation

# ----------------------------------

Section "un.FreeCAD" un.SecUnProgramFiles

  SectionIn RO
    
  # delete start menu folder
  ReadRegStr $0 SHCTX "${APP_UNINST_KEY}" "StartMenu"
  RMDir /r "$0"
  # delete desktop icon
  Delete "$DESKTOP\${APP_NAME} ${APP_SERIES_NAME}.lnk"
  
  # remove file extension .FCStd
  ReadRegStr $R0 SHCTX "Software\Classes\${APP_EXT}" ""
  ${if} $R0 == "${APP_REGNAME_DOC}"
   DeleteRegKey SHCTX "Software\Classes\${APP_EXT}"
  ${endif}
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\${APP_EXT}"
  
  # remove further FC-specific file extension
  DeleteRegKey SHCTX "Software\Classes\${APP_EXT1}" # .FCStd1
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\${APP_EXT1}"
  DeleteRegKey SHCTX "Software\Classes\${APP_EXT_BAK}" # .FCBak
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\${APP_EXT_BAK}"
  DeleteRegKey SHCTX "Software\Classes\${APP_EXT_MACRO}" # .FCMacro
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\${APP_EXT_MACRO}"
  DeleteRegKey SHCTX "Software\Classes\${APP_EXT_MAT}" # .FCMat
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\${APP_EXT_MAT}"
  DeleteRegKey SHCTX "Software\Classes\${APP_EXT_SCRIPT}" # .FCScript
  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\${APP_EXT_SCRIPT}"
  
  ${if} $MultiUser.Privileges == "Admin"
   DeleteRegKey HKCR "${APP_REGNAME_DOC}"
   # see https://nsis.sourceforge.io/Docs/AppendixB.html#library_install for a description of UnInstallLib
   !insertmacro UnInstallLib REGDLL NOTSHARED NOREBOOT_NOTPROTECTED $SYSDIR\FCStdThumbnail.dll
  ${endif}

  # Uninstaller itself
  Delete "$INSTDIR\${SETUP_UNINSTALLER}"
  
  # Application folder
  SetOutPath "$TEMP"
  RMDir /r "$INSTDIR"
  
  # Registry keys and values
  DeleteRegKey SHCTX "${APP_REGKEY_SETUP}"
  DeleteRegKey SHCTX "${APP_REGKEY}"
  DeleteRegKey SHCTX "${APP_UNINST_KEY}"
  DeleteRegKey HKCR "Applications\${BIN_FREECAD}"
  DeleteRegValue HKCR "${APP_NAME}.Document\Shell\open\command" ""
  DeleteRegValue HKCR "${APP_NAME}.Document\DefaultIcon" ""
  
  # File associations
  ReadRegStr $FileAssociation SHELL_CONTEXT "Software\Classes\${APP_EXT}" ""
  
  ${If} $FileAssociation == "${APP_REGNAME_DOC}"
     DeleteRegKey SHELL_CONTEXT "Software\Classes\${APP_EXT}"
  ${EndIf}
  
  # clean other registry entry
  DeleteRegKey SHCTX "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\${APP_NAME}.exe"
  
  # Eventually refresh shell icons
   ${RefreshShellIcons}

SectionEnd

#---------------------------------
# user preferences
Section /o "un.$(UnFreeCADPreferencesTitle)" un.SecUnPreferences

 # issue a warning dialog
 MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION $(DialogUnPreferences) /SD IDYES IDYES +2 # continue if yes
  Goto NotPreferences
 # remove FreeCAD's config files
 StrCpy $AppSubfolder ${APP_DIR_USERDATA}
 Call un.DelAppPathSub # function from Utils.nsh
 # remove the registry key that stores the main window parameters
 DeleteRegKey HKCU "SOFTWARE\${APP_NAME}"
 NotPreferences:
  
SectionEnd

#---------------------------------
# Section descriptions
!insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${un.SecUnPreferences} "$(SecUnPreferencesDescription)"
!insertmacro MUI_DESCRIPTION_TEXT ${un.SecUnProgramFiles} "$(SecUnProgramFilesDescription)"
!insertmacro MUI_UNFUNCTION_DESCRIPTION_END
