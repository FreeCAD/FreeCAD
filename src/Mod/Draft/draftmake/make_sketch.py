# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
# *   Copyright (c) 2024 FreeCAD Project Association                        *
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
"""Provides functions to create Sketch objects from Draft objects."""
## @package make_sketch
# \ingroup draftmake
# \brief Provides functions to create Sketch objects from Draft objects.

## \addtogroup draftmake
# @{
import math

import FreeCAD as App
import DraftVecUtils
import DraftGeomUtils
from draftutils import gui_utils
from draftutils import utils
from draftutils.translate import translate


def make_sketch(objects_list, autoconstraints=False, addTo=None,
                delete=False, name="Sketch", radiusPrecision=-1, tol=1e-3):
    """make_sketch(objects_list, [autoconstraints], [addTo], [delete],
                   [name], [radiusPrecision], [tol])

    Makes a Sketch objects_list with the given Draft objects.

    Parameters
    ----------
    objects_list: can be single or list of objects of Draft type objects,
        Part::Feature, Part.Shape, or mix of them.

    autoconstraints(False): if True, coincident, horizontal and vertical
        constraints will be added automatically.

    addTo(None): if set to an existing sketch, geometry will be added to it
        instead of creating a new one.

    delete(False): if True, the original object will be deleted.
        If set to a string "all" the object and all its linked object will be
        deleted.

    name("Sketch"): the name for the new sketch object.

    radiusPrecision(-1): If <0, disable radius constraint. If =0, add individual
        radius constraint. If >0, the radius will be rounded according to this
        precision, and "Equal" constraint will be added to curve with equal
        radius within precision.

    tol(1e-3): Tolerance used to check if the shapes are planar and coplanar.
        Consider change to tol=-1 for a more accurate analysis.
    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    import Part
    from Sketcher import Constraint

    # lists to accumulate shapes with defined normal and undefined normal
    shape_norm_yes = list()
    shape_norm_no = list()

    if not isinstance(objects_list,(list,tuple)):
        objects_list = [objects_list]

    for obj in objects_list:
        if isinstance(obj,Part.Shape):
            shape = obj
        elif not hasattr(obj, "Shape"):
            App.Console.PrintError(translate("draft",
                                   "No shape found") + "\n")
            return None
        else:
            shape = obj.Shape

        if not DraftGeomUtils.is_planar(shape, tol):
            App.Console.PrintError(translate("draft",
                                   "All Shapes must be planar") + "\n")
            return None

        if DraftGeomUtils.get_normal(shape, tol):
            shape_norm_yes.append(shape)
        else:
            shape_norm_no.append(shape)


    shapes_list = shape_norm_yes + shape_norm_no

    # test if all shapes are coplanar
    if len(shape_norm_yes) >= 1:
        for shape in shapes_list[1:]:
            if not DraftGeomUtils.are_coplanar(shapes_list[0], shape, tol):
                App.Console.PrintError(translate("draft",
                                       "All Shapes must be coplanar") + "\n")
                return None
        # define sketch normal
        normal = DraftGeomUtils.get_normal(shapes_list[0], tol)

    else:
        # suppose all geometries are straight lines or points
        points = [vertex.Point for shape in shapes_list for vertex in shape.Vertexes]
        if len(points) >= 2:
            try:
                poly = Part.makePolygon(points)
            except Part.OCCError:
                # all points coincide
                normal = App.Vector(0, 0, 1)
            else:
                if not DraftGeomUtils.is_planar(poly, tol):
                    App.Console.PrintError(translate("draft",
                                           "All Shapes must be coplanar") + "\n")
                    return None
                normal = DraftGeomUtils.get_shape_normal(poly)
        else:
            # only one point
            normal = App.Vector(0, 0, 1)

    if addTo:
        nobj = addTo
    else:
        nobj = App.ActiveDocument.addObject("Sketcher::SketchObject", name)

    # Collect constraints and add in one go to improve performance
    constraints = []
    radiuses = {}

    def addRadiusConstraint(edge):
        try:
            if radiusPrecision<0:
                return
            if radiusPrecision==0:
                constraints.append(Constraint("Radius",
                        nobj.GeometryCount-1, edge.Curve.Radius))
                return
            r = round(edge.Curve.Radius,radiusPrecision)
            constraints.append(Constraint("Equal",
                    radiuses[r], nobj.GeometryCount-1))
        except KeyError:
            radiuses[r] = nobj.GeometryCount-1
            constraints.append(Constraint("Radius", nobj.GeometryCount-1, r))
        except AttributeError:
            pass

    def convertBezier(edge):
        if DraftGeomUtils.geomType(edge) == "BezierCurve":
            return(edge.Curve.toBSpline(edge.FirstParameter,edge.LastParameter).toShape())
        else:
            return edge

    axis = App.Vector(0, 0, 1).cross(normal)
    if axis.Length > 1e-6:
        axis.normalize()
    elif normal.z >= 0:
        axis = App.Vector(0, 0, 1)
    else:
        axis = App.Vector(0, 0, -1)
    angle = math.degrees(DraftVecUtils.angle(normal, App.Vector(0, 0, 1)))
    rotation = App.Rotation(axis, angle)

    point = shapes_list[0].Vertexes[0].Point
    base = App.Vector(normal)
    base.Length = point.dot(base.normalize()) # See https://forum.freecad.org/viewtopic.php?f=22&t=69304#p601149

    nobj.Placement = App.Placement(base, rotation)

    for obj in objects_list:
        ok = False
        tp = utils.get_type(obj)

        if tp == "Point":
            # obj.Shape.copy() does not work properly for a Draft_Point.
            # The coords of the point are multiplied by 2.
            # We therefore create a Part Vertex instead.
            shape = Part.Vertex(obj.Shape.Point)
            if angle:
                shape.rotate(App.Vector(0, 0, 0), axis, -angle)
            point = Part.Point(shape.Point)
            nobj.addGeometry(point)
            ok = True

        elif tp == "Shape" or hasattr(obj, "Shape"):
            shape = obj if tp == "Shape" else obj.Shape
            for e in shape.Edges:
                newedge = convertBezier(e)
                nobj.addGeometry(DraftGeomUtils.orientEdge(
                                    newedge, normal, make_arc=True))
                addRadiusConstraint(newedge)
            ok = True

        if ok and delete:
            def delObj(obj):
                if obj.InList:
                    App.Console.PrintWarning(translate("draft",
                        "Cannot delete object {} with dependency".format(obj.Label)) + "\n")
                else:
                    obj.Document.removeObject(obj.Name)
            try:
                if delete == "all":
                    objs = [obj]
                    while objs:
                        obj = objs[0]
                        objs = objs[1:] + obj.OutList
                        delObj(obj)
                else:
                    delObj(obj)
            except Exception as ex:
                App.Console.PrintWarning(translate("draft",
                    "Failed to delete object {}: {}".format(obj.Label, ex)) + "\n")

    nobj.addConstraint(constraints)
    if autoconstraints:
        nobj.detectMissingPointOnPointConstraints(utils.tolerance())
        nobj.makeMissingPointOnPointCoincident(False)
        nobj.detectMissingVerticalHorizontalConstraints(utils.tolerance())
        nobj.makeMissingVerticalHorizontal(False)
        # The MissingVerticalHorizontal functions do not work properly.
        # If elements are added to an existing sketch redundant constraints are created.
        # This can happen if DXF files are imported with the legacy importer.
        # https://forum.freecad.org/viewtopic.php?t=97072
        # https://github.com/FreeCAD/FreeCAD/issues/21396
        # To address this we check for redundant constraints. This can be necessary even
        # the functions were to work properly. For example with this scenario:
        # https://github.com/FreeCAD/FreeCAD/issues/19978
        nobj.solve()
        for idx in nobj.RedundantConstraints[::-1]:
            # Output of RedundantConstraints is one-based.
            nobj.delConstraint(idx - 1)

    return nobj


makeSketch = make_sketch

## @}
