# Microsoft Developer Studio Project File - Name="FCThumbs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=FCThumbs - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FCThumbs.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FCThumbs.mak" CFG="FCThumbs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FCThumbs - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "FCThumbs - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FCThumbs - Win32 Release"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../" /I "../../3rdParty/CxImage/zlib" /I "../../3rdParty/CxImage" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "FCBase" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /D "NDEBUG" /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x407 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 qt-mt323.lib gdi32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Custom Build - Performing Registration on $(InputPath)
OutDir=.\Release
TargetPath=.\Release\FCThumbs.dll
InputPath=.\Release\FCThumbs.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "FCThumbs - Win32 Debug"

# PROP BASE Use_MFC 2
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../" /I "../../3rdParty/CxImage/zlib" /I "../../3rdParty/CxImage" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_USRDLL" /D "FCBase" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /D "_DEBUG" /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x407 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 gdi32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# Begin Custom Build - Performing Registration on $(InputPath)
OutDir=.\Debug
TargetPath=.\Debug\FCThumbs.dll
InputPath=.\Debug\FCThumbs.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "FCThumbs - Win32 Release"
# Name "FCThumbs - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\FCStdExtractor.cpp
# End Source File
# Begin Source File

SOURCE=.\FCThumbs.def
# End Source File
# Begin Source File

SOURCE=.\FCThumbs.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\ThumbFCStd.idl
# ADD MTL /tlb "./ThumbFCStd.tlb" /h "ThumbFCStd_i.h" /iid "ThumbFCStd_i.c" /Oicf
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\FCStdExtractor.h
# End Source File
# Begin Source File

SOURCE=.\IExtractImage.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\FCThumbs.rgs
# End Source File
# End Group
# Begin Group "zipios"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Base\zipios\backbuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\basicentry.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\basicentry.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\collcoll.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\collcoll.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\deflateoutputstreambuf.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\deflateoutputstreambuf.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\dircoll.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\dircoll.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\directory.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\directory.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\fcoll.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\fcoll.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\fcollexceptions.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\fcollexceptions.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\fileentry.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\fileentry.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\filepath.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\filepath.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\filterinputstreambuf.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\filterinputstreambuf.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\filteroutputstreambuf.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\filteroutputstreambuf.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\gzipoutputstream.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\gzipoutputstream.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\gzipoutputstreambuf.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\gzipoutputstreambuf.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\inflateinputstreambuf.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\inflateinputstreambuf.h
# End Source File
# Begin Source File

SOURCE="..\..\Base\zipios\meta-iostreams.h"
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\outputstringstream.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\simplesmartptr.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\virtualseeker.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipfile.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipfile.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\ziphead.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\ziphead.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipheadio.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipheadio.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipinputstream.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipinputstream.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipinputstreambuf.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipinputstreambuf.h
# End Source File
# Begin Source File

SOURCE="..\..\Base\zipios\zipios-config.h"
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipios_common.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipios_defs.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipoutputstream.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipoutputstream.h
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipoutputstreambuf.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\Base\zipios\zipoutputstreambuf.h
# End Source File
# End Group
# End Target
# End Project
