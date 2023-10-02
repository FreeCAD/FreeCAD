# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 FreeCAD Developers                                 *
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
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

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

    autoconstraints(False): if True, constraints will be automatically added to
        wire nodes, rectangles and circles.

    addTo(None) : if set to an existing sketch, geometry will be added to it
        instead of creating a new one.

    delete(False): if True, the original object will be deleted.
        If set to a string 'all' the object and all its linked object will be
        deleted.

    name('Sketch'): the name for the new sketch object.

    radiusPrecision(-1): If <0, disable radius constraint. If =0, add individual
        radius constraint. If >0, the radius will be rounded according to this
        precision, and 'Equal' constraint will be added to curve with equal
        radius within precision.

    tol(1e-3): Tolerance used to check if the shapes are planar and coplanar.
        Consider change to tol=-1 for a more accurate analysis.
    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    import Part
    from Sketcher import Constraint
    import Sketcher

    start_point = 1
    end_point = 2
    middle_point = 3

    if App.GuiUp:
        v_dir = gui_utils.get_3d_view().getViewDirection()
    else:
        v_dir = App.Base.Vector(0,0,-1)

    # lists to accumulate shapes with defined normal and undefined normal
    shape_norm_yes = list()
    shape_norm_no = list()

    if not isinstance(objects_list,(list,tuple)):
        objects_list = [objects_list]

    for obj in objects_list:
        if isinstance(obj,Part.Shape):
            shape = obj
        elif not hasattr(obj,'Shape'):
            App.Console.PrintError(translate("draft",
                                   "No shape found")+"\n")
            return None
        else:
            shape = obj.Shape

        if not DraftGeomUtils.is_planar(shape, tol):
            App.Console.PrintError(translate("draft",
                                   "All Shapes must be planar")+"\n")
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
                                       "All Shapes must be coplanar")+"\n")
                return None
        # define sketch normal
        normal = DraftGeomUtils.get_normal(shapes_list[0], tol)

    else:
        # suppose all geometries are straight lines or points
        points = [vertex.Point for shape in shapes_list for vertex in shape.Vertexes]
        if len(points) >= 2:
            poly = Part.makePolygon(points)
            if not DraftGeomUtils.is_planar(poly, tol):
                App.Console.PrintError(translate("draft",
                                       "All Shapes must be coplanar")+"\n")
                return None
            normal = DraftGeomUtils.get_normal(poly, tol)
            if not normal:
                # all points aligned
                poly_dir = poly.Edges[0].Curve.Direction
                normal = (v_dir - v_dir.dot(poly_dir)*poly_dir).normalize()
                normal = normal.negative()
        else:
            # only one point
            normal = v_dir.negative()


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
                constraints.append(Constraint('Radius',
                        nobj.GeometryCount-1, edge.Curve.Radius))
                return
            r = round(edge.Curve.Radius,radiusPrecision)
            constraints.append(Constraint('Equal',
                    radiuses[r],nobj.GeometryCount-1))
        except KeyError:
            radiuses[r] = nobj.GeometryCount-1
            constraints.append(Constraint('Radius',nobj.GeometryCount-1, r))
        except AttributeError:
            pass

    def convertBezier(edge):
        if DraftGeomUtils.geomType(edge) == "BezierCurve":
            return(edge.Curve.toBSpline(edge.FirstParameter,edge.LastParameter).toShape())
        else:
            return edge


    axis = App.Vector(0, 0, 1).cross(normal)
    angle = DraftVecUtils.angle(normal, App.Vector(0, 0, 1)) * App.Units.Radian
    rotation = App.Rotation(axis, angle)

    point = shapes_list[0].Vertexes[0].Point
    base = App.Vector(normal)
    base.Length = point.dot(base.normalize()) # See https://forum.freecad.org/viewtopic.php?f=22&t=69304#p601149

    nobj.Placement = App.Placement(base, rotation)

    for obj in objects_list:
        ok = False
        tp = utils.get_type(obj)

        if tp in ["Circle","Ellipse"]:
            if obj.Shape.Edges:
                edge = obj.Shape.Edges[0]
                if len(edge.Vertexes) == 1:
                    newedge = DraftGeomUtils.orientEdge(edge, normal)
                    nobj.addGeometry(newedge)
                else:
                    # make new ArcOfCircle
                    circle = DraftGeomUtils.orientEdge(edge, normal)
                    first  = math.radians(obj.FirstAngle)
                    last   = math.radians(obj.LastAngle)
                    arc    = Part.ArcOfCircle(circle, first, last)
                    nobj.addGeometry(arc)
                addRadiusConstraint(edge)
                ok = True

        elif tp in ["Wire", "Rectangle", "Polygon"] and obj.FilletRadius.Value == 0:
            if obj.Shape.Edges:
                for edge in obj.Shape.Edges:
                    nobj.addGeometry(DraftGeomUtils.orientEdge(edge, normal))
                if autoconstraints:
                    closed = tp in ["Rectangle", "Polygon"] or obj.Closed
                    last = nobj.GeometryCount
                    segs = list(range(last - len(obj.Shape.Edges), last))
                    nexts = segs[1:] + ([segs[0]] if closed else [None])
                    for seg, next in zip(segs, nexts):
                        if next is not None:
                            constraints.append(Constraint("Coincident",seg, end_point, next, start_point))
                        if DraftGeomUtils.isAligned(nobj.Geometry[seg], "x"):
                            constraints.append(Constraint("Vertical", seg))
                        elif DraftGeomUtils.isAligned(nobj.Geometry[seg], "y"):
                            constraints.append(Constraint("Horizontal", seg))
                ok = True

        elif tp == "BSpline":
            if obj.Shape.Edges:
                edge = DraftGeomUtils.orientEdge(obj.Shape.Edges[0], normal)
                nobj.addGeometry(edge)
                nobj.exposeInternalGeometry(nobj.GeometryCount-1)
                ok = True

        elif tp == "BezCurve":
            if obj.Shape.Edges:
                for piece in obj.Shape.Edges:
                    bez = piece.Curve
                    bsp = bez.toBSpline(bez.FirstParameter,bez.LastParameter).toShape()
                    edge = DraftGeomUtils.orientEdge(bsp.Edges[0], normal)
                    nobj.addGeometry(edge)
                    nobj.exposeInternalGeometry(nobj.GeometryCount-1)
                ok = True
                # TODO: set coincident constraint for vertexes in multi-edge bezier curve

        elif  tp == "Point":
            shape = obj.Shape.copy()
            if angle:
                shape.rotate(App.Base.Vector(0,0,0), axis, -1*angle)
            point = Part.Point(shape.Point)
            nobj.addGeometry(point)
            ok = True

        elif tp == 'Shape' or hasattr(obj,'Shape'):
            shape = obj if tp == 'Shape' else obj.Shape
            if not shape.Wires:
                for e in shape.Edges:
                    # unconnected edges
                    newedge = convertBezier(e)
                    nobj.addGeometry(DraftGeomUtils.orientEdge(
                                        newedge, normal, make_arc=True))
                    addRadiusConstraint(newedge)

            if autoconstraints:
                for wire in shape.Wires:
                    last_count = nobj.GeometryCount
                    edges = wire.OrderedEdges
                    for edge in edges:
                        newedge = convertBezier(edge)
                        nobj.addGeometry(DraftGeomUtils.orientEdge(
                                            newedge, normal, make_arc=True))
                        addRadiusConstraint(newedge)
                    for i,g in enumerate(nobj.Geometry[last_count:]):
                        if edges[i].Closed:
                            continue
                        seg = last_count+i

                        if DraftGeomUtils.isAligned(g,"x"):
                            constraints.append(Constraint("Vertical",seg))
                        elif DraftGeomUtils.isAligned(g,"y"):
                            constraints.append(Constraint("Horizontal",seg))

                        if seg == nobj.GeometryCount-1:
                            if not wire.isClosed():
                                break
                            g2 = nobj.Geometry[last_count]
                            seg2 = last_count
                        else:
                            seg2 = seg+1
                            g2 = nobj.Geometry[seg2]

                        end1 = g.value(g.LastParameter)
                        start2 = g2.value(g2.FirstParameter)
                        if DraftVecUtils.equals(end1,start2) :
                            constraints.append(Constraint(
                                "Coincident",seg,end_point,seg2,start_point))
                            continue
                        end2 = g2.value(g2.LastParameter)
                        start1 = g.value(g.FirstParameter)
                        if DraftVecUtils.equals(end2,start1):
                            constraints.append(Constraint(
                                "Coincident",seg,start_point,seg2,end_point))
                        elif DraftVecUtils.equals(start1,start2):
                            constraints.append(Constraint(
                                "Coincident",seg,start_point,seg2,start_point))
                        elif DraftVecUtils.equals(end1,end2):
                            constraints.append(Constraint(
                                "Coincident",seg,end_point,seg2,end_point))
            else:
                for wire in shape.Wires:
                    for edge in wire.OrderedEdges:
                        newedge = convertBezier(edge)
                        nobj.addGeometry(DraftGeomUtils.orientEdge(
                                            newedge, normal, make_arc=True))
            ok = True
        gui_utils.format_object(nobj,obj)
        if ok and delete and hasattr(obj,'Shape'):
            doc = obj.Document
            def delObj(obj):
                if obj.InList:
                    App.Console.PrintWarning(translate("draft",
                        "Cannot delete object {} with dependency".format(obj.Label))+"\n")
                else:
                    doc.removeObject(obj.Name)
            try:
                if delete == 'all':
                    objs = [obj]
                    while objs:
                        obj = objs[0]
                        objs = objs[1:] + obj.OutList
                        delObj(obj)
                else:
                    delObj(obj)
            except Exception as ex:
                App.Console.PrintWarning(translate("draft",
                    "Failed to delete object {}: {}".format(obj.Label,ex))+"\n")


    nobj.addConstraint(constraints)

    return nobj


makeSketch = make_sketch

## @}
