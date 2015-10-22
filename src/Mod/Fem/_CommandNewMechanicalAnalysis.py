#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

__title__ = "Command New Mechanical Analysis"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

import FreeCAD

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtCore


class _CommandNewMechanicalAnalysis:
    "the Fem Analysis command definition"
    def GetResources(self):
        return {'Pixmap': 'fem-analysis',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "New mechanical analysis"),
                'Accel': "N, A",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "Create a new mechanical analysis")}

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("MechanicalAnalysis")
        #FreeCADGui.doCommand("FreeCADGui.ActiveDocument.ActiveView.setAxisCross(True)")
        FreeCADGui.doCommand("MechanicalAnalysis.makeMechanicalAnalysis('MechanicalAnalysis')")
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(App.activeDocument().ActiveObject)")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument()." + sel[0].Name + "]")
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', '" + sel[0].Name + "_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()." + sel[0].Name)
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
                #FreeCADGui.doCommand("Gui.activeDocument().hide('" + sel[0].Name + "')")
                #FreeCADGui.doCommand("App.activeDocument().ActiveObject.touch()")
                #FreeCADGui.doCommand("App.activeDocument().recompute()")
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        #FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.clearSelection()

    def IsActive(self):
        return FreeCADGui.ActiveDocument is not None and FemGui.getActiveAnalysis() is None


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_NewMechanicalAnalysis', _CommandNewMechanicalAnalysis())
