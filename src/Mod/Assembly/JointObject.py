# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
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
# ***************************************************************************/

import math

import FreeCAD as App
import Part

from PySide.QtCore import QT_TRANSLATE_NOOP

if App.GuiUp:
    import FreeCADGui as Gui

# translate = App.Qt.translate

__title__ = "Assembly Joint object"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"

from pivy import coin
import UtilsAssembly

JointTypes = [
    QT_TRANSLATE_NOOP("AssemblyJoint", "Fixed"),
    QT_TRANSLATE_NOOP("AssemblyJoint", "Revolute"),
    QT_TRANSLATE_NOOP("AssemblyJoint", "Cylindrical"),
    QT_TRANSLATE_NOOP("AssemblyJoint", "Slider"),
    QT_TRANSLATE_NOOP("AssemblyJoint", "Ball"),
    QT_TRANSLATE_NOOP("AssemblyJoint", "Planar"),
    QT_TRANSLATE_NOOP("AssemblyJoint", "Parallel"),
    QT_TRANSLATE_NOOP("AssemblyJoint", "Tangent"),
]


class Joint:
    def __init__(self, joint, type_index):
        joint.Proxy = self
        self.joint = joint

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
            "App::PropertyLink",
            "Object1",
            "Joint Connector 1",
            QT_TRANSLATE_NOOP("App::Property", "The first object of the joint"),
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
                "This is the local coordinate system within the object1 that will be used to joint.",
            ),
        )

        # Second Joint Connector
        joint.addProperty(
            "App::PropertyLink",
            "Object2",
            "Joint Connector 2",
            QT_TRANSLATE_NOOP("App::Property", "The second object of the joint"),
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
                "This is the local coordinate system within the object2 that will be used to joint.",
            ),
        )

        self.setJointConnectors([])

    def onChanged(self, fp, prop):
        """Do something when a property has changed"""
        # App.Console.PrintMessage("Change property: " + str(prop) + "\n")
        pass

    def execute(self, fp):
        """Do something when doing a recomputation, this method is mandatory"""
        # App.Console.PrintMessage("Recompute Python Box feature\n")
        pass

    def setJointConnectors(self, current_selection):
        # current selection is a vector of strings like "Assembly.Assembly1.Assembly2.Body.Pad.Edge16" including both what selection return as obj_name and obj_sub

        if len(current_selection) >= 1:
            self.joint.Object1 = current_selection[0]["object"]
            self.joint.Element1 = current_selection[0]["element_name"]
            self.joint.Vertex1 = current_selection[0]["vertex_name"]
            self.joint.Placement1 = self.findPlacement(
                self.joint.Object1, self.joint.Element1, self.joint.Vertex1
            )
        else:
            self.joint.Object1 = None
            self.joint.Element1 = ""
            self.joint.Vertex1 = ""
            self.joint.Placement1 = UtilsAssembly.activeAssembly().Placement

        if len(current_selection) >= 2:
            self.joint.Object2 = current_selection[1]["object"]
            self.joint.Element2 = current_selection[1]["element_name"]
            self.joint.Vertex2 = current_selection[1]["vertex_name"]
            self.joint.Placement2 = self.findPlacement(
                self.joint.Object2, self.joint.Element2, self.joint.Vertex2
            )
        else:
            self.joint.Object2 = None
            self.joint.Element2 = ""
            self.joint.Vertex2 = ""
            self.joint.Placement2 = UtilsAssembly.activeAssembly().Placement

    """
    So here we want to find a placement that corresponds to a local coordinate system that would be placed at the selected vertex.
    - obj is usually a App::Link to a PartDesign::Body, or primitive, fasteners. But can also be directly the object.1
    - elt can be a face, an edge or a vertex.
    - If elt is a vertex, then vtx = elt And placement is vtx coordinates without rotation.
    - if elt is an edge, then vtx = edge start/end vertex depending on which is closer. If elt is an arc or circle, vtx can also be the center. The rotation is the plane normal to the line positioned at vtx. Or for arcs/circle, the plane of the arc.
    - if elt is a plane face, vtx is the face vertex (to the list of vertex we need to add arc/circle centers) the closer to the mouse. The placement is the plane rotation positioned at vtx
    - if elt is a cylindrical face, vtx can also be the center of the arcs of the cylindrical face.
    """

    def findPlacement(self, obj, elt, vtx):
        plc = App.Placement(obj.Placement)
        elt_type, elt_index = UtilsAssembly.extract_type_and_number(elt)
        vtx_type, vtx_index = UtilsAssembly.extract_type_and_number(vtx)

        if elt_type == "Vertex":
            vertex = obj.Shape.Vertexes[elt_index - 1]
            plc.Base = (vertex.X, vertex.Y, vertex.Z)
        elif elt_type == "Edge":
            edge = obj.Shape.Edges[elt_index - 1]
            curve = edge.Curve

            # First we find the translation
            if vtx_type == "Edge":
                # In this case the edge is a circle/arc and the wanted vertex is its center.
                if curve.TypeId == "Part::GeomCircle":
                    center_point = curve.Location
                    plc.Base = (center_point.x, center_point.y, center_point.z)
            else:
                vertex = obj.Shape.Vertexes[vtx_index - 1]
                plc.Base = (vertex.X, vertex.Y, vertex.Z)

            # Then we find the Rotation
            if curve.TypeId == "Part::GeomCircle":
                plc.Rotation = App.Rotation(curve.Rotation)

            if curve.TypeId == "Part::GeomLine":
                plane_normal = curve.Direction
                plane_origin = App.Vector(0, 0, 0)
                plane = Part.Plane(plane_origin, plane_normal)
                plc.Rotation = App.Rotation(plane.Rotation)

        elif elt_type == "Face":
            face = obj.Shape.Faces[elt_index - 1]

            # First we find the translation
            if vtx_type == "Edge":
                # In this case the edge is a circle/arc and the wanted vertex is its center.
                circleOrArc = face.Edges[vtx_index - 1]
                curve = circleOrArc.Curve
                if curve.TypeId == "Part::GeomCircle":
                    center_point = curve.Location
                    plc.Base = (center_point.x, center_point.y, center_point.z)

            else:
                vertex = obj.Shape.Vertexes[vtx_index - 1]
                plc.Base = (vertex.X, vertex.Y, vertex.Z)

            # Then we find the Rotation
            surface = face.Surface
            if surface.TypeId == "Part::GeomPlane":
                plc.Rotation = App.Rotation(surface.Rotation)

        return plc


