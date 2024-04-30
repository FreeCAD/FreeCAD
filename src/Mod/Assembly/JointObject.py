# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

import math

import FreeCAD as App
import Part

from PySide import QtCore
from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui

# translate = App.Qt.translate

__title__ = "Assembly Joint object"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"

from pivy import coin
import UtilsAssembly
import Preferences

translate = App.Qt.translate

TranslatedJointTypes = [
    translate("Assembly", "Fixed"),
    translate("Assembly", "Revolute"),
    translate("Assembly", "Cylindrical"),
    translate("Assembly", "Slider"),
    translate("Assembly", "Ball"),
    translate("Assembly", "Distance"),
]

JointTypes = [
    "Fixed",
    "Revolute",
    "Cylindrical",
    "Slider",
    "Ball",
    "Distance",
]

JointUsingDistance = [
    "Distance",
]

JointUsingOffset = [
    "Fixed",
    "Revolute",
]

JointUsingRotation = [
    "Fixed",
    "Slider",
]

JointUsingReverse = [
    "Fixed",
    "Revolute",
    "Cylindrical",
    "Slider",
    "Distance",
]


def solveIfAllowed(assembly, storePrev=False):
    if assembly.Type == "Assembly" and Preferences.preferences().GetBool(
        "SolveInJointCreation", True
    ):
        assembly.solve(storePrev)


