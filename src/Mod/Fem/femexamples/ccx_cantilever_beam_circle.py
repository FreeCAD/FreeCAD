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

from . import manager
from .ccx_cantilever_base_edge import setup_cantilever_base_edge
from .manager import init_doc


def get_information():
    return {
        "name": "CCX cantilever beam circle",
        "meshtype": "edge",
        "meshelement": "Seg3",
        "constraints": ["fixed", "force"],
        "solvers": ["calculix", "ccxtools"],
        "material": "solid",
        "equations": ["mechanical"]
    }


def get_explanation(header=""):
    return header + """

To run the example from Python console use:
from femexamples.ccx_cantilever_beam_circle import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=16044

CalculiX cantilever:
- modeled with seg3 beam elements

CrossSection:
- circle
- diameter 1000 mm

max deflection:
doc = App.ActiveDocument
len = doc.CantileverLine.Shape.Length
iyy = doc.CrossSectionFace.Shape.MatrixOfInertia.A22  # Iyy
force = doc.ConstraintForce.Force
from FreeCAD import Units
ym = Units.Quantity(doc.MechanicalMaterial.Material["YoungsModulus"]).getValueAs("MPa")
w = force * len**3 / (3 * ym * iyy)
w  # should print 149.0 mm

CalculiX FEM max deflection:
- 146.7 mm
- Delta ca. 1.5 %

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    diameter = 1000
    cs_wire = doc.addObject("Part::Circle", "WireOfCrossSection")
    cs_wire.Radius = diameter / 2
    cs_wire.Placement = FreeCAD.Placement(
        FreeCAD.Vector(0, 500, 500),
        FreeCAD.Rotation(0, 90, 0),
        FreeCAD.Vector(0, 0, 0),
    )
    doc.recompute()
    cs_face = doc.addObject("Part::Face", "CrossSectionFace")
    cs_face.Sources = cs_wire
    doc.recompute()

    # setup CalculiX cantilever
    doc = setup_cantilever_base_edge(doc, solvertype)
    beamsection_obj = doc.getObject("BeamCrossSection")

    # change cross section to a circular section
    beamsection_obj.SectionType = "Circular"
    beamsection_obj.CircDiameter = diameter

    doc.recompute()
    return doc
