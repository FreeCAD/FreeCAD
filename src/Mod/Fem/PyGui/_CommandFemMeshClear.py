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

__title__ = "Clear the FemMesh of a FEM mesh object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandFemMeshClear
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandFemMeshClear(FemCommands):
    "the FEM_MeshClear command definition"
    def __init__(self):
        super(_CommandFemMeshClear, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-clear-mesh',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshClear", "Clear FEM mesh"),
                          # 'Accel': "Z, Z",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshClear", "Clear the Mesh of a FEM mesh object")}
        self.is_active = 'with_femmesh'

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1 and sel[0].isDerivedFrom("Fem::FemMeshObject"):
            FreeCAD.ActiveDocument.openTransaction("Clear FEM mesh")
            FreeCADGui.addModule("Fem")
            FreeCADGui.doCommand("App.ActiveDocument." + sel[0].Name + ".FemMesh = Fem.FemMesh()")
            FreeCADGui.doCommand("App.ActiveDocument.recompute()")

        FreeCADGui.Selection.clearSelection()

FreeCADGui.addCommand('FEM_MeshClear', _CommandFemMeshClear())
