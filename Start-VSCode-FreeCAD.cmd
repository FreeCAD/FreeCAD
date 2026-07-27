@echo off
setlocal

set "WORKSPACE=C:\Users\askoh\Documents\GitHub\aiksiongkoh\FreeCAD\feature-mbdfem"
set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"

if not exist "%VSDEVCMD%" (
    set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
)

call "%VSDEVCMD%" -arch=x64 -host_arch=x64
cd /d "%WORKSPACE%"
code --new-window .
