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

__title__ = "Commend Fem From Shape"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

import FreeCAD
from FemCommands import FemCommands

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore


class _CommandFemFromShape(FemCommands):
    def __init__(self):
        super(_CommandFemFromShape, self).__init__()
        self.resources = {'Pixmap': 'fem-fem-mesh-from-shape',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_CreateFromShape", "Create FEM mesh"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_CreateFromShape", "Create FEM mesh from shape")}
        self.is_active = 'with_part_feature'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh")
        FreeCADGui.addModule("FemGui")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', '" + sel[0].Name + "_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()." + sel[0].Name)
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        FreeCADGui.Selection.clearSelection()


if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_CreateFromShape', _CommandFemFromShape())
