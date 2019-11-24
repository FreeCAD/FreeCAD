!macro LANG LANG_NAME
  # NSIS language file
  !insertmacro MUI_LANGUAGE "${LANG_NAME}"
  # FreeCAD language file
  !insertmacro LANGFILE_INCLUDE_WITHDEFAULT "lang\${LANG_NAME}.nsh" "lang\english.nsh"
!macroend

# list of all languages the installer is translated to
!insertmacro LANG "english" # first language is the default
!insertmacro LANG "arabic"
!insertmacro LANG "basque"
!insertmacro LANG "catalan"
!insertmacro LANG "czech"
!insertmacro LANG "danish"
!insertmacro LANG "dutch"
!insertmacro LANG "french"
!insertmacro LANG "german"
!insertmacro LANG "galician"
!insertmacro LANG "hungarian"
!insertmacro LANG "indonesian"
!insertmacro LANG "italian"
!insertmacro LANG "japanese"
!insertmacro LANG "norwegian"
!insertmacro LANG "polish"
!insertmacro LANG "portuguese"
!insertmacro LANG "portugueseBR"
!insertmacro LANG "romanian"
!insertmacro LANG "russian"
!insertmacro LANG "slovak"
!insertmacro LANG "spanish"
!insertmacro LANG "swedish"
!insertmacro LANG "turkish"
!insertmacro LANG "ukrainian"
