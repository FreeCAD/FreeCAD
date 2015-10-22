@echo off
rem   Build script, uses vcbuild to completetly build FreeCAD

rem start again nice (LOW)
if "%1"=="" (
    start /WAIT /LOW /B cmd.exe /V /C %~s0 go_ahead
    goto:eof
)
rem  set the aprobiated Variables here or outside in the system
if NOT DEFINED VCDIR set VCDIR=C:\Program Files\Microsoft Visual Studio 9.0

rem Register VS Build programms
call "%VCDIR%\VC\vcvarsall.bat"

rem "C:\Program Files\TortoiseSVN\bin\TortoiseProc.exe" /command:update /path:"C:\SW_Projects\CAD\FreeCAD_10" /closeonend:3


rem Start the Visuall Studio build process
"%VCDIR%\VC\vcpackages\vcbuild.exe" FreeCAD_trunk.sln "Debug|Win32" 
"%VCDIR%\VC\vcpackages\vcbuild.exe" FreeCAD_trunk.sln "Debug|Win32" 
"%VCDIR%\VC\vcpackages\vcbuild.exe" FreeCAD_trunk.sln "Release|Win32" 
