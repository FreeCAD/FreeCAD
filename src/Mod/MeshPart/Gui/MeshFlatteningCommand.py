# ***************************************************************************
# *   Copyright (c) 2017 Lorenz Lechner                                     *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

import Mesh
import FreeCAD as App
import FreeCADGui as Gui
import Part
import MeshPartGui

from PySide.QtCore import QT_TRANSLATE_NOOP  # for translations


class BaseCommand(object):
    def __init__(self):
        pass

    def IsActive(self):
        if App.ActiveDocument is None:
            return False
        else:
            return True


class CreateFlatMesh(BaseCommand):
    """create flat wires from a meshed face"""

    def GetResources(self):
        return {
            "Pixmap": "MeshPart_CreateFlatMesh.svg",
            "MenuText": QT_TRANSLATE_NOOP("MeshPart_CreateFlatMesh", "Unwrap Mesh"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "MeshPart_CreateFlatMesh", "Find a flat representation of a mesh."
            ),
        }

    def Activated(self):
        import numpy as np
        import flatmesh

        obj = Gui.Selection.getSelection()[
            0
        ]  # obj must be a Mesh (Mesh-Design->Meshes->Create-Mesh)
        points = np.array([[i.x, i.y, i.z] for i in obj.Mesh.Points])
        faces = np.array([list(i) for i in obj.Mesh.Topology[1]])
        flattener = flatmesh.FaceUnwrapper(points, faces)
        flattener.findFlatNodes(5, 0.95)
        boundaries = flattener.getFlatBoundaryNodes()
        # print('number of nodes: {}'.format(len(flattener.ze_nodes)))
        # print('number of faces: {}'.format(len(flattener.tris)))

        wires = []
        for edge in boundaries:
            pi = Part.makePolygon([App.Vector(*node) for node in edge])
            Part.show(Part.Wire(pi))

    def IsActive(self):
        assert super(CreateFlatMesh, self).IsActive()
        assert isinstance(Gui.Selection.getSelection()[0].Mesh, Mesh.Mesh)
        return True


class CreateFlatFace(BaseCommand):
    """create a flat face from a single face
    only full faces are supported right now"""

    def GetResources(self):
        return {
            "Pixmap": "MeshPart_CreateFlatFace.svg",
            "MenuText": QT_TRANSLATE_NOOP("MeshPart_CreateFlatFace", "Unwrap Face"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "MeshPart_CreateFlatFace", "Find a flat representation of a face."
            ),
        }

    def Activated(self):
        import numpy as np
        import flatmesh

        face = Gui.Selection.getSelectionEx()[0].SubObjects[0]
        shape = face.toNurbs()
        face = shape.Faces[0]
        nurbs = face.Surface
        nurbs.setUNotPeriodic()
        nurbs.setVNotPeriodic()
        bs = nurbs.toBSpline(1, "C0", "C0", 3, 3, 10)
        face = bs.toShape()
        face.tessellate(0.01)
        flattener = flatmesh.FaceUnwrapper(face)
        flattener.findFlatNodes(5, 0.99)
        poles = flattener.interpolateFlatFace(face)
        num_u_poles = len(bs.getPoles())
        num_v_poles = len(bs.getPoles()[0])
        i = 0
        for u in range(num_u_poles):
            for v in range(num_v_poles):
                bs.setPole(u + 1, v + 1, App.Vector(poles[i]))
                i += 1
        Part.show(bs.toShape())

    def IsActive(self):
        assert super(CreateFlatFace, self).IsActive()
        assert isinstance(Gui.Selection.getSelectionEx()[0].SubObjects[0], Part.Face)
        return True


# Test if pybind11 dependency is available
try:
    import flatmesh

    Gui.addCommand("MeshPart_CreateFlatMesh", CreateFlatMesh())
    Gui.addCommand("MeshPart_CreateFlatFace", CreateFlatFace())
except ImportError:
    App.Console.PrintLog("flatmesh-commands are not available\n")
    App.Console.PrintLog("flatmesh needs pybind11 as build dependency\n")
except AttributeError:
    # Can happen when running FreeCAD in headless mode
    pass
