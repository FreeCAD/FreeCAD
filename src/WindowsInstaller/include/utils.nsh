# This script contains the following functions:
#
# - un.DelAppPathSub and UnAppPreSuff,
#    (delete the folder ~\Documents and Settings\username\Application Data\FreeCAD for all users), uses:
#    un.GetParentA
#    un.GetUsers
#    un.StrPoint
#    StrPointer
#    StrPoint
#    UnAppPreSuff
#
# - FileCheck (checks if a given file exists)
#
#--------------------------

!macro StrPointer FindStr SearchStr Pointer
 # searches for a string/character (SearchStr) in another string (FindStr)
 # and returns the number of the character in the FindStr where the SearchStr was found (Pointer)
 # if nothing was found or the search is impossible the Pointer is set to -1
 
 StrLen $R2 ${SearchStr}
 StrLen $R4 ${FindStr}
 StrCpy $R5 0
 ${if} $R2 == 0
 ${orif} $R4 == 0
  Goto NotFound
 ${endif}
 IntCmp $R4 $R2 loopA NotFound
 loopA:
  StrCpy $R3 ${FindStr} $R2 $R5
  StrCmp $R3 ${SearchStr} Found
  IntOp $R5 $R5 + 1
  IntCmp $R4 $R5 loopA NotFound
  Goto loopA
 Found:
  StrCpy ${Pointer} $R5
  Goto done
 NotFound:
  StrCpy ${Pointer} "-1"
 done:

!macroend
 
#--------------------------------

Function StrPoint
 !insertmacro StrPointer $String $Search $Pointer
FunctionEnd

#--------------------------------

!macro RevStrPointer FindStr SearchStr Pointer
 # searches for a string/character (SearchStr) in another string (FindStr) in reverse order
 # and returns the number of the character in the FindStr where the SearchStr was found (Pointer)
 # if nothing was found or the search is impossible the Pointer is set to +1
 
 StrLen $R2 ${SearchStr}
 StrLen $R4 ${FindStr}
 ${if} $R2 == 0
 ${orif} $R4 == 0
  Goto NotFound
 ${endif}
 IntCmp $R4 $R2 loopA NotFound
 StrCpy $R5 "-$R2"
 loopA:
  StrCpy $R3 ${FindStr} $R2 $R5
  StrCmp $R3 ${SearchStr} Found
  IntOp $R5 $R5 - 1
  IntCmp "$R5" "-$R4" loopA NotFound
  Goto loopA
 Found:
  StrCpy ${Pointer} $R5
  Goto done
 NotFound:
  StrCpy ${Pointer} "+1"
 done:

!macroend
 
#--------------------------------

!macro AppPreSuff AppPre AppSuff
 # the APPDATA path for a local user has for WinXP and 2000 the following structure:
 # C:\Documents and Settings\username\Application Data
 # for Win Vista the structure is:
 # C:\Users\username\AppData\Roaming
 # this macro saves the "C:\Documents and Settings\" substring into the variable "AppPre"
 # and the "Application Data" substring into the variable "AppSuff"
  
  # switch temporarily to local user because the all users application data path is in
  # Vista only C:\ProgramData 
  SetShellVarContext current
  StrCpy $String "$APPDATA"
  Var /GLOBAL APPDATemp
  StrCpy $APPDATemp "$APPDATA"
  ${If} $MultiUser.Privileges == "Admin"
   ${OrIf} $MultiUser.Privileges == "Power"
    SetShellVarContext all # move back to all users
  ${endif}
  StrCpy $Search "\"
  Call StrPoint # search for the first "\"
  IntOp $Pointer $Pointer + 1 # jump after the "\"
  StrCpy $String $String "" $Pointer # cut off the part before the first "\"
  StrCpy $0 $Pointer
  Call StrPoint # search for the second "\"
  IntOp $0 $0 + $Pointer # $0 is now the pointer to the second "\" in the APPDATA string
  StrCpy ${AppPre} $APPDATemp $0 # save the part before the second "\"
  IntOp $Pointer $Pointer + 1 # jump after the "\"
  StrCpy $String $String "" $Pointer # cut off the part before the second "\"
  Call StrPoint # search for the third "\"
  IntOp $Pointer $Pointer + 1 # jump after the "\"
  StrCpy ${AppSuff} $String "" $Pointer # save the part after the third "\"

!macroend

#--------------------------------

Function un.GetParentA
 # deletes a subfolder of the APPDATA path for all users
 # used by the function "un.getUsers"

  Exch $R0
  Push $R1
  Push $R2
  Push $R3
  StrCpy $R1 0
  StrLen $R2 $R0
  loop:
   IntOp $R1 $R1 + 1
   IntCmp $R1 $R2 get 0 get
   StrCpy $R3 $R0 1 -$R1
   StrCmp $R3 "\" get
  Goto loop
  get:
   StrCpy $R0 $R0 -$R1
   Pop $R3
   Pop $R2
   Pop $R1
   Exch $R0
   