# The joint object consists of 2 JCS (joint coordinate systems) and a Joint Type.
# A JCS is a placement that is computed (unless it is detached) from :
# - An Object name: this is the name of the solid. It can be any Part::Feature solid.
# Or a PartDesign Body. Or a App::Link to those. We use the name and not directly the DocumentObject
# because the object can be external.
# - A Part DocumentObject : This is the lowest level containing part. It can be either the Object itself if it
# stands alone. Or a App::Part. Or a App::Link to a App::Part.
# For example :
# Assembly.Assembly1.Part1.Part2.Box : Object is Box, part is 'Part1'
# Assembly.Assembly1.LinkToPart1.Part2.Box : Object is Box, part is 'LinkToPart1'
# - An element name: This can be either a face, an edge, a vertex or empty. Empty means that the Object placement will be used
# - A vertex name: For faces and edges, we need to specify which vertex of said face/edge to use
# From these a placement is computed. It is relative to the Object.
class Joint:
    def __init__(self, joint, type_index):
        joint.Proxy = self

        joint.addProperty(
            "App::PropertyEnumeration",
            "JointType",
            "Joint",
            QT_TRANSLATE_NOOP("App::Property", "The type of the joint"),
        )
        joint.JointType = JointTypes  # sets the list
        joint.JointType = JointTypes[type_index]  # set the initial value

        # First Joint Connector
        joint.addProperty(
            "App::PropertyString",  # Not PropertyLink because they don't support external objects
            "Object1",
            "Joint Connector 1",
            QT_TRANSLATE_NOOP("App::Property", "The first object of the joint"),
        )

        joint.addProperty(
            "App::PropertyLink",
            "Part1",
            "Joint Connector 1",
            QT_TRANSLATE_NOOP("App::Property", "The first part of the joint"),
        )

        joint.addProperty(
            "App::PropertyString",
            "Element1",
            "Joint Connector 1",
            QT_TRANSLATE_NOOP("App::Property", "The selected element of the first object"),
        )

        joint.addProperty(
            "App::PropertyString",
            "Vertex1",
            "Joint Connector 1",
            QT_TRANSLATE_NOOP("App::Property", "The selected vertex of the first object"),
        )

        joint.addProperty(
            "App::PropertyPlacement",
            "Placement1",
            "Joint Connector 1",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This is the local coordinate system within object1 that will be used for the joint.",
            ),
        )

        joint.addProperty(
            "App::PropertyBool",
            "Detach1",
            "Joint Connector 1",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This prevents Placement1 from recomputing, enabling custom positioning of the placement.",
            ),
        )

        # Second Joint Connector
        joint.addProperty(
            "App::PropertyString",
            "Object2",
            "Joint Connector 2",
            QT_TRANSLATE_NOOP("App::Property", "The second object of the joint"),
        )

        joint.addProperty(
            "App::PropertyLink",
            "Part2",
            "Joint Connector 2",
            QT_TRANSLATE_NOOP("App::Property", "The second part of the joint"),
        )

        joint.addProperty(
            "App::PropertyString",
            "Element2",
            "Joint Connector 2",
            QT_TRANSLATE_NOOP("App::Property", "The selected element of the second object"),
        )

        joint.addProperty(
            "App::PropertyString",
            "Vertex2",
            "Joint Connector 2",
            QT_TRANSLATE_NOOP("App::Property", "The selected vertex of the second object"),
        )

        joint.addProperty(
            "App::PropertyPlacement",
            "Placement2",
            "Joint Connector 2",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This is the local coordinate system within object2 that will be used for the joint.",
            ),
        )

        joint.addProperty(
            "App::PropertyBool",
            "Detach2",
            "Joint Connector 2",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This prevents Placement2 from recomputing, enabling custom positioning of the placement.",
            ),
        )

        joint.addProperty(
            "App::PropertyFloat",
            "Distance",
            "Joint",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This is the distance of the joint. It is used only by the distance joint.",
            ),
        )

        joint.addProperty(
            "App::PropertyFloat",
            "Rotation",
            "Joint",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This is the rotation of the joint.",
            ),
        )

        joint.addProperty(
            "App::PropertyVector",
            "Offset",
            "Joint",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This is the offset vector of the joint.",
            ),
        )

        joint.addProperty(
            "App::PropertyBool",
            "Activated",
            "Joint",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This indicates if the joint is active.",
            ),
        )
        joint.Activated = True

        self.setJointConnectors(joint, [])

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def getAssembly(self, joint):
        return joint.InList[0]

    def setJointType(self, joint, jointType):
        joint.JointType = jointType
        joint.Label = jointType.replace(" ", "")

    def onChanged(self, joint, prop):
        """Do something when a property has changed"""
        # App.Console.PrintMessage("Change property: " + str(prop) + "\n")

        if prop == "Rotation" or prop == "Offset":
            # during loading the onchanged may be triggered before full init.
            if hasattr(joint, "Vertex1"):  # so we check Vertex1
                self.updateJCSPlacements(joint)
                obj1 = UtilsAssembly.getObjectInPart(joint.Object1, joint.Part1)
                obj2 = UtilsAssembly.getObjectInPart(joint.Object2, joint.Part2)
                presolved = self.preSolve(
                    joint,
                    obj1,
                    joint.Part1,
                    obj2,
                    joint.Part2,
                    False,
                )

                isAssembly = self.getAssembly(joint).Type == "Assembly"
                if isAssembly and not presolved:
                    solveIfAllowed(self.getAssembly(joint))
                else:
                    self.updateJCSPlacements(joint)

        if prop == "Distance":
            # during loading the onchanged may be triggered before full init.
            if hasattr(joint, "Vertex1"):  # so we check Vertex1
                solveIfAllowed(self.getAssembly(joint))

    def execute(self, fp):
        """Do something when doing a recomputation, this method is mandatory"""
        # App.Console.PrintMessage("Recompute Python Box feature\n")
        pass

    def setJointConnectors(self, joint, current_selection):
        # current selection is a vector of strings like "Assembly.Assembly1.Assembly2.Body.Pad.Edge16" including both what selection return as obj_name and obj_sub
        assembly = self.getAssembly(joint)
        isAssembly = assembly.Type == "Assembly"

        if len(current_selection) >= 1:
            joint.Part1 = None

            joint.Object1 = current_selection[0]["object"].Name
            joint.Part1 = current_selection[0]["part"]
            joint.Element1 = current_selection[0]["element_name"]
            joint.Vertex1 = current_selection[0]["vertex_name"]
            joint.Placement1 = self.findPlacement(
                joint, joint.Object1, joint.Part1, joint.Element1, joint.Vertex1
            )
        else:
            joint.Object1 = ""
            joint.Part1 = None
            joint.Element1 = ""
            joint.Vertex1 = ""
            joint.Placement1 = App.Placement()
            self.partMovedByPresolved = None

        if len(current_selection) >= 2:
            joint.Part2 = None

            joint.Object2 = current_selection[1]["object"].Name
            joint.Part2 = current_selection[1]["part"]
            joint.Element2 = current_selection[1]["element_name"]
            joint.Vertex2 = current_selection[1]["vertex_name"]
            joint.Placement2 = self.findPlacement(
                joint, joint.Object2, joint.Part2, joint.Element2, joint.Vertex2, True
            )
            if joint.JointType != "Distance":
                self.preSolve(
                    joint,
                    current_selection[0]["object"],
                    joint.Part1,
                    current_selection[1]["object"],
                    joint.Part2,
                )

            if isAssembly:
                solveIfAllowed(assembly, True)
            else:
                self.updateJCSPlacements(joint)

        else:
            joint.Object2 = ""
            joint.Part2 = None
            joint.Element2 = ""
            joint.Vertex2 = ""
            joint.Placement2 = App.Placement()
            if isAssembly:
                assembly.undoSolve()
            self.undoPreSolve()

    def updateJCSPlacements(self, joint):
        if not joint.Detach1:
            joint.Placement1 = self.findPlacement(
                joint, joint.Object1, joint.Part1, joint.Element1, joint.Vertex1
            )

        if not joint.Detach2:
            joint.Placement2 = self.findPlacement(
                joint, joint.Object2, joint.Part2, joint.Element2, joint.Vertex2, True
            )

    """
    So here we want to find a placement that corresponds to a local coordinate system that would be placed at the selected vertex.
    - obj is usually a App::Link to a PartDesign::Body, or primitive, fasteners. But can also be directly the object.1
    - elt can be a face, an edge or a vertex.
    - If elt is a vertex, then vtx = elt And placement is vtx coordinates without rotation.
    - if elt is an edge, then vtx = edge start/end vertex depending on which is closer. If elt is an arc or circle, vtx can also be the center. The rotation is the plane normal to the line positioned at vtx. Or for arcs/circle, the plane of the arc.
    - if elt is a plane face, vtx is the face vertex (to the list of vertex we need to add arc/circle centers) the closer to the mouse. The placement is the plane rotation positioned at vtx
    - if elt is a cylindrical face, vtx can also be the center of the arcs of the cylindrical face.
    """

    def findPlacement(self, joint, objName, part, elt, vtx, isSecond=False):
        if not objName or not part:
            return App.Placement()

        obj = UtilsAssembly.getObjectInPart(objName, part)
        plc = App.Placement()

        if not obj:
            return App.Placement()

        if not elt or not vtx:
            # case of whole parts such as PartDesign::Body or PartDesign::CordinateSystem/Point/Line/Plane.
            return App.Placement()

        elt_type, elt_index = UtilsAssembly.extract_type_and_number(elt)
        vtx_type, vtx_index = UtilsAssembly.extract_type_and_number(vtx)

        isLine = False

        if elt_type == "Vertex":
            vertex = obj.Shape.Vertexes[elt_index - 1]
            plc.Base = (vertex.X, vertex.Y, vertex.Z)
        elif elt_type == "Edge":
            edge = obj.Shape.Edges[elt_index - 1]
            curve = edge.Curve

            # First we find the translation
            if vtx_type == "Edge" or joint.JointType == "Distance":
                # In this case the wanted vertex is the center.
                if curve.TypeId == "Part::GeomCircle":
                    center_point = curve.Location
                    plc.Base = (center_point.x, center_point.y, center_point.z)
                elif curve.TypeId == "Part::GeomLine":
                    edge_points = UtilsAssembly.getPointsFromVertexes(edge.Vertexes)
                    line_middle = (edge_points[0] + edge_points[1]) * 0.5
                    plc.Base = line_middle
            else:
                vertex = obj.Shape.Vertexes[vtx_index - 1]
                plc.Base = (vertex.X, vertex.Y, vertex.Z)

            # Then we find the Rotation
            if curve.TypeId == "Part::GeomCircle":
                plc.Rotation = App.Rotation(curve.Rotation)

            if curve.TypeId == "Part::GeomLine":
                isLine = True
                plane_normal = curve.Direction
                plane_origin = App.Vector(0, 0, 0)
                plane = Part.Plane(plane_origin, plane_normal)
                plc.Rotation = App.Rotation(plane.Rotation)
        elif elt_type == "Face":
            face = obj.Shape.Faces[elt_index - 1]
            surface = face.Surface

            # First we find the translation
            if vtx_type == "Face" or joint.JointType == "Distance":
                if surface.TypeId == "Part::GeomCylinder" or surface.TypeId == "Part::GeomCone":
                    centerOfG = face.CenterOfGravity - surface.Center
                    centerPoint = surface.Center + centerOfG
                    centerPoint = centerPoint + App.Vector().projectToLine(centerOfG, surface.Axis)
                    plc.Base = centerPoint
                elif surface.TypeId == "Part::GeomTorus" or surface.TypeId == "Part::GeomSphere":
                    plc.Base = surface.Center
                else:
                    plc.Base = face.CenterOfGravity
            elif vtx_type == "Edge":
                # In this case the edge is a circle/arc and the wanted vertex is its center.
                edge = face.Edges[vtx_index - 1]
                curve = edge.Curve
                if curve.TypeId == "Part::GeomCircle":
                    center_point = curve.Location
                    plc.Base = (center_point.x, center_point.y, center_point.z)

                elif (
                    surface.TypeId == "Part::GeomCylinder"
                    and curve.TypeId == "Part::GeomBSplineCurve"
                ):
                    # handle special case of 2 cylinder intersecting.
                    plc.Base = self.findCylindersIntersection(obj, surface, edge, elt_index)

            else:
                vertex = obj.Shape.Vertexes[vtx_index - 1]
                plc.Base = (vertex.X, vertex.Y, vertex.Z)

            # Then we find the Rotation
            if surface.TypeId == "Part::GeomPlane":
                plc.Rotation = App.Rotation(surface.Rotation)
            else:
                plc.Rotation = surface.Rotation

        # Now plc is the placement relative to the origin determined by the object placement.
        # But it does not take into account Part placements. So if the solid is in a part and
        # if the part has a placement then plc is wrong.

        # change plc to be relative to the object placement.
        plc = obj.Placement.inverse() * plc

        # post-process of plc for some special cases
        if elt_type == "Vertex":
            plc.Rotation = App.Rotation()
        elif isLine:
            plane_normal = plc.Rotation.multVec(App.Vector(0, 0, 1))
            plane_origin = App.Vector(0, 0, 0)
            plane = Part.Plane(plane_origin, plane_normal)
            plc.Rotation = App.Rotation(plane.Rotation)

        # change plc to be relative to the origin of the document.
        # global_plc = UtilsAssembly.getGlobalPlacement(obj, part)
        # plc = global_plc * plc

        # change plc to be relative to the assembly.
        # assembly = self.getAssembly(joint)
        # plc = assembly.Placement.inverse() * plc

        # We apply rotation / reverse / offset it necessary, but only to the second JCS.
        if isSecond:
            if joint.Offset.Length != 0.0:
                plc = self.applyOffsetToPlacement(plc, joint.Offset)
            if joint.Rotation != 0.0:
                plc = self.applyRotationToPlacement(plc, joint.Rotation)

        return plc

    def applyOffsetToPlacement(self, plc, offset):
        plc.Base = plc.Base + plc.Rotation.multVec(offset)
        return plc

    def applyRotationToPlacement(self, plc, angle):
        return self.applyRotationToPlacementAlongAxis(plc, angle, App.Vector(0, 0, 1))

    def applyRotationToPlacementAlongAxis(self, plc, angle, axis):
        rot = plc.Rotation
        zRotation = App.Rotation(axis, angle)
        plc.Rotation = rot * zRotation
        return plc

    def flipPlacement(self, plc):
        return self.applyRotationToPlacementAlongAxis(plc, 180, App.Vector(1, 0, 0))

    def flipOnePart(self, joint):
        assembly = self.getAssembly(joint)
        part2ConnectedByJoint = assembly.isJointConnectingPartToGround(joint, "Part2")
        part1Grounded = assembly.isPartGrounded(joint.Part1)
        part2Grounded = assembly.isPartGrounded(joint.Part2)
        if part2ConnectedByJoint and not part2Grounded:
            print("flipOnePart if hasattr(self, part2Connected) and not self.part2Connected")
            print(joint.Part2.Name)
            jcsPlc = UtilsAssembly.getJcsPlcRelativeToPart(
                joint.Placement2, joint.Object2, joint.Part2
            )
            globalJcsPlc = UtilsAssembly.getJcsGlobalPlc(
                joint.Placement2, joint.Object2, joint.Part2
            )
            jcsPlc = self.flipPlacement(jcsPlc)
            joint.Part2.Placement = globalJcsPlc * jcsPlc.inverse()

        elif not part1Grounded:
            print("flipOnePart else")
            print(joint.Part1.Name)
            jcsPlc = UtilsAssembly.getJcsPlcRelativeToPart(
                joint.Placement1, joint.Object1, joint.Part1
            )
            globalJcsPlc = UtilsAssembly.getJcsGlobalPlc(
                joint.Placement1, joint.Object1, joint.Part1
            )
            jcsPlc = self.flipPlacement(jcsPlc)
            joint.Part1.Placement = globalJcsPlc * jcsPlc.inverse()

        solveIfAllowed(self.getAssembly(joint))

    def findCylindersIntersection(self, obj, surface, edge, elt_index):
        for j, facej in enumerate(obj.Shape.Faces):
            surfacej = facej.Surface
            if (elt_index - 1) == j or surfacej.TypeId != "Part::GeomCylinder":
                continue

            for edgej in facej.Edges:
                if (
                    edgej.Curve.TypeId == "Part::GeomBSplineCurve"
                    and edgej.CenterOfGravity == edge.CenterOfGravity
                    and edgej.Length == edge.Length
                ):
                    # we need intersection between the 2 cylinder axis.
                    line1 = Part.Line(surface.Center, surface.Center + surface.Axis)
                    line2 = Part.Line(surfacej.Center, surfacej.Center + surfacej.Axis)

                    res = line1.intersect(line2, Part.Precision.confusion())

                    if res:
                        return App.Vector(res[0].X, res[0].Y, res[0].Z)
        return surface.Center

    def preSolve(self, joint, obj1, part1, obj2, part2, savePlc=True):
        # The goal of this is to put the part in the correct position to avoid wrong placement by the solve.

        # we actually don't want to match perfectly the JCS, it is best to match them
        # in the current closest direction, ie either matched or flipped.
        sameDir = self.areJcsSameDir(joint)
        assembly = self.getAssembly(joint)
        part1ConnectedByJoint = assembly.isJointConnectingPartToGround(joint, "Part1")
        part2ConnectedByJoint = assembly.isJointConnectingPartToGround(joint, "Part2")
        if part2ConnectedByJoint:
            if savePlc:
                self.partMovedByPresolved = joint.Part2
                self.presolveBackupPlc = joint.Part2.Placement

            globalJcsPlc1 = UtilsAssembly.getJcsGlobalPlc(
                joint.Placement1, joint.Object1, joint.Part1
            )
            jcsPlc2 = UtilsAssembly.getJcsPlcRelativeToPart(
                joint.Placement2, joint.Object2, joint.Part2
            )
            if not sameDir:
                jcsPlc2 = self.flipPlacement(jcsPlc2)
            joint.Part2.Placement = globalJcsPlc1 * jcsPlc2.inverse()
            return True

        elif part1ConnectedByJoint:
            if savePlc:
                self.partMovedByPresolved = joint.Part1
                self.presolveBackupPlc = joint.Part1.Placement

            globalJcsPlc2 = UtilsAssembly.getJcsGlobalPlc(
                joint.Placement2, joint.Object2, joint.Part2
            )
            jcsPlc1 = UtilsAssembly.getJcsPlcRelativeToPart(
                joint.Placement1, joint.Object1, joint.Part1
            )
            if not sameDir:
                jcsPlc1 = self.flipPlacement(jcsPlc1)
            joint.Part1.Placement = globalJcsPlc2 * jcsPlc1.inverse()
            return True
        return False

    def undoPreSolve(self):
        if self.partMovedByPresolved:
            self.partMovedByPresolved.Placement = self.presolveBackupPlc
            self.partMovedByPresolved = None

    def areJcsSameDir(self, joint):
        globalJcsPlc1 = UtilsAssembly.getJcsGlobalPlc(joint.Placement1, joint.Object1, joint.Part1)
        globalJcsPlc2 = UtilsAssembly.getJcsGlobalPlc(joint.Placement2, joint.Object2, joint.Part2)

        zAxis1 = globalJcsPlc1.Rotation.multVec(App.Vector(0, 0, 1))
        zAxis2 = globalJcsPlc2.Rotation.multVec(App.Vector(0, 0, 1))
        return zAxis1.dot(zAxis2) > 0


