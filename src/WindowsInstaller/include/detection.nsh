/*

detection.nsh

Detection of external component locations

*/

# This script contains the following functions:
#
# - SearchExternal, calls the functions:
#    MissingPrograms
#
# - MissingPrograms, (check if third-party programs are installed), uses:
#    SEARCH_MIKTEX
#    SEARCH_TEXLIVE
#
#--------------------------

Function SearchExternal
  Call MissingPrograms
FunctionEnd

# ---------------------------------------

Function MissingPrograms
  # check if third-party programs are installed

 ${if} ${RunningX64}
   SetRegView 64
  ${endif}

  # test if Inkscape is installed
  ReadRegStr $SVGPath HKLM "SOFTWARE\Classes\inkscape.svg\DefaultIcon" ""
  ${if} $SVGPath != ""
   StrCpy $SVGPath $SVGPath "" 1 # remove the leading quote
   StrCpy $SVGPath $SVGPath -14 # # delete '\inkscape.exe"'
  ${endif}
  ${if} $SVGPath == ""
   # this was used before Inkscape 0.91:
   ReadRegStr $SVGPath HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Inkscape" "InstallLocation"
  ${endif}

FunctionEnd

# ---------------------------------------
