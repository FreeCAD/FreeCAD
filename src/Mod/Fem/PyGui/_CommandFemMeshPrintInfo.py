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

__title__ = "Print info of FEM mesh object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandFemMeshPrintInfo
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandFemMeshPrintInfo(FemCommands):
    "the FEM_MeshPrintInfo command definition"
    def __init__(self):
        super(_CommandFemMeshPrintInfo, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-print-info',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MeshPrintInfo", "Print FEM mesh info"),
                          # 'Accel': "Z, Z",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MeshPrintInfo", "Print FEM mesh info")}
        self.is_active = 'with_femmesh'

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1 and sel[0].isDerivedFrom("Fem::FemMeshObject"):
            FreeCAD.ActiveDocument.openTransaction("Print FEM mesh info")
            FreeCADGui.doCommand("print(App.ActiveDocument." + sel[0].Name + ".FemMesh)")

            FreeCADGui.addModule("PySide")
            FreeCADGui.doCommand("mesh_info = str(App.ActiveDocument." + sel[0].Name + ".FemMesh)")
            FreeCADGui.doCommand("PySide.QtGui.QMessageBox.information(None, 'FEM Mesh Info', mesh_info)")

        FreeCADGui.Selection.clearSelection()

FreeCADGui.addCommand('FEM_MeshPrintInfo', _CommandFemMeshPrintInfo())
