/*

install.nsh

Installation of program files, dictionaries and external components

*/

#--------------------------------
# Program files
!include LogicLib.nsh

Section -ProgramFiles SecProgramFiles
  ${If} $RemoveInstDir == "true"
    RMDir /r "$INSTDIR"
    ${If} ${Errors}
      MessageBox MB_OK|MB_ICONSTOP "$(RMInstDirFailed)" /SD IDOK
      Abort
    ${EndIf}
  ${EndIf}

  # turn on logging
  # Note that this can first be done here since the log file is written to $INSTDIR
  # to $INSTDIR must have a valid path before logging can be turned on
  LogSet on

  # Install and register the core FreeCAD files
  
  # Initializes the plug-ins dir ($PLUGINSDIR) if not already initialized.
  # $PLUGINSDIR is automatically deleted when the installer exits.
  InitPluginsDir
  
  # Binaries
  SetOutPath "$INSTDIR\bin"
  # recursively copy all files under bin
  File /r "${FILES_FREECAD}\bin\*.*"
  
  # MSVC redistributable DLLs
  !ifdef FILES_DEPS
    !echo "Including MSVC Redist files from ${FILES_DEPS}"
    SetOutPath "$INSTDIR\bin"
    File "${FILES_DEPS}\*.*"
  !endif
  
  # Others
  SetOutPath "$INSTDIR\data"
  File /r "${FILES_FREECAD}\data\*.*"
  SetOutPath "$INSTDIR\doc"
  File /r "${FILES_FREECAD}\doc\*.*"
  SetOutPath "$INSTDIR\Ext"
  File /r "${FILES_FREECAD}\Ext\*.*"
  SetOutPath "$INSTDIR\lib"
  File /r "${FILES_FREECAD}\lib\*.*"
  SetOutPath "$INSTDIR\Mod"
  File /r "${FILES_FREECAD}\Mod\*.*"
  SetOutPath "$INSTDIR"
  File /r "${FILES_THUMBS}"
    
  # Create uninstaller
  WriteUninstaller "$INSTDIR\${SETUP_UNINSTALLER}"

SectionEnd