class ViewProviderJoint:
    def __init__(self, vobj):
        """Set this object to the proxy object of the actual view provider"""

        vobj.Proxy = self

    def attach(self, vobj):
        """Setup the scene sub-graph of the view provider, this method is mandatory"""
        self.axis_thickness = 3

        view_params = App.ParamGet("User parameter:BaseApp/Preferences/View")
        param_x_axis_color = view_params.GetUnsigned("AxisXColor", 0xCC333300)
        param_y_axis_color = view_params.GetUnsigned("AxisYColor", 0x33CC3300)
        param_z_axis_color = view_params.GetUnsigned("AxisZColor", 0x3333CC00)

        self.x_axis_so_color = coin.SoBaseColor()
        self.x_axis_so_color.rgb.setValue(UtilsAssembly.color_from_unsigned(param_x_axis_color))
        self.y_axis_so_color = coin.SoBaseColor()
        self.y_axis_so_color.rgb.setValue(UtilsAssembly.color_from_unsigned(param_y_axis_color))
        self.z_axis_so_color = coin.SoBaseColor()
        self.z_axis_so_color.rgb.setValue(UtilsAssembly.color_from_unsigned(param_z_axis_color))

        camera = Gui.ActiveDocument.ActiveView.getCameraNode()
        self.cameraSensor = coin.SoFieldSensor(self.camera_callback, camera)
        if isinstance(camera, coin.SoPerspectiveCamera):
            self.cameraSensor.attach(camera.focalDistance)
        elif isinstance(camera, coin.SoOrthographicCamera):
            self.cameraSensor.attach(camera.height)

        self.app_obj = vobj.Object

        self.transform1 = coin.SoTransform()
        self.transform2 = coin.SoTransform()
        self.transform3 = coin.SoTransform()

        scaleF = self.get_JCS_size()
        self.axisScale = coin.SoScale()
        self.axisScale.scaleFactor.setValue(scaleF, scaleF, scaleF)

        self.draw_style = coin.SoDrawStyle()
        self.draw_style.style = coin.SoDrawStyle.LINES
        self.draw_style.lineWidth = self.axis_thickness

        self.switch_JCS1 = self.JCS_sep(self.transform1)
        self.switch_JCS2 = self.JCS_sep(self.transform2)
        self.switch_JCS_preview = self.JCS_sep(self.transform3)

        self.pick = coin.SoPickStyle()
        self.setPickableState(True)

        self.display_mode = coin.SoType.fromName("SoFCSelection").createInstance()
        self.display_mode.addChild(self.pick)
        self.display_mode.addChild(self.switch_JCS1)
        self.display_mode.addChild(self.switch_JCS2)
        self.display_mode.addChild(self.switch_JCS_preview)
        vobj.addDisplayMode(self.display_mode, "Wireframe")

    def camera_callback(self, *args):
        scaleF = self.get_JCS_size()
        self.axisScale.scaleFactor.setValue(scaleF, scaleF, scaleF)

    def JCS_sep(self, soTransform):
        JCS = coin.SoAnnotation()
        JCS.addChild(soTransform)

        base_plane_sep = self.plane_sep(0.4, 15)
        X_axis_sep = self.line_sep([0.5, 0, 0], [1, 0, 0], self.x_axis_so_color)
        Y_axis_sep = self.line_sep([0, 0.5, 0], [0, 1, 0], self.y_axis_so_color)
        Z_axis_sep = self.line_sep([0, 0, 0], [0, 0, 1], self.z_axis_so_color)

        JCS.addChild(base_plane_sep)
        JCS.addChild(X_axis_sep)
        JCS.addChild(Y_axis_sep)
        JCS.addChild(Z_axis_sep)

        switch_JCS = coin.SoSwitch()
        switch_JCS.addChild(JCS)
        switch_JCS.whichChild = coin.SO_SWITCH_NONE
        return switch_JCS

    def line_sep(self, startPoint, endPoint, soColor):
        line = coin.SoLineSet()
        line.numVertices.setValue(2)
        coords = coin.SoCoordinate3()
        coords.point.setValues(0, [startPoint, endPoint])

        axis_sep = coin.SoAnnotation()
        axis_sep.addChild(self.axisScale)
        axis_sep.addChild(self.draw_style)
        axis_sep.addChild(soColor)
        axis_sep.addChild(coords)
        axis_sep.addChild(line)
        return axis_sep

    def plane_sep(self, size, num_vertices):
        coords = coin.SoCoordinate3()

        for i in range(num_vertices):
            angle = float(i) / num_vertices * 2.0 * math.pi
            x = math.cos(angle) * size
            y = math.sin(angle) * size
            coords.point.set1Value(i, x, y, 0)

        face = coin.SoFaceSet()
        face.numVertices.setValue(num_vertices)

        transform = coin.SoTransform()
        transform.translation.setValue(0, 0, 0)

        draw_style = coin.SoDrawStyle()
        draw_style.style = coin.SoDrawStyle.FILLED

        material = coin.SoMaterial()
        material.diffuseColor.setValue([0.5, 0.5, 0.5])
        material.ambientColor.setValue([0.5, 0.5, 0.5])
        material.specularColor.setValue([0.5, 0.5, 0.5])
        material.emissiveColor.setValue([0.5, 0.5, 0.5])
        material.transparency.setValue(0.3)

        face_sep = coin.SoAnnotation()
        face_sep.addChild(self.axisScale)
        face_sep.addChild(transform)
        face_sep.addChild(draw_style)
        face_sep.addChild(material)
        face_sep.addChild(coords)
        face_sep.addChild(face)
        return face_sep

    def get_JCS_size(self):
        camera = Gui.ActiveDocument.ActiveView.getCameraNode()

        # Check if the camera is a perspective camera
        if isinstance(camera, coin.SoPerspectiveCamera):
            return camera.focalDistance.getValue() / 20
        elif isinstance(camera, coin.SoOrthographicCamera):
            return camera.height.getValue() / 20
        else:
            # Default value if camera type is unknown
            return 10

        return camera.height.getValue() / 20

    def set_JCS_placement(self, soTransform, placement, objName, part):
        # change plc to be relative to the origin of the document.
        obj = UtilsAssembly.getObjectInPart(objName, part)
        global_plc = UtilsAssembly.getGlobalPlacement(obj, part)
        placement = global_plc * placement

        t = placement.Base
        soTransform.translation.setValue(t.x, t.y, t.z)

        r = placement.Rotation.Q
        soTransform.rotation.setValue(r[0], r[1], r[2], r[3])

    def updateData(self, joint, prop):
        """If a property of the handled feature has changed we have the chance to handle this here"""
        # joint is the handled feature, prop is the name of the property that has changed
        if prop == "Placement1":
            if joint.Object1:
                plc = joint.Placement1
                self.switch_JCS1.whichChild = coin.SO_SWITCH_ALL

                if joint.Part1:
                    self.set_JCS_placement(self.transform1, plc, joint.Object1, joint.Part1)
            else:
                self.switch_JCS1.whichChild = coin.SO_SWITCH_NONE

        if prop == "Placement2":
            if joint.Object2:
                plc = joint.Placement2
                self.switch_JCS2.whichChild = coin.SO_SWITCH_ALL

                if joint.Part2:
                    self.set_JCS_placement(self.transform2, plc, joint.Object2, joint.Part2)
            else:
                self.switch_JCS2.whichChild = coin.SO_SWITCH_NONE

    def showPreviewJCS(self, visible, placement=None, objName="", part=None):
        if visible:
            self.switch_JCS_preview.whichChild = coin.SO_SWITCH_ALL
            self.set_JCS_placement(self.transform3, placement, objName, part)
        else:
            self.switch_JCS_preview.whichChild = coin.SO_SWITCH_NONE

    def setPickableState(self, state: bool):
        """Set JCS selectable or unselectable in 3D view"""
        if not state:
            self.pick.style.setValue(coin.SoPickStyle.UNPICKABLE)
        else:
            self.pick.style.setValue(coin.SoPickStyle.SHAPE_ON_TOP)

    def getDisplayModes(self, obj):
        """Return a list of display modes."""
        modes = []
        modes.append("Wireframe")
        return modes

    def getDefaultDisplayMode(self):
        """Return the name of the default display mode. It must be defined in getDisplayModes."""
        return "Wireframe"

    def onChanged(self, vp, prop):
        """Here we can do something when a single property got changed"""
        # App.Console.PrintMessage("Change property: " + str(prop) + "\n")
        if prop == "color_X_axis":
            c = vp.getPropertyByName("color_X_axis")
            self.x_axis_so_color.rgb.setValue(c[0], c[1], c[2])
        if prop == "color_Y_axis":
            c = vp.getPropertyByName("color_Y_axis")
            self.x_axis_so_color.rgb.setValue(c[0], c[1], c[2])
        if prop == "color_Z_axis":
            c = vp.getPropertyByName("color_Z_axis")
            self.x_axis_so_color.rgb.setValue(c[0], c[1], c[2])

    def getIcon(self):
        if self.app_obj.JointType == "Fixed":
            return ":/icons/Assembly_CreateJointFixed.svg"
        elif self.app_obj.JointType == "Revolute":
            return ":/icons/Assembly_CreateJointRevolute.svg"
        elif self.app_obj.JointType == "Cylindrical":
            return ":/icons/Assembly_CreateJointCylindrical.svg"
        elif self.app_obj.JointType == "Slider":
            return ":/icons/Assembly_CreateJointSlider.svg"
        elif self.app_obj.JointType == "Ball":
            return ":/icons/Assembly_CreateJointBall.svg"
        elif self.app_obj.JointType == "Distance":
            return ":/icons/Assembly_CreateJointDistance.svg"

        return ":/icons/Assembly_CreateJoint.svg"

    def dumps(self):
        """When saving the document this object gets stored using Python's json module.\
                Since we have some un-serializable parts here -- the Coin stuff -- we must define this method\
                to return a tuple of all serializable objects or None."""
        return None

    def loads(self, state):
        """When restoring the serialized object from document we have the chance to set some internals here.\
                Since no data were serialized nothing needs to be done here."""
        return None

    def doubleClicked(self, vobj):
        assembly = vobj.Object.InList[0]
        if UtilsAssembly.activeAssembly() != assembly:
            Gui.ActiveDocument.setEdit(assembly)

        panel = TaskAssemblyCreateJoint(0, vobj.Object)
        Gui.Control.showDialog(panel)


