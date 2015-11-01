#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - FreeCAD Developers                               *
#*   Author (c) 2015 - Qingfeng Xia <qingfeng xia eng.ox.ac.uk>                    *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************


"""This file can be pasted into FemWorkbench's python console for testing
Material and Mesh need redo in GUI to work, since GUI operation is not recorded fully
"""


App.newDocument("Unnamed")
App.setActiveDocument("Unnamed")
App.ActiveDocument=App.getDocument("Unnamed")
Gui.ActiveDocument=Gui.getDocument("Unnamed")

# copy from this line, if you want to create both CFD and FEM analysise

App.ActiveDocument.addObject("Part::Box","Box")
App.ActiveDocument.ActiveObject.Label = "Cube"
App.ActiveDocument.recompute()
Gui.SendMsgToActiveView("ViewFit")
#
FreeCAD.getDocument("Unnamed").getObject("Box").Length = '20 mm'
FreeCAD.getDocument("Unnamed").getObject("Box").Width = '20 mm'
FreeCAD.getDocument("Unnamed").getObject("Box").Height = '20 mm'
#
Gui.activateWorkbench("FemWorkbench")
import FemGui
App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', 'Box_Mesh')
App.activeDocument().ActiveObject.Shape = App.activeDocument().Box
Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)
Gui.activeDocument().resetEdit()
#
#import MechanicalAnalysis #deprecated
#FemGui.setActiveAnalysis(MechanicalAnalysis.makeMechanicalAnalysis('MechanicalAnalysis'))
import CaeAnalysis  #using NEW API for mech analysis
FemGui.setActiveAnalysis(CaeAnalysis.makeMechanicalAnalysis('MechanicalAnalysis'))
FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().Box_Mesh]

import MechanicalMaterial
MechanicalMaterial.makeMechanicalMaterial('MechanicalMaterial')
App.activeDocument().MechanicalAnalysis.Member = App.activeDocument().MechanicalAnalysis.Member + [App.ActiveDocument.ActiveObject]
Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name,0)

App.activeDocument().addObject("Fem::ConstraintFixed","FemConstraintFixed")
App.activeDocument().MechanicalAnalysis.Member = App.activeDocument().MechanicalAnalysis.Member + [App.activeDocument().FemConstraintFixed]
App.ActiveDocument.recompute()
Gui.activeDocument().setEdit('FemConstraintFixed')
App.ActiveDocument.FemConstraintFixed.References = [(App.ActiveDocument.Box,"Face6")]
App.ActiveDocument.recompute()
Gui.activeDocument().resetEdit()
App.activeDocument().addObject("Fem::ConstraintForce","FemConstraintForce")
App.activeDocument().FemConstraintForce.Force = 0.0
App.activeDocument().MechanicalAnalysis.Member = App.activeDocument().MechanicalAnalysis.Member + [App.activeDocument().FemConstraintForce]
App.ActiveDocument.recompute()
Gui.activeDocument().setEdit('FemConstraintForce')
App.ActiveDocument.FemConstraintForce.Force = 0.000000
App.ActiveDocument.FemConstraintForce.Direction = None
App.ActiveDocument.FemConstraintForce.Reversed = False
App.ActiveDocument.FemConstraintForce.References = [(App.ActiveDocument.Box,"Face5")]
App.ActiveDocument.recompute()
App.ActiveDocument.FemConstraintForce.Force = 5000.000000
App.ActiveDocument.FemConstraintForce.Direction = None
App.ActiveDocument.FemConstraintForce.Reversed = False
App.ActiveDocument.FemConstraintForce.References = [(App.ActiveDocument.Box,"Face4")]
App.ActiveDocument.recompute()
Gui.activeDocument().resetEdit()

Gui.getDocument("Unnamed").getObject("Box_Mesh").Visibility=False
Gui.getDocument("Unnamed").getObject("Box").Visibility=True
#
#need to select material, and refine mesh before solve, or you will got ERROR!!!
#
