# ***************************************************************************
# *   Copyright (c) 2010 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2010 Ken Cline <cline@frii.com>                         *
# *   Copyright (c) 2020 Carlo Pavan <carlopav@gmail.com>                   *
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
"""Provides support functions to edit Draft objects.

All functions in this module work with Object coordinate space.
No conversion to global coordinate system is needed.

To support an new Object, Draft_Edit needs at least two functions:
getObjectPts(obj): returns a list of points on which Draft_Edit will display
    edit trackers
updateObject(obj, nodeIndex, v): update the give object according to the
    index of a moved edit tracker and the vector of the displacement
TODO: Abstract the code that handles the preview and move the object specific
    code to this module from main Draft_Edit module
"""
## @package gui_edit_draft_objects
# \ingroup draftguitools
# \brief Provides support functions to edit Draft objects.

__title__ = "FreeCAD Draft Edit Tool"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin, Carlo Pavan")
__url__ = "https://www.freecadweb.org"

## \addtogroup draftguitools
# @{
import math
import FreeCAD as App
import DraftVecUtils

from draftutils.translate import translate
import draftutils.utils as utils

def get_supported_draft_objects():
    return ["Wire", "BSpline", "Rectangle", "Circle", "Ellipse", "Polygon",
            "BezCurve",
            "Dimension", "LinearDimension"]


# -------------------------------------------------------------------------
# EDIT OBJECT TOOLS : Line/Wire/BSpline
# -------------------------------------------------------------------------

def getWirePts(obj):
    editpoints = []
    for p in obj.Points:
        editpoints.append(p)
    return editpoints

def updateWire(obj, nodeIndex, v):
    pts = obj.Points
    tol = 0.001 # TODO : Use default precision
    if (nodeIndex == 0 and (v - pts[-1]).Length < tol ):
        # DNC: user moved first point over last point -> Close curve
        obj.Closed = True
        pts[0] = v
        del pts[-1]
        obj.Points = pts
        return
    elif nodeIndex == len(pts) - 1 and (v - pts[0]).Length < tol:
        # DNC: user moved last point over first point -> Close curve
        obj.Closed = True
        del pts[-1]
        obj.Points = pts
        return
    elif v in pts:
        # DNC: checks if point enter is equal to other, this could cause a OCC problem
        _err = translate("draft", "This object does not support possible "
                                  "coincident points, please try again.")
        App.Console.PrintMessage(_err + "\n")
        return

    if obj.Closed:
        # DNC: project the new point to the plane of the face if present
        if hasattr(obj.Shape, "normalAt"):
            normal = obj.Shape.normalAt(0,0)
            point_on_plane = obj.Shape.Vertexes[0].Point
            v.projectToPlane(point_on_plane, normal)

    pts[nodeIndex] = v
    obj.Points = pts


# -------------------------------------------------------------------------
# EDIT OBJECT TOOLS : Rectangle
# -------------------------------------------------------------------------

def getRectanglePts(obj):
    """Return the list of edipoints for the given Draft Rectangle.

    0 : Placement.Base
    1 : Length
    2 : Height
    """
    editpoints = []
    editpoints.append(App.Vector(0, 0, 0))
    editpoints.append(App.Vector(obj.Length, 0, 0))
    editpoints.append(App.Vector(0, obj.Height, 0))
    return editpoints

