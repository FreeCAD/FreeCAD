# Microsoft Developer Studio Project File - Name="coin4" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=coin4 - Win32 DLL (Release)
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "coin4.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "coin4.mak" CFG="coin4 - Win32 DLL (Debug)"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "coin4 - Win32 LIB (Release)" (based on "Win32 (x86) Static Library")
!MESSAGE "coin4 - Win32 LIB (Debug)" (based on "Win32 (x86) Static Library")
!MESSAGE "coin4 - Win32 DLL (Release)" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "coin4 - Win32 DLL (Debug)" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "coin4 - Win32 LIB (Release)"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "StaticRelease"
# PROP BASE Intermediate_Dir "StaticRelease"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "StaticRelease"
# PROP Intermediate_Dir "StaticRelease"
# PROP Target_Dir ""
MTL=midl.exe
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /Ox /Gy /Zi /I "." /I "include" /I "..\..\include" /I "src" /I "..\..\src" /I "..\..\include\Inventor\annex" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_LIB" /D COIN_DEBUG=0  /D "HAVE_CONFIG_H" /D "YY_NO_UNISTD_H" /D "COIN_INTERNAL" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Ox /Gy /Zi /I "." /I "include" /I "..\..\include" /I "src" /I "..\..\src" /I "..\..\include\Inventor\annex" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_LIB" /D COIN_DEBUG=0  /D "HAVE_CONFIG_H" /D "YY_NO_UNISTD_H" /D "COIN_INTERNAL" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x414 /d "NDEBUG"
# ADD RSC /l 0x414 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /machine:I386 /out:"coin4s.lib"
# ADD LIB32 /nologo /machine:I386 /out:"coin4s.lib"

!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "StaticDebug"
# PROP BASE Intermediate_Dir "StaticDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "StaticDebug"
# PROP Intermediate_Dir "StaticDebug"
# PROP Target_Dir ""
MTL=midl.exe
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /GX /GZ /Od /Zi /I "." /I "include" /I "..\..\include" /I "src" /I "..\..\src" /I "..\..\include\Inventor\annex" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_LIB" /D COIN_DEBUG=1  /D "HAVE_CONFIG_H" /D "YY_NO_UNISTD_H" /D "COIN_INTERNAL" /FD /c
# ADD CPP /nologo /MDd /W3 /GX /GZ /Od /Zi /I "." /I "include" /I "..\..\include" /I "src" /I "..\..\src" /I "..\..\include\Inventor\annex" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_LIB" /D COIN_DEBUG=1  /D "HAVE_CONFIG_H" /D "YY_NO_UNISTD_H" /D "COIN_INTERNAL" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x414 /d "_DEBUG"
# ADD RSC /l 0x414 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /machine:I386 /out:"coin4sd.lib"
# ADD LIB32 /nologo /machine:I386 /out:"coin4sd.lib"

!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Release)"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /Ox /Gy /Zi /I "." /I "include" /I "..\..\include" /I "src" /I "..\..\src" /I "..\..\include\Inventor\annex" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D COIN_DEBUG=0 /D "HAVE_CONFIG_H" /D "COIN_MAKE_DLL" /D "YY_NO_UNISTD_H" /D "COIN_INTERNAL" /FD /c
# ADD CPP /nologo /MD /W3 /GX /Ox /Gy /Zi /I "." /I "include" /I "..\..\include" /I "src" /I "..\..\src" /I "..\..\include\Inventor\annex" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D COIN_DEBUG=0 /D "HAVE_CONFIG_H" /D "COIN_MAKE_DLL" /D "YY_NO_UNISTD_H" /D "COIN_INTERNAL" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RCS=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 opengl32.lib gdi32.lib winmm.lib user32.lib /nologo /dll /release /machine:I386 /pdbtype:sept
# ADD LINK32 opengl32.lib gdi32.lib winmm.lib user32.lib /nologo /dll /release /machine:I386 /pdbtype:sept /out:"coin4.dll" /opt:nowin98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /GZ /Zi /Od /I "." /I "include" /I "..\..\include" /I "src" /I "..\..\src" /I "..\..\include\Inventor\annex" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D COIN_DEBUG=1 /D "HAVE_CONFIG_H" /D "COIN_MAKE_DLL" /D "YY_NO_UNISTD_H" /D "COIN_INTERNAL" /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /GZ /Zi /Od /I "." /I "include" /I "..\..\include" /I "src" /I "..\..\src" /I "..\..\include\Inventor\annex" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D COIN_DEBUG=1 /D "HAVE_CONFIG_H" /D "COIN_MAKE_DLL" /D "YY_NO_UNISTD_H" /D "COIN_INTERNAL" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RCS=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 opengl32.lib gdi32.lib winmm.lib user32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 opengl32.lib gdi32.lib winmm.lib user32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /out:"coin4d.dll" /opt:nowin98

!ENDIF

# Begin Target

# Name "coin4 - Win32 DLL (Release)"
# Name "coin4 - Win32 DLL (Debug)"
# Name "coin4 - Win32 LIB (Release)"
# Name "coin4 - Win32 LIB (Debug)"
# Begin Group "Documents"
# PROP Default_Filter ";txt"
# Begin Source File

SOURCE=..\..\README
# End Source File
# Begin Source File

SOURCE=..\..\README.WIN32
# End Source File
# Begin Source File

SOURCE=..\..\NEWS
# End Source File
# Begin Source File

SOURCE=..\..\RELNOTES
# End Source File
# Begin Source File

SOURCE=..\..\COPYING
# End Source File
# Begin Source File

SOURCE=..\..\THANKS
# End Source File
# End Group
# Begin Group "Template Files"
# PROP Default_Filter "in"
# End Group
# Begin Group "Source Files"
# PROP Default_Filter "c;cpp;ic;icc;h"

