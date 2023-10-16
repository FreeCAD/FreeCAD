# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Copyright (c) 2020 Sudhanshu Dubey <sudhanshu.thethunder@gmail.com    *
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

import ObjectsFem

from . import manager
from .ccx_cantilever_base_solid import setup_cantilever_base_solid
from .manager import init_doc


def get_information():
    return {
        "name": "CCX cantilever face load",
        "meshtype": "solid",
        "meshelement": "Tet10",
        "constraints": ["fixed", "force"],
        "solvers": ["calculix", "ccxtools", "elmer", "mystran", "z88"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.ccx_cantilever_faceload import setup
setup()


See forum topic post:
...

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    # setup CalculiX cantilever
    doc = setup_cantilever_base_solid(doc, solvertype)
    analysis = doc.Analysis
    geom_obj = doc.Box

    # constraint force
    con_force = ObjectsFem.makeConstraintForce(doc, "ConstraintForce")
    con_force.References = [(geom_obj, "Face2")]
    con_force.Force = "9000000.0 N"
    con_force.Direction = (geom_obj, ["Edge5"])
    con_force.Reversed = True
    analysis.addObject(con_force)

    doc.recompute()
    return doc