class ViewProviderJoint:
    def __init__(self, obj, app_obj):
        """Set this object to the proxy object of the actual view provider"""
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
        self.cameraSensor.attach(camera.height)

        self.app_obj = app_obj
        obj.Proxy = self

    def attach(self, obj):
        """Setup the scene sub-graph of the view provider, this method is mandatory"""
        self.transform1 = coin.SoTransform()
        self.transform2 = coin.SoTransform()
        self.transform3 = coin.SoTransform()

        scaleF = self.get_JCS_size()
        self.axisScale = coin.SoScale()
        self.axisScale.scaleFactor.setValue(scaleF, scaleF, scaleF)

        self.draw_style = coin.SoDrawStyle()
        self.draw_style.style = coin.SoDrawStyle.LINES
        self.draw_style.lineWidth = self.axis_thickness

        self.switch_JCS1 = self.JCS_sep(obj, self.transform1)
        self.switch_JCS2 = self.JCS_sep(obj, self.transform2)
        self.switch_JCS_preview = self.JCS_sep(obj, self.transform3)

        self.display_mode = coin.SoGroup()
        self.display_mode.addChild(self.switch_JCS1)
        self.display_mode.addChild(self.switch_JCS2)
        self.display_mode.addChild(self.switch_JCS_preview)
        obj.addDisplayMode(self.display_mode, "Wireframe")

    def camera_callback(self, *args):
        scaleF = self.get_JCS_size()
        self.axisScale.scaleFactor.setValue(scaleF, scaleF, scaleF)

    def JCS_sep(self, obj, soTransform):
        pick = coin.SoPickStyle()
        pick.style.setValue(coin.SoPickStyle.UNPICKABLE)

        JCS = coin.SoAnnotation()
        JCS.addChild(soTransform)
        JCS.addChild(pick)

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
        material.diffuseColor.setValue([1, 1, 1])
        material.ambientColor.setValue([1, 1, 1])
        material.specularColor.setValue([1, 1, 1])
        material.emissiveColor.setValue([1, 1, 1])
        material.transparency.setValue(0.7)

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
        if not camera:
            return 10

        return camera.height.getValue() / 20

    def set_JCS_placement(self, soTransform, placement):
        t = placement.Base
        soTransform.translation.setValue(t.x, t.y, t.z)

        r = placement.Rotation.Q
        soTransform.rotation.setValue(r[0], r[1], r[2], r[3])

    def updateData(self, fp, prop):
        """If a property of the handled feature has changed we have the chance to handle this here"""
        # fp is the handled feature, prop is the name of the property that has changed
        if prop == "Placement1":
            plc = fp.getPropertyByName("Placement1")
            if fp.getPropertyByName("Object1"):
                self.switch_JCS1.whichChild = coin.SO_SWITCH_ALL
                self.set_JCS_placement(self.transform1, plc)
            else:
                self.switch_JCS1.whichChild = coin.SO_SWITCH_NONE

        if prop == "Placement2":
            plc = fp.getPropertyByName("Placement2")
            if fp.getPropertyByName("Object2"):
                self.switch_JCS2.whichChild = coin.SO_SWITCH_ALL
                self.set_JCS_placement(self.transform2, plc)
            else:
                self.switch_JCS2.whichChild = coin.SO_SWITCH_NONE

    def showPreviewJCS(self, visible, placement=None):
        if visible:
            self.switch_JCS_preview.whichChild = coin.SO_SWITCH_ALL
            self.set_JCS_placement(self.transform3, placement)
        else:
            self.switch_JCS_preview.whichChild = coin.SO_SWITCH_NONE

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
        if self.app_obj.getPropertyByName("JointType") == "Fixed":
            return ":/icons/Assembly_CreateJointFixed.svg"
        elif self.app_obj.getPropertyByName("JointType") == "Revolute":
            return ":/icons/Assembly_CreateJointRevolute.svg"
        elif self.app_obj.getPropertyByName("JointType") == "Cylindrical":
            return ":/icons/Assembly_CreateJointCylindrical.svg"
        elif self.app_obj.getPropertyByName("JointType") == "Slider":
            return ":/icons/Assembly_CreateJointSlider.svg"
        elif self.app_obj.getPropertyByName("JointType") == "Ball":
            return ":/icons/Assembly_CreateJointBall.svg"
        elif self.app_obj.getPropertyByName("JointType") == "Planar":
            return ":/icons/Assembly_CreateJointPlanar.svg"
        elif self.app_obj.getPropertyByName("JointType") == "Parallel":
            return ":/icons/Assembly_CreateJointParallel.svg"
        elif self.app_obj.getPropertyByName("JointType") == "Tangent":
            return ":/icons/Assembly_CreateJointTangent.svg"

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
