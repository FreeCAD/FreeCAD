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

from SoSwitchMarker import SoSwitchMarker

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


# The joint object consists of 2 JCS (joint coordinate systems) and a Joint Type.
# A JCS is a placement that is computed (unless it is detached) from references (PropertyXLinkSubHidden) that links to :
# - An object: this can be any Part::Feature solid. Or a PartDesign Body. Or a App::Link to those.
# - An element name: This can be either a face, an edge, a vertex or empty. Empty means that the Object placement will be used
# - A vertex name: For faces and edges, we need to specify which vertex of said face/edge to use
# Both element names hold the full path to the object.
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
        self.migrationScript2(joint)
        self.migrationScript3(joint)
        self.migrationScript4(joint)

        # First Joint Connector
        if not hasattr(joint, "Reference1"):
            joint.addProperty(
                "App::PropertyXLinkSubHidden",
                "Reference1",
                "Joint Connector 1",
                QT_TRANSLATE_NOOP("App::Property", "The first reference of the joint"),
            )

        if not hasattr(joint, "Placement1"):
            joint.addProperty(
                "App::PropertyPlacement",
                "Placement1",
                "Joint Connector 1",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the local coordinate system within Reference1's object that will be used for the joint.",
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

        if not hasattr(joint, "Offset1"):
            joint.addProperty(
                "App::PropertyPlacement",
                "Offset1",
                "Joint Connector 1",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the attachment offset of the first connector of the joint.",
                ),
            )

        # Second Joint Connector
        if not hasattr(joint, "Reference2"):
            joint.addProperty(
                "App::PropertyXLinkSubHidden",
                "Reference2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP("App::Property", "The second reference of the joint"),
            )

        if not hasattr(joint, "Placement2"):
            joint.addProperty(
                "App::PropertyPlacement",
                "Placement2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the local coordinate system within Reference2's object that will be used for the joint.",
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

        if not hasattr(joint, "Offset2"):
            joint.addProperty(
                "App::PropertyPlacement",
                "Offset2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the attachment offset of the second connector of the joint.",
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

    def migrationScript2(self, joint):
        if hasattr(joint, "Object1"):
            joint.addProperty(
                "App::PropertyXLinkSubHidden",
                "Reference1",
                "Joint Connector 1",
                QT_TRANSLATE_NOOP("App::Property", "The first reference of the joint"),
            )

            if joint.Object1 is not None:
                obj = joint.Object1[0]
                part = joint.Part1
                elt = joint.Object1[1][0]
                vtx = joint.Object1[1][1]

                # now we need to get the 'selection-root-obj' and the global path
                rootObj, path = UtilsAssembly.getRootPath(obj, part)
                obj = rootObj
                elt = path + elt
                vtx = path + vtx

                joint.Reference1 = [obj, [elt, vtx]]

            joint.removeProperty("Object1")
            joint.removeProperty("Part1")

        if hasattr(joint, "Object2"):
            joint.addProperty(
                "App::PropertyXLinkSubHidden",
                "Reference2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP("App::Property", "The second reference of the joint"),
            )

            if joint.Object2 is not None:
                obj = joint.Object2[0]
                part = joint.Part2
                elt = joint.Object2[1][0]
                vtx = joint.Object2[1][1]

                rootObj, path = UtilsAssembly.getRootPath(obj, part)
                obj = rootObj
                elt = path + elt
                vtx = path + vtx

                joint.Reference2 = [obj, [elt, vtx]]

            joint.removeProperty("Object2")
            joint.removeProperty("Part2")

    def migrationScript3(self, joint):
        if hasattr(joint, "Offset"):
            current_offset = joint.Offset  # App.Vector
            current_rotation = joint.Rotation  # float

            joint.removeProperty("Offset")
            joint.removeProperty("Rotation")

            joint.addProperty(
                "App::PropertyPlacement",
                "Offset1",
                "Joint Connector 1",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the attachment offset of the first connector of the joint.",
                ),
            )

            joint.addProperty(
                "App::PropertyPlacement",
                "Offset2",
                "Joint Connector 2",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "This is the attachment offset of the second connector of the joint.",
                ),
            )

            joint.Offset2 = App.Placement(current_offset, App.Rotation(current_rotation, 0, 0))

    def migrationScript4(self, joint):
        if hasattr(joint, "Reference1") and joint.Reference1[0] is not None:
            doc_name = joint.Reference1[0].Document.Name
            sub1 = joint.Reference1[1][0]
            sub1 = UtilsAssembly.fixBodyExtraFeatureInSub(doc_name, sub1)
            sub2 = joint.Reference1[1][1]
            sub2 = UtilsAssembly.fixBodyExtraFeatureInSub(doc_name, sub2)

            if sub1 != joint.Reference1[1][0] or sub2 != joint.Reference1[1][1]:
                joint.Reference1 = (joint.Reference1[0], [sub1, sub2])

        if hasattr(joint, "Reference2") and joint.Reference2[0] is not None:
            doc_name = joint.Reference2[0].Document.Name
            sub1 = joint.Reference2[1][0]
            sub1 = UtilsAssembly.fixBodyExtraFeatureInSub(doc_name, sub1)
            sub2 = joint.Reference2[1][1]
            sub2 = UtilsAssembly.fixBodyExtraFeatureInSub(doc_name, sub2)

            if sub1 != joint.Reference2[1][0] or sub2 != joint.Reference2[1][1]:
                joint.Reference2 = (joint.Reference2[0], [sub1, sub2])

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def getAssembly(self, joint):
        for obj in joint.InList:
            if obj.isDerivedFrom("Assembly::AssemblyObject"):
                return obj
        return None

    def setJointType(self, joint, newType):
        oldType = joint.JointType
        if newType != oldType:
            joint.JointType = newType

            # try to replace the joint type in the label.
            tr_old_type = TranslatedJointTypes[JointTypes.index(oldType)]
            tr_new_type = TranslatedJointTypes[JointTypes.index(newType)]
            if tr_old_type in joint.Label:
                joint.Label = joint.Label.replace(tr_old_type, tr_new_type)

    def onChanged(self, joint, prop):
        """Do something when a property has changed"""
        # App.Console.PrintMessage("Change property: " + str(prop) + "\n")

        # during loading the onchanged may be triggered before full init.
        if App.isRestoring():
            return

        if prop == "Offset1" or prop == "Offset2":
            if joint.Reference1 is None or joint.Reference2 is None:
                return

            self.updateJCSPlacements(joint)

            presolved = self.preSolve(joint, False)

            isAssembly = self.getAssembly(joint).Type == "Assembly"
            if isAssembly and not presolved:
                solveIfAllowed(self.getAssembly(joint))
            else:
                self.updateJCSPlacements(joint)

        if prop == "Distance" and (joint.JointType == "Distance" or joint.JointType == "Angle"):
            if joint.Reference1 is None or joint.Reference2 is None:
                return

            if joint.JointType == "Angle" and joint.Distance != 0.0:
                self.preventParallel(joint)
            solveIfAllowed(self.getAssembly(joint))

    def execute(self, fp):
        """Do something when doing a recomputation, this method is mandatory"""
        # App.Console.PrintMessage("Recompute Python Box feature\n")
        pass

    def setJointConnectors(self, joint, refs):
        # current selection is a vector of strings like "Assembly.Assembly1.Assembly2.Body.Pad.Edge16" including both what selection return as obj_name and obj_sub
        assembly = self.getAssembly(joint)
        isAssembly = assembly.Type == "Assembly"

        if len(refs) >= 1:
            joint.Reference1 = refs[0]
            joint.Placement1 = self.findPlacement(joint, joint.Reference1, 0)
        else:
            joint.Reference1 = None
            joint.Placement1 = App.Placement()
            self.partMovedByPresolved = None

        if len(refs) >= 2:
            joint.Reference2 = refs[1]
            joint.Placement2 = self.findPlacement(joint, joint.Reference2, 1)
            if joint.JointType in JointUsingPreSolve:
                self.preSolve(joint)
            elif joint.JointType in JointParallelForbidden:
                self.preventParallel(joint)

            if isAssembly:
                solveIfAllowed(assembly, True)
            else:
                self.updateJCSPlacements(joint)

        else:
            joint.Reference2 = None
            joint.Placement2 = App.Placement()
            if isAssembly:
                assembly.undoSolve()
            self.undoPreSolve(joint)

    def updateJCSPlacements(self, joint):
        if not joint.Detach1:
            joint.Placement1 = self.findPlacement(joint, joint.Reference1, 0)

        if not joint.Detach2:
            joint.Placement2 = self.findPlacement(joint, joint.Reference2, 1)

    """
    So here we want to find a placement that corresponds to a local coordinate system that would be placed at the selected vertex.
    - obj is usually a App::Link to a PartDesign::Body, or primitive, fasteners. But can also be directly the object.1
    - elt can be a face, an edge or a vertex.
    - If elt is a vertex, then vtx = elt And placement is vtx coordinates without rotation.
    - if elt is an edge, then vtx = edge start/end vertex depending on which is closer. If elt is an arc or circle, vtx can also be the center. The rotation is the plane normal to the line positioned at vtx. Or for arcs/circle, the plane of the arc.
    - if elt is a plane face, vtx is the face vertex (to the list of vertex we need to add arc/circle centers) the closer to the mouse. The placement is the plane rotation positioned at vtx
    - if elt is a cylindrical face, vtx can also be the center of the arcs of the cylindrical face.
    """

    def findPlacement(self, joint, ref, index=0):
        ignoreVertex = joint.JointType == "Distance"
        plc = UtilsAssembly.findPlacement(ref, ignoreVertex)

        # We apply the attachment offsets.
        if index == 0:
            plc = plc * joint.Offset1
        else:
            plc = plc * joint.Offset2

        return plc

    def flipOnePart(self, joint):
        assembly = self.getAssembly(joint)

        part2 = UtilsAssembly.getMovingPart(assembly, joint.Reference2)
        if part2 is not None:
            part2ConnectedByJoint = assembly.isJointConnectingPartToGround(joint, "Reference2")
            part2Grounded = assembly.isPartGrounded(part2)
            if part2ConnectedByJoint and not part2Grounded:
                jcsPlc = UtilsAssembly.getJcsPlcRelativeToPart(
                    assembly, joint.Placement2, joint.Reference2
                )
                globalJcsPlc = UtilsAssembly.getJcsGlobalPlc(joint.Placement2, joint.Reference2)
                jcsPlc = UtilsAssembly.flipPlacement(jcsPlc)
                part2.Placement = globalJcsPlc * jcsPlc.inverse()
                solveIfAllowed(self.getAssembly(joint))
                return

        part1 = UtilsAssembly.getMovingPart(assembly, joint.Reference1)
        if part1 is not None:
            part1Grounded = assembly.isPartGrounded(part1)
            if not part1Grounded:
                jcsPlc = UtilsAssembly.getJcsPlcRelativeToPart(
                    assembly, joint.Placement1, joint.Reference1
                )
                globalJcsPlc = UtilsAssembly.getJcsGlobalPlc(joint.Placement1, joint.Reference1)
                jcsPlc = UtilsAssembly.flipPlacement(jcsPlc)
                part1.Placement = globalJcsPlc * jcsPlc.inverse()
                return

    def preSolve(self, joint, savePlc=True):
        # The goal of this is to put the part in the correct position to avoid wrong placement by the solve.

        # we actually don't want to match perfectly the JCS, it is best to match them
        # in the current closest direction, ie either matched or flipped.

        sameDir = self.areJcsSameDir(joint)
        assembly = self.getAssembly(joint)

        part1 = UtilsAssembly.getMovingPart(assembly, joint.Reference1)
        part2 = UtilsAssembly.getMovingPart(assembly, joint.Reference2)

        isAssembly = assembly.Type == "Assembly"
        if isAssembly:
            joint.Activated = False
            part1Connected = assembly.isPartConnected(part1)
            part2Connected = assembly.isPartConnected(part2)
            joint.Activated = True
        else:
            part1Connected = False
            part2Connected = True

        if not part2Connected:
            if savePlc:
                self.partMovedByPresolved = part2
                self.presolveBackupPlc = part2.Placement

            globalJcsPlc1 = UtilsAssembly.getJcsGlobalPlc(joint.Placement1, joint.Reference1)
            jcsPlc2 = UtilsAssembly.getJcsPlcRelativeToPart(
                assembly, joint.Placement2, joint.Reference2
            )
            if not sameDir:
                jcsPlc2 = UtilsAssembly.flipPlacement(jcsPlc2)

            # For link groups and sub-assemblies we have to take into account
            # the parent placement (ie the linkgroup plc) as the linkgroup is not the moving part
            # But instead of doing as follow, we rather enforce identity placement for linkgroups.
            # parentPlc = UtilsAssembly.getParentPlacementIfNeeded(part2)
            # part2.Placement = globalJcsPlc1 * jcsPlc2.inverse() * parentPlc.inverse()

            part2.Placement = globalJcsPlc1 * jcsPlc2.inverse()
            return True

        elif not part1Connected:
            if savePlc:
                self.partMovedByPresolved = part1
                self.presolveBackupPlc = part1.Placement

            globalJcsPlc2 = UtilsAssembly.getJcsGlobalPlc(joint.Placement2, joint.Reference2)
            jcsPlc1 = UtilsAssembly.getJcsPlcRelativeToPart(
                assembly, joint.Placement1, joint.Reference1
            )
            if not sameDir:
                jcsPlc1 = UtilsAssembly.flipPlacement(jcsPlc1)

            part1.Placement = globalJcsPlc2 * jcsPlc1.inverse()
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

        part1 = UtilsAssembly.getMovingPart(assembly, joint.Reference1)
        part2 = UtilsAssembly.getMovingPart(assembly, joint.Reference2)

        isAssembly = assembly.Type == "Assembly"
        if isAssembly:
            part1ConnectedByJoint = assembly.isJointConnectingPartToGround(joint, "Reference1")
            part2ConnectedByJoint = assembly.isJointConnectingPartToGround(joint, "Reference2")
        else:
            part1ConnectedByJoint = False
            part2ConnectedByJoint = True

        if part2ConnectedByJoint:
            self.partMovedByPresolved = part2
            self.presolveBackupPlc = part2.Placement

            part2.Placement = UtilsAssembly.applyRotationToPlacementAlongAxis(
                part2.Placement, 10, App.Vector(1, 0, 0)
            )

        elif part1ConnectedByJoint:
            self.partMovedByPresolved = part1
            self.presolveBackupPlc = part1.Placement

            part1.Placement = UtilsAssembly.applyRotationToPlacementAlongAxis(
                part1.Placement, 10, App.Vector(1, 0, 0)
            )

    def areJcsSameDir(self, joint):
        globalJcsPlc1 = UtilsAssembly.getJcsGlobalPlc(joint.Placement1, joint.Reference1)
        globalJcsPlc2 = UtilsAssembly.getJcsGlobalPlc(joint.Placement2, joint.Reference2)

        return UtilsAssembly.arePlacementSameDir(globalJcsPlc1, globalJcsPlc2)

    def areJcsZParallel(self, joint):
        globalJcsPlc1 = UtilsAssembly.getJcsGlobalPlc(joint.Placement1, joint.Reference1)
        globalJcsPlc2 = UtilsAssembly.getJcsGlobalPlc(joint.Placement2, joint.Reference2)

        return UtilsAssembly.arePlacementZParallel(globalJcsPlc1, globalJcsPlc2)


