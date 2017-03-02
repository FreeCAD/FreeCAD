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

        import PyGui._CommandShowResult
        import PyGui._CommandClearMesh
        import PyGui._CommandPrintMeshInfo
        import PyGui._CommandFEMMesh2Mesh
        import PyGui._CommandMeshGmshFromShape
        import PyGui._CommandMeshNetgenFromShape
        import PyGui._CommandMeshGroup
        import PyGui._CommandMeshRegion
        import PyGui._CommandAnalysis
        import PyGui._CommandShellThickness
        import PyGui._CommandBeamSection
        import PyGui._CommandFluidSection
        import PyGui._CommandMaterialSolid
        import PyGui._CommandMaterialFluid
        import PyGui._CommandMaterialMechanicalNonlinear
        import PyGui._CommandResultsPurge
        import PyGui._CommandSolverCalculix
        import PyGui._CommandSolverControl
        import PyGui._CommandSolverRun
        import PyGui._CommandSolverZ88
        import PyGui._CommandConstraintSelfWeight

    def GetClassName(self):
        return "FemGui::Workbench"

FreeCADGui.addWorkbench(FemWorkbench())