# Begin Group "xml/expat sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\xml\expat\xmlparse.c
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\xml\expat"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\xml\expat"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\xml\expat"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\xml\expat"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\xml\expat\xmlrole.c
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\xml\expat"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\xml\expat"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\xml\expat"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\xml\expat"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\xml\expat\xmltok.c
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\xml\expat"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\xml\expat"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\xml\expat"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\xml\expat"
!ENDIF
# End Source File
# End Group
# Begin Group "xml sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\xml\document.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\xml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\xml\element.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\xml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\xml\attribute.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\xml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\xml\entity.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\xml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\xml\utils.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\xml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\xml\path.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\xml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\xml"
!ENDIF
# End Source File
# End Group
# Begin Group "actions sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\actions\SoAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoBoxHighlightRenderAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoCallbackAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoGLRenderAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoGetBoundingBoxAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoGetMatrixAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoGetPrimitiveCountAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoHandleEventAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoLineHighlightRenderAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoPickAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoRayPickAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoReorganizeAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoSearchAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoSimplifyAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoToVRMLAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoToVRML2Action.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoWriteAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoAudioRenderAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\actions"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\actions"
!ENDIF
# End Source File
# End Group
# Begin Group "base sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\base\dict.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\hash.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\heap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\list.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\memalloc.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\rbptree.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\time.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\string.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\dynarray.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\namemap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbBSPTree.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbByteBuffer.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbBox2s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbBox2i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbBox2f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbBox2d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbBox3s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbBox3i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbBox3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbBox3d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbClip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbColor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbColor4f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbCylinder.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbDict.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbDPLine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbDPMatrix.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbDPPlane.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbDPRotation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbHeap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbImage.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbLine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbMatrix.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbName.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbOctTree.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbPlane.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbRotation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbSphere.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbString.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbTesselator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbGLUTessellator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbTime.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec2b.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec2ub.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec2s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec2us.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec2i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec2ui32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec2f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec2d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec3b.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec3ub.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec3s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec3us.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec3i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec3ui32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec3d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec4b.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec4ub.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec4s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec4us.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec4i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec4ui32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec4f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbVec4d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbViewVolume.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbDPViewVolume.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbViewportRegion.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbXfBox3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\base\SbXfBox3d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\base"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\base"
!ENDIF
# End Source File
# End Group
# Begin Group "bundles sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\bundles\SoBundle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\bundles"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\bundles\SoMaterialBundle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\bundles"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\bundles\SoNormalBundle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\bundles"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\bundles\SoVertexAttributeBundle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\bundles"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\bundles\SoTextureCoordinateBundle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\bundles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\bundles"
!ENDIF
# End Source File
# End Group
# Begin Group "caches sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\caches\SoBoundingBoxCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoConvexDataCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoGLCacheList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoGLRenderCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoNormalCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoTextureCoordinateCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoPrimitiveVertexCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoGlyphCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoShaderProgramCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\caches\SoVBOCache.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\caches"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\caches"
!ENDIF
# End Source File
# End Group
# Begin Group "details sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\details\SoDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\details"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\details\SoConeDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\details"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\details\SoCubeDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\details"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\details\SoCylinderDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\details"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\details\SoFaceDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\details"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\details\SoLineDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\details"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\details\SoNodeKitDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\details"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\details\SoPointDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\details"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\details\SoTextDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\details"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\details"
!ENDIF
# End Source File
# End Group
# Begin Group "draggers sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\draggers\SoDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoCenterballDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoDirectionalLightDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoDragPointDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoHandleBoxDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoJackDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoPointLightDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoRotateCylindricalDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoRotateDiscDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoRotateSphericalDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoScale1Dragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoScale2Dragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoScale2UniformDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoScaleUniformDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoSpotLightDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoTabBoxDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoTabPlaneDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoTrackballDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoTransformBoxDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoTransformerDragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoTranslate1Dragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\draggers\SoTranslate2Dragger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\draggers"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\draggers"
!ENDIF
# End Source File
# End Group
# Begin Group "elements/GL sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLCacheContextElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLClipPlaneElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLColorIndexElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLCoordinateElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLDepthBufferElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLDrawStyleElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLEnvironmentElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLLazyElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLLightIdElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLLinePatternElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLLineWidthElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLModelMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLMultiTextureCoordinateElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLMultiTextureEnabledElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLMultiTextureMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLMultiTextureImageElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLNormalElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLPointSizeElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLPolygonOffsetElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLProjectionMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLRenderPassElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLShapeHintsElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLUpdateAreaElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLVBOElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLViewingMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLViewportRegionElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLDisplayList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoGLVertexAttributeElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\GL\SoResetMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements\GL"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements\GL"
!ENDIF
# End Source File
# End Group
# Begin Group "elements sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\elements\SoAccumulatedElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoAmbientColorElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoAnnoText3CharOrientElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoAnnoText3FontSizeHintElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoAnnoText3RenderPrintElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoBBoxModelMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoBumpMapElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoBumpMapCoordinateElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoBumpMapMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoBumpMappingPropertyElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoCacheElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoCacheHintElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoClipPlaneElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoComplexityElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoComplexityTypeElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoCoordinateElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoCreaseAngleElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoCullElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoDecimationPercentageElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoDecimationTypeElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoDepthBufferElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoDiffuseColorElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoDrawStyleElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoEmissiveColorElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoEnvironmentElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoFloatElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoFocalDistanceElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoFontNameElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoFontSizeElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoInt32Element.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoLazyElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoLightAttenuationElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoLightElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoLightModelElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoLinePatternElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoLineWidthElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoLocalBBoxMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoMaterialBindingElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoVertexAttributeBindingElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoMultiTextureCoordinateElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoMultiTextureEnabledElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoMultiTextureMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoMultiTextureImageElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoModelMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoNormalBindingElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoNormalElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoOverrideElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoPickRayElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoPickStyleElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoPointSizeElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoPolygonOffsetElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoProfileCoordinateElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoProfileElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoProjectionMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoReplacedElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoShapeHintsElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoShapeStyleElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoShininessElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoSpecularColorElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoSwitchElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTextOutlineEnabledElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTextureCombineElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTextureCoordinateBindingElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTextureOverrideElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTextureScalePolicyElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTextureScaleQualityElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTextureUnitElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTextureQualityElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTransparencyElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoUnitsElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoViewVolumeElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoViewingMatrixElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoViewportRegionElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoWindowElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoListenerPositionElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoListenerOrientationElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoListenerGainElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoListenerDopplerElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoSoundElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoVertexAttributeElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\elements"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\elements"
!ENDIF
# End Source File
# End Group
# Begin Group "engines sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\engines\SoBoolOperation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoCalculator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoComposeMatrix.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoComposeRotation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoComposeRotationFromTo.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoComposeVec2f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoComposeVec3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoComposeVec4f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoComputeBoundingBox.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoConcatenate.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoConvertAll.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoCounter.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoDecomposeMatrix.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoDecomposeRotation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoDecomposeVec2f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoDecomposeVec3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoDecomposeVec4f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoElapsedTime.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoEngine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoEngineOutput.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoFieldConverter.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoGate.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoInterpolate.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoInterpolateFloat.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoInterpolateRotation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoInterpolateVec2f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoInterpolateVec3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoInterpolateVec4f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoNodeEngine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoOnOff.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoOneShot.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoOutputData.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoSelectOne.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoTimeCounter.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoTransformVec3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoTriggerAny.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoTexture2Convert.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoHeightMapToNormalMap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\evaluator.c
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\evaluator_tab.c
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\engines"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\engines"
!ENDIF
# End Source File
# End Group
# Begin Group "errors sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\errors\error.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\errors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\errors\debugerror.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\errors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\errors\SoDebugError.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\errors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\errors\SoError.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\errors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\errors\SoMemoryError.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\errors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\errors\SoReadError.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\errors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\errors"
!ENDIF
# End Source File
# End Group
# Begin Group "events sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\events\SoButtonEvent.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\events"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\events\SoEvent.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\events"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\events\SoKeyboardEvent.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\events"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\events\SoLocation2Event.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\events"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\events\SoMotion3Event.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\events"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\events\SoMouseButtonEvent.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\events"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\events\SoSpaceballButtonEvent.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\events"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\events"
!ENDIF
# End Source File
# End Group
# Begin Group "fields sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\fields\SoField.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoFieldContainer.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoFieldData.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFBitMask.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFBool.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFColor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFColorRGBA.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFDouble.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFEngine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFEnum.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFFloat.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFInt32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFMatrix.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFName.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFNode.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFPath.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFPlane.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFRotation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFShort.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFString.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFTime.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFUInt32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFUShort.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec2b.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec2s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec2i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec2f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec2d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec3b.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec3s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec3i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec3d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec4b.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec4ub.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec4s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec4us.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec4i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec4ui32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec4f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMFVec4d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoMField.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBitMask.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBool.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBox2s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBox2i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBox2f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBox2d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBox3s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBox3i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBox3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFBox3d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFColor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFColorRGBA.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFDouble.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFEngine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFEnum.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFFloat.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFImage.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFImage3.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFInt32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFMatrix.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFName.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFNode.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFPath.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFPlane.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFRotation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFShort.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFString.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFTime.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFTrigger.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFUInt32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFUShort.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec2b.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec2s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec2i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec2f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec2d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec3b.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec3s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec3i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec3d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec4b.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec4ub.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec4s.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec4us.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec4i32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec4ui32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec4f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSFVec4d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSField.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoGlobalField.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\shared.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fields"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fields"
!ENDIF
# End Source File
# End Group
# Begin Group "fonts sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\fonts\fontlib_wrapper.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\win32.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\freetype.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\fontspec.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\glyph.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\glyph2d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\glyph3d.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\default3dfont.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\default2dfont.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\common.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\fonts"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\fonts"
!ENDIF
# End Source File
# End Group
# Begin Group "glue sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\glue\cg.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\spidermonkey.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\dl.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\gl.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\gl_wgl.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\gl_agl.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\gl_cgl.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\gl_glx.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\GLUWrapper.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\simage_wrapper.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\openal_wrapper.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\win32api.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\zlib.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\bzip2.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\freetype.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\normalization_cubemap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\glue"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\glue"
!ENDIF
# End Source File
# End Group
# Begin Group "io sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\io\SoInput.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoInput_FileInfo.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoInput_Reader.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoOutput.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoOutput_Writer.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoByteStream.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoTranSender.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoTranReceiver.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoWriterefCounter.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\io\gzmemio.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\io"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\io"
!ENDIF
# End Source File
# End Group
# Begin Group "manips sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\manips\SoClipPlaneManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoDirectionalLightManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoPointLightManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoSpotLightManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoTransformManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoCenterballManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoHandleBoxManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoJackManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoTabBoxManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoTrackballManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoTransformBoxManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\SoTransformerManip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\manips\commoncode.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\manips"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\manips"
!ENDIF
# End Source File
# End Group
# Begin Group "misc sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\misc\AudioTools.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\CoinStaticObjectInDLL.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoAudioDevice.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoBase.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoBaseP.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoChildList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoCompactPathList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoContextHandler.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoDB.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoDebug.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoFullPath.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoGenerate.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoGlyph.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoInteraction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoJavaScriptEngine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoLightPath.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoLockManager.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoNormalGenerator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoNotRec.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoNotification.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoPath.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoPick.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoPickedPoint.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoPrimitiveVertex.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoProto.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoProtoInstance.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoSceneManager.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoSceneManagerP.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoShaderGenerator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoState.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoTempPath.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoType.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\CoinResources.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoDBP.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoEventManager.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\misc"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\misc"
!ENDIF
# End Source File
# End Group
# Begin Group "rendering sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\rendering\SoGL.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoGLBigImage.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoGLDriverDatabase.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoGLImage.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoGLCubeMapImage.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoGLNurbs.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoRenderManager.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoRenderManagerP.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoOffscreenRenderer.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoOffscreenCGData.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoOffscreenGLXData.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoOffscreenWGLData.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoVBO.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoVertexArrayIndexer.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\CoinOffscreenGLCanvas.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\rendering"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\rendering"
!ENDIF
# End Source File
# End Group
# Begin Group "lists sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\lists\SbList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SbPList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SbIntList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SbVec3fList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SbStringList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoActionMethodList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoAuditorList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoBaseList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoCallbackList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoDetailList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoEnabledElementsList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoEngineList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoEngineOutputList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoFieldList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoNodeList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoPathList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoPickedPointList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\lists\SoTypeList.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\lists"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\lists"
!ENDIF
# End Source File
# End Group
# Begin Group "nodekits sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\nodekits\SoNodeKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoNodeKitPath.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoNodeKitListPart.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoNodekitCatalog.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoBaseKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoAppearanceKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoCameraKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoInteractionKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoLightKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoSceneKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoSeparatorKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoShapeKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodekits\SoWrapperKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodekits"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodekits"
!ENDIF
# End Source File
# End Group
# Begin Group "navigation sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\navigation\SoCameraUtils.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLNavigation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLNavigationTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLPanTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLRotateTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLSeekTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLSpinTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLZoomTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLDollyTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLMiscTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLFlightControlTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\navigation\SoScXMLMotionTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\navigation"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\navigation"
!ENDIF
# End Source File
# End Group
# Begin Group "nodes sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\nodes\SoAlphaTest.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoAnnotation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoAntiSquish.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoArray.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoBaseColor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoBlinker.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoBumpMap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoBumpMapCoordinate.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoBumpMapTransform.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoCallback.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoCacheHint.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoCamera.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoClipPlane.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoColorIndex.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoComplexity.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoCoordinate3.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoCoordinate4.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoDepthBuffer.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoDirectionalLight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoDrawStyle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoEnvironment.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoEventCallback.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoExtSelection.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoFile.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoFont.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoFontStyle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoFrustumCamera.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoGroup.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoInfo.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoLOD.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoLabel.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoLevelOfDetail.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoLight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoLightModel.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoLinearProfile.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoListener.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoLocateHighlight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoMaterial.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoMaterialBinding.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoMatrixTransform.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoMultipleCopy.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoNode.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoNormal.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoNormalBinding.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoNurbsProfile.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoOrthographicCamera.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoPackedColor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoPathSwitch.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoPendulum.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoPerspectiveCamera.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoPickStyle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoPointLight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoPolygonOffset.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoProfile.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoProfileCoordinate2.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoProfileCoordinate3.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoResetTransform.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoReversePerspectiveCamera.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoRotation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoRotationXYZ.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoRotor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoScale.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoSceneTexture2.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoSceneTextureCubeMap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoSelection.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoSeparator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoShapeHints.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoShuttle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoSpotLight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoSurroundScale.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoSwitch.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTexture.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTexture2.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTexture3.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTexture2Transform.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTexture3Transform.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCombine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinate2.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinate3.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateBinding.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateCube.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateCylinder.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateSphere.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateDefault.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateEnvironment.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateFunction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinatePlane.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateNormalMap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateReflectionMap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCoordinateObject.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureCubeMap.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureMatrixTransform.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureScalePolicy.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTextureUnit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTransform.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTransparencyType.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTransformSeparator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTransformation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoTranslation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoUnits.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoUnknownNode.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoVertexProperty.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoVertexAttribute.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoVertexAttributeBinding.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoWWWAnchor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoWWWInline.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\nodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\nodes"
!ENDIF
# End Source File
# End Group
# Begin Group "shapenodes sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\shapenodes\SoAsciiText.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoCone.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoCube.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoCylinder.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoFaceSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoImage.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoIndexedFaceSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoIndexedLineSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoIndexedMarkerSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoIndexedNurbsCurve.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoIndexedNurbsSurface.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoIndexedPointSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoIndexedShape.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoIndexedTriangleStripSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoLineSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoMarkerSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoNonIndexedShape.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoNurbsCurve.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoNurbsSurface.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoPointSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoQuadMesh.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoShape.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoSphere.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoText2.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoText3.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoTriangleStripSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\SoVertexShape.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\soshape_bigtexture.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\soshape_bumprender.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\soshape_primdata.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\soshape_trianglesort.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shapenodes"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shapenodes"
!ENDIF
# End Source File
# End Group
# Begin Group "projectors sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\projectors\SbCylinderPlaneProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbCylinderProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbCylinderSectionProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbCylinderSheetProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbLineProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbPlaneProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbSpherePlaneProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbSphereProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbSphereSectionProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\projectors\SbSphereSheetProjector.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\projectors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\projectors"
!ENDIF
# End Source File
# End Group
# Begin Group "sensors sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\sensors\SoAlarmSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoDataSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoDelayQueueSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoFieldSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoIdleSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoNodeSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoOneShotSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoPathSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoSensorManager.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoTimerQueueSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\sensors\SoTimerSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\sensors"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\sensors"
!ENDIF
# End Source File
# End Group
# Begin Group "upgraders sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\upgraders\SoUpgrader.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\upgraders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\upgraders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\upgraders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\upgraders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\upgraders\SoPackedColorV20.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\upgraders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\upgraders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\upgraders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\upgraders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\upgraders\SoShapeHintsV10.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\upgraders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\upgraders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\upgraders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\upgraders"
!ENDIF
# End Source File
# End Group
# Begin Group "3ds sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\3ds\3dsLoader.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\3ds"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\3ds"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\3ds"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\3ds"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\3ds\SoStream.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\3ds"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\3ds"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\3ds"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\3ds"
!ENDIF
# End Source File
# End Group
# Begin Group "collision sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\collision\SbTri3f.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\collision"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\collision"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\collision"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\collision"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\collision\SoIntersectionDetectionAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\collision"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\collision"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\collision"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\collision"
!ENDIF
# End Source File
# End Group
# Begin Group "hardcopy sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\hardcopy\HardCopy.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\hardcopy"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\hardcopy\PSVectorOutput.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\hardcopy"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\hardcopy\VectorOutput.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\hardcopy"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\hardcopy\VectorizeAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\hardcopy"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\hardcopy\VectorizeActionP.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\hardcopy"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\hardcopy\VectorizePSAction.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\hardcopy"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\hardcopy"
!ENDIF
# End Source File
# End Group
# Begin Group "shadows sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\shadows\SoShadowGroup.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shadows"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shadows\SoShadowStyle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shadows"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shadows\SoShadowSpotLight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shadows"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shadows\SoShadowDirectionalLight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shadows"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shadows\SoShadowStyleElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shadows"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shadows\SoShadowCulling.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shadows"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shadows\SoGLShadowCullingElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shadows"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shadows"
!ENDIF
# End Source File
# End Group
# Begin Group "geo sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\geo\SoGeo.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SoGeoOrigin.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SoGeoLocation.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SoGeoElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SoGeoSeparator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SoGeoCoordinate.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SbGeoAngle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SbGeoEllipsoid.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SbGeoProjection.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SbPolarStereographic.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\geo\SbUTMProjection.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\geo"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\geo"
!ENDIF
# End Source File
# End Group
# Begin Group "threads sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\threads\common.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\thread.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\mutex.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\rwmutex.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\storage.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\condvar.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\worker.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\wpool.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\recmutex.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\sched.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\sync.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\fifo.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\barrier.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\threads"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\threads"
!ENDIF
# End Source File
# End Group
# Begin Group "shaders sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\shaders\SoFragmentShader.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGeometryShader.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLARBShaderParameter.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLARBShaderObject.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLARBShaderProgram.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLCgShaderObject.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLCgShaderParameter.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLCgShaderProgram.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLSLShaderParameter.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLSLShaderObject.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLSLShaderProgram.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLShaderObject.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLShaderParameter.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLShaderProgram.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLShaderProgramElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoShaderObject.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoShaderParameter.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoShaderProgram.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoShader.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoVertexShader.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\shaders"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\shaders"
!ENDIF
# End Source File
# End Group
# Begin Group "profiler sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\profiler\SoProfiler.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SoProfilerElement.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SoProfilerOverlayKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SoProfilerStats.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SoProfilingReportGenerator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SoProfilerTopEngine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SoScrollingGraphKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SoNodeVisualize.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SoProfilerTopKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SoProfilerVisualizeKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\profiler\SbProfilingData.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\profiler"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\profiler"
!ENDIF
# End Source File
# End Group
# Begin Group "vrml97 sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\vrml97\Anchor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Appearance.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\AudioClip.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Background.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Billboard.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Box.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Collision.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Color.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\ColorInterpolator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Cone.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Coordinate.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\CoordinateInterpolator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Cylinder.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\CylinderSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\DirectionalLight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\DragSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\ElevationGrid.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Extrusion.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Fog.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\FontStyle.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Geometry.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Group.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\ImageTexture.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\IndexedFaceSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\IndexedLine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\IndexedLineSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\IndexedShape.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Init.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Inline.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Interpolator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\LOD.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Light.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Material.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\MovieTexture.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\NavigationInfo.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Normal.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\NormalInterpolator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\OrientationInterpolator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Parent.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\PixelTexture.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\PlaneSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\PointLight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\PointSet.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\PositionInterpolator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\ProximitySensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\ScalarInterpolator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Script.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Sensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Shape.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Sound.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Sphere.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\SphereSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\SpotLight.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Switch.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Text.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Texture.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\TextureCoordinate.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\TextureTransform.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\TimeSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\TouchSensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Transform.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\VertexLine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\VertexPoint.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\VertexShape.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\Viewpoint.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\VisibilitySensor.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\WorldInfo.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\JS_VRMLClasses.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\vrml97"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\vrml97"
!ENDIF
# End Source File
# End Group
# Begin Group "foreignfiles sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\foreignfiles\SoForeignFileKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\foreignfiles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\foreignfiles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\foreignfiles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\foreignfiles"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\foreignfiles\SoSTLFileKit.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\foreignfiles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\foreignfiles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\foreignfiles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\foreignfiles"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\foreignfiles\steel-wrapper.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\foreignfiles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\foreignfiles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\foreignfiles"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\foreignfiles"
!ENDIF
# End Source File
# End Group
# Begin Group "scxml sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\scxml\SbStringConvert.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXML.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLObject.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLEvent.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLEventTarget.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLStateMachine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLDocument.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLScxmlElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLInitialElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLAbstractStateElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLStateElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLParallelElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLFinalElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLOnEntryElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLOnExitElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLTransitionElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLHistoryElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLExecutableElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLEventElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLIfElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLElseIfElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLElseElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLLogElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLDataModelElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLDataElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLAssignElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLValidateElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLSendElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLInvokeElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLParamElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLFinalizeElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLContentElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLAnchorElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLScriptElt.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLEvaluator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLMinimumEvaluator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\eval-minimum.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\eval-minimum-tab.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLXPathEvaluator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\scxml\ScXMLECMAScriptEvaluator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\scxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\scxml"
!ENDIF
# End Source File
# End Group
# Begin Group "soscxml sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\soscxml\ScXMLCoinEvaluator.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\soscxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\soscxml\SoScXMLEvent.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\soscxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\soscxml\SoScXMLStateMachine.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\soscxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\soscxml\eval-coin-tab.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\soscxml"
!ENDIF
# End Source File
# Begin Source File

