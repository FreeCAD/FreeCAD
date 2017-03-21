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

__title__ = "Command FEMMesh to Mesh"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandFemMesh2Mesh
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands

import FreeCADGui
from PySide import QtCore


class _CommandFemMesh2Mesh(FemCommands):
    # the FEM_FemMesh2Mesh command definition
    def __init__(self):
        super(_CommandFemMesh2Mesh, self).__init__()
        self.resources = {'Pixmap': 'fem-femmesh-to-mesh',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_FEMMesh2Mesh", "FEM mesh to mesh"),
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_FEMMesh2Mesh", "Convert the surface of a FEM mesh to a mesh")}
        self.is_active = 'with_femmesh_andor_res'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FEM mesh")
        FreeCADGui.addModule("FemGui")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCAD.ActiveDocument.openTransaction("Create Mesh from FEMMesh")
                FreeCADGui.addModule("FemMesh2Mesh")
                FreeCADGui.doCommand("out_mesh = FemMesh2Mesh.femmesh_2_mesh(App.ActiveDocument." + sel[0].Name + ".FemMesh)")
                FreeCADGui.addModule("Mesh")
                FreeCADGui.doCommand("Mesh.show(Mesh.Mesh(out_mesh))")
                FreeCADGui.doCommand("App.ActiveDocument." + sel[0].Name + ".ViewObject.hide()")
        if (len(sel) == 2):
            femmesh = None
            res = None
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                if(sel[1].isDerivedFrom("Fem::FemResultObject")):
                    femmesh = sel[0]
                    res = sel[1]
            elif(sel[1].isDerivedFrom("Fem::FemMeshObject")):
                if(sel[0].isDerivedFrom("Fem::FemResultObject")):
                    femmesh = sel[1]
                    res = sel[0]
            if femmesh and res:
                FreeCAD.ActiveDocument.openTransaction("Create Mesh from FEMMesh")
                FreeCADGui.addModule("FemMesh2Mesh")
                FreeCADGui.doCommand("out_mesh = FemMesh2Mesh.femmesh_2_mesh(App.ActiveDocument." + femmesh.Name + ".FemMesh, App.ActiveDocument." + res.Name + ")")
                FreeCADGui.addModule("Mesh")
                FreeCADGui.doCommand("Mesh.show(Mesh.Mesh(out_mesh))")
                FreeCADGui.doCommand("App.ActiveDocument." + femmesh.Name + ".ViewObject.hide()")

        FreeCADGui.Selection.clearSelection()


FreeCADGui.addCommand('FEM_FEMMesh2Mesh', _CommandFemMesh2Mesh())
