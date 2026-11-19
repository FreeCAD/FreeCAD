Var RegisteredDir

Function ReadRegisteredDir
  ReadRegStr $RegisteredDir SHCTX "${APP_REGKEY}" ""
  ReadRegStr $0 SHCTX "${APP_UNINST_KEY}" "UninstallString"
  ${If} $0 != '"$RegisteredDir\${SETUP_UNINSTALLER}"'
    StrCpy $RegisteredDir ""
  ${EndIf}
FunctionEnd

Function CheckLegacyBase
  ClearErrors
  FindFirst $3 $4 "$RegisteredDir\*.*"
  ${If} ${Errors}
    Goto blocked
  ${EndIf}

  # Legacy uninstallers remove the whole tree. Reject unknown top-level entries.
  ${DoWhile} $4 != ""
    StrCmp $4 "." next
    StrCmp $4 ".." next
    !insertmacro ReadPathAttrs "$RegisteredDir\$4"
    StrCmp $0 -1 invalidEntry
    IntOp $0 $0 & ${FILE_ATTRIBUTE_DIRECTORY}
    ${If} $0 = ${FILE_ATTRIBUTE_DIRECTORY}
      !insertmacro MatchPayloadDir $4 next
      StrCmp $4 "thumbnail" next
    ${ElseIf} $0 = 0
      StrCmp $4 "${SETUP_UNINSTALLER}" next
      StrCmp $4 "install.log" next
    ${EndIf}

    invalidEntry:
    FindClose $3
    Goto blocked

    next:
      FindNext $3 $4
  ${Loop}
  FindClose $3
  Return

  blocked:
    Rename "$RegisteredDir\${SETUP_UNINSTALLER}" "$RegisteredDir\${SETUP_UNINSTALLER}.disabled"
    MessageBox MB_OK|MB_ICONSTOP "$(UnsafeLegacyFolder)" /SD IDOK
    SetErrorLevel 1
    Abort
FunctionEnd

Function ReplaceRegistered
  Call ReadRegisteredDir
  ${If} $RegisteredDir == ""
  ${OrIf} $RegisteredDir != "$INSTDIR"
    Return
  ${EndIf}
  !insertmacro ReadPathAttrs "$RegisteredDir\${SETUP_UNINSTALLER}"
  ${If} $0 = -1
    !insertmacro ReadPathAttrs "$RegisteredDir\${SETUP_UNINSTALLER}.disabled"
    IntOp $0 $0 & ${FILE_ATTRIBUTE_DIRECTORY}
    ${If} $0 != 0
      Goto failed
    ${EndIf}

    # The caller still rejects the nonempty folder of a disabled copy.
    Return
  ${EndIf}
  IntOp $0 $0 & ${FILE_ATTRIBUTE_DIRECTORY}
  ${If} $0 != 0
    Goto failed
  ${EndIf}

  ${If} ${RunningX64}
    # Older uninstallers read installation paths before selecting the 64-bit view.
    SetRegView 32
    ReadRegStr $0 HKLM "${APP_REGKEY}" ""
    ReadRegStr $1 HKCU "${APP_REGKEY}" ""
    SetRegView 64
    ${If} $0 != ""
    ${AndIf} $0 != $RegisteredDir
      Goto failed
    ${EndIf}
    ${If} $1 != ""
    ${AndIf} $1 != $RegisteredDir
      Goto failed
    ${EndIf}
  ${EndIf}

  ReadRegDWORD $0 SHCTX "${APP_UNINST_KEY}" "UninstallReceipt"
  ${If} $0 != 1
    Call CheckLegacyBase
  ${EndIf}

  # Wait for the real uninstaller without locking the file it needs to delete.
  InitPluginsDir
  ClearErrors
  CopyFiles /SILENT "$RegisteredDir\${SETUP_UNINSTALLER}" "$PLUGINSDIR"
  ${If} ${Errors}
    Goto failed
  ${EndIf}
  SetOutPath "$PLUGINSDIR"
  ExecWait '"$PLUGINSDIR\${SETUP_UNINSTALLER}" /S /$MultiUser.InstallMode _?=$RegisteredDir' $0
  ${If} ${Errors}
  ${OrIf} $0 != 0
    Goto failed
  ${EndIf}

  ClearErrors
  EnumRegValue $0 SHCTX "${APP_UNINST_KEY}" 0
  ${IfNot} ${Errors}
    Goto failed
  ${EndIf}
  Return

  failed:
    MessageBox MB_OK|MB_ICONSTOP "$(RegisteredUninstallFailed)" /SD IDOK
    SetErrorLevel 1
    Abort
FunctionEnd
