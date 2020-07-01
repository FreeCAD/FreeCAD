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


def make_sketch(objectslist, autoconstraints=False, addTo=None,
                delete=False, name="Sketch", radiusPrecision=-1):
    """makeSketch(objectslist,[autoconstraints],[addTo],[delete],[name],[radiusPrecision])

    Makes a Sketch objectslist with the given Draft objects.

    Parameters
    ----------
    objectlist: can be single or list of objects of Draft type objects,
        Part::Feature, Part.Shape, or mix of them.

    autoconstraints(False): if True, constraints will be automatically added to
        wire nodes, rectangles and circles.

    addTo(None) : if set to an existing sketch, geometry will be added to it
        instead of creating a new one.

    delete(False): if True, the original object will be deleted.
        If set to a string 'all' the object and all its linked object will be
        deleted

    name('Sketch'): the name for the new sketch object

    radiusPrecision(-1): If <0, disable radius constraint. If =0, add indiviaul
        radius constraint. If >0, the radius will be rounded according to this
        precision, and 'Equal' constraint will be added to curve with equal
        radius within precision.
    """

    if not App.ActiveDocument:
        App.Console.PrintError("No active document. Aborting\n")
        return

    import Part
    from Sketcher import Constraint
    import Sketcher

    StartPoint = 1
    EndPoint = 2
    MiddlePoint = 3
    deletable = None

    if not isinstance(objectslist,(list,tuple)):
        objectslist = [objectslist]
    for obj in objectslist:
        if isinstance(obj,Part.Shape):
            shape = obj
        elif not hasattr(obj,'Shape'):
            App.Console.PrintError(translate("draft","not shape found"))
            return None
        else:
            shape = obj.Shape
        if not DraftGeomUtils.isPlanar(shape):
            App.Console.PrintError(translate("draft","All Shapes must be co-planar"))
            return None
    if addTo:
        nobj = addTo
    else:
        nobj = App.ActiveDocument.addObject("Sketcher::SketchObject", name)
        deletable = nobj
        if App.GuiUp:
            nobj.ViewObject.Autoconstraints = False

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
            return(edge)

    rotation = None
    for obj in objectslist:
        ok = False
        tp = utils.get_type(obj)
        if tp in ["Circle","Ellipse"]:
            if obj.Shape.Edges:
                if rotation is None:
                    rotation = obj.Placement.Rotation
                edge = obj.Shape.Edges[0]
                if len(edge.Vertexes) == 1:
                    newEdge = DraftGeomUtils.orientEdge(edge)
                    nobj.addGeometry(newEdge)
                else:
                    # make new ArcOfCircle
                    circle = DraftGeomUtils.orientEdge(edge)
                    angle  = edge.Placement.Rotation.Angle
                    axis   = edge.Placement.Rotation.Axis
                    circle.Center = DraftVecUtils.rotate(edge.Curve.Center, -angle, axis)
                    first  = math.radians(obj.FirstAngle)
                    last   = math.radians(obj.LastAngle)
                    arc    = Part.ArcOfCircle(circle, first, last)
                    nobj.addGeometry(arc)
                addRadiusConstraint(edge)
                ok = True
        elif tp == "Rectangle":
            if rotation is None:
                rotation = obj.Placement.Rotation
            if obj.FilletRadius.Value == 0:
                for edge in obj.Shape.Edges:
                    nobj.addGeometry(DraftGeomUtils.orientEdge(edge))
                if autoconstraints:
                    last = nobj.GeometryCount - 1
                    segs = [last-3,last-2,last-1,last]
                    if obj.Placement.Rotation.Q == (0,0,0,1):
                        constraints.append(Constraint("Coincident",last-3,EndPoint,last-2,StartPoint))
                        constraints.append(Constraint("Coincident",last-2,EndPoint,last-1,StartPoint))
                        constraints.append(Constraint("Coincident",last-1,EndPoint,last,StartPoint))
                        constraints.append(Constraint("Coincident",last,EndPoint,last-3,StartPoint))
                    constraints.append(Constraint("Horizontal",last-3))
                    constraints.append(Constraint("Vertical",last-2))
                    constraints.append(Constraint("Horizontal",last-1))
                    constraints.append(Constraint("Vertical",last))
                ok = True
        elif tp in ["Wire","Polygon"]:
            if obj.FilletRadius.Value == 0:
                closed = False
                if tp == "Polygon":
                    closed = True
                elif hasattr(obj,"Closed"):
                    closed = obj.Closed

                if obj.Shape.Edges:
                    if (len(obj.Shape.Vertexes) < 3):
                        e = obj.Shape.Edges[0]
                        nobj.addGeometry(Part.LineSegment(e.Curve,e.FirstParameter,e.LastParameter))
                    else:
                        # Use the first three points to make a working plane. We've already
                        # checked to make sure everything is coplanar
                        plane = Part.Plane(*[i.Point for i in obj.Shape.Vertexes[:3]])
                        normal = plane.Axis
                        if rotation is None:
                            axis = App.Vector(0,0,1).cross(normal)
                            angle = DraftVecUtils.angle(normal, App.Vector(0,0,1)) * App.Units.Radian
                            rotation = App.Rotation(axis, angle)
                        for edge in obj.Shape.Edges:
                            # edge.rotate(App.Vector(0,0,0), rotAxis, rotAngle)
                            edge = DraftGeomUtils.orientEdge(edge, normal)
                            nobj.addGeometry(edge)
                        if autoconstraints:
                            last = nobj.GeometryCount
                            segs = list(range(last-len(obj.Shape.Edges),last-1))
                            for seg in segs:
                                constraints.append(Constraint("Coincident",seg,EndPoint,seg+1,StartPoint))
                                if DraftGeomUtils.isAligned(nobj.Geometry[seg],"x"):
                                    constraints.append(Constraint("Vertical",seg))
                                elif DraftGeomUtils.isAligned(nobj.Geometry[seg],"y"):
                                    constraints.append(Constraint("Horizontal",seg))
                            if closed:
                                constraints.append(Constraint("Coincident",last-1,EndPoint,segs[0],StartPoint))
                    ok = True
        elif tp == "BSpline":
            if obj.Shape.Edges:
                nobj.addGeometry(obj.Shape.Edges[0].Curve)
                nobj.exposeInternalGeometry(nobj.GeometryCount-1)
                ok = True
        elif tp == "BezCurve":
            if obj.Shape.Edges:
                bez = obj.Shape.Edges[0].Curve
                bsp = bez.toBSpline(bez.FirstParameter,bez.LastParameter)
                nobj.addGeometry(bsp)
                nobj.exposeInternalGeometry(nobj.GeometryCount-1)
                ok = True
        elif tp == 'Shape' or hasattr(obj,'Shape'):
            shape = obj if tp == 'Shape' else obj.Shape

            if not DraftGeomUtils.isPlanar(shape):
                App.Console.PrintError(translate("draft","The given object is not planar and cannot be converted into a sketch."))
                return None
            if rotation is None:
                #rotation = obj.Placement.Rotation
                norm = DraftGeomUtils.getNormal(shape)
                if norm:
                    rotation = App.Rotation(App.Vector(0,0,1),norm)
                else:
                    App.Console.PrintWarning(translate("draft","Unable to guess the normal direction of this object"))
                    rotation = App.Rotation()
                    norm = obj.Placement.Rotation.Axis
            if not shape.Wires:
                for e in shape.Edges:
                    # unconnected edges
                    newedge = convertBezier(e)
                    nobj.addGeometry(DraftGeomUtils.orientEdge(newedge,norm,make_arc=True))
                    addRadiusConstraint(newedge)

            # if not addTo:
                # nobj.Placement.Rotation = DraftGeomUtils.calculatePlacement(shape).Rotation

            if autoconstraints:
                for wire in shape.Wires:
                    last_count = nobj.GeometryCount
                    edges = wire.OrderedEdges
                    for edge in edges:
                        newedge = convertBezier(edge)
                        nobj.addGeometry(DraftGeomUtils.orientEdge(
                                            newedge,norm,make_arc=True))
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
                                "Coincident",seg,EndPoint,seg2,StartPoint))
                            continue
                        end2 = g2.value(g2.LastParameter)
                        start1 = g.value(g.FirstParameter)
                        if DraftVecUtils.equals(end2,start1):
                            constraints.append(Constraint(
                                "Coincident",seg,StartPoint,seg2,EndPoint))
                        elif DraftVecUtils.equals(start1,start2):
                            constraints.append(Constraint(
                                "Coincident",seg,StartPoint,seg2,StartPoint))
                        elif DraftVecUtils.equals(end1,end2):
                            constraints.append(Constraint(
                                "Coincident",seg,EndPoint,seg2,EndPoint))
            else:
                for wire in shape.Wires:
                    for edge in wire.OrderedEdges:
                        newedge = convertBezier(edge)
                        nobj.addGeometry(DraftGeomUtils.orientEdge(
                                                newedge,norm,make_arc=True))
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
    if rotation:
        nobj.Placement.Rotation = rotation
    else:
        print("-----error!!! rotation is still None...")
    nobj.addConstraint(constraints)

    return nobj


makeSketch = make_sketch

## @}
