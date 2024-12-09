# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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


if App.GuiUp:
    import FreeCADGui as Gui

__title__ = "Assembly Marker Inventor object"
__author__ = "Ondsel"
__url__ = "https://www.freecad.org"

from pivy import coin
import UtilsAssembly
import Preferences


class SoSwitchMarker(coin.SoSwitch):
    def __init__(self, vobj):
        super().__init__()  # Initialize the SoSwitch base class

        self.axis_thickness = 3
        self.scaleFactor = 20

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

        self.transform = coin.SoTransform()

        self.draw_style = coin.SoDrawStyle()
        self.draw_style.style = coin.SoDrawStyle.LINES
        self.draw_style.lineWidth = self.axis_thickness

        self.pick = coin.SoPickStyle()
        self.setPickableState(True)

        JCS = coin.SoAnnotation()
        JCS.addChild(self.transform)
        JCS.addChild(self.pick)

        base_plane_sep = self.plane_sep(0.4, 15)
        X_axis_sep = self.line_sep([0.5, 0, 0], [1, 0, 0], self.x_axis_so_color)
        Y_axis_sep = self.line_sep([0, 0.5, 0], [0, 1, 0], self.y_axis_so_color)
        Z_axis_sep = self.line_sep([0, 0, 0], [0, 0, 1], self.z_axis_so_color)

        JCS.addChild(base_plane_sep)
        JCS.addChild(X_axis_sep)
        JCS.addChild(Y_axis_sep)
        JCS.addChild(Z_axis_sep)

        switch_JCS = coin.SoSwitch()
        self.addChild(JCS)
        self.whichChild = coin.SO_SWITCH_NONE

    def line_sep(self, startPoint, endPoint, soColor):
        line = coin.SoLineSet()
        line.numVertices.setValue(2)
        coords = coin.SoCoordinate3()
        coords.point.setValues(0, [startPoint, endPoint])

        axis_sep = coin.SoAnnotation()
        axis_sep.addChild(self.draw_style)
        axis_sep.addChild(soColor)
        axis_sep.addChild(coords)
        axis_sep.addChild(line)

        scale = coin.SoType.fromName("SoShapeScale").createInstance()
        scale.setPart("shape", axis_sep)
        scale.scaleFactor = self.scaleFactor

        return scale

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
        face_sep.addChild(transform)
        face_sep.addChild(draw_style)
        face_sep.addChild(material)
        face_sep.addChild(coords)
        face_sep.addChild(face)

        scale = coin.SoType.fromName("SoShapeScale").createInstance()
        scale.setPart("shape", face_sep)
        scale.scaleFactor = self.scaleFactor

        return scale

    def set_marker_placement(self, placement, ref):
        # change plc to be relative to the origin of the document.
        global_plc = UtilsAssembly.getGlobalPlacement(ref)
        placement = global_plc * placement

        t = placement.Base
        self.transform.translation.setValue(t.x, t.y, t.z)

        r = placement.Rotation.Q
        self.transform.rotation.setValue(r[0], r[1], r[2], r[3])

    def setPickableState(self, state: bool):
        """Set JCS selectable or unselectable in 3D view"""
        if not state:
            self.pick.style.setValue(coin.SoPickStyle.UNPICKABLE)
        else:
            self.pick.style.setValue(coin.SoPickStyle.SHAPE_ON_TOP)

    def show_marker(self, visible, placement=None, ref=None):
        if visible:
            self.whichChild = coin.SO_SWITCH_ALL
            self.set_marker_placement(placement, ref)
        else:
            self.whichChild = coin.SO_SWITCH_NONE

    def onChanged(self, vp, prop):
        if prop == "color_X_axis":
            c = vp.getPropertyByName("color_X_axis")
            self.x_axis_so_color.rgb.setValue(c[0], c[1], c[2])
        if prop == "color_Y_axis":
            c = vp.getPropertyByName("color_Y_axis")
            self.y_axis_so_color.rgb.setValue(c[0], c[1], c[2])
        if prop == "color_Z_axis":
            c = vp.getPropertyByName("color_Z_axis")
            self.z_axis_so_color.rgb.setValue(c[0], c[1], c[2])
