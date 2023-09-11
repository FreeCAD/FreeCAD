# Microsoft Developer Studio Project File - Name="zipios" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=zipios - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "zipios.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "zipios.mak" CFG="zipios - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "zipios - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "zipios - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "zipios - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\zlib" /I "..\..\zlib" /I ".." /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x406 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "zipios - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\zlib" /I ".." /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_WINDOWS" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x406 /d "_DEBUG"
# ADD RSC /l 0x406 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "zipios - Win32 Release"
# Name "zipios - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\appendzip.cpp
# End Source File
# Begin Source File

SOURCE=.\backbuffer.h
# End Source File
# Begin Source File

SOURCE=.\basicentry.cpp
# End Source File
# Begin Source File

SOURCE=.\collcoll.cpp
# End Source File
# Begin Source File

SOURCE=.\deflateoutputstreambuf.cpp
# End Source File
# Begin Source File

SOURCE=.\dircoll.cpp
# End Source File
# Begin Source File

SOURCE=.\directory.cpp
# End Source File
# Begin Source File

SOURCE=.\directory.h
# End Source File
# Begin Source File

SOURCE=.\fcoll.cpp
# End Source File
# Begin Source File

SOURCE=.\fcollexceptions.cpp
# End Source File
# Begin Source File

SOURCE=.\fileentry.cpp
# End Source File
# Begin Source File

SOURCE=.\filepath.cpp
# End Source File
# Begin Source File

SOURCE=.\filterinputstreambuf.cpp
# End Source File
# Begin Source File

SOURCE=.\filteroutputstreambuf.cpp
# End Source File
# Begin Source File

SOURCE=.\inflateinputstreambuf.cpp
# End Source File
# Begin Source File

SOURCE=.\outputstringstream.h
# End Source File
# Begin Source File

SOURCE=.\zipfile.cpp
# End Source File
# Begin Source File

SOURCE=.\ziphead.cpp
# End Source File
# Begin Source File

SOURCE=.\zipheadio.cpp
# End Source File
# Begin Source File

SOURCE=.\zipinputstream.cpp
# End Source File
# Begin Source File

SOURCE=.\zipinputstreambuf.cpp
# End Source File
# Begin Source File

SOURCE=.\zipios_common.h
# End Source File
# Begin Source File

SOURCE=.\zipoutputstream.cpp
# End Source File
# Begin Source File

SOURCE=.\zipoutputstreambuf.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\zipios++\basicentry.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\collcoll.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\deflateoutputstreambuf.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\dircoll.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\fcoll.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\fcollexceptions.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\fileentry.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\filepath.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\filterinputstreambuf.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\filteroutputstreambuf.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\inflateinputstreambuf.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\meta-iostreams.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\simplesmartptr.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\virtualseeker.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\zipfile.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\ziphead.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\zipheadio.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\zipinputstream.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\zipinputstreambuf.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\zipios-config.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\zipios_defs.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\zipoutputstream.h"
# End Source File
# Begin Source File

SOURCE="..\zipios++\zipoutputstreambuf.h"
# End Source File
# End Group
# End Target
# End Project