SOURCE=..\..\src\soscxml\eval-coin.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\soscxml"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\soscxml"
!ENDIF
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\src\tidbits.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug"
!ENDIF
# End Source File
# Begin Group "share/gl sources"
# PROP Default_Filter "c;cpp;ic;icc;h"
# Begin Source File

SOURCE=..\..\src\share\gl\CoinGLPerformance.cpp
!IF  "$(CFG)" == "coin4 - Win32 DLL (Release)"
# PROP Intermediate_Dir "Release\share\gl"
!ELSEIF  "$(CFG)" == "coin4 - Win32 DLL (Debug)"
# PROP Intermediate_Dir "Debug\share\gl"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Release)"
# PROP Intermediate_Dir "StaticRelease\share\gl"
!ELSEIF  "$(CFG)" == "coin4 - Win32 LIB (Debug)"
# PROP Intermediate_Dir "StaticDebug\share\gl"
!ENDIF
# End Source File
# End Group
# End Group
# Begin Group "Public Headers"

# PROP Default_Filter "h;ic;icc"
# Begin Group "Inventor\C\XML headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\C\XML\attribute.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\XML\document.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\XML\element.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\XML\entity.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\XML\path.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\XML\types.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\XML\parser.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\XML\world.h
# End Source File
# End Group
# Begin Group "Inventor\C\base headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\C\base\hash.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\base\heap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\base\memalloc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\base\rbptree.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\base\time.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\base\string.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\base\list.h
# End Source File
# End Group
# Begin Group "Inventor\C\errors headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\C\errors\error.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\errors\debugerror.h
# End Source File
# End Group
# Begin Group "Inventor\C\glue headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\C\glue\gl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\glue\dl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\glue\spidermonkey.h
# End Source File
# End Group
# Begin Group "Inventor\C\threads headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\common.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\thread.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\mutex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\condvar.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\recmutex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\rwmutex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\storage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\worker.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\wpool.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\sched.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\sync.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\fifo.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\threads\barrier.h
# End Source File
# End Group
# Begin Group "Inventor\C headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=include\Inventor\C\basic.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\C\tidbits.h
# End Source File
# End Group
# Begin Group "Inventor\VRMLnodes headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRML.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLAnchor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLAppearance.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLAudioClip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLBackground.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLBillboard.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLBox.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLCollision.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLColor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLColorInterpolator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLCone.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLCoordinate.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLCoordinateInterpolator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLCylinder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLCylinderSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLDirectionalLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLDragSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLElevationGrid.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLExtrusion.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLFog.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLFontStyle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLGeometry.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLGroup.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLImageTexture.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLIndexedFaceSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLIndexedLine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLIndexedLineSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLIndexedShape.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLInline.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLInterpolator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLLOD.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLMacros.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLMaterial.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLMovieTexture.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLNavigationInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLNodes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLNormal.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLNormalInterpolator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLOrientationInterpolator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLParent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLPixelTexture.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLPlaneSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLPointLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLPointSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLPositionInterpolator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLProximitySensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLScalarInterpolator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLScript.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLShape.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLSound.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLSphere.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLSphereSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLSpotLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLSubInterpolator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLSwitch.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLText.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLTexture.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLTextureCoordinate.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLTextureTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLTimeSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLTouchSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLVertexLine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLVertexPoint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLVertexShape.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLViewpoint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLVisibilitySensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\VRMLnodes\SoVRMLWorldInfo.h
# End Source File
# End Group
# Begin Group "Inventor\actions headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoSubAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoActions.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoBoxHighlightRenderAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoCallbackAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoGLRenderAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoGetBoundingBoxAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoGetMatrixAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoGetPrimitiveCountAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoHandleEventAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoLineHighlightRenderAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoPickAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoRayPickAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoReorganizeAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoSearchAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoSimplifyAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoToVRMLAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoToVRML2Action.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoWriteAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\actions\SoAudioRenderAction.h
# End Source File
# End Group
# Begin Group "HardCopy headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\HardCopy\SoHardCopy.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\HardCopy\SoPSVectorOutput.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\HardCopy\SoVectorOutput.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\HardCopy\SoVectorizeAction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\HardCopy\SoVectorizePSAction.h
# End Source File
# End Group
# Begin Group "ForeignFiles headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\ForeignFiles\SoForeignFileKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\ForeignFiles\SoSTLFileKit.h
# End Source File
# End Group
# Begin Group "FXViz\nodes headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\FXViz\nodes\SoShadowGroup.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\FXViz\nodes\SoShadowStyle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\FXViz\nodes\SoShadowDirectionalLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\FXViz\nodes\SoShadowSpotLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\FXViz\nodes\SoShadowCulling.h
# End Source File
# End Group
# Begin Group "FXViz\elements headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\FXViz\elements\SoShadowStyleElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\FXViz\elements\SoGLShadowCullingElement.h
# End Source File
# End Group
# Begin Group "Profiler\nodes headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\nodes\SoProfilerStats.h
# End Source File
# End Group
# Begin Group "Profiler\elements headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\elements\SoProfilerElement.h
# End Source File
# End Group
# Begin Group "Profiler\nodekits headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\nodekits\SoNodeVisualize.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\nodekits\SoProfilerOverlayKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\nodekits\SoProfilerTopKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\nodekits\SoScrollingGraphKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\nodekits\SoProfilerVisualizeKit.h
# End Source File
# End Group
# Begin Group "Profiler\engines headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\engines\SoProfilerTopEngine.h
# End Source File
# End Group
# Begin Group "Profiler\utils headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\utils\SoProfilingReportGenerator.h
# End Source File
# End Group
# Begin Group "Profiler headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\SbProfilingData.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\annex\Profiler\SoProfiler.h
# End Source File
# End Group
# Begin Group "Inventor\bundles headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\bundles\SoBundle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\bundles\SoMaterialBundle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\bundles\SoNormalBundle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\bundles\SoVertexAttributeBundle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\bundles\SoTextureCoordinateBundle.h
# End Source File
# End Group
# Begin Group "Inventor\caches headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\caches\SoBoundingBoxCache.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\caches\SoCache.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\caches\SoConvexDataCache.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\caches\SoGLCacheList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\caches\SoGLRenderCache.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\caches\SoNormalCache.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\caches\SoPrimitiveVertexCache.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\caches\SoTextureCoordinateCache.h
# End Source File
# End Group
# Begin Group "Inventor\collision headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\collision\SoIntersectionDetectionAction.h
# End Source File
# End Group
# Begin Group "Inventor\details headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoSubDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoDetails.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoConeDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoCubeDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoCylinderDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoFaceDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoLineDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoNodeKitDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoPointDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\details\SoTextDetail.h
# End Source File
# End Group
# Begin Group "Inventor\draggers headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoCenterballDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoDirectionalLightDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoDragPointDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoHandleBoxDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoJackDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoPointLightDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoRotateCylindricalDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoRotateDiscDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoRotateSphericalDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoScale1Dragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoScale2Dragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoScale2UniformDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoScaleUniformDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoSpotLightDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoTabBoxDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoTabPlaneDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoTrackballDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoTransformBoxDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoTransformerDragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoTranslate1Dragger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\draggers\SoTranslate2Dragger.h
# End Source File
# End Group
# Begin Group "Inventor\elements headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoSubElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoElements.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoAccumulatedElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoAmbientColorElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoAnnoText3CharOrientElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoAnnoText3FontSizeHintElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoAnnoText3RenderPrintElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoBBoxModelMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoBumpMapElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoBumpMapCoordinateElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoBumpMapMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoCacheElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoCacheHintElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoClipPlaneElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoComplexityElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoComplexityTypeElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoCoordinateElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoCreaseAngleElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoCullElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoDecimationPercentageElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoDecimationTypeElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoDiffuseColorElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoDrawStyleElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoEmissiveColorElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoEnvironmentElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoFloatElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoFocalDistanceElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoFontNameElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoFontSizeElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGeoElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLCacheContextElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLClipPlaneElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLColorIndexElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLCoordinateElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLDisplayList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLDrawStyleElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLEnvironmentElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLLazyElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLLightIdElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLLinePatternElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLLineWidthElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLModelMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLMultiTextureCoordinateElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLMultiTextureEnabledElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLMultiTextureMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLMultiTextureImageElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLNormalElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLNormalizeElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLPointSizeElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLPolygonOffsetElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLProjectionMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLRenderPassElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLShapeHintsElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLShadeModelElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLTextureCoordinateElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLTextureEnabledElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLTexture3EnabledElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLTextureImageElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLTextureMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLUpdateAreaElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLVBOElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLViewingMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLViewportRegionElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoInt32Element.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoLazyElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoLightAttenuationElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoLightElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoLightModelElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoLinePatternElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoLineWidthElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoLocalBBoxMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoLongElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoMaterialBindingElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoVertexAttributeBindingElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoModelMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoMultiTextureCoordinateElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoMultiTextureEnabledElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoMultiTextureMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoMultiTextureImageElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoNormalBindingElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoNormalElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoOverrideElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoPickRayElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoPickStyleElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoPointSizeElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoPolygonOffsetElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoProfileCoordinateElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoProfileElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoProjectionMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoReplacedElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoShapeHintsElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoShapeStyleElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoShininessElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoSpecularColorElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoSwitchElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextOutlineEnabledElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextureCombineElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextureCoordinateBindingElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextureCoordinateElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextureEnabledElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTexture3EnabledElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextureImageElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextureMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextureOverrideElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextureQualityElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTextureUnitElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoTransparencyElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoUnitsElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoViewVolumeElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoViewingMatrixElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoViewportRegionElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoWindowElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoListenerPositionElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoListenerOrientationElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoListenerGainElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoListenerDopplerElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoSoundElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLShaderProgramElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoDepthBufferElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLDepthBufferElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoVertexAttributeElement.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\elements\SoGLVertexAttributeElement.h
# End Source File
# End Group
# Begin Group "Inventor\engines headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoSubEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoSubNodeEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoEngines.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoBoolOperation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoCalculator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoCompose.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoComposeMatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoComposeRotation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoComposeRotationFromTo.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoComposeVec2f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoComposeVec3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoComposeVec4f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoComputeBoundingBox.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoConcatenate.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoCounter.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoDecomposeMatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoDecomposeRotation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoDecomposeVec2f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoDecomposeVec3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoDecomposeVec4f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoElapsedTime.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoEngineOutput.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoFieldConverter.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoGate.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoInterpolate.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoInterpolateFloat.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoInterpolateRotation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoInterpolateVec2f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoInterpolateVec3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoInterpolateVec4f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoNodeEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoOnOff.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoOneShot.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoOutputData.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoSelectOne.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoTimeCounter.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoTransformVec3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoTriggerAny.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoTexture2Convert.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\engines\SoHeightMapToNormalMap.h
# End Source File
# End Group
# Begin Group "Inventor\errors headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\errors\SoErrors.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\errors\SoDebugError.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\errors\SoError.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\errors\SoMemoryError.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\errors\SoReadError.h
# End Source File
# End Group
# Begin Group "Inventor\events headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\events\SoSubEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\events\SoButtonEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\events\SoEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\events\SoEvents.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\events\SoKeyboardEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\events\SoLocation2Event.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\events\SoMotion3Event.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\events\SoMouseButtonEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\events\SoSpaceballButtonEvent.h
# End Source File
# End Group
# Begin Group "Inventor\fields headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSubField.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoFields.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoField.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoFieldContainer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoFieldData.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFBitMask.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFBool.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFColor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFColorRGBA.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFDouble.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFEnum.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFFloat.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFInt32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFLong.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFMatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFName.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFNode.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFPath.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFPlane.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFRotation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFShort.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFString.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFTime.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFUInt32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFULong.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFUShort.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec2b.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec2s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec2i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec2f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec2d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec3b.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec3s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec3i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec3d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec4b.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec4ub.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec4s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec4us.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec4i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec4ui32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec4f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMFVec4d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoMField.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBitMask.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBool.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBox2s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBox2i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBox2f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBox2d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBox3s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBox3i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBox3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFBox3d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFColor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFColorRGBA.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFDouble.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFEnum.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFFloat.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFImage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFImage3.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFInt32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFLong.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFMatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFName.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFNode.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFPath.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFPlane.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFRotation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFShort.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFString.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFTime.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFTrigger.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFUInt32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFULong.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFUShort.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec2b.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec2s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec2i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec2f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec2d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec3b.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec3s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec3i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec3d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec4b.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec4ub.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec4s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec4us.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec4i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec4ui32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec4f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSFVec4d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\fields\SoSField.h
# End Source File
# End Group
# Begin Group "Inventor\lists headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SbList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SbPList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SbIntList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SbVec3fList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SbStringList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoActionMethodList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoAuditorList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoBaseList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoCallbackList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoDetailList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoEnabledElementsList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoEngineList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoEngineOutputList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoFieldList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoNodeList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoPathList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoPickedPointList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\lists\SoTypeList.h
# End Source File
# End Group
# Begin Group "Inventor\lock headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\lock\SoLockMgr.h
# End Source File
# End Group
# Begin Group "Inventor\manips headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoClipPlaneManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoDirectionalLightManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoPointLightManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoSpotLightManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoTransformManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoCenterballManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoHandleBoxManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoJackManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoTabBoxManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoTrackballManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoTransformBoxManip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\manips\SoTransformerManip.h
# End Source File
# End Group
# Begin Group "Inventor\misc headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\misc\CoinResources.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoAuditorList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoBase.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoBasic.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoByteStream.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoCallbackList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoChildList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoContextHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoGLImage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoGLCubeMapImage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoGLBigImage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoNormalGenerator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoNotification.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoNotRec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoProto.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoProtoInstance.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoTranReceiver.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoState.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoTranscribe.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoTranSender.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoLightPath.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoTempPath.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoGlyph.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoAudioDevice.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoScriptEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoJavaScriptEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\misc\SoGLDriverDatabase.h
# End Source File
# End Group
# Begin Group "Inventor\navigation headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLNavigation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLNavigationTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLMiscTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLPanTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLRotateTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLSeekTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLSpinTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLZoomTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLDollyTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLFlightControlTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\navigation\SoScXMLMotionTarget.h
# End Source File
# End Group
# Begin Group "Inventor\nodekits headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoSubKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoNodeKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoNodeKitListPart.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoNodekitCatalog.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoBaseKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoAppearanceKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoCameraKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoInteractionKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoLightKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoSceneKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoSeparatorKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoShapeKit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodekits\SoWrapperKit.h
# End Source File
# End Group
# Begin Group "Inventor\nodes headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoAlphaTest.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoAnnotation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoAntiSquish.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoArray.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoAsciiText.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoBaseColor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoBlinker.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoBumpMap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoBumpMapCoordinate.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoBumpMapTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoCacheHint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoCamera.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoClipPlane.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoColorIndex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoComplexity.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoCone.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoCoordinate3.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoCoordinate4.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoCube.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoCylinder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoDepthBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoDirectionalLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoDrawStyle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoEnvironment.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoEventCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoExtSelection.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoFaceSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoFile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoFont.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoFontStyle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoFrustumCamera.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoGeoOrigin.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoGeoLocation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoGeoSeparator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoGeoCoordinate.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoGroup.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoGeometryShader.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoImage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoIndexedFaceSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoIndexedLineSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoIndexedMarkerSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoIndexedNurbsCurve.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoIndexedNurbsSurface.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoIndexedPointSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoIndexedShape.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoIndexedTriangleStripSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoLOD.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoLabel.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoLevelOfDetail.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoLightModel.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoLineSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoLinearProfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoListener.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoLocateHighlight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoMarkerSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoMaterial.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoMaterialBinding.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoMatrixTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoMultipleCopy.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoNode.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoNodes.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoNonIndexedShape.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoNormal.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoNormalBinding.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoNurbsCurve.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoNurbsProfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoNurbsSurface.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoOrthographicCamera.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoPackedColor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoPathSwitch.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoPendulum.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoPerspectiveCamera.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoPickStyle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoPointLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoPointSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoPolygonOffset.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoProfile.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoProfileCoordinate2.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoProfileCoordinate3.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoQuadMesh.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoResetTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoReversePerspectiveCamera.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoRotation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoRotationXYZ.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoRotor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoScale.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoSceneTexture2.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoSceneTextureCubeMap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoSelection.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoSeparator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoShape.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoShapeHints.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoShuttle.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoSphere.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoSpotLight.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoSubNode.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoSurroundScale.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoSwitch.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoText2.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoText3.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTexture.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTexture2.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTexture2Transform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTexture3.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTexture3Transform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCombine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinate2.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinate3.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateBinding.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateCube.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateCylinder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateSphere.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateDefault.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateEnvironment.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateFunction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinatePlane.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateNormalMap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateReflectionMap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCoordinateObject.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureCubeMap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureMatrixTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureScalePolicy.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTextureUnit.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTransformSeparator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTransformation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTranslation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTransparencyType.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoTriangleStripSet.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoUnits.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoVertexProperty.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoVertexAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoVertexAttributeBinding.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoVertexShape.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoWWWAnchor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoWWWInline.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoFragmentShader.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoShaderObject.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoShaderParameter.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoShaderProgram.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\nodes\SoVertexShader.h
# End Source File
# End Group
# Begin Group "Inventor\projectors headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbProjectors.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbCylinderPlaneProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbCylinderProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbCylinderSectionProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbCylinderSheetProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbLineProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbPlaneProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbSpherePlaneProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbSphereProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbSphereSectionProjector.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\projectors\SbSphereSheetProjector.h
# End Source File
# End Group
# Begin Group "Inventor\sensors headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoSensors.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoAlarmSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoDataSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoDelayQueueSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoFieldSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoIdleSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoNodeSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoOneShotSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoPathSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoSensorManager.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoTimerQueueSensor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\sensors\SoTimerSensor.h
# End Source File
# End Group
# Begin Group "Inventor\system headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\system\gl.h
# End Source File
# Begin Source File

