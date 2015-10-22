
rem set up Version Number
call ..\src\Build\BuildVersion.bat

rem Build FreeCAD
call ..\BuildAll.bat

rem installer
call BuildInstaller.bat

@pause


