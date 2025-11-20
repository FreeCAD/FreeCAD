# ***************************************************************************
# *   Copyright (c) 2025 Stefan Tröger <stefantroeger@gmx.net>              *
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

__title__ = "GMSH transfinite meshing examples with automation algorithm"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"


# **************************************************************************
#
# Important: The generated file is used for gmsh tests. Do not change
# content (geometry, options, names etc.) or add/remove content without
# regenerating the gmsh test data files
#
# **************************************************************************


import math

import FreeCAD
import Part

import ObjectsFem
from .manager import init_doc

def get_information():
    return {
        "name": "GMSH automatic transfinite meshing",
        "meshgeneration": "Transfinite",
        "hasanalysis": False
    }


def get_explanation(header=""):
    return (
        header
        + """

To run the example from Python console use:
from femexamples.gmsh_transfinite import setup
setup()

Shows many ways how the transfinite gmsh options can be used to create structured meshes.

"""
    )

def create_example(doc, name, description):
    # setup of a full example document structure
    # returns the part and gmsh mesh object

    group = doc.addObject("App::DocumentObjectGroup", name+" Automated")
    part_obj = group.newObject("Part::Feature", "Geometry")
    mesh_obj = ObjectsFem.makeMeshGmsh(doc, "Mesh")
    mesh_obj.Shape = part_obj
    mesh_obj.CharacteristicLengthMax = 2
    group.addObject(mesh_obj)

    explanation = group.newObject("App::TextDocument","Explanation")
    explanation.Text = description

    if FreeCAD.GuiUp:
        mesh_obj.ViewObject.BackfaceCulling = False

    return part_obj, mesh_obj


