# ***************************************************************************
# *   Copyright (c) 2021 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

import FreeCAD

import Fem

from .truss_3d_cs_circle_ele_seg3 import setup as setup_truss_seg3
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "Truss 3D circle cs, seg2 elements",
        "meshtype": "edge",
        "meshelement": "Seg3",
        "constraints": ["fixed", "force"],
        "solvers": ["z88"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def setup(doc=None, solvertype="z88"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # see seg3 for the header
    # for simplicity reason only one explanation is used for both

    # setup cantilever faceload
    doc = setup_truss_seg3(doc, solvertype)
    femmesh_obj = doc.getObject(get_meshname())

    # mesh
    from .meshes.mesh_truss_crane_seg2 import create_nodes, create_elements
    fem_mesh = Fem.FemMesh()
    control = create_nodes(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating nodes.\n")
    control = create_elements(fem_mesh)
    if not control:
        FreeCAD.Console.PrintError("Error on creating elements.\n")

    # overwrite mesh with the hexa20 mesh
    femmesh_obj.FemMesh = fem_mesh

    # set changed properties
    femmesh_obj.ElementOrder = "1st"
    # one element for each bar
    femmesh_obj.CharacteristicLengthMax = "3000.0 mm"
    femmesh_obj.CharacteristicLengthMin = "3000.0 mm"

    doc.recompute()
    return doc
