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
__url__ = "https://www.freecad.org"

## \addtogroup draftguitools
# @{
import math
import FreeCAD as App
import FreeCADGui as Gui
import DraftVecUtils

from draftutils.translate import translate
import draftutils.utils as utils

import draftguitools.gui_trackers as trackers

from draftguitools.gui_edit_base_object import GuiTools



class DraftWireGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        editpoints = []
        for p in obj.Points:
            editpoints.append(p)
        return editpoints

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        pts = obj.Points
        tol = 0.001 # TODO : Use default precision
        if (node_idx == 0 and (v - pts[-1]).Length < tol ):
            # DNC: user moved first point over last point -> Close curve
            obj.Closed = True
            pts[0] = v
            del pts[-1]
            obj.Points = pts
            return
        elif node_idx == len(pts) - 1 and (v - pts[0]).Length < tol:
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

        # TODO: Make consistent operation with trackers and open wires
        # See: https://forum.freecad.org/viewtopic.php?f=23&t=56661
        #if obj.Closed:
        #    # DNC: project the new point to the plane of the face if present
        #    if hasattr(obj.Shape, "normalAt"):
        #        normal = obj.Shape.normalAt(0,0)
        #        point_on_plane = obj.Shape.Vertexes[0].Point
        #        v.projectToPlane(point_on_plane, normal)

        pts[node_idx] = v
        obj.Points = pts

    def get_edit_point_context_menu(self, edit_command, obj, node_idx):
        return [
            (translate("draft", "Delete point"), lambda: self.delete_point(obj, node_idx)),
        ]

    def get_edit_obj_context_menu(self, edit_command, obj, position):
        return [
            (translate("draft", "Add point"), lambda: self.add_point(edit_command, obj, position)),
            (self.get_open_close_menu_text(obj), lambda: self.open_close_wire(obj)),
            (self.get_reverse_menu_text(obj), lambda: self.reverse_wire(obj)),
        ]

    def get_open_close_menu_text(self, obj):
        """This function is overridden in the DraftBSplineGuiTools class.
        """
        if obj.Closed:
            return translate("draft", "Open wire")
        else:
            return translate("draft", "Close wire")

    def get_reverse_menu_text(self, obj):
        """This function is overridden in the DraftBSplineGuiTools class.
        """
        return translate("draft", "Reverse wire")

    def init_preview_object(self, obj):
        return trackers.wireTracker(obj.Shape)

    def update_preview_object(self, edit_command, obj, node_idx, v):
        if utils.get_type(obj) in ["Wire"]:
            pointList = edit_command.globalize_vectors(obj, obj.Points)
            pointList[node_idx] = v
            if obj.Closed:
                pointList.append(pointList[0])
            edit_command.ghost.updateFromPointlist(pointList)

    def add_point(self, edit_command, obj, pos):
        """Add point to obj.
        """
        info, newPoint = edit_command.get_specific_object_info(obj,pos)

        if not info:
            return
        if not 'Edge' in info["Component"]:
            return
        edgeIndex = int(info["Component"][4:])

        newPoints = []
        if hasattr(obj, "ChamferSize") and hasattr(obj, "FilletRadius"):
            # TODO: If Draft_Wire fails to calculate one of the fillets or chamfers
            #       this algo fails to identify the correct edge
            if obj.ChamferSize > 0 and obj.FilletRadius > 0:
                edgeIndex = (edgeIndex + 3) / 4
            elif obj.ChamferSize > 0 or obj.FilletRadius > 0:
                edgeIndex = (edgeIndex + 1) / 2

        for index, point in enumerate(obj.Points):
            if index == edgeIndex:
                newPoints.append(edit_command.localize_vector(obj, newPoint))
            newPoints.append(point)
        if obj.Closed and edgeIndex == len(obj.Points):
            # last segment when object is closed
            newPoints.append(edit_command.localize_vector(obj, newPoint))
        obj.Points = newPoints

        obj.recompute()

    def delete_point(self, obj, node_idx):
        if len(obj.Points) <= 2:
            _msg = translate("draft", "Active object must have more than two points/nodes")
            App.Console.PrintWarning(_msg + "\n")
            return

        pts = obj.Points
        pts.pop(node_idx)
        obj.Points = pts

        obj.recompute()

    def open_close_wire(self, obj):
        obj.Closed = not obj.Closed
        obj.recompute()

    def reverse_wire(self, obj):
        obj.Points = reversed(obj.Points)
        obj.recompute()


