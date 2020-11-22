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

# to run the example use:
"""
from femexamples.ccx_cantilever_prescribeddisplacement import setup
setup()

"""

import FreeCAD

import ObjectsFem

from .ccx_cantilever_faceload import setup_cantileverbase

mesh_name = "Mesh"  # needs to be Mesh to work with unit tests


def init_doc(doc=None):
    if doc is None:
        doc = FreeCAD.newDocument()
    return doc


def get_information():
    info = {"name": "CCX cantilever prescibed displacement",
            "meshtype": "solid",
            "meshelement": "Tet10",
            "constraints": ["fixed", "displacement"],
            "solvers": ["calculix", "elmer"],
            "material": "solid",
            "equation": "mechanical"
            }
    return info


def setup(doc=None, solvertype="ccxtools"):
    # setup CalculiX cantilever
    # apply a prescribed displacement of 250 mm in -z on the front end face

    if solvertype == "z88":
        # constraint displacement is not supported for Z88
        # pass a not valid solver name for z88, thus no solver is created
        solvertype = "z88_not_valid"

    doc = setup_cantileverbase(doc, solvertype)

    # displacement_constraint
    displacement_constraint = doc.Analysis.addObject(
        ObjectsFem.makeConstraintDisplacement(doc, name="ConstraintDisplacmentPrescribed")
    )[0]
    displacement_constraint.References = [(doc.Box, "Face2")]
    displacement_constraint.zFix = False
    displacement_constraint.zFree = False
    displacement_constraint.zDisplacement = -250.0

    doc.recompute()
    return doc
