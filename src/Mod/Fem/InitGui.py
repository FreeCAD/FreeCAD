# Fem gui init module
# (c) 2009 Juergen Riegel
#
# Gathering all the information to start FreeCAD
# This is the second one of three init scripts, the third one
# runs when the gui is up

#***************************************************************************
#*   (c) Juergen Riegel (juergen.riegel@web.de) 2009                       *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Lesser General Public License for more details.                   *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#*   Juergen Riegel 2002                                                   *
#***************************************************************************/


import FreeCAD
import FreeCADGui


class FemWorkbench (Workbench):
    "Fem workbench object"
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Fem/Resources/icons/FemWorkbench.svg"
        self.__class__.MenuText = "FEM"
        self.__class__.ToolTip = "FEM workbench"

    def Initialize(self):
        # load the module
        import Fem
        import FemGui

        import _CommandMechanicalShowResult
        import _CommandQuickAnalysis
        import _CommandPurgeFemResults
        import _CommandSolverJobControl
        import _CommandFemFromShape
        import _CommandNewMechanicalAnalysis
        import _CommandFemShellThickness
        import _CommandFemBeamSection
        import _CommandMechanicalMaterial
        import _CommandFemSolverCalculix

        import subprocess
        from platform import system
        ccx_path = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem").GetString("ccxBinaryPath")
        if not ccx_path:
            try:
                if system() == 'Linux':
                    p1 = subprocess.Popen(['which', 'ccx'], stdout=subprocess.PIPE)
                    if p1.wait() == 0:
                        ccx_path = p1.stdout.read().split('\n')[0]
                elif system() == 'Windows':
                    ccx_path = FreeCAD.getHomePath() + 'bin/ccx.exe'
                if ccx_path:
                    FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem").SetString("ccxBinaryPath", ccx_path)
                else:
                    FreeCAD.Console.PrintError("CalculiX ccx binary not found! Please set it manually in FEM preferences.\n")
            except Exception as e:
                FreeCAD.Console.PrintError(e.message)
        fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")

        import os
        working_dir = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem").GetString("WorkingDir")
        if not (os.path.isdir(working_dir)):
            try:
                os.makedirs(working_dir)
            except:
                print ("Dir \'{}\' from FEM preferences doesn't exist and cannot be created.".format(working_dir))
                import tempfile
                working_dir = tempfile.gettempdir()
                print ("Dir \'{}\' will be used instead.".format(working_dir))
        if working_dir:
            FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem").SetString("WorkingDir", working_dir)
        else:
            FreeCAD.Console.PrintError("Setting working directory \'{}\' for ccx failed!\n")

    def GetClassName(self):
        return "FemGui::Workbench"

FreeCADGui.addWorkbench(FemWorkbench())
