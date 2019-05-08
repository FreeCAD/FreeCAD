echo on

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64 

MSBuild.exe FreeCAD_Trunk.sln /p:CLToolExe=clcache.exe /p:TrackFileAccess=false /p:CLToolPath=C:\Users\travis\build\FreeCAD\FreeCAD /m:2 /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=x64

echo on

xcopy C:\Users\travis\build\FreeCAD\FreeCAD\FreeCADLibs\bin C:\Users\travis\build\FreeCAD\FreeCAD\build\bin /E /Q

C:\Users\travis\build\FreeCAD\FreeCAD\build\bin\FreeCADCmd.exe --run-test 0