FunctionEnd

#--------------------------------

Function un.GetUsers
 # reads the subfolders of the "Documents and Settings" folder to get a list of the users

  StrCpy $R3 ""
  Push "$PROFILE"
  Call un.GetParentA
  Pop $R2
  StrCpy $R2 "$R2"
  FindFirst $R0 $R1 "$R2\*"
  StrCmp $R1 "" findend 0
  findloop:
   IfFileExists "$R2\$R1\*.*" 0 notDir
   StrCmp $R1 "." notDir
   StrCmp $R1 ".." notDir
   StrCmp $R1 "All Users" notDir
   StrCmp $R1 "Default User" notDir
   StrCmp $R1 "All Users.WINNT" notDir
   StrCmp $R1 "Default User.WINNT" notDir  
  StrCpy $R3 "$R3|$R1"
  notDir:
   FindNext $R0 $R1
   StrCmp $R1 "" findend 0
  Goto findloop
  findend:
   FindClose $R0
  
FunctionEnd

#--------------------------------

Function un.StrPoint
 !insertmacro StrPointer $String $Search $Pointer
FunctionEnd

#--------------------------------

!macro UnAppPreSuff AppPre AppSuff
 # the APPDATA path for a local user has for WinXP and 2000 the following structure:
 # C:\Documents and Settings\username\Application Data
 # for Win Vista the structure is:
 # C:\Users\username\AppData\Roaming
 # this macro saves the "C:\Documents and Settings\" substring into the variable "AppPre"
 # and the "Application Data" substring into the variable "AppSuff"
  
  SetShellVarContext current # switch temoprarily to local user
  StrCpy $String "$APPDATA"
  StrCpy $APPDATemp "$APPDATA"
  ${if} $MultiUser.Privileges == "Admin"
  ${orif} $MultiUser.Privileges == "Power"
   SetShellVarContext all # move back to all users
  ${endif}
  StrCpy $Search "\"
  Call un.StrPoint # search for the first "\"
  IntOp $Pointer $Pointer + 1 # jump after the "\"
  StrCpy $String $String "" $Pointer # cut off the part before the first "\"
  StrCpy $0 $Pointer
  Call un.StrPoint # search for the second "\"
  IntOp $0 $0 + $Pointer # $0 is now the pointer to the second "\" in the APPDATA string
  StrCpy ${AppPre} $APPDATemp $0 # save the part before the second "\"
  IntOp $Pointer $Pointer + 1 # jump after the "\"
  StrCpy $String $String "" $Pointer # cut off the part before the second "\"
  Call un.StrPoint # search for the third "\"
  IntOp $Pointer $Pointer + 1 # jump after the "\"
  StrCpy ${AppSuff} $String "" $Pointer # save the part after the third "\"

!macroend

#--------------------------------

Function un.DelAppPathSub
 # deletes a subfolder of the APPDATA path for all users

  # get list of all users
  Push $R0
  Push $R1
  Push $R2
  Push $R3
  Call un.GetUsers
  StrCpy $UserList $R3 "" 1 # cut off the "|" at the end of the list
  Pop $R3
  Pop $R2
  Pop $R1
  Pop $R0
  
  # the usernames in the list of all users is separated by "|"
  loop:
   StrCpy $String "$UserList"
   StrCpy $Search "|"
   Call un.StrPoint # search for the "|"
   StrCmp $Pointer "-1" ready
   StrCpy $0 $UserList $Pointer # $0 contains now the username
   IntOp $Pointer $Pointer + 1 # jump after the "|"
   StrCpy $UserList $UserList "" $Pointer # cut off the first username in the list
   # generate the string for the current user
   # AppPre and AppSuff are generated in the macro "AppPreSuff"
   RMDir /r "$AppPre\$0\$AppSuff\$AppSubfolder" # delete the folder
  Goto loop
  ready:
  StrCpy $0 $UserList
  RMDir /r "$AppPre\$0\$AppSuff\$AppSubfolder" # delete the folder
  
FunctionEnd

#--------------------------------

!macro FileCheck Result FileName FilePath
 # checks if a file exists, returns "True" or "False"

 Push $0
 Push $1
 StrCpy $0 ""
 StrCpy $1 ""
 FileOpen $0 "${FilePath}\${FileName}" r
 ${if} $0 = ""
  StrCpy $1 "False"
 ${Else}
  StrCpy $1 "True"
 ${endif}
 FileClose $0
 StrCpy ${Result} $1
 Pop $1
 Pop $0

!macroend