class DraftBSplineGuiTools(DraftWireGuiTools):

    def get_open_close_menu_text(self, obj):
        if obj.Closed:
            return translate("draft", "Open spline")
        else:
            return translate("draft", "Close spline")

    def get_reverse_menu_text(self, obj):
        return translate("draft", "Reverse spline")

    def init_preview_object(self, obj):
        return trackers.bsplineTracker()

    def update_preview_object(self, edit_command, obj, node_idx, v):
        pointList = edit_command.globalize_vectors(obj, obj.Points)
        pointList[node_idx] = v
        if obj.Closed:
            pointList.append(pointList[0])
        edit_command.ghost.update(pointList)

    def add_point(self, edit_command, obj, pos):
        """Add point to obj.
        """
        info, global_pt = edit_command.get_specific_object_info(obj,pos)
        pt = edit_command.localize_vector(obj, global_pt)

        if not info or (pt is None):
            return

        pts = obj.Points
        if obj.Closed:
            curve = obj.Shape.Edges[0].Curve
        else:
            curve = obj.Shape.Curve
        uNewPoint = curve.parameter(obj.Placement.multVec(pt))
        uPoints = curve.getKnots()

        for i in range(len(uPoints) - 1):
            if ( uNewPoint > uPoints[i] ) and ( uNewPoint < uPoints[i+1] ):
                pts.insert(i + 1, pt)
                break
        # DNC: fix: add points to last segment if curve is closed
        if obj.Closed and (uNewPoint > uPoints[-1]):
            pts.append(pt)
        obj.Points = pts

        obj.recompute()


class DraftRectangleGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
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

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        if node_idx == 0:
            obj.Placement.Base = obj.Placement.multVec(v)
        elif node_idx == 1:
            obj.Length = DraftVecUtils.project(v, App.Vector(1,0,0)).Length
        elif node_idx == 2:
            obj.Height = DraftVecUtils.project(v, App.Vector(0,1,0)).Length


class DraftCircleGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
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
            editpoints.append(self.getArcStart(obj))#First endpoint
            editpoints.append(self.getArcEnd(obj))#Second endpoint
            editpoints.append(self.getArcMid(obj))#Midpoint
        return editpoints

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        if obj.FirstAngle == obj.LastAngle:
            # object is a circle
            if node_idx == 0:
                obj.Placement.Base = obj.Placement.multVec(v)
            elif node_idx == 1:
                obj.Radius = v.Length

        else:
            # obj is an arc
            if alt_edit_mode == 0:
                import Part
                if node_idx == 0:
                    # center point
                    p1 = self.getArcStart(obj)
                    p2 = self.getArcEnd(obj)
                    p0 = DraftVecUtils.project(v, self.getArcMid(obj))
                    obj.Radius = p1.sub(p0).Length
                    obj.FirstAngle = -math.degrees(DraftVecUtils.angle(p1.sub(p0)))
                    obj.LastAngle = -math.degrees(DraftVecUtils.angle(p2.sub(p0)))
                    obj.Placement.Base = obj.Placement.multVec(p0)

                else:
                    """ Edit arc by 3 points.
                    """
                    v= obj.Placement.multVec(v)
                    p1 = obj.Placement.multVec(self.getArcStart(obj))
                    p2 = obj.Placement.multVec(self.getArcMid(obj))
                    p3 = obj.Placement.multVec(self.getArcEnd(obj))

                    if node_idx == 1:  # first point
                        p1 = v
                    elif node_idx == 3:  # midpoint
                        p2 = v
                    elif node_idx == 2:  # second point
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
                if node_idx == 0:
                    obj.Placement.Base = obj.Placement.multVec(v)
                else:
                    dangle = math.degrees(math.atan2(v[1],v[0]))
                    if node_idx == 1:
                        obj.FirstAngle = dangle
                    elif node_idx == 2:
                        obj.LastAngle = dangle
                    elif node_idx == 3:
                        obj.Radius = v.Length


    def get_edit_point_context_menu(self, edit_command, obj, node_idx):
        actions = None
        if obj.FirstAngle != obj.LastAngle:
            if node_idx == 0:  # user is over arc start point
                return [
                    (translate("draft", "Move arc"), lambda: self.handle_move_arc(edit_command, obj, node_idx)),
                ]
            elif node_idx == 1:  # user is over arc start point
                return [
                    (translate("draft", "Set first angle"), lambda: self.handle_set_first_angle(edit_command, obj, node_idx)),
                ]
            elif node_idx == 2:  # user is over arc end point
                return [
                    (translate("draft", "Set last angle"), lambda: self.handle_set_last_angle(edit_command, obj, node_idx)),
                ]
            elif node_idx == 3:  # user is over arc mid point
                return [
                    (translate("draft", "Set radius"), lambda: self.handle_set_radius(edit_command, obj, node_idx)),
                ]

    def handle_move_arc(self, edit_command, obj, node_idx):
        edit_command.alt_edit_mode = 1
        edit_command.startEditing(obj, node_idx)

    def handle_set_first_angle(self, edit_command, obj, node_idx):
        edit_command.alt_edit_mode = 1
        edit_command.startEditing(obj, node_idx)

    def handle_set_last_angle(self, edit_command, obj, node_idx):
        edit_command.alt_edit_mode = 1
        edit_command.startEditing(obj, node_idx)

    def handle_set_radius(self, edit_command, obj, node_idx):
        edit_command.alt_edit_mode = 1
        edit_command.startEditing(obj, node_idx)

    def get_edit_obj_context_menu(self, edit_command, obj, position):
        # Do not show the `Invert arc` option for circles:
        if obj.FirstAngle == obj.LastAngle:
            return

        return [
            (translate("draft", "Invert arc"), lambda: self.arcInvert(obj)),
        ]

    def init_preview_object(self, obj):
        return trackers.arcTracker()

    def update_preview_object(self, edit_command, obj, node_idx, v):
        edit_command.ghost.setCenter(obj.getGlobalPlacement().Base)
        edit_command.ghost.setRadius(obj.Radius)
        if obj.FirstAngle == obj.LastAngle:
            # obj is a circle
            edit_command.ghost.circle = True
            if node_idx == 0:
                edit_command.ghost.setCenter(v)
            elif node_idx == 1:
                radius = v.sub(obj.getGlobalPlacement().Base).Length
                edit_command.ghost.setRadius(radius)
        else:
            if edit_command.alt_edit_mode == 0:
                # edit by 3 points
                if node_idx == 0:
                    # center point
                    p1 = edit_command.localize_vector(obj, obj.Shape.Vertexes[0].Point)
                    p2 = edit_command.localize_vector(obj, obj.Shape.Vertexes[1].Point)
                    p0 = DraftVecUtils.project(edit_command.localize_vector(obj, v),
                                                edit_command.localize_vector(obj, (self.getArcMid(obj, global_placement=True))))
                    edit_command.ghost.autoinvert=False
                    edit_command.ghost.setRadius(p1.sub(p0).Length)
                    edit_command.ghost.setStartPoint(obj.Shape.Vertexes[1].Point)
                    edit_command.ghost.setEndPoint(obj.Shape.Vertexes[0].Point)
                    edit_command.ghost.setCenter(edit_command.globalize_vector(obj, p0))
                    return
                else:
                    p1 = self.getArcStart(obj, global_placement=True)
                    p2 = self.getArcMid(obj, global_placement=True)
                    p3 = self.getArcEnd(obj, global_placement=True)
                    if node_idx == 1:
                        p1=v
                    elif node_idx == 3:
                        p2=v
                    elif node_idx == 2:
                        p3=v
                    edit_command.ghost.setBy3Points(p1,p2,p3)
            elif edit_command.alt_edit_mode == 1:
                # edit by center radius angles
                edit_command.ghost.setStartAngle(math.radians(obj.FirstAngle))
                edit_command.ghost.setEndAngle(math.radians(obj.LastAngle))
                if node_idx == 0:
                    edit_command.ghost.setCenter(v)
                elif node_idx == 1:
                    edit_command.ghost.setStartPoint(v)
                elif node_idx == 2:
                    edit_command.ghost.setEndPoint(v)
                elif node_idx == 3:
                    edit_command.ghost.setRadius(edit_command.localize_vector(obj, v).Length)


    def getArcStart(self, obj, global_placement=False):#Returns object midpoint
        if utils.get_type(obj) == "Circle":
            return self.pointOnCircle(obj, obj.FirstAngle, global_placement)


    def getArcEnd(self, obj, global_placement=False):#Returns object midpoint
        if utils.get_type(obj) == "Circle":
            return self.pointOnCircle(obj, obj.LastAngle, global_placement)


    def getArcMid(self, obj, global_placement=False):#Returns object midpoint
        if utils.get_type(obj) == "Circle":
            if obj.LastAngle > obj.FirstAngle:
                midAngle = obj.FirstAngle + (obj.LastAngle - obj.FirstAngle) / 2.0
            else:
                midAngle = obj.FirstAngle + (obj.LastAngle - obj.FirstAngle) / 2.0
                midAngle += App.Units.Quantity(180,App.Units.Angle)
            return self.pointOnCircle(obj, midAngle, global_placement)


    def pointOnCircle(self, obj, angle, global_placement=False):
        if utils.get_type(obj) == "Circle":
            px = obj.Radius * math.cos(math.radians(angle))
            py = obj.Radius * math.sin(math.radians(angle))
            p = App.Vector(px, py, 0.0)
            if global_placement:
                p = obj.getGlobalPlacement().multVec(p)
            return p
        return None


    def arcInvert(self, obj):
        obj.FirstAngle, obj.LastAngle = obj.LastAngle, obj.FirstAngle
        obj.recompute()


class DraftEllipseGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        editpoints = []
        editpoints.append(App.Vector(0, 0, 0))
        editpoints.append(App.Vector(obj.MajorRadius, 0, 0))
        editpoints.append(App.Vector(0, obj.MinorRadius, 0))
        return editpoints

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        if node_idx == 0:
            obj.Placement.Base = obj.Placement.multVec(v)
        elif node_idx == 1:
            if v.Length >= obj.MinorRadius:
                obj.MajorRadius = v.Length
            else:
                obj.MajorRadius = obj.MinorRadius
        elif node_idx == 2:
            if v.Length <= obj.MajorRadius:
                obj.MinorRadius = v.Length
            else:
                obj.MinorRadius = obj.MajorRadius


class DraftPolygonGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        editpoints = []
        editpoints.append(App.Vector(0, 0, 0))
        if obj.DrawMode == 'inscribed':
            editpoints.append(obj.Placement.inverse().multVec(obj.Shape.Vertexes[0].Point))
        else:
            editpoints.append(obj.Placement.inverse().multVec((obj.Shape.Vertexes[0].Point +
                                                            obj.Shape.Vertexes[1].Point) / 2
                                                            ))
        return editpoints

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        if node_idx == 0:
            obj.Placement.Base = obj.Placement.multVec(v)
        elif node_idx == 1:
            obj.Radius = v.Length
        obj.recompute()


class DraftDimensionGuiTools(GuiTools):

    def __init__(self):
        pass

    def get_edit_points(self, obj):
        editpoints = []
        p = obj.ViewObject.Proxy.textpos.translation.getValue()
        editpoints.append(obj.Start)
        editpoints.append(obj.End)
        editpoints.append(obj.Dimline)
        editpoints.append(App.Vector(p[0], p[1], p[2]))
        return editpoints

    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        if node_idx == 0:
            obj.Start = v
        elif node_idx == 1:
            obj.End = v
        elif node_idx == 2:
            obj.Dimline = v
        elif node_idx == 3:
            obj.ViewObject.TextPosition = v


