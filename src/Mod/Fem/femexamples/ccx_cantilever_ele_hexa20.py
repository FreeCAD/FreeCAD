# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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

from . import manager
from .ccx_cantilever_faceload import setup as setup_with_faceload
from .manager import get_meshname
from .manager import init_doc
from .meshes import generate_mesh


def get_information():
    return {
        "name": "CCX cantilever hexa20 solid elements",
        "meshtype": "solid",
        "meshelement": "Hexa20",
        "constraints": ["fixed", "force"],
        "solvers": ["ccxtools", "z88"], # elmer disabled until mesh has groups
        "material": "solid",
        "equations": ["mechanical"],
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.ccx_cantilever_ele_hexa20 import setup
setup()


See forum topic post:
hexa20 elements and face load
...

"""
    )


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # setup cantilever faceload
    doc = setup_with_faceload(doc, solvertype)
    femmesh_obj = doc.getObject(get_meshname())

    # delete explanation object wrongly added with setup faceload
    if hasattr(doc, "Explanation_Report001"):
        doc.removeObject("Explanation_Report001")
    doc.recompute()

    # load the hexa20 mesh
    from .meshes.mesh_canticcx_hexa20 import create_nodes, create_elements

    fem_mesh = generate_mesh.mesh_from_existing(create_nodes, create_elements)
    femmesh_obj.FemMesh = fem_mesh

    doc.recompute()
    return doc
