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
"""Provide the support functions to Draft_Edit for Draft objects.

All functions in this module work with Object coordinate space.
No conversion to global coordinate system is needed.

To support an new Object Draft_Edit needs at least two functions:
getObjectPts(obj): returns a list of points on which Draft_Edit will display
    edit trackers
updateObject(obj, nodeIndex, v): update the give object according to the
    index of a moved edit tracker and the vector of the displacement
TODO: Abstract the code that handles the preview and move the object specific
    code to this module from main Draft_Edit module
"""
## @package gui_edit_draft_objects
# \ingroup DRAFT
# \brief Provide the support functions to Draft_Edit for Draft objects.

__title__ = "FreeCAD Draft Edit Tool"
__author__ = ("Yorik van Havre, Werner Mayer, Martin Burbaum, Ken Cline, "
              "Dmitry Chigrin, Carlo Pavan")
__url__ = "https://www.freecadweb.org"


import math
import FreeCAD as App
import DraftVecUtils

from draftutils.translate import translate
import draftutils.utils as utils


# -------------------------------------------------------------------------
# EDIT OBJECT TOOLS : Line/Wire/Bspline/Bezcurve
# -------------------------------------------------------------------------

def getWirePts(obj):
    editpoints = []
    for p in obj.Points:
        editpoints.append(p)
    return editpoints

