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
    translate("Assembly", "Parallel"),
    translate("Assembly", "Perpendicular"),
    translate("Assembly", "Angle"),
    translate("Assembly", "RackPinion"),
    translate("Assembly", "Screw"),
    translate("Assembly", "Gears"),
    translate("Assembly", "Belt"),
]

JointTypes = [
    "Fixed",
    "Revolute",
    "Cylindrical",
    "Slider",
    "Ball",
    "Distance",
    "Parallel",
    "Perpendicular",
    "Angle",
    "RackPinion",
    "Screw",
    "Gears",
    "Belt",
]

JointUsingDistance = [
    "Distance",
    "Angle",
    "RackPinion",
    "Screw",
    "Gears",
    "Belt",
]

JointUsingDistance2 = [
    "Gears",
    "Belt",
]

JointNoNegativeDistance = [
    "RackPinion",
    "Screw",
    "Gears",
    "Belt",
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
    "Parallel",
]

JointUsingLimitLength = [
    "Cylindrical",
    "Slider",
]

JointUsingLimitAngle = [
    "Revolute",
    "Cylindrical",
]

JointUsingPreSolve = [
    "Fixed",
    "Revolute",
    "Cylindrical",
    "Slider",
    "Ball",
]

JointParallelForbidden = [
    "Angle",
    "Perpendicular",
]


def solveIfAllowed(assembly, storePrev=False):
    if assembly.Type == "Assembly" and Preferences.preferences().GetBool(
        "SolveInJointCreation", True
    ):
        assembly.solve(storePrev)


def get_camera_height(gui_doc):
    activeView = get_active_view(gui_doc)
    if activeView is None:
        return 200

    camera = activeView.getCameraNode()

    # Check if the camera is a perspective camera
    if isinstance(camera, coin.SoPerspectiveCamera):
        return camera.focalDistance.getValue()
    elif isinstance(camera, coin.SoOrthographicCamera):
        return camera.height.getValue()
    else:
        # Default value if camera type is unknown
        return 200


def get_active_view(gui_doc):
    activeView = gui_doc.ActiveView
    if activeView is None:
        # Fall back on current active document.
        activeView = Gui.ActiveDocument.ActiveView
    return activeView