################ Grounded Joint object #################


class GroundedJoint:
    def __init__(self, joint, obj_to_ground):
        joint.Proxy = self
        self.joint = joint

        joint.addProperty(
            "App::PropertyLink",
            "ObjectToGround",
            "Ground",
            QT_TRANSLATE_NOOP("App::Property", "The object to ground"),
        )

        joint.ObjectToGround = obj_to_ground

        joint.addProperty(
            "App::PropertyPlacement",
            "Placement",
            "Ground",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "This is where the part is grounded.",
            ),
        )

        joint.Placement = obj_to_ground.Placement

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, fp, prop):
        """Do something when a property has changed"""
        # App.Console.PrintMessage("Change property: " + str(prop) + "\n")
        pass

    def execute(self, fp):
        """Do something when doing a recomputation, this method is mandatory"""
        # App.Console.PrintMessage("Recompute Python Box feature\n")
        pass


class ViewProviderGroundedJoint:
    def __init__(self, obj):
        """Set this object to the proxy object of the actual view provider"""
        obj.Proxy = self

    def attach(self, obj):
        """Setup the scene sub-graph of the view provider, this method is mandatory"""
        pass

    def updateData(self, fp, prop):
        """If a property of the handled feature has changed we have the chance to handle this here"""
        # fp is the handled feature, prop is the name of the property that has changed
        pass

    def getDisplayModes(self, obj):
        """Return a list of display modes."""
        modes = ["Wireframe"]
        return modes

    def getDefaultDisplayMode(self):
        """Return the name of the default display mode. It must be defined in getDisplayModes."""
        return "Wireframe"

    def onChanged(self, vp, prop):
        """Here we can do something when a single property got changed"""
        # App.Console.PrintMessage("Change property: " + str(prop) + "\n")
        pass

    def onDelete(self, feature, subelements):  # subelements is a tuple of strings
        # Remove grounded tag.
        if hasattr(feature.Object, "ObjectToGround"):
            obj = feature.Object.ObjectToGround
            if obj is not None and obj.Label.endswith(" 🔒"):
                obj.Label = obj.Label[:-2]

        return True  # If False is returned the object won't be deleted

    def getIcon(self):
        return ":/icons/Assembly_ToggleGrounded.svg"