def updateRectangle(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.Placement.Base = obj.Placement.multVec(v)
    elif nodeIndex == 1:
        obj.Length = DraftVecUtils.project(v, App.Vector(1,0,0)).Length
    elif nodeIndex == 2:
        obj.Height = DraftVecUtils.project(v, App.Vector(0,1,0)).Length


# -------------------------------------------------------------------------
# EDIT OBJECT TOOLS : Circle/Arc
# -------------------------------------------------------------------------

def getCirclePts(obj):
    """Return the list of edipoints for the given Draft Arc or Circle.

    circle:
    0 : Placement.Base or center
    1 : radius

    arc:
    0 : Placement.Base or center
    1 : first endpoint
    2 : second endpoint
    3 : midpoint
    """
    editpoints = []
    editpoints.append(App.Vector(0, 0, 0))
    if obj.FirstAngle == obj.LastAngle:
        # obj is a circle
        editpoints.append(App.Vector(obj.Radius,0,0))
    else:
        # obj is an arc
        editpoints.append(getArcStart(obj))#First endpoint
        editpoints.append(getArcEnd(obj))#Second endpoint
        editpoints.append(getArcMid(obj))#Midpoint
    return editpoints


def updateCircle(obj, nodeIndex, v, alt_edit_mode=0):
    if obj.FirstAngle == obj.LastAngle:
        # object is a circle
        if nodeIndex == 0:
            obj.Placement.Base = obj.Placement.multVec(v)
        elif nodeIndex == 1:
            obj.Radius = v.Length

    else:
        # obj is an arc
        if alt_edit_mode == 0:
            import Part
            if nodeIndex == 0:
                # center point
                p1 = getArcStart(obj)
                p2 = getArcEnd(obj)
                p0 = DraftVecUtils.project(v, getArcMid(obj))
                obj.Radius = p1.sub(p0).Length
                obj.FirstAngle = -math.degrees(DraftVecUtils.angle(p1.sub(p0)))
                obj.LastAngle = -math.degrees(DraftVecUtils.angle(p2.sub(p0)))
                obj.Placement.Base = obj.Placement.multVec(p0)

            else:
                """ Edit arc by 3 points.
                """
                v= obj.Placement.multVec(v)
                p1 = obj.Placement.multVec(getArcStart(obj))
                p2 = obj.Placement.multVec(getArcMid(obj))
                p3 = obj.Placement.multVec(getArcEnd(obj))
                
                if nodeIndex == 1:  # first point
                    p1 = v
                elif nodeIndex == 3:  # midpoint
                    p2 = v
                elif nodeIndex == 2:  # second point
                    p3 = v

                arc=Part.ArcOfCircle(p1, p2, p3)
                import Part
                s = arc.toShape()
                # Part.show(s) DEBUG
                p0 = arc.Location
                obj.Placement.Base = p0
                obj.Radius = arc.Radius
                
                delta = s.Vertexes[0].Point
                obj.FirstAngle = -math.degrees(DraftVecUtils.angle(p1.sub(p0)))
                delta = s.Vertexes[1].Point
                obj.LastAngle = -math.degrees(DraftVecUtils.angle(p3.sub(p0)))

        elif alt_edit_mode == 1:
            # edit arc by center radius FirstAngle LastAngle
            if nodeIndex == 0:
                obj.Placement.Base = obj.Placement.multVec(v)
            else:
                dangle = math.degrees(math.atan2(v[1],v[0]))
                if nodeIndex == 1:
                    obj.FirstAngle = dangle
                elif nodeIndex == 2:
                    obj.LastAngle = dangle
                elif nodeIndex == 3:
                    obj.Radius = v.Length

    obj.recompute()


def getArcStart(obj, global_placement=False):#Returns object midpoint
    if utils.get_type(obj) == "Circle":
        return pointOnCircle(obj, obj.FirstAngle, global_placement)


def getArcEnd(obj, global_placement=False):#Returns object midpoint
    if utils.get_type(obj) == "Circle":
        return pointOnCircle(obj, obj.LastAngle, global_placement)


def getArcMid(obj, global_placement=False):#Returns object midpoint
    if utils.get_type(obj) == "Circle":
        if obj.LastAngle > obj.FirstAngle:
            midAngle = obj.FirstAngle + (obj.LastAngle - obj.FirstAngle) / 2.0
        else:
            midAngle = obj.FirstAngle + (obj.LastAngle - obj.FirstAngle) / 2.0
            midAngle += App.Units.Quantity(180,App.Units.Angle)
        return pointOnCircle(obj, midAngle, global_placement)


def pointOnCircle(obj, angle, global_placement=False):
    if utils.get_type(obj) == "Circle":
        px = obj.Radius * math.cos(math.radians(angle))
        py = obj.Radius * math.sin(math.radians(angle))
        p = App.Vector(px, py, 0.0)
        if global_placement == True:
            p = obj.getGlobalPlacement().multVec(p)
        return p
    return None


def arcInvert(obj):
    obj.FirstAngle, obj.LastAngle = obj.LastAngle, obj.FirstAngle
    obj.recompute()


# -------------------------------------------------------------------------
# EDIT OBJECT TOOLS : Ellipse
# -------------------------------------------------------------------------

def getEllipsePts(obj):
    editpoints = []
    editpoints.append(App.Vector(0, 0, 0))
    editpoints.append(App.Vector(obj.MajorRadius, 0, 0))
    editpoints.append(App.Vector(0, obj.MinorRadius, 0))
    return editpoints

def updateEllipse(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.Placement.Base = obj.Placement.multVec(v)
    elif nodeIndex == 1:
        if v.Length >= obj.MinorRadius:
            obj.MajorRadius = v.Length
        else:
            obj.MajorRadius = obj.MinorRadius
    elif nodeIndex == 2:
        if v.Length <= obj.MajorRadius:
            obj.MinorRadius = v.Length
        else:
            obj.MinorRadius = obj.MajorRadius
    obj.recompute()


# -------------------------------------------------------------------------
# EDIT OBJECT TOOLS : Polygon 
# -------------------------------------------------------------------------

def getPolygonPts(obj):
    editpoints = []
    editpoints.append(App.Vector(0, 0, 0))
    if obj.DrawMode == 'inscribed':
        editpoints.append(obj.Placement.inverse().multVec(obj.Shape.Vertexes[0].Point))
    else:
        editpoints.append(obj.Placement.inverse().multVec((obj.Shape.Vertexes[0].Point + 
                                                          obj.Shape.Vertexes[1].Point) / 2
                                                         ))
    return editpoints

def updatePolygon(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.Placement.Base = obj.Placement.multVec(v)
    elif nodeIndex == 1:
        obj.Radius = v.Length
    obj.recompute()


# -------------------------------------------------------------------------
# EDIT OBJECT TOOLS : Dimension (point on dimension line is not clickable)
# -------------------------------------------------------------------------

def getDimensionPts(obj):
    editpoints = []
    p = obj.ViewObject.Proxy.textpos.translation.getValue()
    editpoints.append(obj.Start)
    editpoints.append(obj.End)
    editpoints.append(obj.Dimline)
    editpoints.append(App.Vector(p[0], p[1], p[2]))
    return editpoints

def updateDimension(obj, nodeIndex, v):
    if nodeIndex == 0:
        obj.Start = v
    elif nodeIndex == 1:
        obj.End = v
    elif nodeIndex == 2:
        obj.Dimline = v
    elif nodeIndex == 3:
        obj.ViewObject.TextPosition = v


# -------------------------------------------------------------------------
# EDIT OBJECT TOOLS : BezCurve
# -------------------------------------------------------------------------

def updateBezCurve(obj, nodeIndex, v): #TODO: Fix it
    pts = obj.Points
    # DNC: check for coincident startpoint/endpoint to auto close the curve
    tol = 0.001
    if ( ( nodeIndex == 0 ) and ( (v - pts[-1]).Length < tol) ) or ( 
            nodeIndex == len(pts) - 1 ) and ( (v - pts[0]).Length < tol):
        obj.Closed = True
    # DNC: checks if point enter is equal to other, this could cause a OCC problem
    if v in pts:
        _err = translate("draft", "This object does not support possible "
                                  "coincident points, please try again.")
        App.Console.PrintMessage(_err + "\n")
        return
    
    pts = recomputePointsBezier(obj, pts, nodeIndex, v, obj.Degree, moveTrackers=False)

    if obj.Closed:
        # check that the new point lies on the plane of the wire
        if hasattr(obj.Shape,"normalAt"):
            normal = obj.Shape.normalAt(0,0)
            point_on_plane = obj.Shape.Vertexes[0].Point
            v.projectToPlane(point_on_plane, normal)
    pts[nodeIndex] = v
    obj.Points = pts


def recomputePointsBezier(obj, pts, idx, v,
                            degree, moveTrackers=False):
    """
    (object, Points as list, nodeIndex as Int, App.Vector of new point, moveTrackers as Bool)
    return the new point list, applying the App.Vector to the given index point
    """
    # DNC: allows to close the curve by placing ends close to each other
    tol = 0.001
    if ( ( idx == 0 ) and ( (v - pts[-1]).Length < tol) ) or (
            idx == len(pts) - 1 ) and ( (v - pts[0]).Length < tol):
        obj.Closed = True
    # DNC: fix error message if edited point coincides with one of the existing points
    #if ( v in pts ) == False:
    knot = None
    ispole = idx % degree

    if ispole == 0: #knot
        if degree >= 3:
            if idx >= 1: #move left pole
                knotidx = idx if idx < len(pts) else 0
                pts[idx-1] = pts[idx-1] + v - pts[knotidx]
                #if moveTrackers: # trackers are reset after editing
                #    self.trackers[obj.Name][idx-1].set(pts[idx-1])
            if idx < len(pts)-1: #move right pole
                pts[idx+1] = pts[idx+1] + v - pts[idx]
                #if moveTrackers:
                #    self.trackers[obj.Name][idx+1].set(pts[idx+1])
            if idx == 0 and obj.Closed: # move last pole
                pts[-1] = pts [-1] + v -pts[idx]
                #if moveTrackers:
                #    self.trackers[obj.Name][-1].set(pts[-1])

    elif ispole == 1 and (idx >=2 or obj.Closed): #right pole
        knot = idx -1
        changep = idx - 2  # -1 in case of closed curve

    elif ispole == degree-1 and idx <= len(pts)-3:  # left pole
        knot = idx + 1
        changep = idx + 2

    elif ispole == degree-1 and obj.Closed and idx == len(pts)-1: #last pole
        knot = 0
        changep = 1

    if knot is not None:  # we need to modify the opposite pole
        segment = int(knot / degree) - 1
        cont = obj.Continuity[segment] if len(obj.Continuity) > segment else 0
        if cont == 1: #tangent
            pts[changep] = obj.Proxy.modifytangentpole(pts[knot],
                v,pts[changep])
            #if moveTrackers:
            #    self.trackers[obj.Name][changep].set(pts[changep])
        elif cont == 2: #symmetric
            pts[changep] = obj.Proxy.modifysymmetricpole(pts[knot],v)
            #if moveTrackers:
            #    self.trackers[obj.Name][changep].set(pts[changep])
    pts[idx] = v

    return pts  # returns the list of new points, taking into account knot continuity


def smoothBezPoint(obj, point, style='Symmetric'):
    "called when changing the continuity of a knot"
    style2cont = {'Sharp':0,'Tangent':1,'Symmetric':2}
    if point is None:
        return
    if not (utils.get_type(obj) == "BezCurve"):
        return
    pts = obj.Points
    deg = obj.Degree
    if deg < 2:
        return
    if point % deg != 0:  # point is a pole
        if deg >=3:  # allow to select poles
            if (point % deg == 1) and (point > 2 or obj.Closed): #right pole
                knot = point -1
                keepp = point
                changep = point -2
            elif point < len(pts) -3 and point % deg == deg -1: #left pole
                knot = point +1
                keepp = point
                changep = point +2
            elif point == len(pts)-1 and obj.Closed: #last pole
                # if the curve is closed the last pole has the last
                # index in the points lists
                knot = 0
                keepp = point
                changep = 1
            else:
                App.Console.PrintWarning(translate("draft",
                                                    "Can't change Knot belonging to pole %d"%point)
                                                    + "\n")
                return
            if knot:
                if style == 'Tangent':
                    pts[changep] = obj.Proxy.modifytangentpole(pts[knot],
                        pts[keepp],pts[changep])
                elif style == 'Symmetric':
                    pts[changep] = obj.Proxy.modifysymmetricpole(pts[knot],
                        pts[keepp])
                else: #sharp
                    pass #
        else:
            App.Console.PrintWarning(translate("draft",
                                                "Selection is not a Knot")
                                                + "\n")
            return
    else: #point is a knot
        if style == 'Sharp':
            if obj.Closed and point == len(pts)-1:
                knot = 0
            else:
                knot = point
        elif style == 'Tangent' and point > 0 and point < len(pts)-1:
            prev, next = obj.Proxy.tangentpoles(pts[point], pts[point-1], pts[point+1])
            pts[point-1] = prev
            pts[point+1] = next
            knot = point  # index for continuity
        elif style == 'Symmetric' and point > 0 and point < len(pts)-1:
            prev, next = obj.Proxy.symmetricpoles(pts[point], pts[point-1], pts[point+1])
            pts[point-1] = prev
            pts[point+1] = next
            knot = point  # index for continuity
        elif obj.Closed and (style == 'Symmetric' or style == 'Tangent'):
            if style == 'Tangent':
                pts[1], pts[-1] = obj.Proxy.tangentpoles(pts[0], pts[1], pts[-1])
            elif style == 'Symmetric':
                pts[1], pts[-1] = obj.Proxy.symmetricpoles(pts[0], pts[1], pts[-1])
            knot = 0
        else:
            App.Console.PrintWarning(translate("draft",
                                                    "Endpoint of BezCurve can't be smoothed")
                                                    + "\n")
            return
    segment = knot // deg  # segment index
    newcont = obj.Continuity[:]  # don't edit a property inplace !!!
    if not obj.Closed and (len(obj.Continuity) == segment -1 or
        segment == 0) :
        pass # open curve
    elif (len(obj.Continuity) >= segment or obj.Closed and segment == 0 and
            len(obj.Continuity) >1):
        newcont[segment-1] = style2cont.get(style)
    else: #should not happen
        App.Console.PrintWarning('Continuity indexing error:'
                                    + 'point:%d deg:%d len(cont):%d' % (knot,deg,
                                        len(obj.Continuity)))
    obj.Points = pts
    obj.Continuity = newcont

## @}
