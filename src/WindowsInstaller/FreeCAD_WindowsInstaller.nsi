#  (c) Juergen Riegel (FreeCAD@juergen-riegel.net) 2014                       
#                                                                        
#   This file is part of the FreeCAD CAx development system.             
#                                                                        
#   This program is free software; you can redistribute it and/or modify 
#   it under the terms of the GNU Library General Public License (LGPL)  
#   as published by the Free Software Foundation; either version 2 of    
#   the License, or (at your option) any later version.                  
#   for detail see the LICENCE text file.                                
#                                                                        
#   FreeCAD is distributed in the hope that it will be useful,           
#   but WITHOUT ANY WARRANTY; without even the implied warranty of       
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
#   GNU Library General Public License for more details.                 
#                                                                        
#   You should have received a copy of the GNU Library General Public    
#   License along with FreeCAD; if not, write to the Free Software       
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 
#   USA                                                                  
#                                                                        
#   Juergen Riegel 2014

#http://www.fredshack.com/docs/nsis.html
# include the Version information
!include Version.nsi

# All the other settings can be tweaked by editing the !defines at the top of this script
!define APPNAME "FreeCAD"
!define PUPNAME "Juergen Riegel"
!define DESCRIPTION "A free open source CAD system"

# These will be displayed by the "Click here for support information" link in "Add/Remove Programs"
# It is possible to use "mailto:" links in here to open the email client
!define HELPURL "http://freecadweb.org" # "Support Information" link
!define UPDATEURL "http://freecadweb.org" # "Product Updates" link
!define ABOUTURL "http://freecadweb.org" # "Publisher" link
# This is the size (in kB) of all the files copied into "Program Files"
!define INSTALLSIZE 200000

!define FULLNAME "${APPNAME} ${VERSIONMAJOR}.${VERSIONMINOR}"
 
RequestExecutionLevel admin ;Require admin rights on NT6+ (When UAC is turned on)
 
InstallDir "$PROGRAMFILES\${FULLNAME}"
 
# rtf or txt file - remember if it is txt, it must be in the DOS text format (\r\n)
LicenseData "License.rtf"
# This will be in the installer/uninstaller's title bar
Name "${FULLNAME}"
#Icon "logo.ico"
outFile "..\..\${FULLNAME}.${VERSIONBUILD}_x86_unstable_setup.exe"
 
!include LogicLib.nsh
 
# Just three pages - license agreement, install location, and installation
page license
page directory
Page instfiles
 
!macro VerifyUserIsAdmin
UserInfo::GetAccountType
pop $0
${If} $0 != "admin" ;Require admin rights on NT4+
        messageBox mb_iconstop "Administrator rights required!"
        setErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
        quit
${EndIf}
!macroend
 
function .onInit
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
functionEnd

section "install"
	# Files for the install directory - to build the installer, these should be in the same directory as the install script (this file)
	setOutPath $INSTDIR\bin
	# Files added here should be removed by the uninstaller (see section "uninstall")
	file /r /X *.idb /X *.pyc /X *.pyo "..\..\bin\"
	setOutPath $INSTDIR\lib
	file /r /X *.lib /X *.pyc /X *.pyo "..\..\lib\"
	setOutPath $INSTDIR\Mod
	file /r /X *.idb  "..\..\Mod\"
	setOutPath $INSTDIR\doc
	file /r "..\..\doc\"
	setOutPath $INSTDIR\data
    file  /r /X CMakeFiles /X *.cmake /X *.dir /X *.vcproj /X CMakeLists.txt /X *.am "..\..\data\"
	setOutPath $INSTDIR
    file  "vcredist_x86.exe"
	
	# Install the Visual Studio redistributable 
    ExecWait '"$INSTDIR\vcredist_x86.exe" /q:a /c:"VCREDI~1.EXE /q:a /c:""msiexec /i vcredist.msi /qb!"" "'  
 
	# Uninstaller - See function un.onInit and section "uninstall" for configuration
	writeUninstaller "$INSTDIR\uninstall.exe"
 
	# Start Menu
	createDirectory "$SMPROGRAMS\${FULLNAME}"
	createShortCut "$SMPROGRAMS\${FULLNAME}\${APPNAME}.lnk" "$INSTDIR\bin\FreeCAD.exe" "" ""
 
	# Registry information for add/remove programs
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "DisplayName" "${FULLNAME} - ${DESCRIPTION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "InstallLocation" "$\"$INSTDIR$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "DisplayIcon" "$\"$INSTDIR\logo.ico$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "Publisher" "${PUPNAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "HelpLink" "$\"${HELPURL}$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "URLUpdateInfo" "$\"${UPDATEURL}$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "URLInfoAbout" "$\"${ABOUTURL}$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "DisplayVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "VersionMajor" ${VERSIONMAJOR}
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "VersionMinor" ${VERSIONMINOR}
	# There is no option for modifying or repairing the install
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "NoRepair" 1
	# Set the INSTALLSIZE constant (!defined at the top of this script) so Add/Remove Programs can accurately report the size
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}" "EstimatedSize" ${INSTALLSIZE}
	# Set PYTHONPATH for FreeCAD
	#WriteRegStr HKLM "Software\Python\PythonCore\2.7\PythonPath\${FULLNAME}" "" "$INSTDIR\bin"
sectionEnd
 
# Uninstaller
 
function un.onInit
	SetShellVarContext all
 
	#Verify the uninstaller - last chance to back out
	MessageBox MB_OKCANCEL "Permanantly remove ${APPNAME}?" IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
functionEnd
 
section "uninstall"
 
	# Remove Start Menu launcher
	delete "$SMPROGRAMS\${FULLNAME}\${APPNAME}.lnk"
	# Try to remove the Start Menu folder - this will only happen if it is empty
	rmDir "$SMPROGRAMS\${FULLNAME}"
 
	# Remove files
	rmDir /r "$INSTDIR\bin"
	rmDir /r "$INSTDIR\lib"
	rmDir /r "$INSTDIR\doc"
	rmDir /r "$INSTDIR\data"
	rmDir /r "$INSTDIR\Mod"
 
	# Always delete uninstaller as the last action
	delete $INSTDIR\uninstall.exe
	delete $INSTDIR\vcredist_x86.exe
 	# Try to remove the install directory - this will only happen if it is empty
	rmDir $INSTDIR
 
	# Remove uninstaller information from the registry
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${FULLNAME}"
sectionEnd