SOURCE=include\Inventor\system\gl-headers.h
# End Source File
# Begin Source File

SOURCE=include\Inventor\system\inttypes.h
# End Source File
# End Group
# Begin Group "Inventor\threads headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbThread.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbMutex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbThreadMutex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbRWMutex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbCondVar.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbStorage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbTypedStorage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbFifo.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbBarrier.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\threads\SbThreadAutoLock.h
# End Source File
# End Group
# Begin Group "Inventor\tools headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\tools\SbPimplPtr.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\tools\SbPimplPtr.hpp
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\tools\SbLazyPimplPtr.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\tools\SbLazyPimplPtr.hpp
# End Source File
# End Group
# Begin Group "Inventor\scxml headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXML.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLSubObject.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLObject.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLEventTarget.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLStateMachine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLDocument.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLAbstractStateElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLExecutableElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLAnchorElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLAssignElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLContentElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLDataElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLDataModelElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLElseElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLElseIfElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLEventElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLFinalElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLFinalizeElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLHistoryElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLIfElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLInitialElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLInvokeElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLLogElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLOnEntryElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLOnExitElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLParallelElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLParamElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLScriptElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLScxmlElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLSendElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLStateElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLTransitionElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLValidateElt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLEvaluator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLMinimumEvaluator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLXPathEvaluator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLECMAScriptEvaluator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\ScXMLCoinEvaluator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\SoScXMLEvent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\scxml\SoScXMLStateMachine.h
# End Source File
# End Group
# Begin Group "Inventor headers"
# Set Default_Filter "h"
# Begin Source File