class MakeJointSelGate:
    def __init__(self, taskbox, assembly):
        self.taskbox = taskbox
        self.assembly = assembly

    def allow(self, doc, obj, sub):
        if not sub:
            return False

        objs_names, element_name = UtilsAssembly.getObjsNamesAndElement(obj.Name, sub)

        if self.assembly.Name not in objs_names:
            # Only objects within the assembly.
            return False

        if Gui.Selection.isSelected(obj, sub, Gui.Selection.ResolveMode.NoResolve):
            # If it's to deselect then it's ok
            return True

        if len(self.taskbox.current_selection) >= 2:
            # No more than 2 elements can be selected for basic joints.
            return False

        full_obj_name = ".".join(objs_names)
        full_element_name = full_obj_name + "." + element_name
        selected_object = UtilsAssembly.getObject(full_element_name)

        part_containing_selected_object = UtilsAssembly.getContainingPart(
            full_element_name, selected_object, self.assembly
        )

        for selection_dict in self.taskbox.current_selection:
            if selection_dict["part"] == part_containing_selected_object:
                # Can't join a solid to itself. So the user need to select 2 different parts.
                return False

        return True


activeTask = None


class TaskAssemblyCreateJoint(QtCore.QObject):
    def __init__(self, jointTypeIndex, jointObj=None):
        super().__init__()

        global activeTask
        activeTask = self

        self.assembly = UtilsAssembly.activeAssembly()
        if not self.assembly:
            self.assembly = UtilsAssembly.activePart()
            self.activeType = "Part"
        else:
            self.activeType = "Assembly"

        self.view = Gui.activeDocument().activeView()
        self.doc = App.ActiveDocument

        if not self.assembly or not self.view or not self.doc:
            return

        if self.activeType == "Assembly":
            self.assembly.ViewObject.EnableMovement = False

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyCreateJoint.ui")

        if self.activeType == "Part":
            self.form.setWindowTitle("Match parts")
            self.form.jointType.hide()
        self.form.jointType.addItems(TranslatedJointTypes)
        self.form.jointType.setCurrentIndex(jointTypeIndex)
        self.form.jointType.currentIndexChanged.connect(self.onJointTypeChanged)

        self.form.distanceSpinbox.valueChanged.connect(self.onDistanceChanged)
        self.form.offsetSpinbox.valueChanged.connect(self.onOffsetChanged)
        self.form.rotationSpinbox.valueChanged.connect(self.onRotationChanged)
        self.form.PushButtonReverse.clicked.connect(self.onReverseClicked)

        if jointObj:
            Gui.Selection.clearSelection()
            self.creating = False
            self.joint = jointObj
            self.jointName = jointObj.Label
            App.setActiveTransaction("Edit " + self.jointName + " Joint")

            self.updateTaskboxFromJoint()
            self.visibilityBackup = self.joint.Visibility
            self.joint.Visibility = True

        else:
            self.creating = True
            self.jointName = self.form.jointType.currentText().replace(" ", "")
            if self.activeType == "Part":
                App.setActiveTransaction("Transform")
            else:
                App.setActiveTransaction("Create " + self.jointName + " Joint")

            self.current_selection = []
            self.preselection_dict = None

            self.createJointObject()
            self.visibilityBackup = False
            self.handleInitialSelection()

        self.toggleDistanceVisibility()
        self.toggleOffsetVisibility()
        self.toggleRotationVisibility()
        self.toggleReverseVisibility()

        self.setJointsPickableState(False)

        Gui.Selection.addSelectionGate(
            MakeJointSelGate(self, self.assembly), Gui.Selection.ResolveMode.NoResolve
        )
        Gui.Selection.addObserver(self, Gui.Selection.ResolveMode.NoResolve)
        Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.GreedySelection)

        self.callbackMove = self.view.addEventCallback("SoLocation2Event", self.moveMouse)
        self.callbackKey = self.view.addEventCallback("SoKeyboardEvent", self.KeyboardEvent)

    def accept(self):
        if len(self.current_selection) != 2:
            App.Console.PrintWarning("You need to select 2 elements from 2 separate parts.")
            return False

        self.deactivate()

        solveIfAllowed(self.assembly)
        if self.activeType == "Assembly":
            self.joint.Visibility = self.visibilityBackup
        else:
            self.joint.Document.removeObject(self.joint.Name)

        App.closeActiveTransaction()
        return True

    def reject(self):
        self.deactivate()
        App.closeActiveTransaction(True)
        if not self.creating:  # update visibility only if we are editing the joint
            self.joint.Visibility = self.visibilityBackup
        return True

    def deactivate(self):
        global activeTask
        activeTask = None

        if self.activeType == "Assembly":
            self.assembly.clearUndo()
            self.assembly.ViewObject.EnableMovement = True

        Gui.Selection.removeSelectionGate()
        Gui.Selection.removeObserver(self)
        Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.NormalSelection)
        Gui.Selection.clearSelection()
        self.view.removeEventCallback("SoLocation2Event", self.callbackMove)
        self.view.removeEventCallback("SoKeyboardEvent", self.callbackKey)
        self.setJointsPickableState(True)
        if Gui.Control.activeDialog():
            Gui.Control.closeDialog()

    def handleInitialSelection(self):
        selection = Gui.Selection.getSelectionEx("*", 0)
        if not selection:
            return
        for sel in selection:
            # If you select 2 solids (bodies for example) within an assembly.
            # There'll be a single sel but 2 SubElementNames.

            if not sel.SubElementNames:
                # no subnames, so its a root assembly itself that is selected.
                Gui.Selection.removeSelection(sel.Object)
                continue

            for sub_name in sel.SubElementNames:
                # Only objects within the assembly.
                objs_names, element_name = UtilsAssembly.getObjsNamesAndElement(
                    sel.ObjectName, sub_name
                )
                if self.assembly.Name not in objs_names:
                    Gui.Selection.removeSelection(sel.Object, sub_name)
                    continue

                obj_name = sel.ObjectName

                full_obj_name = UtilsAssembly.getFullObjName(obj_name, sub_name)
                full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
                selected_object = UtilsAssembly.getObject(full_element_name)
                element_name = UtilsAssembly.getElementName(full_element_name)
                part_containing_selected_object = self.getContainingPart(
                    full_element_name, selected_object
                )

                if selected_object == self.assembly:
                    # do not accept selection of assembly itself
                    Gui.Selection.removeSelection(sel.Object, sub_name)
                    continue

                if (
                    len(self.current_selection) == 1
                    and selected_object == self.current_selection[0]["object"]
                ):
                    # do not select several feature of the same object.
                    self.current_selection.clear()
                    Gui.Selection.clearSelection()
                    return

                selection_dict = {
                    "object": selected_object,
                    "part": part_containing_selected_object,
                    "element_name": element_name,
                    "full_element_name": full_element_name,
                    "full_obj_name": full_obj_name,
                    "vertex_name": element_name,
                }

                self.current_selection.append(selection_dict)

        # do not accept initial selection if we don't have 2 selected features
        if len(self.current_selection) != 2:
            self.current_selection.clear()
            Gui.Selection.clearSelection()
        else:
            self.updateJoint()

    def createJointObject(self):
        type_index = self.form.jointType.currentIndex()

        if self.activeType == "Part":
            self.joint = self.assembly.newObject("App::FeaturePython", "Temporary joint")
        else:
            joint_group = UtilsAssembly.getJointGroup(self.assembly)
            self.joint = joint_group.newObject("App::FeaturePython", self.jointName)

        Joint(self.joint, type_index)
        ViewProviderJoint(self.joint.ViewObject)

    def onJointTypeChanged(self, index):

        self.joint.Proxy.setJointType(self.joint, JointTypes[self.form.jointType.currentIndex()])
        self.toggleDistanceVisibility()
        self.toggleOffsetVisibility()
        self.toggleRotationVisibility()
        self.toggleReverseVisibility()

    def onDistanceChanged(self, quantity):
        self.joint.Distance = self.form.distanceSpinbox.property("rawValue")

    def onOffsetChanged(self, quantity):
        self.joint.Offset = App.Vector(0, 0, self.form.offsetSpinbox.property("rawValue"))

    def onRotationChanged(self, quantity):
        self.joint.Rotation = self.form.rotationSpinbox.property("rawValue")

    def onReverseClicked(self):
        self.joint.Proxy.flipOnePart(self.joint)

    def toggleDistanceVisibility(self):
        if JointTypes[self.form.jointType.currentIndex()] in JointUsingDistance:
            self.form.distanceLabel.show()
            self.form.distanceSpinbox.show()
        else:
            self.form.distanceLabel.hide()
            self.form.distanceSpinbox.hide()

    def toggleOffsetVisibility(self):
        if JointTypes[self.form.jointType.currentIndex()] in JointUsingOffset:
            self.form.offsetLabel.show()
            self.form.offsetSpinbox.show()
        else:
            self.form.offsetLabel.hide()
            self.form.offsetSpinbox.hide()

    def toggleRotationVisibility(self):
        if JointTypes[self.form.jointType.currentIndex()] in JointUsingRotation:
            self.form.rotationLabel.show()
            self.form.rotationSpinbox.show()
        else:
            self.form.rotationLabel.hide()
            self.form.rotationSpinbox.hide()

    def toggleReverseVisibility(self):
        if JointTypes[self.form.jointType.currentIndex()] in JointUsingReverse:
            self.form.PushButtonReverse.show()
        else:
            self.form.PushButtonReverse.hide()

    def updateTaskboxFromJoint(self):
        self.current_selection = []
        self.preselection_dict = None

        obj1 = UtilsAssembly.getObjectInPart(self.joint.Object1, self.joint.Part1)
        obj2 = UtilsAssembly.getObjectInPart(self.joint.Object2, self.joint.Part2)

        selection_dict1 = {
            "object": obj1,
            "part": self.joint.Part1,
            "element_name": self.joint.Element1,
            "vertex_name": self.joint.Vertex1,
        }

        selection_dict2 = {
            "object": obj2,
            "part": self.joint.Part2,
            "element_name": self.joint.Element2,
            "vertex_name": self.joint.Vertex2,
        }

        self.current_selection.append(selection_dict1)
        self.current_selection.append(selection_dict2)

        # Add the elements to the selection. Note we cannot do :
        # Gui.Selection.addSelection(self.doc.Name, obj1.Name, elName)
        # Because obj1 can be external in which case addSelection will fail. And
        # Gui.Selection.addSelection(obj1.Document.Name, obj1.Name, elName)
        # will not select in the assembly doc.
        elName = self.getSubnameForSelection(obj1, self.joint.Part1, self.joint.Element1)
        Gui.Selection.addSelection(self.doc.Name, self.joint.Part1.Name, elName)

        elName = self.getSubnameForSelection(obj2, self.joint.Part2, self.joint.Element2)
        Gui.Selection.addSelection(self.doc.Name, self.joint.Part2.Name, elName)

        self.form.distanceSpinbox.setProperty("rawValue", self.joint.Distance)
        self.form.offsetSpinbox.setProperty("rawValue", self.joint.Offset.z)
        self.form.rotationSpinbox.setProperty("rawValue", self.joint.Rotation)

        self.form.jointType.setCurrentIndex(JointTypes.index(self.joint.JointType))
        self.updateJointList()

    def getSubnameForSelection(self, obj, part, elName):
        # We need the subname starting from the part.
        # Example for : Assembly.Part1.LinkToPart2.Part3.Body.Tip.Face1
        # part is Part1 and obj is Body
        # we should get : LinkToPart2.Part3.Body.Tip.Face1

        if obj is None or part is None:
            return elName

        if obj.TypeId == "PartDesign::Body":
            elName = obj.Tip.Name + "." + elName
        elif obj.TypeId == "App::Link":
            linked_obj = obj.getLinkedObject()
            if linked_obj.TypeId == "PartDesign::Body":
                elName = linked_obj.Tip.Name + "." + elName

        if obj != part and obj in part.OutListRecursive:
            bSub = ""
            currentObj = part

            limit = 0
            while limit < 1000:
                limit = limit + 1

                if currentObj != part:
                    if bSub != "":
                        bSub = bSub + "."
                    bSub = bSub + currentObj.Name

                if currentObj == obj:
                    break

                if currentObj.TypeId == "App::Link":
                    currentObj = currentObj.getLinkedObject()

                for obji in currentObj.OutList:
                    if obji == obj or obj in obji.OutListRecursive:
                        currentObj = obji
                        break

            elName = bSub + "." + elName
        return elName

    def updateJoint(self):
        # First we build the listwidget
        self.updateJointList()

        # Then we pass the new list to the join object
        self.joint.Proxy.setJointConnectors(self.joint, self.current_selection)

    def updateJointList(self):
        self.form.featureList.clear()
        simplified_names = []
        for sel in self.current_selection:
            sname = sel["object"].Label
            if sel["element_name"] != "":
                sname = sname + "." + sel["element_name"]
            simplified_names.append(sname)
        self.form.featureList.addItems(simplified_names)

    def moveMouse(self, info):
        if len(self.current_selection) >= 2 or (
            len(self.current_selection) == 1
            and (
                not self.preselection_dict
                or self.current_selection[0]["part"] == self.preselection_dict["part"]
            )
        ):
            self.joint.ViewObject.Proxy.showPreviewJCS(False)
            return

        cursor_pos = self.view.getCursorPos()
        cursor_info = self.view.getObjectInfo(cursor_pos)
        # cursor_info example  {'x': 41.515, 'y': 7.449, 'z': 16.861, 'ParentObject': <Part object>, 'SubName': 'Body002.Pad.Face5', 'Document': 'part3', 'Object': 'Pad', 'Component': 'Face5'}

        if (
            not cursor_info
            or not self.preselection_dict
            # or cursor_info["SubName"] != self.preselection_dict["sub_name"]
            # Removed because they are not equal when hovering a line endpoints.
            # But we don't actually need to test because if there's no preselection then not cursor is None
        ):
            self.joint.ViewObject.Proxy.showPreviewJCS(False)
            return

        # newPos = self.view.getPoint(*info["Position"]) # This is not what we want, it's not pos on the object but on the focal plane

        newPos = App.Vector(cursor_info["x"], cursor_info["y"], cursor_info["z"])
        self.preselection_dict["mouse_pos"] = newPos

        if self.preselection_dict["element_name"] == "":
            self.preselection_dict["vertex_name"] = ""
        else:
            self.preselection_dict["vertex_name"] = UtilsAssembly.findElementClosestVertex(
                self.preselection_dict
            )

        isSecond = len(self.current_selection) == 1
        objName = self.preselection_dict["object"].Name
        part = self.preselection_dict["part"]
        placement = self.joint.Proxy.findPlacement(
            self.joint,
            objName,
            part,
            self.preselection_dict["element_name"],
            self.preselection_dict["vertex_name"],
            isSecond,
        )
        self.joint.ViewObject.Proxy.showPreviewJCS(True, placement, objName, part)
        self.previewJCSVisible = True

    # 3D view keyboard handler
    def KeyboardEvent(self, info):
        if info["State"] == "UP" and info["Key"] == "ESCAPE":
            self.reject()

        if info["State"] == "UP" and info["Key"] == "RETURN":
            self.accept()

    def getContainingPart(self, full_element_name, obj):
        return UtilsAssembly.getContainingPart(full_element_name, obj, self.assembly)

    # selectionObserver stuff
    def addSelection(self, doc_name, obj_name, sub_name, mousePos):
        full_obj_name = UtilsAssembly.getFullObjName(obj_name, sub_name)
        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
        selected_object = UtilsAssembly.getObject(full_element_name)
        element_name = UtilsAssembly.getElementName(full_element_name)
        part_containing_selected_object = self.getContainingPart(full_element_name, selected_object)

        selection_dict = {
            "object": selected_object,
            "part": part_containing_selected_object,
            "element_name": element_name,
            "full_element_name": full_element_name,
            "full_obj_name": full_obj_name,
            "mouse_pos": App.Vector(mousePos[0], mousePos[1], mousePos[2]),
        }
        if element_name == "":
            selection_dict["vertex_name"] = ""
        else:
            selection_dict["vertex_name"] = UtilsAssembly.findElementClosestVertex(selection_dict)

        self.current_selection.append(selection_dict)
        self.updateJoint()

        # We hide the preview JCS if we just added to the selection
        self.joint.ViewObject.Proxy.showPreviewJCS(False)

    def removeSelection(self, doc_name, obj_name, sub_name, mousePos=None):
        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
        selected_object = UtilsAssembly.getObject(full_element_name)
        element_name = UtilsAssembly.getElementName(full_element_name)
        part_containing_selected_object = self.getContainingPart(full_element_name, selected_object)

        # Find and remove the corresponding dictionary from the combined list
        selection_dict_to_remove = None
        for selection_dict in self.current_selection:
            if selection_dict["part"] == part_containing_selected_object:
                selection_dict_to_remove = selection_dict
                break

        if selection_dict_to_remove is not None:
            self.current_selection.remove(selection_dict_to_remove)

        self.updateJoint()

    def setPreselection(self, doc_name, obj_name, sub_name):
        if not sub_name:
            self.preselection_dict = None
            return

        full_obj_name = UtilsAssembly.getFullObjName(obj_name, sub_name)
        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
        selected_object = UtilsAssembly.getObject(full_element_name)
        element_name = UtilsAssembly.getElementName(full_element_name)
        part_containing_selected_object = self.getContainingPart(full_element_name, selected_object)

        self.preselection_dict = {
            "object": selected_object,
            "part": part_containing_selected_object,
            "sub_name": sub_name,
            "element_name": element_name,
            "full_element_name": full_element_name,
            "full_obj_name": full_obj_name,
        }

    def clearSelection(self, doc_name):
        self.current_selection.clear()
        self.updateJoint()

    def setJointsPickableState(self, state: bool):
        """Make all joints in assembly selectable (True) or unselectable (False) in 3D view"""
        if self.activeType == "Assembly":
            jointGroup = UtilsAssembly.getJointGroup(self.assembly)
            for joint in jointGroup.Group:
                if hasattr(joint, "JointType"):
                    joint.ViewObject.Proxy.setPickableState(state)
        else:
            for obj in self.assembly.OutList:
                if obj.TypeId == "App::FeaturePython" and hasattr(obj, "JointType"):
                    obj.ViewObject.Proxy.setPickableState(state)