def setup(doc=None, solver=None):

    # init FreeCAD document
    if doc is None:
        doc = init_doc()

    triangle = Part.makePolygon([FreeCAD.Vector(0,0,0),
                                 FreeCAD.Vector(10,0,0),
                                 FreeCAD.Vector(10,7,0),
                                 FreeCAD.Vector(0,0,0)])

    # 1: simple 3 edged face with automatic tf surface
    explanation = ("")

    part_obj, mesh_obj = create_example(doc, "Triangle Surface", explanation)
    part_obj.Shape = Part.makeFace(triangle)
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True

    # 2: simple 3 edged face with tf curve on guiding edge
    explanation = ("")

    part_obj, mesh_obj = create_example(doc, "Triangle Surface Guided", explanation)
    part_obj.Shape = Part.makeFace(triangle).translate(FreeCAD.Vector(15, 0, 0))
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge1"])]
    tf_curv.Nodes = 5
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True

    # 3: simple 3 edged face with tf curve on non-guiding edge
    explanation = ("")

    part_obj, mesh_obj = create_example(doc, "Triangle Surface Guided Opposing", explanation)
    part_obj.Shape = Part.makeFace(triangle).translate(FreeCAD.Vector(30, 0, 0))
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge2"])]
    tf_curv.Nodes = 5
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True


    # 4: Triangle strip
    triangle_2 = Part.makePolygon([FreeCAD.Vector(10,0,0),
                                   FreeCAD.Vector(10,7,0),
                                   FreeCAD.Vector(20,0,0),
                                   FreeCAD.Vector(10,0,0),])

    triangle_3 = Part.makePolygon([FreeCAD.Vector(10,7,0),
                                   FreeCAD.Vector(20,0,0),
                                   FreeCAD.Vector(20,7,0),
                                   FreeCAD.Vector(10,7,0)])

    triangle_strip = Part.makeFace(triangle).generalFuse([Part.makeFace(triangle_2), Part.makeFace(triangle_3)])[0]

    explanation = ("")

    part_obj, mesh_obj = create_example(doc, "Triangle Strip", explanation)
    part_obj.Shape = triangle_strip.copy().translate(FreeCAD.Vector(45, 0, 0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, [f"Face{i}" for i in range(1,4)])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True

    # 4: Triangle strip guided
    explanation = ("")

    part_obj, mesh_obj = create_example(doc, "Triangle Strip Guided", explanation)
    part_obj.Shape = triangle_strip.copy().translate(FreeCAD.Vector(70, 0, 0))
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge5"])]
    tf_curv.Nodes = 5
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, [f"Face{i}" for i in range(1,4)])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True

    # 5: Triangle strip oriented
    explanation = ("")

    part_obj, mesh_obj = create_example(doc, "Triangle Strip Oriented", explanation)
    part_obj.Shape = triangle_strip.copy().translate(FreeCAD.Vector(95, 0, 0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1", "Face2", "Face3", "Vertex3"])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True

    # 6: Triangle strip oriented guided
    explanation = ("")

    part_obj, mesh_obj = create_example(doc, "Triangle Strip Oriented Guided", explanation)
    part_obj.Shape = triangle_strip.copy().translate(FreeCAD.Vector(120, 0, 0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1", "Face2", "Face3", "Vertex3"])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge7", "Edge5"])]
    tf_curv.Nodes = 5


    # 7: Simple 4 edged face with automatic tf surface for unequal side lengths
    quad = Part.makePolygon([FreeCAD.Vector(0,0,0),
                              FreeCAD.Vector(10,0,0),
                              FreeCAD.Vector(10,10,0),
                              FreeCAD.Vector(4,7,0),
                              FreeCAD.Vector(0,0,0)])
    explanation = ("")

    part_obj, mesh_obj = create_example(doc, "Quad Surface", explanation)
    part_obj.Shape = Part.makeFace(quad).translate(FreeCAD.Vector(0, -15, 0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True

    # 8: Quad/Triangle strip with tr surface for unequal side lengths
    quad1 = Part.makePolygon([FreeCAD.Vector(0,3,0),
                              FreeCAD.Vector(10,5,0),
                              FreeCAD.Vector(10,10,0),
                              FreeCAD.Vector(4,7,0),
                              FreeCAD.Vector(0,3,0)])

    triangle1 = Part.makePolygon([FreeCAD.Vector(0,3,0),
                                  FreeCAD.Vector(10,0,0),
                                  FreeCAD.Vector(10,5,0),
                                  FreeCAD.Vector(0,3,0)])

    quad2 = Part.makePolygon([FreeCAD.Vector(10,0,0),
                              FreeCAD.Vector(10,5,0),
                              FreeCAD.Vector(17,5,0),
                              FreeCAD.Vector(20,0,0),
                              FreeCAD.Vector(10,0,0)])

    triangle2 = Part.makePolygon([FreeCAD.Vector(10,5,0),
                                  FreeCAD.Vector(17,5,0),
                                  FreeCAD.Vector(10,10,0),
                                  FreeCAD.Vector(10,5,0)])

    quad_strip = Part.makeFace(quad1).generalFuse([Part.makeFace(triangle1),
                                                   Part.makeFace(quad2),
                                                   Part.makeFace(triangle2)])[0]
    part_obj, mesh_obj = create_example(doc, "Quad Strip Surface", explanation)
    part_obj.Shape = quad_strip.copy().translate(FreeCAD.Vector(20, -15, 0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, [f"Face{i}" for i in range(1,5)])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True

    # 9: Quad/Triangle strip with tr surface for unequal side lengths but oriented
    part_obj, mesh_obj = create_example(doc, "Quad Strip Surface Oriented", explanation)
    part_obj.Shape = quad_strip.copy().translate(FreeCAD.Vector(45, -15, 0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, [f"Face{i}" for i in range(1,5)] + ["Vertex2"])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True

    # 9: Quad/Triangle strip with tr surface for unequal side lengths but oriented
    part_obj, mesh_obj = create_example(doc, "Quad Strip Surface Oriented Bumped", explanation)
    part_obj.Shape = quad_strip.copy().translate(FreeCAD.Vector(70, -15, 0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, [f"Face{i}" for i in range(1,5)] + ["Vertex2"])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge4", "Edge8"])]
    tf_curv.Nodes = 5
    tf_curv.Distribution = "Bump"
    tf_curv.Coefficient = 5

    # 10: Quad/Triangle strip with tr surface for unequal side lengths but oriented
    part_obj, mesh_obj = create_example(doc, "Quad Strip Surface Double Oriented Bumped", explanation)
    part_obj.Shape = quad_strip.copy().translate(FreeCAD.Vector(95, -15, 0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, [f"Face{i}" for i in range(1,5)] + ["Vertex3", "Vertex5"])]
    tf_surf.Recombine = True
    tf_surf.UseAutomation = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge4", "Edge8"])]
    tf_curv.Nodes = 5
    tf_curv.Distribution = "Bump"
    tf_curv.Coefficient = 5


    # 11: Wedge default
    wedge = Part.makeFace(triangle).extrude(FreeCAD.Vector(2,0,10))
    part_obj, mesh_obj = create_example(doc, "Wedge Volume", explanation)
    part_obj.Shape = wedge.copy().translate(FreeCAD.Vector(0, -30, 0))
    tf_vol = ObjectsFem.makeMeshTransfiniteVolume(doc, mesh_obj)
    tf_vol.References = [(part_obj, ["Solid1"])]
    tf_vol.UseAutomation = True
    tf_vol.Recombine = True

    # 11: Wedge default with a surface to orient the triangle
    wedge = Part.makeFace(triangle).extrude(FreeCAD.Vector(2,0,10))
    part_obj, mesh_obj = create_example(doc, "Wedge Volume Oriented", explanation)
    part_obj.Shape = wedge.copy().translate(FreeCAD.Vector(15, -30, 0))
    tf_vol = ObjectsFem.makeMeshTransfiniteVolume(doc, mesh_obj)
    tf_vol.References = [(part_obj, ["Solid1"])]
    tf_vol.UseAutomation = True
    tf_vol.Recombine = True
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face5", "Vertex4"])]
    tf_surf.Recombine = True

    # 12: Wedge default with a surface to orient the triangle
    wedge = Part.makeFace(triangle).extrude(FreeCAD.Vector(2,0,10))
    part_obj, mesh_obj = create_example(doc, "Wedge Volume Oriented Bumped", explanation)
    part_obj.Shape = wedge.copy().translate(FreeCAD.Vector(30, -30, 0))
    tf_vol = ObjectsFem.makeMeshTransfiniteVolume(doc, mesh_obj)
    tf_vol.References = [(part_obj, ["Solid1"])]
    tf_vol.UseAutomation = True
    tf_vol.Recombine = True
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face5", "Vertex4"])]
    tf_surf.Recombine = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge2", "Edge4"])]
    tf_curv.Nodes = 7
    tf_curv.Distribution = "Bump"
    tf_curv.Coefficient = 5
    tf_curv.Invert = True


    return doc