SOURCE=..\..\include\Inventor\Sb.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBSPTree.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBasic.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBox.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBox2s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBox2i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBox2f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBox2d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBox3d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBox3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBox3i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbBox3s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbByteBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbByteBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbClip.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbColor.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbColor4f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbColorRGBA.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbCylinder.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbDPLine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbDPLinear.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbDPMatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbDPPlane.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbDPRotation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbDPViewVolume.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbDict.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbHeap.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbImage.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbLine.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbLinear.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbMatrix.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbName.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbOctTree.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbPList.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbPlane.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbRotation.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbSphere.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbString.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbTesselator.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbTime.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbTypeInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec2b.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec2ub.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec2s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec2us.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec2i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec2ui32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec2f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec2d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec3b.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec3ub.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec3s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec3us.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec3i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec3ui32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec3d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec4b.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec4ub.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec4s.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec4us.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec4i32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec4ui32.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec4f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbVec4d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbViewVolume.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbViewportRegion.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbXfBox3f.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SbXfBox3d.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\So.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoDB.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoFullPath.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoInput.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoInteraction.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoLists.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoNodeKitPath.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoOffscreenRenderer.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoOutput.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoPath.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoPickedPoint.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoPrimitiveVertex.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoSceneManager.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoRenderManager.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoEventManager.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\SoType.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\non_winsys.h
# End Source File
# Begin Source File