class ViewProviderJoint:
    def __init__(self, vobj):
        """Set this object to the proxy object of the actual view provider"""

        vobj.Proxy = self

    def attach(self, vobj):
        """Setup the scene sub-graph of the view provider, this method is mandatory"""
        self.app_obj = vobj.Object

        self.switch_JCS1 = SoSwitchMarker(vobj)
        self.switch_JCS2 = SoSwitchMarker(vobj)
        self.switch_JCS_preview = SoSwitchMarker(vobj)

        self.display_mode = coin.SoType.fromName("SoFCSelection").createInstance()
        self.display_mode.addChild(self.switch_JCS1)
        self.display_mode.addChild(self.switch_JCS2)
        self.display_mode.addChild(self.switch_JCS_preview)
        vobj.addDisplayMode(self.display_mode, "Wireframe")

    def updateData(self, joint, prop):
        """If a property of the handled feature has changed we have the chance to handle this here"""
        # joint is the handled feature, prop is the name of the property that has changed
        if prop == "Placement1":
            if hasattr(joint, "Reference1") and joint.Reference1:
                plc = joint.Placement1
                self.switch_JCS1.whichChild = coin.SO_SWITCH_ALL

                self.switch_JCS1.set_marker_placement(plc, joint.Reference1)
            else:
                self.switch_JCS1.whichChild = coin.SO_SWITCH_NONE

        if prop == "Placement2":
            if hasattr(joint, "Reference2") and joint.Reference2:
                plc = joint.Placement2
                self.switch_JCS2.whichChild = coin.SO_SWITCH_ALL

                self.switch_JCS2.set_marker_placement(plc, joint.Reference2)
            else:
                self.switch_JCS2.whichChild = coin.SO_SWITCH_NONE

    def showPreviewJCS(self, visible, placement=None, ref=None):
        if visible:
            self.switch_JCS_preview.whichChild = coin.SO_SWITCH_ALL
            self.switch_JCS_preview.set_marker_placement(placement, ref)
        else:
            self.switch_JCS_preview.whichChild = coin.SO_SWITCH_NONE

    def setPickableState(self, state: bool):
        """Set JCS selectable or unselectable in 3D view"""
        self.switch_JCS1.setPickableState(state)
        self.switch_JCS2.setPickableState(state)
        self.switch_JCS_preview.setPickableState(state)

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
        if prop == "color_X_axis" or prop == "color_Y_axis" or prop == "color_Z_axis":
            self.switch_JCS1.onChanged(vp, prop)
            self.switch_JCS2.onChanged(vp, prop)
            self.switch_JCS_preview.onChanged(vp, prop)

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
        dialog = Gui.Control.showDialog(panel)
        if dialog is not None:
            dialog.setAutoCloseOnTransactionChange(True)
            dialog.setDocumentName(App.ActiveDocument.Name)

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

        self.scaleFactor = 1.5

        lockpadColorInt = Preferences.preferences().GetUnsigned("AssemblyConstraints", 0xCC333300)
        self.lockpadColor = coin.SoBaseColor()
        self.lockpadColor.rgb.setValue(UtilsAssembly.color_from_unsigned(lockpadColorInt))

        self.app_obj = vobj.Object
        app_doc = self.app_obj.Document
        self.gui_doc = Gui.getDocument(app_doc)

        # Create transformation (position and orientation)
        self.transform = coin.SoTransform()
        self.set_lock_position(groundedObj)

        # Create the 2D components of the lockpad: a square and two arcs
        self.square = self.create_square()

        # Creating the arcs (approximated with line segments)
        self.arc = self.create_arc(0, 4, 4, 0, 180)

        self.pick = coin.SoPickStyle()
        self.pick.style.setValue(coin.SoPickStyle.SHAPE_ON_TOP)

        # Assemble the parts into a scenegraph
        self.lockpadSeparator = coin.SoSeparator()
        self.lockpadSeparator.addChild(self.lockpadColor)
        self.lockpadSeparator.addChild(self.square)
        self.lockpadSeparator.addChild(self.arc)

        # Use SoVRMLBillboard to make sure the lockpad always faces the camera
        self.billboard = coin.SoVRMLBillboard()
        self.billboard.addChild(self.lockpadSeparator)

        self.scale = coin.SoType.fromName("SoShapeScale").createInstance()
        self.scale.setPart("shape", self.billboard)
        self.scale.scaleFactor = self.scaleFactor

        self.transformSeparator = coin.SoSeparator()
        self.transformSeparator.addChild(self.transform)
        self.transformSeparator.addChild(self.pick)
        self.transformSeparator.addChild(self.scale)

        # Attach the scenegraph to the view provider
        vobj.addDisplayMode(self.transformSeparator, "Wireframe")

    def create_square(self):
        coords = [
            (-5, -4, 0),
            (5, -4, 0),
            (5, 4, 0),
            (-5, 4, 0),
        ]
        vertices = coin.SoCoordinate3()
        vertices.point.setValues(0, 4, coords)

        squareFace = coin.SoFaceSet()
        squareFace.numVertices.setValue(4)

        square = coin.SoAnnotation()
        square.addChild(vertices)
        square.addChild(squareFace)

        return square

    def create_arc(self, centerX, centerY, radius, startAngle, endAngle):
        coords = []
        for angle in range(
            startAngle, endAngle + 1, 5
        ):  # Increment can be adjusted for smoother arcs
            rad = math.radians(angle)
            x = centerX + math.cos(rad) * radius
            y = centerY + math.sin(rad) * radius
            coords.append((x, y, 0))

        radius = radius * 0.7
        for angle in range(endAngle + 1, startAngle - 1, -5):  # Step backward
            rad = math.radians(angle)
            x = centerX + math.cos(rad) * radius
            y = centerY + math.sin(rad) * radius
            coords.append((x, y, 0))

        vertices = coin.SoCoordinate3()
        vertices.point.setValues(0, len(coords), coords)

        shapeHints = coin.SoShapeHints()
        shapeHints.faceType = coin.SoShapeHints.UNKNOWN_FACE_TYPE

        line = coin.SoFaceSet()
        line.numVertices.setValue(len(coords))

        arc = coin.SoAnnotation()
        arc.addChild(shapeHints)
        arc.addChild(vertices)
        arc.addChild(line)

        return arc

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

        ref = [obj, [sub]]
        selected_object = UtilsAssembly.getObject(ref)

        if not (
            selected_object.isDerivedFrom("Part::Feature")
            or selected_object.isDerivedFrom("App::Part")
        ):
            if UtilsAssembly.isLink(selected_object):
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
        self.blockOffsetRotation = False

        self.assembly = UtilsAssembly.activeAssembly()
        if not self.assembly:
            self.assembly = UtilsAssembly.activePart()
            self.activeType = "Part"
        else:
            self.activeType = "Assembly"
            self.assembly.ensureIdentityPlacements()

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
        self.form.offset1Button.clicked.connect(self.onOffset1Clicked)
        self.form.offset2Button.clicked.connect(self.onOffset2Clicked)

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

        self.form.offsetTabs.currentChanged.connect(self.on_offset_tab_changed)

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

            self.refs = []
            self.presel_ref = None

            self.createJointObject()
            self.visibilityBackup = False

        self.adaptUi()

        if self.creating:
            # This has to be after adaptUi so that properties default values are adapted
            # if needed. For instance for gears adaptUi will prevent radii from being 0
            # before handleInitialSelection tries to solve.
            self.handleInitialSelection()

        UtilsAssembly.setJointsPickableState(self.doc, False)

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
        if len(self.refs) != 2:
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

        cmds = UtilsAssembly.generatePropertySettings("obj", self.joint)
        Gui.doCommand(cmds)

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
        UtilsAssembly.setJointsPickableState(self.doc, True)
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
                # We add sub_name twice because the joints references have element name + vertex name
                # and in the case of initial selection, both are the same.
                ref = [sel.Object, [sub_name, sub_name]]
                moving_part = self.getMovingPart(ref)

                # Only objects within the assembly.
                if moving_part is None:
                    Gui.Selection.removeSelection(sel.Object, sub_name)
                    continue

                if len(self.refs) == 1 and moving_part == self.getMovingPart(self.refs[0]):
                    # do not select several feature of the same object.
                    self.refs.clear()
                    Gui.Selection.clearSelection()
                    return

                self.refs.append(ref)

        # do not accept initial selection if we don't have 2 selected features
        if len(self.refs) != 2:
            self.refs.clear()
            Gui.Selection.clearSelection()
        else:
            self.updateJoint()

    def createJointObject(self):
        type_index = self.form.jointType.currentIndex()

        if self.activeType == "Part":
            self.joint = self.assembly.newObject("App::FeaturePython", "Temporary joint")
        else:
            joint_group = UtilsAssembly.getJointGroup(self.assembly)
            self.joint = joint_group.newObject("App::FeaturePython", "Joint")
            self.joint.Label = self.jointName

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
        if self.blockOffsetRotation:
            return

        self.joint.Offset2.Base = App.Vector(0, 0, self.form.offsetSpinbox.property("rawValue"))

    def onRotationChanged(self, quantity):
        if self.blockOffsetRotation:
            return

        yaw = self.form.rotationSpinbox.property("rawValue")
        ypr = self.joint.Offset2.Rotation.getYawPitchRoll()
        self.joint.Offset2.Rotation.setYawPitchRoll(yaw, ypr[1], ypr[2])

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
            self.form.jointType.setCurrentIndex(JointTypes.index("Gears"))
        else:
            self.form.jointType.setCurrentIndex(JointTypes.index("Belt"))

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

        self.updateOffsetWidgets()

    def updateOffsetWidgets(self):
        # Makes sure the values in both the simplified and advanced tabs are sync.
        pos = self.joint.Offset1.Base
        self.form.offset1Button.setText(f"({pos.x}, {pos.y}, {pos.z})")

        pos = self.joint.Offset2.Base
        self.form.offset2Button.setText(f"({pos.x}, {pos.y}, {pos.z})")

        self.blockOffsetRotation = True
        self.form.offsetSpinbox.setProperty("rawValue", pos.z)
        self.form.rotationSpinbox.setProperty(
            "rawValue", self.joint.Offset2.Rotation.getYawPitchRoll()[0]
        )
        self.blockOffsetRotation = False

    def on_offset_tab_changed(self):
        self.updateOffsetWidgets()

    def onOffset1Clicked(self):
        UtilsAssembly.openEditingPlacementDialog(self.joint, "Offset1")
        self.updateOffsetWidgets()

    def onOffset2Clicked(self):
        UtilsAssembly.openEditingPlacementDialog(self.joint, "Offset2")
        self.updateOffsetWidgets()

    def updateTaskboxFromJoint(self):
        self.refs = []
        self.presel_ref = None

        ref1 = self.joint.Reference1
        ref2 = self.joint.Reference2

        self.refs.append(ref1)
        self.refs.append(ref2)

        Gui.Selection.addSelection(ref1[0].Document.Name, ref1[0].Name, ref1[1][0])
        Gui.Selection.addSelection(ref2[0].Document.Name, ref2[0].Name, ref2[1][0])

        self.form.distanceSpinbox.setProperty("rawValue", self.joint.Distance)
        self.form.distanceSpinbox2.setProperty("rawValue", self.joint.Distance2)
        self.form.offsetSpinbox.setProperty("rawValue", self.joint.Offset2.Base.z)
        self.form.rotationSpinbox.setProperty(
            "rawValue", self.joint.Offset2.Rotation.getYawPitchRoll()[0]
        )

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

    def updateJoint(self):
        # First we build the listwidget
        self.updateJointList()

        # Then we pass the new list to the joint object
        self.joint.Proxy.setJointConnectors(self.joint, self.refs)

    def updateJointList(self):
        self.form.featureList.clear()
        simplified_names = []
        for ref in self.refs:

            sname = UtilsAssembly.getObject(ref).Label

            element_name = UtilsAssembly.getElementName(ref[1][0])
            if element_name != "":
                sname = sname + "." + element_name
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
        if len(self.refs) >= 2 or (
            len(self.refs) == 1
            and (
                not self.presel_ref
                or self.getMovingPart(self.refs[0]) == self.getMovingPart(self.presel_ref)
            )
        ):
            self.joint.ViewObject.Proxy.showPreviewJCS(False)
            if len(self.refs) >= 2:
                self.updateLimits()
            return

        cursor_pos = self.view.getCursorPos()
        cursor_info = self.view.getObjectInfo(cursor_pos)
        # cursor_info example  {'x': 41.515, 'y': 7.449, 'z': 16.861, 'ParentObject': <Part object>, 'SubName': 'Body002.Pad.Face5', 'Document': 'part3', 'Object': 'Pad', 'Component': 'Face5'}

        if (
            not cursor_info
            or not self.presel_ref
            # or cursor_info["SubName"] != self.presel_ref["sub_name"]
            # Removed because they are not equal when hovering a line endpoints.
            # But we don't actually need to test because if there's no preselection then not cursor is None
        ):
            self.joint.ViewObject.Proxy.showPreviewJCS(False)
            return

        ref = self.presel_ref

        # newPos = self.view.getPoint(*info["Position"]) is not OK: it's not pos on the object but on the focal plane
        newPos = App.Vector(cursor_info["x"], cursor_info["y"], cursor_info["z"])
        vertex_name = UtilsAssembly.findElementClosestVertex(ref, newPos)

        ref = UtilsAssembly.addVertexToReference(ref, vertex_name)

        placement = self.joint.Proxy.findPlacement(self.joint, ref, 0)
        self.joint.ViewObject.Proxy.showPreviewJCS(True, placement, ref)
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
                        if row < len(self.refs):
                            ref = self.refs[row]

                            Gui.Selection.removeSelection(ref[0], ref[1][0])

                    return True  # Consume the event

        return super().eventFilter(watched, event)

    def getMovingPart(self, ref):
        return UtilsAssembly.getMovingPart(self.assembly, ref)

    # selectionObserver stuff
    def addSelection(self, doc_name, obj_name, sub_name, mousePos):
        rootObj = App.getDocument(doc_name).getObject(obj_name)

        # We do not need the full TNP string like :"Part.Body.Pad.;#a:1;:G0;XTR;:Hc94:8,F.Face6"
        # instead we need : "Part.Body.Pad.Face6"
        resolved = rootObj.resolveSubElement(sub_name, True)
        sub_name = resolved[2]

        sub_name = UtilsAssembly.fixBodyExtraFeatureInSub(doc_name, sub_name)

        ref = [rootObj, [sub_name]]
        moving_part = self.getMovingPart(ref)

        # Check if the addition is acceptable (we are not doing this in selection gate to let user move objects)
        acceptable = True
        if len(self.refs) >= 2:
            # No more than 2 elements can be selected for basic joints.
            acceptable = False

        for reference in self.refs:
            sel_moving_part = self.getMovingPart(reference)
            if sel_moving_part == moving_part:
                # Can't join a solid to itself. So the user need to select 2 different parts.
                acceptable = False

        if not acceptable:
            self.addition_rejected = True
            Gui.Selection.removeSelection(doc_name, obj_name, sub_name)
            return

        # Selection is acceptable so add it

        mousePos = App.Vector(mousePos[0], mousePos[1], mousePos[2])
        vertex_name = UtilsAssembly.findElementClosestVertex(ref, mousePos)

        # add the vertex name to the reference
        ref = UtilsAssembly.addVertexToReference(ref, vertex_name)

        self.refs.append(ref)
        self.updateJoint()

        # We hide the preview JCS if we just added to the selection
        self.joint.ViewObject.Proxy.showPreviewJCS(False)

    def removeSelection(self, doc_name, obj_name, sub_name, mousePos=None):
        if self.addition_rejected:
            self.addition_rejected = False
            return

        ref = [App.getDocument(doc_name).getObject(obj_name), [sub_name]]
        moving_part = self.getMovingPart(ref)

        # Find and remove the corresponding dictionary from the combined list
        for reference in self.refs:
            sel_moving_part = self.getMovingPart(reference)
            if sel_moving_part == moving_part:
                self.refs.remove(reference)
                break

        self.updateJoint()

    def setPreselection(self, doc_name, obj_name, sub_name):
        if not sub_name:
            self.presel_ref = None
            return

        self.presel_ref = [App.getDocument(doc_name).getObject(obj_name), [sub_name]]

    def clearSelection(self, doc_name):
        self.refs.clear()
        self.updateJoint()