def updateWire(obj, nodeIndex, v): #TODO: Fix it
    pts = obj.Points
    editPnt = v
    # DNC: allows to close the curve by placing ends close to each other
    tol = 0.001
    if ( ( nodeIndex == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or ( 
            nodeIndex == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
        obj.Closed = True
    # DNC: fix error message if edited point coincides with one of the existing points
    if ( editPnt in pts ) == True: # checks if point enter is equal to other, this could cause a OCC problem
        App.Console.PrintMessage(translate("draft", 
                                            "This object does not support possible "
                                            "coincident points, please try again.")
                                    + "\n")
        if utils.get_type(obj) in ["BezCurve"]: # TODO: Remove code to recompute trackers
            self.resetTrackers(obj)
        else:
            self.trackers[obj.Name][nodeIndex].set(obj.getGlobalPlacement().
                multVec(obj.Points[nodeIndex]))
        return
    if utils.get_type(obj) in ["BezCurve"]:
        pts = recomputePointsBezier(obj,pts,nodeIndex,v,obj.Degree,moveTrackers=False)

    if obj.Closed:
        # check that the new point lies on the plane of the wire
        if hasattr(obj.Shape,"normalAt"):
            normal = obj.Shape.normalAt(0,0)
            point_on_plane = obj.Shape.Vertexes[0].Point
            print(v)
            v.projectToPlane(point_on_plane, normal)
            print(v)
            editPnt = obj.getGlobalPlacement().inverse().multVec(v)
    pts[nodeIndex] = editPnt
    obj.Points = pts


def recomputePointsBezier(obj, pts, idx, v,
                            degree, moveTrackers=True):
    """
    (object, Points as list, nodeIndex as Int, App.Vector of new point, moveTrackers as Bool)
    return the new point list, applying the App.Vector to the given index point
    """
    editPnt = v
    # DNC: allows to close the curve by placing ends close to each other
    tol = 0.001
    if ( ( idx == 0 ) and ( (editPnt - pts[-1]).Length < tol) ) or (
            idx == len(pts) - 1 ) and ( (editPnt - pts[0]).Length < tol):
        obj.Closed = True
    # DNC: fix error message if edited point coincides with one of the existing points
    #if ( editPnt in pts ) == False:
    knot = None
    ispole = idx % degree

    if ispole == 0: #knot
        if degree >= 3:
            if idx >= 1: #move left pole
                knotidx = idx if idx < len(pts) else 0
                pts[idx-1] = pts[idx-1] + editPnt - pts[knotidx]
                if moveTrackers:
                    self.trackers[obj.Name][idx-1].set(pts[idx-1]) # TODO: Remove code to recompute trackers
            if idx < len(pts)-1: #move right pole
                pts[idx+1] = pts[idx+1] + editPnt - pts[idx]
                if moveTrackers:
                    self.trackers[obj.Name][idx+1].set(pts[idx+1])
            if idx == 0 and obj.Closed: # move last pole
                pts[-1] = pts [-1] + editPnt -pts[idx]
                if moveTrackers:
                    self.trackers[obj.Name][-1].set(pts[-1])

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
                editPnt,pts[changep])
            if moveTrackers:
                self.trackers[obj.Name][changep].set(pts[changep])
        elif cont == 2: #symmetric
            pts[changep] = obj.Proxy.modifysymmetricpole(pts[knot],editPnt)
            if moveTrackers:
                self.trackers[obj.Name][changep].set(pts[changep])
    pts[idx] = v

    return pts  # returns the list of new points, taking into account knot continuity

def resetTrackersBezier(obj):
    # in future move tracker definition to DraftTrackers
    from pivy import coin
    knotmarkers = (coin.SoMarkerSet.DIAMOND_FILLED_9_9,#sharp
            coin.SoMarkerSet.SQUARE_FILLED_9_9,        #tangent
            coin.SoMarkerSet.HOURGLASS_FILLED_9_9)     #symmetric
    polemarker = coin.SoMarkerSet.CIRCLE_FILLED_9_9    #pole
    self.trackers[obj.Name] = []
    cont = obj.Continuity
    firstknotcont = cont[-1] if (obj.Closed and cont) else 0
    pointswithmarkers = [(obj.Shape.Edges[0].Curve.
            getPole(1),knotmarkers[firstknotcont])]
    for edgeindex, edge in enumerate(obj.Shape.Edges):
        poles = edge.Curve.getPoles()
        pointswithmarkers.extend([(point,polemarker) for point in poles[1:-1]])
        if not obj.Closed or len(obj.Shape.Edges) > edgeindex +1:
            knotmarkeri = cont[edgeindex] if len(cont) > edgeindex else 0
            pointswithmarkers.append((poles[-1],knotmarkers[knotmarkeri]))
    for index, pwm in enumerate(pointswithmarkers):
        p, marker = pwm
        # if self.pl: p = self.pl.multVec(p)
        self.trackers[obj.Name].append(trackers.editTracker(p,obj.Name,
            index,obj.ViewObject.LineColor,marker=marker))

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
    editpoints.append(obj.getGlobalPlacement().Base)
    if obj.FirstAngle == obj.LastAngle:
        # obj is a circle
        editpoints.append(obj.getGlobalPlacement().multVec(App.Vector(obj.Radius,0,0)))
    else:
        # obj is an arc
        editpoints.append(getArcStart(obj, global_placement=True))#First endpoint
        editpoints.append(getArcEnd(obj, global_placement=True))#Second endpoint
        editpoints.append(getArcMid(obj, global_placement=True))#Midpoint
    return editpoints


def updateCircle(obj, nodeIndex, v, alt_edit_mode=0):
    delta = obj.getGlobalPlacement().inverse().multVec(v)
    local_v = obj.Placement.multVec(delta)

    if obj.FirstAngle == obj.LastAngle:
        # object is a circle
        if nodeIndex == 0:
            obj.Placement.Base = local_v
        elif nodeIndex == 1:
            obj.Radius = delta.Length

    else:
        # obj is an arc
        if alt_edit_mode == 0:
            # edit arc by 3 points
            import Part
            if nodeIndex == 0:
                # center point
                p1 = getArcStart(obj)
                p2 = getArcEnd(obj)
                p0 = DraftVecUtils.project(delta, getArcMid(obj))
                obj.Radius = p1.sub(p0).Length
                obj.FirstAngle = -math.degrees(DraftVecUtils.angle(p1.sub(p0)))
                obj.LastAngle = -math.degrees(DraftVecUtils.angle(p2.sub(p0)))
                obj.Placement.Base = obj.Placement.multVec(p0)

            else:
                if nodeIndex == 1:  # first point
                    p1 = v
                    p2 = getArcMid(obj, global_placement=True)
                    p3 = getArcEnd(obj, global_placement=True)
                elif nodeIndex == 3:  # midpoint
                    p1 = getArcStart(obj, global_placement=True)
                    p2 = v
                    p3 = getArcEnd(obj, global_placement=True)
                elif nodeIndex == 2:  # second point
                    p1 = getArcStart(obj, global_placement=True)
                    p2 = getArcMid(obj, global_placement=True)
                    p3 = v
                arc=Part.ArcOfCircle(p1,p2,p3)
                obj.Placement.Base = obj.Placement.multVec(obj.getGlobalPlacement().inverse().multVec(arc.Location))
                obj.Radius = arc.Radius
                delta = obj.Placement.inverse().multVec(p1)
                obj.FirstAngle = math.degrees(math.atan2(delta[1],delta[0]))
                delta = obj.Placement.inverse().multVec(p3)
                obj.LastAngle = math.degrees(math.atan2(delta[1],delta[0]))

        elif alt_edit_mode == 1:
            # edit arc by center radius FirstAngle LastAngle
            if nodeIndex == 0:
                obj.Placement.Base = local_v
            else:
                dangle = math.degrees(math.atan2(delta[1],delta[0]))
                if nodeIndex == 1:
                    obj.FirstAngle = dangle
                elif nodeIndex == 2:
                    obj.LastAngle = dangle
                elif nodeIndex == 3:
                    obj.Radius = delta.Length

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
        obj.MajorRadius = v.Length
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


