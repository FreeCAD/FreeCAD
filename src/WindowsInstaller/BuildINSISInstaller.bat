
if NOT DEFINED NSIS set NSIS=C:\Program Files (x86)\NSIS

rem in order to build an x64 installer set PLATFORM to x64 
if not defined PLATFORM set PLATFORM=x86

C:\Python26\python.exe ../Tools/WinVersion.py --dir=../.. --src=Version.nsi.in --out=Version.nsi
C:\Python26\python.exe ../Tools/WinVersion.py --dir=../.. --src=../Build/Version.h.in   --out=../Build/Version.h

SET /P M=Reebuild and press enter 

"%NSIS%\makensis.exe" FreeCAD_WindowsInstaller.nsi

