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

__title__ = "_CommandMeshBoundaryLayer"
__author__ = "Bernd Hahnebach, Qingfeng Xia"
__url__ = "http://www.freecadweb.org"

## @package CommandFemMeshBoundaryLayer
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandFemMeshBoundaryLayer(FemCommands):
    "The FEM_MeshBoundaryLayer command definition"
    def __init__(self):
        super(_CommandFemMeshBoundaryLayer, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-boundary-layer',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshBoundaryLayer", "FEM mesh boundary layer"),
                          'Accel': "M, B",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshBoundaryLayer", "Creates a FEM mesh boundary layer")}
        self.is_active = 'with_gmsh_femmesh'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemMeshBoundaryLayer")
        FreeCADGui.addModule("ObjectsFem")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            sobj = sel[0]
            if len(sel) == 1 and hasattr(sobj, "Proxy") and sobj.Proxy.Type == "FemMeshGmsh":
                FreeCADGui.doCommand("ObjectsFem.makeMeshBoundaryLayer(App.ActiveDocument." + sobj.Name + ")")

        FreeCADGui.Selection.clearSelection()

FreeCADGui.addCommand('FEM_MeshBoundaryLayer', _CommandFemMeshBoundaryLayer())
