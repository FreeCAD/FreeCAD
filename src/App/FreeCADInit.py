# FreeCAD init module
# (c) 2001 Jürgen Riegel
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Jürgen Riegel (juergen.riegel@web.de) 2002                        *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU General Public License (GPL)            *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/


# imports the one and only
import FreeCAD


def InitApplications():
	try:
		import sys,os
	except:
		FreeCAD.PrintError("\n\nSeems the python standard libs are not installed, bailing out!\n\n")
		raise
	# Checking on FreeCAD module path ++++++++++++++++++++++++++++++++++++++++++
	ModDir = FreeCAD.getHomePath()+'Mod'
	ModDir = os.path.realpath(ModDir)
	BinDir = FreeCAD.getHomePath()+'bin'
	BinDir = os.path.realpath(BinDir)
	LibDir = FreeCAD.getHomePath()+'lib'
	LibDir = os.path.realpath(LibDir)
	AddPath = FreeCAD.ConfigGet("AdditionalModulePaths").split(";")
	HomeMod = FreeCAD.ConfigGet("UserAppData")+"Mod"
	HomeMod = os.path.realpath(HomeMod)
	MacroDir = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Macro").GetString("MacroPath")
	MacroMod = os.path.realpath(MacroDir+"/Mod")
	ModPar = FreeCAD.ParamGet("System parameter:Modules")

	#print FreeCAD.getHomePath()
	if os.path.isdir(FreeCAD.getHomePath()+'src\\Tools'):
		sys.path.append(FreeCAD.getHomePath()+'src\\Tools')
	# Searching for module dirs +++++++++++++++++++++++++++++++++++++++++++++++++++
	# Use dict to handle duplicated module names
	ModDict = {}
	if os.path.isdir(ModDir):
		ModDirs = os.listdir(ModDir)
		for i in ModDirs: ModDict[i.lower()] = os.path.join(ModDir,i)
	else:
		Wrn ("No modules found in " + ModDir + "\n")
	# Search for additional modules in the home directory
	if os.path.isdir(HomeMod):
		HomeMods = os.listdir(HomeMod)
		for i in HomeMods: ModDict[i.lower()] = os.path.join(HomeMod,i)
	# Search for additional modules in the macro directory
	if os.path.isdir(MacroMod):
		MacroMods = os.listdir(MacroMod)
		for i in MacroMods:
			key = i.lower()
			if key not in ModDict: ModDict[key] = os.path.join(MacroMod,i)
	# Search for additional modules in command line
	for i in AddPath:
		if os.path.isdir(i): ModDict[i] = i
	#AddModPaths = App.ParamGet("System parameter:AdditionalModulePaths")
	#Err( AddModPaths)
	# add also this path so that all modules search for libraries
	# they depend on first here
	PathExtension = BinDir + os.pathsep
	# prepend all module paths to Python search path
	Log('Init:   Searching for modules...\n')
	FreeCAD.__path__ = ModDict.values()
	for Dir in ModDict.values():
		if ((Dir != '') & (Dir != 'CVS') & (Dir != '__init__.py')):
			ModGrp = ModPar.GetGroup(Dir)
			sys.path.insert(0,Dir)
			PathExtension += Dir + os.pathsep
			InstallFile = os.path.join(Dir,"Init.py")
			if (os.path.exists(InstallFile)):
				try:
					execfile(InstallFile)
				except Exception, inst:
					Log('Init:      Initializing ' + Dir + '... failed\n')
					Err('During initialization the error ' + str(inst) + ' occurred in ' + InstallFile + '\n')
				else:
					Log('Init:      Initializing ' + Dir + '... done\n')
			else:
				Log('Init:      Initializing ' + Dir + '(Init.py not found)... ignore\n')
	sys.path.insert(0,LibDir)
	sys.path.insert(0,ModDir)
	Log("Using "+ModDir+" as module path!\n")
	# new paths must be prepended to avoid to load a wrong version of a library
	os.environ["PATH"] = PathExtension + os.environ["PATH"]
	path = os.environ["PATH"].split(os.pathsep)
	Log("System path after init:\n")
	for i in path:
		Log("   " + i + "\n")
	# add MacroDir to path (RFE #0000504)
	sys.path.append(MacroDir)
	# add special path for MacOSX (bug #0000307)
	import platform
	if len(platform.mac_ver()[0]) > 0:
		sys.path.append(os.path.expanduser('~/Library/Application Support/FreeCAD/Mod'))

# some often used shortcuts (for lazy people like me ;-)
App = FreeCAD
Log = FreeCAD.Console.PrintLog
Msg = FreeCAD.Console.PrintMessage
Err = FreeCAD.Console.PrintError
Wrn = FreeCAD.Console.PrintWarning

Log ('Init: starting App::FreeCADInit.py\n')

# init every application by importing Init.py
InitApplications()

FreeCAD.EndingAdd("FreeCAD document (*.FCStd)","FreeCAD")

# set to no gui, is overwritten by InitGui
App.GuiUp = 0

# clean up namespace
del(InitApplications)

Log ('Init: App::FreeCADInit.py done\n')




