/*

install.nsh

Installation of program files, dictionaries and external components

*/

#--------------------------------
# Program files

Section -ProgramFiles SecProgramFiles
  # this can make install significantly faster but disables output to "details view"
  # on the install page, which is unreadable with full verbosity anyways
  # this is recommended in docs when working with many small files
  # https://nsis.sourceforge.io/Reference/SetDetailsPrint
  SetDetailsPrint textonly
  ${If} $RemoveInstDir == "true"
    ${DetailPrintToBoth} "Cleaning install folder '$INSTDIR'"
    ClearErrors
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

  ${DetailPrintToBoth} "Extracting files..."

  # Binaries
  ${DetailPrintToBoth} "Extracting files to '$INSTDIR\bin\'"
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
  ${DetailPrintToBoth} "Extracting files to '$INSTDIR\data\'"
  SetOutPath "$INSTDIR\data"
  File /r "${FILES_FREECAD}\data\*.*"
  ${DetailPrintToBoth} "Extracting files to '$INSTDIR\doc\'"
  SetOutPath "$INSTDIR\doc"
  File /r "${FILES_FREECAD}\doc\*.*"
  ${DetailPrintToBoth} "Extracting files to '$INSTDIR\Ext\'"
  SetOutPath "$INSTDIR\Ext"
  File /r "${FILES_FREECAD}\Ext\*.*"
  ${DetailPrintToBoth} "Extracting files to '$INSTDIR\lib\'"
  SetOutPath "$INSTDIR\lib"
  File /r "${FILES_FREECAD}\lib\*.*"
  ${DetailPrintToBoth} "Extracting files to '$INSTDIR\Mod\'"
  SetOutPath "$INSTDIR\Mod"
  File /r "${FILES_FREECAD}\Mod\*.*"
  ${DetailPrintToBoth} "Extracting thumbnailer"
  SetOutPath "$INSTDIR"
  File /r "${FILES_THUMBS}"

  SetDetailsPrint both
  DetailPrint "Writing uninstaller to '$INSTDIR'"
  WriteUninstaller "$INSTDIR\${SETUP_UNINSTALLER}"

SectionEnd
