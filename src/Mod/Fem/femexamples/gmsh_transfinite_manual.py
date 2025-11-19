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

__title__ = "GMSH transfinite meshing examples"
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
        "name": "GMSH manual transfinite meshing",
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

    group = doc.addObject("App::DocumentObjectGroup", name)
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
                                 FreeCAD.Vector(5,8,0),
                                 FreeCAD.Vector(0,0,0)])

    # 1: simple 3 sided face with tf surface
    explanation = ("A 3 sided furface can be made transfinite. For this the two guiding "
                   "edges need to have the same amount of nodes on them.\n\n"
                   " - The guiding edges of a triangle are Edge1 and Edge3 by default\n"
                   " - In this example the number of nodes on those edges is equal, as they "
                   "are the same length and we use CharachteristicLengthMax setting to subdivide")

    part_obj, mesh_obj = create_example(doc, "Triangle Surface", explanation)
    part_obj.Shape = Part.makeFace(triangle)
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]

    # 2: simple 3 sided face with tf surface and quad mesh
    explanation = ("The transfinite surface of the left example can be created with quad element "
                   "instead of triangles by using the Recombine option")

    part_obj, mesh_obj = create_example(doc, "Triangle Surface Recombined", explanation)
    part_obj.Shape = Part.makeFace(triangle).translate(FreeCAD.Vector(15,0,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True

    # 3: simple 3 sided face with tf surface, quad mesh and transfinite curve guiding
    explanation = ("All edges of the transfinite surface can have arbitrary node distribution, and "
                   "we can manipulate them for example with transfinite curves\n\n"
                   " - The requirement of same amount of nodes for Edge1 and Edge3 still holds. "
                   "This is why we applied the same transfinite curve to those 2\n"
                   " - The two guiding edges can have different transfinite curve settings, as long "
                   "as the node count is the same\n"
                   " - It is also possible to add a transfinite curve to the non guiding surface "
                   "Edge2, this can have any number of nodes")

    part_obj, mesh_obj = create_example(doc, "Triangle Surface Recombined Bumped", explanation)
    part_obj.Shape = Part.makeFace(triangle).translate(FreeCAD.Vector(30,0,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge1", "Edge3"])]
    tf_curv.Distribution = "Bump"
    tf_curv.Coefficient = 5

    # 4: simple 3 sided face with tf surface, quad mesh but with changed orientation
    explanation = ("Sometimes the default orientation of the transfinite meshing is not wanted, and "
                   "we want to use different guide edges. This can be achieved with specifying the "
                   "vertex order manual in the transfinite surface\n\n"
                   " - The order of selection is important: The vertex which connects both guide faces "
                   "needs to be the first one in the list\n"
                   " - You always need to add all 3 vertexes\n"
                   " - This only works for a single surface, when specifying vertexes you cannot use "
                   "multiple surfaces in that object. But you can ad more transfinite surface objects "
                   "gmsh, each having a single face with vertexes")

    part_obj, mesh_obj = create_example(doc, "Triangle Surface Recombined Orientated", explanation)
    part_obj.Shape = Part.makeFace(triangle).translate(FreeCAD.Vector(45,0,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1", "Vertex3", "Vertex2", "Vertex1"])]
    tf_surf.Recombine = True

    smalltriangle = Part.makePolygon([FreeCAD.Vector(0,0,0),
                                      FreeCAD.Vector(10,0,0),
                                      FreeCAD.Vector(5,5,0),
                                      FreeCAD.Vector(0,0,0)])

    # 5: simple 3 sided face with tf surface, made to work with transfinite curves
    explanation = ("If the guiding edges do not have the same amount of nodes by default we need to ensure "
                   "this manually. In this example we have chosen to use transfinite curves to ensure both "
                   "edges have the same amount of nodes")

    part_obj, mesh_obj = create_example(doc, "Triangle Surface Recombined Unequal Guided", explanation)
    part_obj.Shape = Part.makeFace(smalltriangle).translate(FreeCAD.Vector(60,0,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge1", "Edge3"])]

    # 6: simple 3 sided face with tf surface, made to work with changed orientation
    explanation = ("To overcome the unequal amount of nodes in this example we chose different guiding edges, "
                   "choosing the two edges which have equal length and hence equal node count")

    part_obj, mesh_obj = create_example(doc, "Triangle Surface Recombined Unequal Oriented", explanation)
    part_obj.Shape = Part.makeFace(smalltriangle).translate(FreeCAD.Vector(75,0,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1", "Vertex3", "Vertex2", "Vertex1"])]
    tf_surf.Recombine = True

    quad = Part.makePolygon([FreeCAD.Vector(0,0,0),
                             FreeCAD.Vector(10,0,0),
                             FreeCAD.Vector(10,10,0),
                             FreeCAD.Vector(0,10,0),
                             FreeCAD.Vector(0,0,0)])

    # 7: simple 4 sided face with tf surface
    explanation = ("A 4 sided furface can be made transfinite. For this the opposite edges need to have "
                   "the same amount of nodes on them.\n\n"
                   " - For 4 sided faces the edge order does not matter, only the ones opposite to each are relevant\n"
                   " - In this example the number of nodes on those edges is equal, as they have the same length "
                   "and we use CharachteristicLengthMax setting to subdivide")

    part_obj, mesh_obj = create_example(doc, "Quad Surface", explanation)
    part_obj.Shape = Part.makeFace(quad).translate(FreeCAD.Vector(0,-15,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]

    # 8: simple 4 sided face with tf surface and quad mesh
    explanation = ("The transfinite surface of the left example can be created with quad elements "
                   "instead of triangles by using the Recombine option")

    part_obj, mesh_obj = create_example(doc, "Quad Surface Recombined", explanation)
    part_obj.Shape = Part.makeFace(quad).translate(FreeCAD.Vector(15,-15,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True

    # 9: simple 4 sided face with tf surface, quad mesh and transfinite curve guiding
    explanation = ("All edges of the transfinite surface can have arbitrary node distribution, and "
                   "we can manipulate them for example with transfinite curves\n\n"
                   " - The requirement of same amount of nodes for opposite edges still holds. "
                   "This is why we applied the same transfinite curve to the opposite edges\n"
                   " - The opposite edges can have different transfinite curve settings, as long "
                   "as the node count is the same\n"
                   " - It is also possible to only add a transfinite curve to some edges. no matter "
                   "which or how many")

    part_obj, mesh_obj = create_example(doc, "Quad Surface Recombined Bumped", explanation)
    part_obj.Shape = Part.makeFace(quad).translate(FreeCAD.Vector(30,-15,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge1", "Edge3"])]
    tf_curv.Distribution = "Bump"
    tf_curv.Coefficient = 5
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge2", "Edge4"])]
    tf_curv.Distribution = "Bump"
    tf_curv.Coefficient = 5
    tf_curv.Invert = True


    # 10: distorded 4 sided face with tf surface, quad mesh and transfinite curve guiding
    distorted_quad = Part.makePolygon([FreeCAD.Vector(0,0,0),
                                       FreeCAD.Vector(10,3,0),
                                       FreeCAD.Vector(7,10,0),
                                       FreeCAD.Vector(3,5,0),
                                       FreeCAD.Vector(0,0,0)])

    explanation = ("If the edges of the face are not equal length we need to ensure that the node counts"
                   "are correct. The simplest way is to make all 4 edges transfinite.")

    part_obj, mesh_obj = create_example(doc, "Quad Distorted Surface Recombined Guided", explanation)
    part_obj.Shape = Part.makeFace(distorted_quad).translate(FreeCAD.Vector(45,-15,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge1", "Edge2", "Edge3", "Edge4"])]


    # 11: Unregular 4 sided face with tf surface, quad mesh and transfinite curve guiding
    bSpline1 = Part.BSplineCurve()
    bSpline1.interpolate([FreeCAD.Vector(0,0,0), FreeCAD.Vector(3.3,0.5,0),
                          FreeCAD.Vector(6.6,-0.5,0), FreeCAD.Vector(10,0,0)])
    bSpline2 = Part.BSplineCurve()
    bSpline2.interpolate([FreeCAD.Vector(10,0,0), FreeCAD.Vector(10.5,3.3,0),
                          FreeCAD.Vector(9.5,6.6,0), FreeCAD.Vector(10,10,0)])
    bSpline3 = Part.BSplineCurve()
    bSpline3.interpolate([FreeCAD.Vector(10,10,0), FreeCAD.Vector(6.6,10.5,0),
                          FreeCAD.Vector(3.3,9.5,0), FreeCAD.Vector(0,10,0)])
    bSpline4 = Part.BSplineCurve()
    bSpline4.interpolate([FreeCAD.Vector(0,10,0), FreeCAD.Vector(-0.5,7,0), FreeCAD.Vector(0.5,5,0),
                          FreeCAD.Vector(-0.5,3,0), FreeCAD.Vector(0,0,0)])

    wire = Part.Wire([Part.Edge(bSpline1), Part.Edge(bSpline2), Part.Edge(bSpline3), Part.Edge(bSpline4)])
    face = Part.Face(wire)

    explanation = ("The edges of the 4 sided face can be any type, and also be hevaily curved. This holds also "
                   "for triangles, by the way.")

    part_obj, mesh_obj = create_example(doc, "Quad Curved Surface Recombined Guided", explanation)
    part_obj.Shape = face.translate(FreeCAD.Vector(60,-15,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge1", "Edge2", "Edge3", "Edge4"])]


    # 12: Unregular 3D 4 sided face with tf surface, quad mesh and transfinite curve guiding
    bSpline1 = Part.BSplineCurve()
    bSpline1.interpolate([FreeCAD.Vector(0,0,0), FreeCAD.Vector(3.3,1,1),
                          FreeCAD.Vector(6.6,-1,-1), FreeCAD.Vector(10,0,1)])
    bSpline2 = Part.BSplineCurve()
    bSpline2.interpolate([FreeCAD.Vector(10,0,1), FreeCAD.Vector(10,3.3,0),
                          FreeCAD.Vector(10,6.6,2), FreeCAD.Vector(10,10,1)])
    bSpline3 = Part.BSplineCurve()
    bSpline3.interpolate([FreeCAD.Vector(10,10,1), FreeCAD.Vector(6.6,11,-1),
                          FreeCAD.Vector(3.3,9,1), FreeCAD.Vector(0,10,0)])
    bSpline4 = Part.BSplineCurve()
    bSpline4.interpolate([FreeCAD.Vector(0,10,0), FreeCAD.Vector(-1,7,-1), FreeCAD.Vector(1,5,1),
                          FreeCAD.Vector(-1,3,-1), FreeCAD.Vector(0,0,0)])

    wire = Part.Wire([Part.Edge(bSpline1), Part.Edge(bSpline2), Part.Edge(bSpline3), Part.Edge(bSpline4)])
    face = Part.makeFilledFace(wire)

    explanation = ("An of course this works for 3D cuved faces as well")

    part_obj, mesh_obj = create_example(doc, "Quad 3D Curved Surface Recombined Guided", explanation)
    part_obj.Shape = face.translate(FreeCAD.Vector(75,-15,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.References = [(part_obj, ["Face1"])]
    tf_surf.Recombine = True
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge1", "Edge2", "Edge3", "Edge4"])]


    # 13: Hexagon face with 3-sided tf surface
    points = []
    for i in range(6):
        angle = i * 2 * math.pi / 6
        x = 5*math.cos(angle)
        y = 5*math.sin(angle)
        points.append(FreeCAD.Vector(x, y, 0))

    points.append(points[0]) # to close wire
    hexagon = Part.makePolygon(points)

    explanation = ("It is possible to create transfinite surface meshes for faces with arbitrary amount of edges. "
                   "However, for this we need to specify what the corner points are, to guide the transfinite algorithm "
                   "This is done by providing the relevant vertexes in the reference section of transfinite surfaces\n\n"
                   " - There can be 3 or 4 corner points, making the face to a 3 or 4 sided transfinite surface. "
                   "This example uses 3.\n"
                   " - The amount of nodes between the corner points need to be equal for the guiding surface. This "
                   "is the same as for normal 3 sided faces, just now one side is created of multiple edges, for which "
                   "the nodes are added up."
                   " - In the example each edge has the same length, hence nodes. As each side of the transfinite surfce "
                   "consists of 2 edges, the overall node count is the same per side")

    part_obj, mesh_obj = create_example(doc, "Hexagon 3-Sided Surface Recombined", explanation)
    part_obj.Shape = Part.makeFace(hexagon).translate(FreeCAD.Vector(5,-25,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.Recombine = True
    tf_surf.References = [(part_obj, ["Face1", "Vertex1", "Vertex3", "Vertex5"])]

    # 13: Hexagon face with 4-sided tf surface
    explanation = ("The hexagon can also be turned into a 4 sided transfinite surface. For 4 sided transfinite "
                   "surfaces the opposite sides need to have the same amount of nodes. Therefore we group the 6 edges "
                   "in a way to have either 2 edges on opposite side or 1 edge.\n\n"
                   )

    part_obj, mesh_obj = create_example(doc, "Hexagon 4-Sided Surface Recombined", explanation)
    part_obj.Shape = Part.makeFace(hexagon).translate(FreeCAD.Vector(20,-25,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.Recombine = True
    tf_surf.References = [(part_obj, ["Face1", "Vertex2", "Vertex3", "Vertex5", "Vertex6"])]

    # 14: Hexagon face with 3-sided uneven distributed tf surface
    explanation = ("The 6 edges of the hexagon can also be distributed unevenly into 3 sides of the transfinite surface. "
                   "We then need to ensure the same node count for the guiding surfaces. This is done with transfinite curves\n\n"
                   " - The node count of a transfinit curve describes the number of nodes per selected edge. The edge nodes"
                   "include the ones for starting and ending vertex\n"
                   " - If two transfinite edges that share a vertex, this vertex is doubled and one node is dropped.\n"
                   " - Therefore, the total node count of N connected edges is (N x Node - N). This needs to be considerd "
                   "when setting the transfinite curve node number\n"
                   " - For our example: Side 1: 13 Nodes/Edge per edge (1*13-1=12), Side 2: 5 Nodes/Edge (3*5-3=12), Side 3: 7 Nodes/Edge (2*7-2=12) "
                   )

    part_obj, mesh_obj = create_example(doc, "Hexagon 3-Sided Non-symmetric Surface Recombined", explanation)
    part_obj.Shape = Part.makeFace(hexagon).translate(FreeCAD.Vector(35,-25,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.Recombine = True
    tf_surf.References = [(part_obj, ["Face1", "Vertex1", "Vertex2", "Vertex5"])]
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge1"])]
    tf_curv.Nodes = 13
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge2", "Edge3", "Edge4"])]
    tf_curv.Nodes = 5
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge5", "Edge6"])]
    tf_curv.Nodes = 7

    # 15: Hexagon face with 4-sided uneven distributed tf surface
    explanation = ("The 6 edges hexagon can also be used as 4 sided transfinite surface, as shown in this example")

    part_obj, mesh_obj = create_example(doc, "Hexagon 4-Sided Non-symmetric Surface Recombined", explanation)
    part_obj.Shape = Part.makeFace(hexagon).translate(FreeCAD.Vector(50,-25,0))
    tf_surf = ObjectsFem.makeMeshTransfiniteSurface(doc, mesh_obj)
    tf_surf.Recombine = True
    tf_surf.References = [(part_obj, ["Face1", "Vertex1", "Vertex4", "Vertex5", "Vertex6"])]
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge1", "Edge2", "Edge3"])]
    tf_curv.Nodes = 5
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge5"])]
    tf_curv.Nodes = 13
    tf_curv = ObjectsFem.makeMeshTransfiniteCurve(doc, mesh_obj)
    tf_curv.References = [(part_obj, ["Edge4", "Edge6"])]
    tf_curv.Nodes = 6


    return doc