# The joint object consists of 2 JCS (joint coordinate systems) and a Joint Type.
# A JCS is a placement that is computed (unless it is detached) from :
# - An Object: this can be any Part::Feature solid. Or a PartDesign Body. Or a App::Link to those.
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

        self.createProperties(joint)

        self.setJointConnectors(joint, [])

    def onDocumentRestored(self, joint):
        self.createProperties(joint)

    def createProperties(self, joint):
        self.migrationScript(joint)

        # First Joint Connector
        if not hasattr(joint, "Object1"):
            joint.addProperty(
                "App::PropertyXLinkSub",
                "Object1",
                "Joint Connector 1",
                QT_TRANSLATE_NOOP("App::Property", "The first object of the joint"),
            )

        if not hasattr(joint, "Part1"):
            joint.addProperty(
                "App::PropertyLink",
                "Part1",
                "Joint Connector 1",
                QT_TRANSLATE_NOOP("App::Property", "The first part of the joint"),
            )

        if not hasattr(joint, "Placement1"):
            joint.addProperty(
                "App::PropertyPlacement",
                "Placement1",
                "Joint Connector 1",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the local coordinate system within object1 that will be used for the joint.",
                ),
            )

        if not hasattr(joint, "Detach1"):
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
        if not hasattr(joint, "Object2"):
            joint.addProperty(
                "App::PropertyXLinkSub",
                "Object2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP("App::Property", "The second object of the joint"),
            )

        if not hasattr(joint, "Part2"):
            joint.addProperty(
                "App::PropertyLink",
                "Part2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP("App::Property", "The second part of the joint"),
            )

        if not hasattr(joint, "Placement2"):
            joint.addProperty(
                "App::PropertyPlacement",
                "Placement2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the local coordinate system within object2 that will be used for the joint.",
                ),
            )

        if not hasattr(joint, "Detach2"):
            joint.addProperty(
                "App::PropertyBool",
                "Detach2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This prevents Placement2 from recomputing, enabling custom positioning of the placement.",
                ),
            )

        # Other properties
        if not hasattr(joint, "Distance"):
            joint.addProperty(
                "App::PropertyFloat",
                "Distance",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the distance of the joint. It is used only by the Distance joint and Rack and Pinion (pitch radius), Screw and Gears and Belt (radius1)",
                ),
            )

        if not hasattr(joint, "Distance2"):
            joint.addProperty(
                "App::PropertyFloat",
                "Distance2",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the second distance of the joint. It is used only by the gear joint to store the second radius.",
                ),
            )

        if not hasattr(joint, "Rotation"):
            joint.addProperty(
                "App::PropertyFloat",
                "Rotation",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the rotation of the joint.",
                ),
            )

        if not hasattr(joint, "Offset"):
            joint.addProperty(
                "App::PropertyVector",
                "Offset",
                "Joint",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the offset vector of the joint.",
                ),
            )

        if not hasattr(joint, "Activated"):
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

        if not hasattr(joint, "EnableLengthMin"):
            joint.addProperty(
                "App::PropertyBool",
                "EnableLengthMin",
                "Limits",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Enable the minimum length limit of the joint.",
                ),
            )
            joint.EnableLengthMin = False

        if not hasattr(joint, "EnableLengthMax"):
            joint.addProperty(
                "App::PropertyBool",
                "EnableLengthMax",
                "Limits",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Enable the maximum length limit of the joint.",
                ),
            )
            joint.EnableLengthMax = False

        if not hasattr(joint, "EnableAngleMin"):
            joint.addProperty(
                "App::PropertyBool",
                "EnableAngleMin",
                "Limits",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Enable the minimum angle limit of the joint.",
                ),
            )
            joint.EnableAngleMin = False

        if not hasattr(joint, "EnableAngleMax"):
            joint.addProperty(
                "App::PropertyBool",
                "EnableAngleMax",
                "Limits",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Enable the minimum length of the joint.",
                ),
            )
            joint.EnableAngleMax = False

        if not hasattr(joint, "LengthMin"):
            joint.addProperty(
                "App::PropertyFloat",
                "LengthMin",
                "Limits",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the minimum limit for the length between both coordinate systems (along their Z axis).",
                ),
            )

        if not hasattr(joint, "LengthMax"):
            joint.addProperty(
                "App::PropertyFloat",
                "LengthMax",
                "Limits",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the maximum limit for the length between both coordinate systems (along their Z axis).",
                ),
            )

        if not hasattr(joint, "AngleMin"):
            joint.addProperty(
                "App::PropertyFloat",
                "AngleMin",
                "Limits",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the minimum limit for the angle between both coordinate systems (between their X axis).",
                ),
            )

        if not hasattr(joint, "AngleMax"):
            joint.addProperty(
                "App::PropertyFloat",
                "AngleMax",
                "Limits",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the maximum limit for the angle between both coordinate systems (between their X axis).",
                ),
            )

    def migrationScript(self, joint):
        if hasattr(joint, "Object1") and isinstance(joint.Object1, str):
            objName = joint.Object1
            obj1 = UtilsAssembly.getObjectInPart(objName, joint.Part1)
            el1 = joint.Element1
            vtx1 = joint.Vertex1

            joint.removeProperty("Object1")
            joint.removeProperty("Element1")
            joint.removeProperty("Vertex1")

            joint.addProperty(
                "App::PropertyXLinkSub",
                "Object1",
                "Joint Connector 1",
                QT_TRANSLATE_NOOP("App::Property", "The first object of the joint"),
            )

            joint.Object1 = [obj1, [el1, vtx1]]

        if hasattr(joint, "Object2") and isinstance(joint.Object2, str):
            objName = joint.Object2
            obj2 = UtilsAssembly.getObjectInPart(objName, joint.Part2)
            el2 = joint.Element2
            vtx2 = joint.Vertex2

            joint.removeProperty("Object2")
            joint.removeProperty("Element2")
            joint.removeProperty("Vertex2")

            joint.addProperty(
                "App::PropertyXLinkSub",
                "Object2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP("App::Property", "The second object of the joint"),
            )

            joint.Object2 = [obj2, [el2, vtx2]]

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def getAssembly(self, joint):
        for obj in joint.InList:
            if obj.isDerivedFrom("Assembly::AssemblyObject"):
                return obj
        return None

    def setJointType(self, joint, jointType):
        joint.JointType = jointType
        joint.Label = jointType.replace(" ", "")

    def onChanged(self, joint, prop):
        """Do something when a property has changed"""
        # App.Console.PrintMessage("Change property: " + str(prop) + "\n")

        # during loading the onchanged may be triggered before full init.
        if App.isRestoring():
            return

        if prop == "Rotation" or prop == "Offset":
            self.updateJCSPlacements(joint)
            if joint.Object1 is None or joint.Object2 is None:
                return

            presolved = self.preSolve(joint, False)

            isAssembly = self.getAssembly(joint).Type == "Assembly"
            if isAssembly and not presolved:
                solveIfAllowed(self.getAssembly(joint))
            else:
                self.updateJCSPlacements(joint)

        if prop == "Distance" and (joint.JointType == "Distance" or joint.JointType == "Angle"):
            if joint.Part1 and joint.Part2:
                if joint.JointType == "Angle" and joint.Distance != 0.0:
                    self.preventParallel(joint)
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
            joint.Object1 = [
                current_selection[0]["object"],
                [current_selection[0]["element_name"], current_selection[0]["vertex_name"]],
            ]
            joint.Part1 = current_selection[0]["part"]
            joint.Placement1 = self.findPlacement(
                joint, joint.Object1[0], joint.Part1, joint.Object1[1][0], joint.Object1[1][1]
            )
        else:
            joint.Object1 = None
            joint.Part1 = None
            joint.Placement1 = App.Placement()
            self.partMovedByPresolved = None

        if len(current_selection) >= 2:
            joint.Object2 = [
                current_selection[1]["object"],
                [current_selection[1]["element_name"], current_selection[1]["vertex_name"]],
            ]
            joint.Part2 = current_selection[1]["part"]
            joint.Placement2 = self.findPlacement(
                joint, joint.Object2[0], joint.Part2, joint.Object2[1][0], joint.Object2[1][1], True
            )
            if joint.JointType in JointUsingPreSolve:
                self.preSolve(joint)
            elif joint.JointType in JointParallelForbidden:
                self.preventParallel(joint)

            if isAssembly:
                solveIfAllowed(assembly, True)
            else:
                self.updateJCSPlacements(joint)

        else:
            joint.Object2 = None
            joint.Part2 = None
            joint.Placement2 = App.Placement()
            if isAssembly:
                assembly.undoSolve()
            self.undoPreSolve(joint)

    def updateJCSPlacements(self, joint):
        if not joint.Detach1:
            joint.Placement1 = self.findPlacement(
                joint, joint.Object1[0], joint.Part1, joint.Object1[1][0], joint.Object1[1][1]
            )

        if not joint.Detach2:
            joint.Placement2 = self.findPlacement(
                joint, joint.Object2[0], joint.Part2, joint.Object2[1][0], joint.Object2[1][1], True
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

    def findPlacement(self, joint, obj, part, elt, vtx, isSecond=False):
        if not obj or not part:
            return App.Placement()

        ignoreVertex = joint.JointType == "Distance"
        plc = UtilsAssembly.findPlacement(obj, part, elt, vtx, ignoreVertex)

        # We apply rotation / reverse / offset it necessary, but only to the second JCS.
        if isSecond:
            if joint.Offset.Length != 0.0:
                plc = UtilsAssembly.applyOffsetToPlacement(plc, joint.Offset)
            if joint.Rotation != 0.0:
                plc = UtilsAssembly.applyRotationToPlacement(plc, joint.Rotation)

        return plc

    def flipOnePart(self, joint):
        assembly = self.getAssembly(joint)
        part2ConnectedByJoint = assembly.isJointConnectingPartToGround(joint, "Part2")
        part1Grounded = assembly.isPartGrounded(joint.Part1)
        part2Grounded = assembly.isPartGrounded(joint.Part2)
        if part2ConnectedByJoint and not part2Grounded:
            jcsPlc = UtilsAssembly.getJcsPlcRelativeToPart(
                joint.Placement2, joint.Object2[0], joint.Part2
            )
            globalJcsPlc = UtilsAssembly.getJcsGlobalPlc(
                joint.Placement2, joint.Object2[0], joint.Part2
            )
            jcsPlc = UtilsAssembly.flipPlacement(jcsPlc)
            joint.Part2.Placement = globalJcsPlc * jcsPlc.inverse()

        elif not part1Grounded:
            jcsPlc = UtilsAssembly.getJcsPlcRelativeToPart(
                joint.Placement1, joint.Object1[0], joint.Part1
            )
            globalJcsPlc = UtilsAssembly.getJcsGlobalPlc(
                joint.Placement1, joint.Object1[0], joint.Part1
            )
            jcsPlc = UtilsAssembly.flipPlacement(jcsPlc)
            joint.Part1.Placement = globalJcsPlc * jcsPlc.inverse()

        solveIfAllowed(self.getAssembly(joint))

    def preSolve(self, joint, savePlc=True):
        # The goal of this is to put the part in the correct position to avoid wrong placement by the solve.

        # we actually don't want to match perfectly the JCS, it is best to match them
        # in the current closest direction, ie either matched or flipped.
        sameDir = self.areJcsSameDir(joint)
        assembly = self.getAssembly(joint)
        isAssembly = assembly.Type == "Assembly"
        if isAssembly:
            joint.Activated = False
            part1Connected = assembly.isPartConnected(joint.Part1)
            part2Connected = assembly.isPartConnected(joint.Part2)
            joint.Activated = True
        else:
            part1Connected = False
            part2Connected = True

        if not part2Connected:
            if savePlc:
                self.partMovedByPresolved = joint.Part2
                self.presolveBackupPlc = joint.Part2.Placement

            globalJcsPlc1 = UtilsAssembly.getJcsGlobalPlc(
                joint.Placement1, joint.Object1[0], joint.Part1
            )
            jcsPlc2 = UtilsAssembly.getJcsPlcRelativeToPart(
                joint.Placement2, joint.Object2[0], joint.Part2
            )
            if not sameDir:
                jcsPlc2 = UtilsAssembly.flipPlacement(jcsPlc2)
            joint.Part2.Placement = globalJcsPlc1 * jcsPlc2.inverse()
            return True

        elif not part1Connected:
            if savePlc:
                self.partMovedByPresolved = joint.Part1
                self.presolveBackupPlc = joint.Part1.Placement

            globalJcsPlc2 = UtilsAssembly.getJcsGlobalPlc(
                joint.Placement2, joint.Object2[0], joint.Part2
            )
            jcsPlc1 = UtilsAssembly.getJcsPlcRelativeToPart(
                joint.Placement1, joint.Object1[0], joint.Part1
            )
            if not sameDir:
                jcsPlc1 = UtilsAssembly.flipPlacement(jcsPlc1)
            joint.Part1.Placement = globalJcsPlc2 * jcsPlc1.inverse()
            return True
        return False

    def undoPreSolve(self, joint):
        if hasattr(self, "partMovedByPresolved") and self.partMovedByPresolved:
            self.partMovedByPresolved.Placement = self.presolveBackupPlc
            self.partMovedByPresolved = None

            joint.Placement1 = joint.Placement1  # Make sure plc1 is redrawn

    def preventParallel(self, joint):
        # Angle and perpendicular joints in the solver cannot handle the situation where both JCS are Parallel
        parallel = self.areJcsZParallel(joint)
        if not parallel:
            return

        assembly = self.getAssembly(joint)
        isAssembly = assembly.Type == "Assembly"
        if isAssembly:
            part1ConnectedByJoint = assembly.isJointConnectingPartToGround(joint, "Part1")
            part2ConnectedByJoint = assembly.isJointConnectingPartToGround(joint, "Part2")
        else:
            part1ConnectedByJoint = False
            part2ConnectedByJoint = True

        if part2ConnectedByJoint:
            self.partMovedByPresolved = joint.Part2
            self.presolveBackupPlc = joint.Part2.Placement

            joint.Part2.Placement = UtilsAssembly.applyRotationToPlacementAlongAxis(
                joint.Part2.Placement, 10, App.Vector(1, 0, 0)
            )

        elif part1ConnectedByJoint:
            self.partMovedByPresolved = joint.Part1
            self.presolveBackupPlc = joint.Part1.Placement

            joint.Part1.Placement = UtilsAssembly.applyRotationToPlacementAlongAxis(
                joint.Part1.Placement, 10, App.Vector(1, 0, 0)
            )

    def areJcsSameDir(self, joint):
        globalJcsPlc1 = UtilsAssembly.getJcsGlobalPlc(
            joint.Placement1, joint.Object1[0], joint.Part1
        )
        globalJcsPlc2 = UtilsAssembly.getJcsGlobalPlc(
            joint.Placement2, joint.Object2[0], joint.Part2
        )

        return UtilsAssembly.arePlacementSameDir(globalJcsPlc1, globalJcsPlc2)

    def areJcsZParallel(self, joint):
        globalJcsPlc1 = UtilsAssembly.getJcsGlobalPlc(
            joint.Placement1, joint.Object1[0], joint.Part1
        )
        globalJcsPlc2 = UtilsAssembly.getJcsGlobalPlc(
            joint.Placement2, joint.Object2[0], joint.Part2
        )

        return UtilsAssembly.arePlacementZParallel(globalJcsPlc1, globalJcsPlc2)


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

        self.app_obj = vobj.Object
        app_doc = self.app_obj.Document
        self.gui_doc = Gui.getDocument(app_doc)
        activeView = get_active_view(self.gui_doc)
        if activeView is not None:
            camera = activeView.getCameraNode()
            self.cameraSensor = coin.SoFieldSensor(self.camera_callback, camera)
            if isinstance(camera, coin.SoPerspectiveCamera):
                self.cameraSensor.attach(camera.focalDistance)
            elif isinstance(camera, coin.SoOrthographicCamera):
                self.cameraSensor.attach(camera.height)

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
        return get_camera_height(self.gui_doc) / 20

    def set_JCS_placement(self, soTransform, placement, obj, part):
        # change plc to be relative to the origin of the document.
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
                    self.set_JCS_placement(self.transform1, plc, joint.Object1[0], joint.Part1)
            else:
                self.switch_JCS1.whichChild = coin.SO_SWITCH_NONE

        if prop == "Placement2":
            if joint.Object2:
                plc = joint.Placement2
                self.switch_JCS2.whichChild = coin.SO_SWITCH_ALL

                if joint.Part2:
                    self.set_JCS_placement(self.transform2, plc, joint.Object2[0], joint.Part2)
            else:
                self.switch_JCS2.whichChild = coin.SO_SWITCH_NONE

    def showPreviewJCS(self, visible, placement=None, obj=None, part=None):
        if visible:
            self.switch_JCS_preview.whichChild = coin.SO_SWITCH_ALL
            self.set_JCS_placement(self.transform3, placement, obj, part)
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
        elif self.app_obj.JointType == "Parallel":
            return ":/icons/Assembly_CreateJointParallel.svg"
        elif self.app_obj.JointType == "Perpendicular":
            return ":/icons/Assembly_CreateJointPerpendicular.svg"
        elif self.app_obj.JointType == "Angle":
            return ":/icons/Assembly_CreateJointAngle.svg"
        elif self.app_obj.JointType == "RackPinion":
            return ":/icons/Assembly_CreateJointRackPinion.svg"
        elif self.app_obj.JointType == "Screw":
            return ":/icons/Assembly_CreateJointScrew.svg"
        elif self.app_obj.JointType == "Gears":
            return ":/icons/Assembly_CreateJointGears.svg"
        elif self.app_obj.JointType == "Belt":
            return ":/icons/Assembly_CreateJointPulleys.svg"

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
        task = Gui.Control.activeTaskDialog()
        if task:
            task.reject()

        assembly = vobj.Object.Proxy.getAssembly(vobj.Object)

        if assembly is None:
            return False

        if UtilsAssembly.activeAssembly() != assembly:
            self.gui_doc.setEdit(assembly)

        panel = TaskAssemblyCreateJoint(0, vobj.Object)
        Gui.Control.showDialog(panel)

        return True

    def canDelete(self, _obj):
        return True


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

    def attach(self, vobj):
        """Setup the scene sub-graph of the view provider, this method is mandatory"""
        app_obj = vobj.Object
        if app_obj is None:
            return
        groundedObj = app_obj.ObjectToGround
        if groundedObj is None:
            return

        lockpadColorInt = Preferences.preferences().GetUnsigned("AssemblyConstraints", 0xCC333300)
        self.lockpadColor = coin.SoBaseColor()
        self.lockpadColor.rgb.setValue(UtilsAssembly.color_from_unsigned(lockpadColorInt))

        self.app_obj = vobj.Object
        app_doc = self.app_obj.Document
        self.gui_doc = Gui.getDocument(app_doc)

        activeView = get_active_view(self.gui_doc)
        if activeView is not None:
            camera = activeView.getCameraNode()

            self.cameraSensor = coin.SoFieldSensor(self.camera_callback, camera)
            if isinstance(camera, coin.SoPerspectiveCamera):
                self.cameraSensor.attach(camera.focalDistance)
            elif isinstance(camera, coin.SoOrthographicCamera):
                self.cameraSensor.attach(camera.height)

            self.cameraSensorRot = coin.SoFieldSensor(self.camera_callback_rotation, camera)
            self.cameraSensorRot.attach(camera.orientation)

        factor = self.get_lock_factor()
        self.scale = coin.SoScale()
        self.scale.scaleFactor.setValue(factor, factor, factor)

        self.draw_style = coin.SoDrawStyle()
        self.draw_style.lineWidth = 5

        # Create transformation (position and orientation)
        self.transform = coin.SoTransform()
        self.set_lock_position(groundedObj)
        self.set_lock_rotation()

        # Create the 2D components of the lockpad: a square and two arcs
        # Creating a square
        squareCoords = [
            (-5, -4, 0),
            (5, -4, 0),
            (5, 4, 0),
            (-5, 4, 0),
        ]  # Simple square, adjust size as needed
        self.square = coin.SoAnnotation()
        squareVertices = coin.SoCoordinate3()
        squareVertices.point.setValues(0, 4, squareCoords)
        squareFace = coin.SoFaceSet()
        squareFace.numVertices.setValue(4)
        self.square.addChild(squareVertices)
        self.square.addChild(squareFace)

        # Creating the arcs (approximated with line segments)
        self.arc = self.create_arc(0, 4, 3.5, 0, 180)

        self.pick = coin.SoPickStyle()
        self.pick.style.setValue(coin.SoPickStyle.SHAPE_ON_TOP)

        # Assemble the parts into a scenegraph
        self.lockpadSeparator = coin.SoAnnotation()
        self.lockpadSeparator.addChild(self.pick)
        self.lockpadSeparator.addChild(self.transform)
        self.lockpadSeparator.addChild(self.scale)
        self.lockpadSeparator.addChild(self.lockpadColor)
        self.lockpadSeparator.addChild(self.square)
        self.lockpadSeparator.addChild(self.arc)

        # Attach the scenegraph to the view provider
        vobj.addDisplayMode(self.lockpadSeparator, "Wireframe")

    def create_arc(self, centerX, centerY, radius, startAngle, endAngle):
        arc = coin.SoAnnotation()
        coords = coin.SoCoordinate3()
        points = []
        for angle in range(startAngle, endAngle + 1):  # Increment can be adjusted for smoother arcs
            rad = math.radians(angle)
            x = centerX + math.cos(rad) * radius
            y = centerY + math.sin(rad) * radius
            points.append((x, y, 0))
        coords.point.setValues(0, len(points), points)
        line = coin.SoLineSet()
        line.numVertices.setValue(len(points))
        arc.addChild(coords)
        arc.addChild(self.draw_style)
        arc.addChild(line)
        return arc

    def camera_callback(self, *args):
        factor = self.get_lock_factor()
        self.scale.scaleFactor.setValue(factor, factor, factor)

    def camera_callback_rotation(self, *args):
        self.set_lock_rotation()

    def set_lock_rotation(self):
        activeView = get_active_view(self.gui_doc)
        if activeView is not None:
            camera = activeView.getCameraNode()
            rotation = camera.orientation.getValue()

            q = rotation.getValue()
            self.transform.rotation.setValue(q[0], q[1], q[2], q[3])

    def get_lock_factor(self):
        return get_camera_height(self.gui_doc) / 300

    def set_lock_position(self, groundedObj):
        bBox = groundedObj.ViewObject.getBoundingBox()
        if bBox.isValid():
            pos = bBox.Center
        else:
            pos = groundedObj.Placement.Base

        self.transform.translation.setValue(pos.x, pos.y, pos.z)

    def updateData(self, fp, prop):
        """If a property of the handled feature has changed we have the chance to handle this here"""
        # fp is the handled feature, prop is the name of the property that has changed

        if prop == "Placement" and fp.ObjectToGround:
            self.set_lock_position(fp.ObjectToGround)

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

    def getIcon(self):
        return ":/icons/Assembly_ToggleGrounded.svg"

    def dumps(self):
        """When saving the document this object gets stored using Python's json module.\
                Since we have some un-serializable parts here -- the Coin stuff -- we must define this method\
                to return a tuple of all serializable objects or None."""
        return None

    def loads(self, state):
        """When restoring the serialized object from document we have the chance to set some internals here.\
                Since no data were serialized nothing needs to be done here."""
        return None

    def canDelete(self, _obj):
        return True


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

        full_obj_name = ".".join(objs_names)
        full_element_name = full_obj_name + "." + element_name
        selected_object = UtilsAssembly.getObject(full_element_name)

        if not (
            selected_object.isDerivedFrom("Part::Feature")
            or selected_object.isDerivedFrom("App::Part")
        ):
            if selected_object.isDerivedFrom("App::Link"):
                linked = selected_object.getLinkedObject()

                if not (linked.isDerivedFrom("Part::Feature") or linked.isDerivedFrom("App::Part")):
                    return False
            else:
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

        self.doc = self.assembly.Document
        self.gui_doc = Gui.getDocument(self.doc)

        self.view = self.gui_doc.activeView()

        if not self.assembly or not self.view or not self.doc:
            return

        if self.activeType == "Assembly":
            self.assembly.ViewObject.MoveOnlyPreselected = True
            self.assembly.ViewObject.MoveInCommand = False

        self.form = Gui.PySideUic.loadUi(":/panels/TaskAssemblyCreateJoint.ui")

        if self.activeType == "Part":
            self.form.setWindowTitle("Match parts")
            self.form.jointType.hide()

        self.form.jointType.addItems(TranslatedJointTypes)

        self.form.jointType.setCurrentIndex(jointTypeIndex)
        self.jType = JointTypes[self.form.jointType.currentIndex()]
        self.form.jointType.currentIndexChanged.connect(self.onJointTypeChanged)

        self.form.distanceSpinbox.valueChanged.connect(self.onDistanceChanged)
        self.form.distanceSpinbox2.valueChanged.connect(self.onDistance2Changed)
        self.form.offsetSpinbox.valueChanged.connect(self.onOffsetChanged)
        self.form.rotationSpinbox.valueChanged.connect(self.onRotationChanged)
        self.form.PushButtonReverse.clicked.connect(self.onReverseClicked)

        self.form.limitCheckbox1.stateChanged.connect(self.adaptUi)
        self.form.limitCheckbox2.stateChanged.connect(self.adaptUi)
        self.form.limitCheckbox3.stateChanged.connect(self.adaptUi)
        self.form.limitCheckbox4.stateChanged.connect(self.adaptUi)
        self.form.limitLenMinSpinbox.valueChanged.connect(self.onLimitLenMinChanged)
        self.form.limitLenMaxSpinbox.valueChanged.connect(self.onLimitLenMaxChanged)
        self.form.limitRotMinSpinbox.valueChanged.connect(self.onLimitRotMinChanged)
        self.form.limitRotMaxSpinbox.valueChanged.connect(self.onLimitRotMaxChanged)

        self.form.reverseRotCheckbox.setChecked(self.jType == "Gears")
        self.form.reverseRotCheckbox.stateChanged.connect(self.reverseRotToggled)

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

        self.adaptUi()

        if self.creating:
            # This has to be after adaptUi so that properties default values are adapted
            # if needed. For instance for gears adaptUi will prevent radii from being 0
            # before handleInitialSelection tries to solve.
            self.handleInitialSelection()

        self.setJointsPickableState(False)

        Gui.Selection.addSelectionGate(
            MakeJointSelGate(self, self.assembly), Gui.Selection.ResolveMode.NoResolve
        )
        Gui.Selection.addObserver(self, Gui.Selection.ResolveMode.NoResolve)
        Gui.Selection.setSelectionStyle(Gui.Selection.SelectionStyle.GreedySelection)

        self.callbackMove = self.view.addEventCallback("SoLocation2Event", self.moveMouse)
        self.callbackKey = self.view.addEventCallback("SoKeyboardEvent", self.KeyboardEvent)

        self.form.featureList.installEventFilter(self)

        self.addition_rejected = False

    def accept(self):
        if len(self.current_selection) != 2:
            App.Console.PrintWarning(
                translate("Assembly", "You need to select 2 elements from 2 separate parts.")
            )
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

    def autoClosedOnTransactionChange(self):
        self.reject()

    def deactivate(self):
        global activeTask
        activeTask = None

        if self.activeType == "Assembly":
            self.assembly.clearUndo()
            self.assembly.ViewObject.MoveOnlyPreselected = False
            self.assembly.ViewObject.MoveInCommand = True

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
        self.jType = JointTypes[self.form.jointType.currentIndex()]
        self.joint.Proxy.setJointType(self.joint, self.jType)
        self.adaptUi()

    def onDistanceChanged(self, quantity):
        self.joint.Distance = self.form.distanceSpinbox.property("rawValue")

    def onDistance2Changed(self, quantity):
        self.joint.Distance2 = self.form.distanceSpinbox2.property("rawValue")

    def onOffsetChanged(self, quantity):
        self.joint.Offset = App.Vector(0, 0, self.form.offsetSpinbox.property("rawValue"))

    def onRotationChanged(self, quantity):
        self.joint.Rotation = self.form.rotationSpinbox.property("rawValue")

    def onLimitLenMinChanged(self, quantity):
        if self.form.limitCheckbox1.isChecked():
            self.joint.LengthMin = self.form.limitLenMinSpinbox.property("rawValue")

    def onLimitLenMaxChanged(self, quantity):
        if self.form.limitCheckbox2.isChecked():
            self.joint.LengthMax = self.form.limitLenMaxSpinbox.property("rawValue")

    def onLimitRotMinChanged(self, quantity):
        if self.form.limitCheckbox3.isChecked():
            self.joint.AngleMin = self.form.limitRotMinSpinbox.property("rawValue")

    def onLimitRotMaxChanged(self, quantity):
        if self.form.limitCheckbox4.isChecked():
            self.joint.AngleMax = self.form.limitRotMaxSpinbox.property("rawValue")

    def onReverseClicked(self):
        self.joint.Proxy.flipOnePart(self.joint)

    def reverseRotToggled(self, val):
        if val:
            self.form.jointType.setCurrentIndex(8)
        else:
            self.form.jointType.setCurrentIndex(9)

    def adaptUi(self):
        jType = self.jType

        if jType in JointUsingDistance:
            self.form.distanceLabel.show()
            self.form.distanceSpinbox.show()
            if jType == "Distance":
                self.form.distanceLabel.setText(translate("Assembly", "Distance"))
            elif jType == "Angle":
                self.form.distanceLabel.setText(translate("Assembly", "Angle"))
            elif jType == "Gears" or jType == "Belt":
                self.form.distanceLabel.setText(translate("Assembly", "Radius 1"))
            else:
                self.form.distanceLabel.setText(translate("Assembly", "Pitch radius"))

            if jType == "Angle":
                self.form.distanceSpinbox.setProperty("unit", "deg")
            else:
                self.form.distanceSpinbox.setProperty("unit", "mm")

        else:
            self.form.distanceLabel.hide()
            self.form.distanceSpinbox.hide()

        if jType in JointUsingDistance2:
            self.form.distanceLabel2.show()
            self.form.distanceSpinbox2.show()
            self.form.reverseRotCheckbox.show()

        else:
            self.form.distanceLabel2.hide()
            self.form.distanceSpinbox2.hide()
            self.form.reverseRotCheckbox.hide()

        if jType in JointNoNegativeDistance:
            # Setting minimum to 0.01 to prevent 0 and negative values
            self.form.distanceSpinbox.setProperty("minimum", 1e-7)
            if self.form.distanceSpinbox.property("rawValue") == 0.0:
                self.form.distanceSpinbox.setProperty("rawValue", 1.0)

            if jType == "Gears" or jType == "Belt":
                self.form.distanceSpinbox2.setProperty("minimum", 1e-7)
                if self.form.distanceSpinbox2.property("rawValue") == 0.0:
                    self.form.distanceSpinbox2.setProperty("rawValue", 1.0)
        else:
            self.form.distanceSpinbox.setProperty("minimum", float("-inf"))
            self.form.distanceSpinbox2.setProperty("minimum", float("-inf"))

        if jType in JointUsingOffset:
            self.form.offsetLabel.show()
            self.form.offsetSpinbox.show()
        else:
            self.form.offsetLabel.hide()
            self.form.offsetSpinbox.hide()

        if jType in JointUsingRotation:
            self.form.rotationLabel.show()
            self.form.rotationSpinbox.show()
        else:
            self.form.rotationLabel.hide()
            self.form.rotationSpinbox.hide()

        if jType in JointUsingReverse:
            self.form.PushButtonReverse.show()
        else:
            self.form.PushButtonReverse.hide()

        needLengthLimits = jType in JointUsingLimitLength
        needAngleLimits = jType in JointUsingLimitAngle

        if needLengthLimits or needAngleLimits:
            self.form.groupBox_limits.show()

            self.joint.EnableLengthMin = self.form.limitCheckbox1.isChecked()
            self.joint.EnableLengthMax = self.form.limitCheckbox2.isChecked()
            self.joint.EnableAngleMin = self.form.limitCheckbox3.isChecked()
            self.joint.EnableAngleMax = self.form.limitCheckbox4.isChecked()

            if needLengthLimits:
                self.form.limitCheckbox1.show()
                self.form.limitCheckbox2.show()
                self.form.limitLenMinSpinbox.show()
                self.form.limitLenMaxSpinbox.show()
                self.form.limitLenMinSpinbox.setEnabled(self.joint.EnableLengthMin)
                self.form.limitLenMaxSpinbox.setEnabled(self.joint.EnableLengthMax)
                self.onLimitLenMinChanged(0)  # dummy value
                self.onLimitLenMaxChanged(0)
            else:
                self.form.limitCheckbox1.hide()
                self.form.limitCheckbox2.hide()
                self.form.limitLenMinSpinbox.hide()
                self.form.limitLenMaxSpinbox.hide()

            if needAngleLimits:
                self.form.limitCheckbox3.show()
                self.form.limitCheckbox4.show()
                self.form.limitRotMinSpinbox.show()
                self.form.limitRotMaxSpinbox.show()
                self.form.limitRotMinSpinbox.setEnabled(self.joint.EnableAngleMin)
                self.form.limitRotMaxSpinbox.setEnabled(self.joint.EnableAngleMax)
                self.onLimitRotMinChanged(0)
                self.onLimitRotMaxChanged(0)
            else:
                self.form.limitCheckbox3.hide()
                self.form.limitCheckbox4.hide()
                self.form.limitRotMinSpinbox.hide()
                self.form.limitRotMaxSpinbox.hide()

        else:
            self.form.groupBox_limits.hide()

    def updateTaskboxFromJoint(self):
        self.current_selection = []
        self.preselection_dict = None

        obj1 = self.joint.Object1[0]
        part1 = self.joint.Part1
        el1 = self.joint.Object1[1][0]
        vtx1 = self.joint.Object1[1][1]

        obj2 = self.joint.Object2[0]
        part2 = self.joint.Part2
        el2 = self.joint.Object2[1][0]
        vtx2 = self.joint.Object2[1][1]

        selection_dict1 = {
            "object": obj1,
            "part": part1,
            "element_name": el1,
            "vertex_name": vtx1,
        }

        selection_dict2 = {
            "object": obj2,
            "part": part2,
            "element_name": el2,
            "vertex_name": vtx2,
        }

        self.current_selection.append(selection_dict1)
        self.current_selection.append(selection_dict2)

        # Add the elements to the selection. Note we cannot do :
        # Gui.Selection.addSelection(self.doc.Name, obj1.Name, elName)
        # Because obj1 can be external in which case addSelection will fail. And
        # Gui.Selection.addSelection(obj1.Document.Name, obj1.Name, elName)
        # will not select in the assembly doc.
        elName = self.getSubnameForSelection(obj1, part1, el1)
        Gui.Selection.addSelection(self.doc.Name, part1.Name, elName)

        elName = self.getSubnameForSelection(obj2, part2, el2)
        Gui.Selection.addSelection(self.doc.Name, part2.Name, elName)

        self.form.distanceSpinbox.setProperty("rawValue", self.joint.Distance)
        self.form.distanceSpinbox2.setProperty("rawValue", self.joint.Distance2)
        self.form.offsetSpinbox.setProperty("rawValue", self.joint.Offset.z)
        self.form.rotationSpinbox.setProperty("rawValue", self.joint.Rotation)

        self.form.limitCheckbox1.setChecked(self.joint.EnableLengthMin)
        self.form.limitCheckbox2.setChecked(self.joint.EnableLengthMax)
        self.form.limitCheckbox3.setChecked(self.joint.EnableAngleMin)
        self.form.limitCheckbox4.setChecked(self.joint.EnableAngleMax)
        self.form.limitLenMinSpinbox.setProperty("rawValue", self.joint.LengthMin)
        self.form.limitLenMaxSpinbox.setProperty("rawValue", self.joint.LengthMax)
        self.form.limitRotMinSpinbox.setProperty("rawValue", self.joint.AngleMin)
        self.form.limitRotMaxSpinbox.setProperty("rawValue", self.joint.AngleMax)

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

        # Then we pass the new list to the joint object
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

    def updateLimits(self):
        needLengthLimits = self.jType in JointUsingLimitLength
        needAngleLimits = self.jType in JointUsingLimitAngle
        if needLengthLimits:
            distance = UtilsAssembly.getJointDistance(self.joint)
            if not self.form.limitCheckbox1.isChecked():
                self.form.limitLenMinSpinbox.setProperty("rawValue", distance)
            if not self.form.limitCheckbox2.isChecked():
                self.form.limitLenMaxSpinbox.setProperty("rawValue", distance)

        if needAngleLimits:
            angle = UtilsAssembly.getJointXYAngle(self.joint) / math.pi * 180
            if not self.form.limitCheckbox3.isChecked():
                self.form.limitRotMinSpinbox.setProperty("rawValue", angle)
            if not self.form.limitCheckbox4.isChecked():
                self.form.limitRotMaxSpinbox.setProperty("rawValue", angle)

    def moveMouse(self, info):
        if len(self.current_selection) >= 2 or (
            len(self.current_selection) == 1
            and (
                not self.preselection_dict
                or self.current_selection[0]["part"] == self.preselection_dict["part"]
            )
        ):
            self.joint.ViewObject.Proxy.showPreviewJCS(False)
            if len(self.current_selection) >= 2:
                self.updateLimits()
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
        obj = self.preselection_dict["object"]
        part = self.preselection_dict["part"]
        placement = self.joint.Proxy.findPlacement(
            self.joint,
            obj,
            part,
            self.preselection_dict["element_name"],
            self.preselection_dict["vertex_name"],
            isSecond,
        )
        self.joint.ViewObject.Proxy.showPreviewJCS(True, placement, obj, part)
        self.previewJCSVisible = True

    # 3D view keyboard handler
    def KeyboardEvent(self, info):
        if info["State"] == "UP" and info["Key"] == "ESCAPE":
            self.reject()

        if info["State"] == "UP" and info["Key"] == "RETURN":
            self.accept()

    def eventFilter(self, watched, event):
        if self.form is not None and watched == self.form.featureList:
            if event.type() == QtCore.QEvent.ShortcutOverride:
                if event.key() == QtCore.Qt.Key_Delete:
                    event.accept()  # Accept the event only if the key is Delete
                    return True  # Indicate that the event has been handled
                return False

            elif event.type() == QtCore.QEvent.KeyPress:
                if event.key() == QtCore.Qt.Key_Delete:
                    selected_indexes = self.form.featureList.selectedIndexes()

                    for index in selected_indexes:
                        row = index.row()
                        if row < len(self.current_selection):
                            selection_dict = self.current_selection[row]
                            elName = self.getSubnameForSelection(
                                selection_dict["object"],
                                selection_dict["part"],
                                selection_dict["element_name"],
                            )
                            Gui.Selection.removeSelection(selection_dict["object"], elName)

                    return True  # Consume the event

        return super().eventFilter(watched, event)

    def getContainingPart(self, full_element_name, obj):
        return UtilsAssembly.getContainingPart(full_element_name, obj, self.assembly)

    # selectionObserver stuff
    def addSelection(self, doc_name, obj_name, sub_name, mousePos):
        full_obj_name = UtilsAssembly.getFullObjName(obj_name, sub_name)
        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
        selected_object = UtilsAssembly.getObject(full_element_name)
        element_name = UtilsAssembly.getElementName(full_element_name)
        part_containing_selected_object = self.getContainingPart(full_element_name, selected_object)

        # Check if the addition is acceptable (we are not doing this in selection gate to let user move objects)
        acceptable = True
        if len(self.current_selection) >= 2:
            # No more than 2 elements can be selected for basic joints.
            acceptable = False

        for selection_dict in self.current_selection:
            if selection_dict["part"] == part_containing_selected_object:
                # Can't join a solid to itself. So the user need to select 2 different parts.
                acceptable = False

        if not acceptable:
            self.addition_rejected = True
            Gui.Selection.removeSelection(doc_name, obj_name, sub_name)
            return

        # Selection is acceptable so add it
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
        if self.addition_rejected:
            self.addition_rejected = False
            return

        full_element_name = UtilsAssembly.getFullElementName(obj_name, sub_name)
        selected_object = UtilsAssembly.getObject(full_element_name)
        element_name = UtilsAssembly.getElementName(full_element_name)
        part_containing_selected_object = self.getContainingPart(full_element_name, selected_object)

        # Find and remove the corresponding dictionary from the combined list
        for selection_dict in self.current_selection:
            if selection_dict["part"] == part_containing_selected_object:
                self.current_selection.remove(selection_dict)
                break

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
