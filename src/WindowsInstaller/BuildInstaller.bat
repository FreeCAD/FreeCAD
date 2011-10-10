
if NOT DEFINED WIXDIR set WIXDIR=C:\Program Files (x86)\Windows Installer XML v3

rem in order to build an x64 installer set PLATFORM to x64 
if not defined PLATFORM set PLATFORM=x86

"C:\Program Files\TortoiseSVN\bin\SubWCRev.exe" ..\.. Version.wxi.in Version.wxi
"C:\Program Files\TortoiseSVN\bin\SubWCRev.exe" ..\.. CopyRelease.bat.in CopyRelease.bat
"C:\Program Files\TortoiseSVN\bin\SubWCRev.exe" ..\.. ..\Build\Version.h.in ..\Build\Version.h

SET /P M=Reebuild and press enter 

"%WIXDIR%\bin\candle.exe" -dProcessorArchitecture=%PLATFORM% -out FreeCADBase.wxobj    FreeCADBase.wxs
"%WIXDIR%\bin\candle.exe" -dProcessorArchitecture=%PLATFORM% -out LibPack.wxobj        LibPack.wxs
"%WIXDIR%\bin\candle.exe" -dProcessorArchitecture=%PLATFORM% -out FreeCADDoc.wxobj     FreeCADDoc.wxs
"%WIXDIR%\bin\candle.exe" -dProcessorArchitecture=%PLATFORM% -out FreeCADModules.wxobj FreeCADModules.wxs
"%WIXDIR%\bin\candle.exe" -dProcessorArchitecture=%PLATFORM% -out FreeCADData.wxobj    FreeCADData.wxs
"%WIXDIR%\bin\candle.exe" -dProcessorArchitecture=%PLATFORM% -out FreeCAD.wxobj        FreeCAD.wxs

"%WIXDIR%\bin\light.exe"  -dWixUIBannerBmp=Bitmaps/BanerBitmap.bmp -dWixUIDialogBmp=Bitmaps/BackgroundBitmap.bmp -ext WixUIExtension -sice:ICE03 -sice:ICE60 -sice:ICE82 -sice:ICE83 -cultures:en-us -out FreeCAD.msi *.wxobj 

rem making of the bin zip file

"c:\Program Files\7-Zip\7z.exe" a -t7z FreeCAD.7z "-xr!*.idb" "-xr!*.pdb" "-xr!*.ilk" "-xr!*.rule" "-xr!*.svn-base" "-xr!*.pyc" "-xr!*.stamp" "-xr!*.cmake" "-xr!*.svn*" "-xr!*.vcproj" "-xr!*.am" ..\..\bin ..\..\Mod ..\..\Doc ..\..\data 

call CopyRelease.bat

del FreeCAD.7z
del FreeCAD.msi