class DraftBezCurveGuiTools(GuiTools):

    def __init__(self):
        pass


    def get_edit_points(self, obj):
        editpoints = []
        for p in obj.Points:
            editpoints.append(p)
        return editpoints


    def update_object_from_edit_points(self, obj, node_idx, v, alt_edit_mode=0):
        pts = obj.Points
        # DNC: check for coincident startpoint/endpoint to auto close the curve
        tol = 0.001
        if ( ( node_idx == 0 ) and ( (v - pts[-1]).Length < tol) ) or (
                node_idx == len(pts) - 1 ) and ( (v - pts[0]).Length < tol):
            obj.Closed = True
        # DNC: checks if point enter is equal to other, this could cause a OCC problem
        if v in pts:
            _err = translate("draft", "This object does not support possible "
                                    "coincident points, please try again.")
            App.Console.PrintMessage(_err + "\n")
            return

        pts = self.recomputePointsBezier(obj, pts, node_idx, v, obj.Degree, moveTrackers=False)

        if obj.Closed:
            # check that the new point lies on the plane of the wire
            if hasattr(obj.Shape,"normalAt"):
                normal = obj.Shape.normalAt(0,0)
                point_on_plane = obj.Shape.Vertexes[0].Point
                v.projectToPlane(point_on_plane, normal)
        pts[node_idx] = v
        obj.Points = pts


    def get_edit_point_context_menu(self, edit_command, obj, node_idx):
        return [
            (translate("draft", "Delete point"), lambda: self.delete_point(obj, node_idx)),
            (translate("draft", "Make sharp"), lambda: self.smoothBezPoint(obj, node_idx, "Sharp")),
            (translate("draft", "Make tangent"), lambda: self.smoothBezPoint(obj, node_idx, "Tangent")),
            (translate("draft", "Make symmetric"), lambda: self.smoothBezPoint(obj, node_idx, "Symmetric")),
        ]

    def get_edit_obj_context_menu(self, edit_command, obj, position):
        return [
            (translate("draft", "Add point"), lambda: self.add_point(edit_command, obj, position)),
            (self.get_open_close_menu_text(obj), lambda: self.open_close_wire(obj)),
            (translate("draft", "Reverse curve"), lambda: self.reverse_wire(obj)),
        ]

    def get_open_close_menu_text(self, obj):
        if obj.Closed:
            return translate("draft", "Open curve")
        else:
            return translate("draft", "Close curve")

    def init_preview_object(self, obj):
        return trackers.bezcurveTracker()

    def update_preview_object(self, edit_command, obj, node_idx, v):
        plist = edit_command.globalize_vectors(obj, obj.Points)
        pointList = self.recomputePointsBezier(obj,plist,node_idx,v,obj.Degree,moveTrackers=False)
        edit_command.ghost.update(pointList, obj.Degree)

    def recomputePointsBezier(self, obj, pts, idx, v,
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


    def smoothBezPoint(self, obj, point, style='Symmetric'):
        """called when changing the continuity of a knot
        """
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

    def add_point(self, edit_command, obj, pos):
        """Add point to obj and reset trackers.
        """
        info, pt = edit_command.get_specific_object_info(obj,pos)
        if not info or (pt is None):
            return

        import Part

        pts = obj.Points
        if not info['Component'].startswith('Edge'):
            return  # clicked control point
        edgeindex = int(info['Component'].lstrip('Edge')) - 1
        wire = obj.Shape.Wires[0]
        bz = wire.Edges[edgeindex].Curve
        param = bz.parameter(pt)
        seg1 = wire.Edges[edgeindex].copy().Curve
        seg2 = wire.Edges[edgeindex].copy().Curve
        seg1.segment(seg1.FirstParameter, param)
        seg2.segment(param, seg2.LastParameter)
        if edgeindex == len(wire.Edges):
            # we hit the last segment, we need to fix the degree
            degree=wire.Edges[0].Curve.Degree
            seg1.increase(degree)
            seg2.increase(degree)
        edges = wire.Edges[0:edgeindex] + [Part.Edge(seg1),Part.Edge(seg2)] \
            + wire.Edges[edgeindex + 1:]
        pts = edges[0].Curve.getPoles()[0:1]
        for edge in edges:
            pts.extend(edge.Curve.getPoles()[1:])
        if obj.Closed:
            pts.pop()
        c = obj.Continuity
        # assume we have a tangent continuity for an arbitrarily split
        # segment, unless it's linear
        cont = 1 if (obj.Degree >= 2) else 0
        obj.Continuity = c[0:edgeindex] + [cont] + c[edgeindex:]

        obj.Points = pts

        obj.recompute()

    def delete_point(self, obj, node_idx):
        if len(obj.Points) <= 2:
            _msg = translate("draft", "Active object must have more than two points/nodes")
            App.Console.PrintWarning(_msg + "\n")
            return

        pts = obj.Points
        pts.pop(node_idx)
        obj.Points = pts
        obj.Proxy.resetcontinuity(obj)
        obj.recompute()

    def open_close_wire(self, obj):
        obj.Closed = not obj.Closed
        obj.recompute()

    def reverse_wire(self, obj):
        obj.Points = reversed(obj.Points)
        obj.recompute()

## @}
