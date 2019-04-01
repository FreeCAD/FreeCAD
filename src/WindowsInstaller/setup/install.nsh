/*

install.nsh

Installation of program files, dictionaries and external components

*/

#--------------------------------
# Program files

Section -ProgramFiles SecProgramFiles

  # if the $INSTDIR does not contain "FreeCAD" we must add a subfolder to avoid that FreeCAD will e.g.
  # be installed directly to C:\programs - the uninstaller will then delete the whole
  # C:\programs directory
  StrCpy $String $INSTDIR
  StrCpy $Search "FreeCAD"
  Call StrPoint # function from Utils.nsh
  ${if} $Pointer == "-1"
   StrCpy $INSTDIR "$INSTDIR\${APP_DIR}"
  ${endif}

   # Install and register the core FreeCAD files
  
  # Initializes the plug-ins dir ($PLUGINSDIR) if not already initialized.
  # $PLUGINSDIR is automatically deleted when the installer exits.
  InitPluginsDir
  
  # Binaries
  SetOutPath "$INSTDIR"
  # recursively copy all files under bin
  File /r "${FILES_FREECAD}\bin"
  
  # MSVC redistributable DLLs
  SetOutPath "$INSTDIR\bin"
  File "${FILES_DEPS}\*.*"
  
  # Others
  SetOutPath "$INSTDIR"
  File /r "${FILES_FREECAD}\data"
  File /r "${FILES_FREECAD}\doc"
  File /r "${FILES_FREECAD}\Ext"
  File /r "${FILES_FREECAD}\lib"
  File /r "${FILES_FREECAD}\Mod"
    
  # Create uninstaller
  WriteUninstaller "$INSTDIR\${SETUP_UNINSTALLER}"

SectionEnd
