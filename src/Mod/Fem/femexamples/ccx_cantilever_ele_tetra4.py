# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com>   *
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

import Fem

from . import manager
from .ccx_cantilever_faceload import setup as setup_with_faceload
from .manager import get_meshname
from .manager import init_doc


def get_information():
    return {
        "name": "CCX cantilever tetra4 solid elements",
        "meshtype": "solid",
        "meshelement": "Tetra4",
        "constraints": ["fixed", "force"],
        "solvers": ["calculix", "ccxtools", "elmer", "mystran", "z88"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.ccx_cantilever_ele_tetra4 import setup
setup()


Tetra4 elements. There are really a lot needed thus mesh is cleared.
Mesh before run the example.
...

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # delete explanation object wrongly added with setup faceload
    if hasattr(doc, "Explanation_Report001"):
        doc.removeObject("Explanation_Report001")
    doc.recompute()

    # setup cantilever faceload and exchange the mesh
    doc = setup_with_faceload(doc, solvertype)
    femmesh_obj = doc.getObject(get_meshname())
    geom_obj = doc.getObject("Box")

    # clear mesh and set meshing parameter
    femmesh_obj.FemMesh = Fem.FemMesh()
    femmesh_obj.Part = geom_obj
    femmesh_obj.SecondOrderLinear = False
    femmesh_obj.ElementDimension = "3D"
    femmesh_obj.ElementOrder = "1st"
    femmesh_obj.CharacteristicLengthMax = "150.0 mm"
    femmesh_obj.CharacteristicLengthMin = "150.0 mm"

    doc.recompute()
    return doc
