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

class FemWorkbench ( Workbench ):
    """Cae workbench object"""
    def __init__(self):
        self.__class__.Icon = FreeCAD.getResourceDir() + "Mod/Fem/Resources/icons/preferences-fem.svg"
        self.__class__.MenuText = "Fem"
        self.__class__.ToolTip = "CAE(FEM+CFD) workbench"
    def Initialize(self):
        # load the c++ module
        import Fem
        import FemGui
        
        #setup ccx path code has been moved into makeCaeSolver() and specific solver init section 
        
        import FemShellThickness
        import FemBeamSection
        import MechanicalMaterial
        #import CaeAnalysis  #FreeCADGui.addModule() 
        #import FemCommands  #MechanicalAnalysis  is loaded from C++, but it is not a good name
        
    def GetClassName(self):
        return "FemGui::Workbench"  # <FemToCae> change name later if C++ code got renamed

Gui.addWorkbench(FemWorkbench()) #<FemToCae> change name later if C++ code got renamed
