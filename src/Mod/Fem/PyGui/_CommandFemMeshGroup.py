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

__title__ = "_CommandMeshGroup"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandFemMeshGroup
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandFemMeshGroup(FemCommands):
    "The FEM_MeshGroup command definition"
    def __init__(self):
        super(_CommandFemMeshGroup, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-from-shape',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshGroup", "FEM mesh group"),
                          'Accel': "M, G",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshGroup", "Creates a FEM mesh group")}
        self.is_active = 'with_gmsh_femmesh'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemMeshGroup")
        FreeCADGui.addModule("ObjectsFem")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            sobj = sel[0]
            if len(sel) == 1 and hasattr(sobj, "Proxy") and sobj.Proxy.Type == "FemMeshGmsh":
                FreeCADGui.doCommand("ObjectsFem.makeMeshGroup(App.ActiveDocument." + sobj.Name + ")")

        FreeCADGui.Selection.clearSelection()

FreeCADGui.addCommand('FEM_MeshGroup', _CommandFemMeshGroup())