SOURCE=..\..\include\Inventor\oivwin32.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\include\SoWinEnterScope.h
# End Source File
# Begin Source File

SOURCE=..\..\include\SoWinLeaveScope.h
# End Source File
# Begin Source File

SOURCE=..\..\include\SoDebug.h
# End Source File
# End Group
# Begin Group "Private Headers"

# PROP Default_Filter "h;ic;icc"
# Begin Group "draggerDefaults local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=.\data\draggerDefaults\centerballDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\directionalLightDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\dragPointDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\handleBoxDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\jackDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\pointLightDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\rotateCylindricalDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\rotateDiscDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\rotateSphericalDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\scale1Dragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\scale2Dragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\scale2UniformDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\scaleUniformDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\spotLightDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\tabBoxDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\tabPlaneDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\trackballDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\transformBoxDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\transformerDragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\translate1Dragger.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\draggerDefaults\translate2Dragger.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "lights local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=.\data\shaders\lights\DirSpotLight.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\shaders\lights\DirectionalLight.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\shaders\lights\PointLight.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\data\shaders\lights\SpotLight.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "vsm local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=.\data\shaders\vsm\VsmLookup.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "src local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=.\src\config-debug.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\config-release.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\config.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\setup.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "actions local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\actions\SoActionP.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\actions\SoSubActionP.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "base local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\base\SbGLUTessellator.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\base\dict.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\base\dictp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\base\hashp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\base\heapp.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "caches local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\caches\SoGlyphCache.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "src local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\coindefs.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "elements local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\elements\SoTextureScalePolicyElement.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\elements\SoTextureScaleQualityElement.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "engines local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\engines\SoConvertAll.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoSubEngineP.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\SoSubNodeEngineP.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\evaluator.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\engines\so_eval.ic

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "fields local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\fields\SoGlobalField.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fields\SoSubFieldP.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "fonts local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\fonts\builtin2dfonts.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\common.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\defaultfonts.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\font13.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\font17.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\font25.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\font33.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\fontlib_wrapper.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\fontspec.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\freetype.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\glyph.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\glyph2d.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\glyph3d.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\fonts\win32.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "glue local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\glue\GLUWrapper.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\bzip2.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\cg.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\dlp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\freetype.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\gl_agl.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\gl_cgl.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\gl_glx.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\gl_wgl.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\glp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\openal_wrapper.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\simage_wrapper.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\win32api.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\glue\zlib.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "io local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\io\SoInputP.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoInput_FileInfo.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoInput_Reader.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoOutput_Writer.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\io\SoWriterefCounter.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "misc local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\misc\AudioTools.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\CoinStaticObjectInDLL.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SbHash.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoBaseP.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoCompactPathList.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoDBP.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoGenerate.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoPick.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\SoShaderGenerator.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\cppmangle.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\misc\systemsanity.icc

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "nodekits local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\nodekits\SoSubKitP.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "nodes local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\nodes\SoSoundElementHelper.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoSubNodeP.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\nodes\SoUnknownNode.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "profiler local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\profiler\inventormaps.icc

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "rendering local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\rendering\CoinOffscreenGLCanvas.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoGL.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoGLNurbs.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoOffscreenCGData.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoOffscreenGLXData.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoOffscreenWGLData.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoVBO.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\rendering\SoVertexArrayIndexer.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "shaders local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\shaders\SoGLARBShaderObject.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLARBShaderParameter.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLARBShaderProgram.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLCgShaderObject.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLCgShaderParameter.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLCgShaderProgram.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLSLShaderObject.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLSLShaderParameter.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLSLShaderProgram.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLShaderObject.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLShaderParameter.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoGLShaderProgram.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shaders\SoShader.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "shapenodes local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\shapenodes\soshape_bigtexture.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\soshape_primdata.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\shapenodes\soshape_trianglesort.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "threads local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\threads\barrierp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\condvar_pthread.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\condvar_win32.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\condvarp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\fifop.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\mutex_pthread.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\mutex_win32cs.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\mutex_win32mutex.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\mutexp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\recmutexp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\rwmutexp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\schedp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\storagep.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\syncp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\thread_pthread.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\thread_win32.icc

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\threadp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\threadsutilp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\workerp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\threads\wpoolp.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "src local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\tidbitsp.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\unconfig.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "upgraders local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\upgraders\SoPackedColorV20.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\upgraders\SoShapeHintsV10.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\upgraders\SoUpgrader.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "vrml97 local includes"

# PROP Default_Filter "h;ic;icc"
# Begin Source File

SOURCE=..\..\src\vrml97\JS_VRMLClasses.h

# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\src\vrml97\SoVRMLSubInterpolatorP.h

# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Group
# End Target
# End Project
