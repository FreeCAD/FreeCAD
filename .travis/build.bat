echo on

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" amd64 

MSBuild.exe FreeCAD.sln /p:CLToolExe=clcache.exe /p:TrackFileAccess=false /p:CLToolPath=C:\Users\travis\build\FreeCAD\FreeCAD /m:2 /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=x64

