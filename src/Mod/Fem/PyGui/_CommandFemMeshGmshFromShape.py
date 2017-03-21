# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "Command GMSH Mesh From Shape"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandFemMeshGmshFromShape
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands
import FreeCADGui
import FemGui
from PySide import QtCore


class _CommandFemMeshGmshFromShape(FemCommands):
    # the FEM_MeshGmshFromShape command definition
    def __init__(self):
        super(_CommandFemMeshGmshFromShape, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-gmsh-from-shape',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshGmshFromShape", "FEM mesh from shape by GMSH"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshGmshFromShape", "Create a FEM mesh from a shape by GMSH mesher")}
        self.is_active = 'with_part_feature'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh by GMSH")
        FreeCADGui.addModule("FemGui")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Part::Feature")):
                mesh_obj_name = sel[0].Name + "_Mesh"
                FreeCADGui.addModule("ObjectsFem")
                FreeCADGui.doCommand("ObjectsFem.makeMeshGmsh('" + mesh_obj_name + "')")
                FreeCADGui.doCommand("App.ActiveDocument.ActiveObject.Part = App.ActiveDocument." + sel[0].Name)
                if FemGui.getActiveAnalysis():
                    FreeCADGui.addModule("FemGui")
                    FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.ActiveDocument.ActiveObject]")
                FreeCADGui.doCommand("Gui.ActiveDocument.setEdit(App.ActiveDocument.ActiveObject.Name)")

        FreeCADGui.Selection.clearSelection()


FreeCADGui.addCommand('FEM_MeshGmshFromShape', _CommandFemMeshGmshFromShape())
