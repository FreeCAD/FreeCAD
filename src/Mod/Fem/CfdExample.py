#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Qingfeng Xia @iesensor.com                 *
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

App.newDocument("Unnamed")
App.setActiveDocument("Unnamed")
App.ActiveDocument=App.getDocument("Unnamed")
Gui.ActiveDocument=Gui.getDocument("Unnamed")
#
Gui.activateWorkbench("PartWorkbench")
App.ActiveDocument.addObject("Part::Cylinder","Cylinder")
App.ActiveDocument.ActiveObject.Label = "Cylinder"
App.ActiveDocument.recompute()
Gui.SendMsgToActiveView("ViewFit")
Gui.activateWorkbench("FemWorkbench")
#
App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', 'Cylinder_Mesh')
App.activeDocument().ActiveObject.Shape = App.activeDocument().Cylinder
Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)
Gui.activeDocument().resetEdit()
#
import FemGui
import CaeAnalysis
import CaeSolver
CaeAnalysis._makeCaeAnalysis("OpenFOAMAnalysis")
FemGui.setActiveAnalysis(App.activeDocument().ActiveObject)
CaeSolver.makeCaeSolver('OpenFOAM')
#
Gui.getDocument("Unnamed").getObject("Cylinder_Mesh").Visibility=False
Gui.getDocument("Unnamed").getObject("Cylinder").Visibility=True
#pressure inlet
import MechanicalMaterial
MechanicalMaterial.makeMechanicalMaterial('MechanicalMaterial')
App.activeDocument().OpenFOAMAnalysis.Member = App.activeDocument().OpenFOAMAnalysis.Member + [App.ActiveDocument.ActiveObject]
Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name,0)
#
App.activeDocument().addObject("Fem::ConstraintPressure","FemConstraintPressure")
App.activeDocument().FemConstraintPressure.Pressure = 0.0
App.activeDocument().OpenFOAMAnalysis.Member = App.activeDocument().OpenFOAMAnalysis.Member + [App.activeDocument().FemConstraintPressure]
App.ActiveDocument.recompute()
Gui.activeDocument().setEdit('FemConstraintPressure')
App.ActiveDocument.FemConstraintPressure.Pressure = 0.010000
App.ActiveDocument.FemConstraintPressure.Reversed = False
App.ActiveDocument.FemConstraintPressure.References = [(App.ActiveDocument.Cylinder,"Face3")]
App.ActiveDocument.recompute()
Gui.activeDocument().resetEdit()
#pressure outlet
App.activeDocument().addObject("Fem::ConstraintPressure","FemConstraintPressure001")
App.activeDocument().FemConstraintPressure001.Pressure = 0.0
App.activeDocument().OpenFOAMAnalysis.Member = App.activeDocument().OpenFOAMAnalysis.Member + [App.activeDocument().FemConstraintPressure001]
App.ActiveDocument.recompute()
Gui.activeDocument().setEdit('FemConstraintPressure001')
App.ActiveDocument.FemConstraintPressure001.Pressure = 0.000010
App.ActiveDocument.FemConstraintPressure001.Reversed = True
App.ActiveDocument.FemConstraintPressure001.References = [(App.ActiveDocument.Cylinder,"Face2")]
App.ActiveDocument.recompute()
Gui.activeDocument().resetEdit()
#


