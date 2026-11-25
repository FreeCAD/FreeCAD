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
  Call ReplaceRegistered
  Call ValidateInstallDir

  # Create $INSTDIR before enabling logging; the log is written there.
  SetOutPath "$INSTDIR"
  LogSet on

  # Install and register the core FreeCAD files

  # Initializes the plug-ins dir ($PLUGINSDIR) if not already initialized.
  # $PLUGINSDIR is automatically deleted when the installer exits.
  InitPluginsDir

  ${DetailPrintToBoth} "Extracting files..."

  ClearErrors
  !insertmacro InstallPayload

  SetDetailsPrint both
  DetailPrint "Writing uninstaller to '$INSTDIR'"
  WriteUninstaller "$INSTDIR\${SETUP_UNINSTALLER}"
  ${If} ${Errors}
    Abort
  ${EndIf}

SectionEnd
