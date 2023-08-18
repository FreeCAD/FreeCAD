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
        "name": "CCX cantilever beam pipe",
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
from femexamples.ccx_cantilever_beam_pipe import setup
setup()


See forum topic post:
https://forum.freecad.org/viewtopic.php?f=18&t=16044

CalculiX cantilever:
- modeled with seg3 beam elements

CrossSection:
- pipe
- outer diameter 1000 mm
- thickness 100 mm
- inner diameter = 1000 - 2x100 = 800
- inner radius = 1000/2 - 100 = 400

max deflection:
doc = App.ActiveDocument
len = doc.CantileverLine.Shape.Length
iyy = doc.CrossSectionFace.Shape.MatrixOfInertia.A22  # Iyy
force = doc.ConstraintForce.Force
from FreeCAD import Units
ym = Units.Quantity(doc.MechanicalMaterial.Material["YoungsModulus"]).getValueAs("MPa")
w = force * len**3 / (3 * ym * iyy)
w  # should print 252.4 mm

CalculiX FEM max deflection:
- edit the solver input: element type B32R instead B32R
- 249.8 mm
- Delta ca. 1.0 %

"""


def setup(doc=None, solvertype="ccxtools"):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    # explanation object
    # just keep the following line and change text string in get_explanation method
    manager.add_explanation_obj(doc, get_explanation(manager.get_header(get_information())))

    diameter = 1000
    thickness = 100
    cs_wire_outer = doc.addObject("Part::Circle", "OuterWireOfCrossSection")
    cs_wire_outer.Radius = diameter / 2
    cs_wire_outer.Placement = FreeCAD.Placement(
        FreeCAD.Vector(0, 500, 500),
        FreeCAD.Rotation(0, 90, 0),
        FreeCAD.Vector(0, 0, 0),
    )
    cs_wire_inner = doc.addObject("Part::Circle", "InnerWireOfCrossSection")
    cs_wire_inner.Radius = (diameter / 2) - thickness
    cs_wire_inner.Placement = cs_wire_outer.Placement
    doc.recompute()
    cs_face = doc.addObject("Part::Face", "CrossSectionFace")
    cs_face.Sources = [cs_wire_outer, cs_wire_inner]
    doc.recompute()

    # setup CalculiX cantilever
    doc = setup_cantilever_base_edge(doc, solvertype)
    beamsection_obj = doc.getObject("BeamCrossSection")

    # change cross section to a pipe section
    beamsection_obj.SectionType = "Pipe"
    beamsection_obj.PipeDiameter = diameter
    beamsection_obj.PipeThickness = thickness

    doc.recompute()
    return doc
