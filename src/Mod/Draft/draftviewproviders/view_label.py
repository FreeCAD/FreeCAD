# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
# *   Copyright (c) 2021 FreeCAD Developers                                 *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides the viewprovider code for the Label object."""
## @package view_label
# \ingroup draftviewproviders
# \brief Provides the viewprovider code for the Label object.

## \addtogroup draftviewproviders
# @{
import math
import sys
import pivy.coin as coin
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import draftutils.utils as utils
import draftutils.gui_utils as gui_utils

from draftviewproviders.view_draft_annotation \
    import ViewProviderDraftAnnotation

if App.GuiUp:
    import FreeCADGui as Gui

param = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Draft")


class ViewProviderLabel(ViewProviderDraftAnnotation):
    """Viewprovider for the Label annotation object."""

    def __init__(self, vobj):
        super(ViewProviderLabel, self).__init__(vobj)

        self.set_properties(vobj)
        self.Object = vobj.Object
        vobj.Proxy = self

    def set_properties(self, vobj):
        """Set the properties only if they don't already exist."""
        super(ViewProviderLabel, self).set_properties(vobj)

        properties = vobj.PropertiesList
        self.set_text_properties(vobj, properties)
        self.set_graphics_properties(vobj, properties)

    def set_text_properties(self, vobj, properties):
        """Set text properties only if they don't already exist."""
        if "TextSize" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The size of the text")
            vobj.addProperty("App::PropertyLength",
                             "TextSize",
                             "Text",
                             _tip)
            vobj.TextSize = utils.get_param("textheight", 1)

        if "TextFont" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The font of the text")
            vobj.addProperty("App::PropertyFont",
                             "TextFont",
                             "Text",
                             _tip)
            vobj.TextFont = utils.get_param("textfont")

        if "TextAlignment" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The vertical alignment of the text")
            vobj.addProperty("App::PropertyEnumeration",
                             "TextAlignment",
                             "Text",
                             _tip)
            vobj.TextAlignment = ["Top", "Middle", "Bottom"]
            vobj.TextAlignment = "Bottom"

        if "TextColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Text color")
            vobj.addProperty("App::PropertyColor",
                             "TextColor",
                             "Text",
                             _tip)

        if "MaxChars" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The maximum number of characters "
                                     "on each line of the text box")
            vobj.addProperty("App::PropertyInteger",
                             "MaxChars",
                             "Text",
                             _tip)

        if "Justification" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The vertical alignment of the text")
            vobj.addProperty("App::PropertyEnumeration",
                             "Justification",
                             "Text",
                             _tip)
            vobj.Justification = ["Left", "Center", "Right"]

        if "LineSpacing" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Line spacing (relative to font size)")
            vobj.addProperty("App::PropertyFloat",
                             "LineSpacing",
                             "Text",
                             _tip)
            vobj.LineSpacing = 1.0

    def set_graphics_properties(self, vobj, properties):
        """Set graphics properties only if they don't already exist."""
        if "ArrowSize" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The size of the arrow")
            vobj.addProperty("App::PropertyLength",
                             "ArrowSize",
                             "Graphics",
                             _tip)
            vobj.ArrowSize = utils.get_param("arrowsize", 1)

        if "ArrowType" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The type of arrow of this label")
            vobj.addProperty("App::PropertyEnumeration",
                             "ArrowType",
                             "Graphics",
                             _tip)
            vobj.ArrowType = utils.ARROW_TYPES
            vobj.ArrowType = utils.ARROW_TYPES[utils.get_param("dimsymbol")]

        if "Frame" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The type of frame around the text "
                                     "of this object")
            vobj.addProperty("App::PropertyEnumeration",
                             "Frame",
                             "Graphics",
                             _tip)
            vobj.Frame = ["None", "Rectangle"]

        if "Line" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Display a leader line or not")
            vobj.addProperty("App::PropertyBool",
                             "Line",
                             "Graphics",
                             _tip)
            vobj.Line = True

        if "LineWidth" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Line width")
            vobj.addProperty("App::PropertyFloat",
                             "LineWidth",
                             "Graphics",
                             _tip)
            vobj.LineWidth = utils.get_param("linewidth", 1)

        if "LineColor" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Line color")
            vobj.addProperty("App::PropertyColor",
                             "LineColor",
                             "Graphics",
                             _tip)

    def getIcon(self):
        """Return the path to the icon used by the viewprovider."""
        return ":/icons/Draft_Label.svg"

    def claimChildren(self):
        """Return objects that will be placed under it in the tree view."""
        return []

    def attach(self, vobj):
        """Set up the scene sub-graph of the viewprovider."""
        # Attributes of the Coin scenegraph
        self.arrow = coin.SoSeparator()
        self.arrowpos = coin.SoTransform()
        self.arrow.addChild(self.arrowpos)

        self.matline = coin.SoMaterial()
        self.drawstyle = coin.SoDrawStyle()
        self.drawstyle.style = coin.SoDrawStyle.LINES

        self.lcoords = coin.SoCoordinate3()
        self.line = coin.SoType.fromName("SoBrepEdgeSet").createInstance()

        self.mattext = coin.SoMaterial()
        self.textpos = coin.SoTransform()
        self.font = coin.SoFont()
        self.text2d = coin.SoText2()  # Faces the camera always
        self.text3d = coin.SoAsciiText()  # Can be oriented in 3D space

        self.fcoords = coin.SoCoordinate3()
        self.frame = coin.SoType.fromName("SoBrepEdgeSet").createInstance()
        self.lineswitch = coin.SoSwitch()

        self.symbol = gui_utils.dim_symbol()

        textdrawstyle = coin.SoDrawStyle()
        textdrawstyle.style = coin.SoDrawStyle.FILLED

        # The text string needs to be initialized to something,
        # otherwise it crashes
        self.text2d.string = self.text3d.string = "Label"
        self.text2d.justification = coin.SoText2.RIGHT
        self.text3d.justification = coin.SoAsciiText.RIGHT
        self.font.name = utils.get_param("textfont")

        switchnode = coin.SoSeparator()
        switchnode.addChild(self.line)
        self.lineswitch.addChild(switchnode)
        self.lineswitch.whichChild = 0

        self.node2dtxt = coin.SoGroup()
        self.node2dtxt.addChild(self.font)
        self.node2dtxt.addChild(self.text2d)

        self.node2d = coin.SoGroup()
        self.node2d.addChild(self.matline)
        self.node2d.addChild(self.arrow)
        self.node2d.addChild(self.drawstyle)
        self.node2d.addChild(self.lcoords)
        self.node2d.addChild(self.lineswitch)
        self.node2d.addChild(self.mattext)
        self.node2d.addChild(textdrawstyle)
        self.node2d.addChild(self.textpos)
        self.node2d.addChild(self.node2dtxt)
        self.node2d.addChild(self.matline)
        self.node2d.addChild(self.drawstyle)
        self.node2d.addChild(self.fcoords)
        self.node2d.addChild(self.frame)

        self.node3dtxt = coin.SoGroup()
        self.node3dtxt.addChild(self.font)
        self.node3dtxt.addChild(self.text3d)

        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.matline)
        self.node3d.addChild(self.arrow)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.lcoords)
        self.node3d.addChild(self.lineswitch)
        self.node3d.addChild(self.mattext)
        self.node3d.addChild(textdrawstyle)
        self.node3d.addChild(self.textpos)
        self.node3d.addChild(self.node3dtxt)
        self.node3d.addChild(self.matline)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.fcoords)
        self.node3d.addChild(self.frame)

        vobj.addDisplayMode(self.node2d, "2D text")
        vobj.addDisplayMode(self.node3d, "3D text")
        self.onChanged(vobj, "LineColor")
        self.onChanged(vobj, "TextColor")
        self.onChanged(vobj, "LineWidth")
        self.onChanged(vobj, "ArrowSize")
        self.onChanged(vobj, "Line")

    def getDisplayModes(self, vobj):
        """Return the display modes that this viewprovider supports."""
        return ["2D text", "3D text"]

    def getDefaultDisplayMode(self):
        """Return the default display mode."""
        return "3D text"

    def getDisplayMode(self, mode):
        """Return the saved display mode."""
        return mode

    def updateData(self, obj, prop):
        """Execute when a property from the Proxy class is changed."""
        if prop == "Points":
            n_points = len(obj.Points)
            if n_points >= 2:
                self.line.coordIndex.deleteValues(0)
                self.lcoords.point.setValues(obj.Points)
                self.line.coordIndex.setValues(0,
                                               n_points,
                                               range(n_points))
                self.onChanged(obj.ViewObject, "ArrowType")

            if obj.StraightDistance > 0:
                self.text2d.justification = coin.SoText2.RIGHT
                self.text3d.justification = coin.SoAsciiText.RIGHT
            else:
                self.text2d.justification = coin.SoText2.LEFT
                self.text3d.justification = coin.SoAsciiText.LEFT

            self.onChanged(obj.ViewObject, "DisplayMode") # Property to trigger update_label and update_frame.
                                                          # We could have used a different property.

        elif prop == "Text" and obj.Text:
            self.text2d.string.setValue("")
            self.text3d.string.setValue("")

            _list = [l for l in obj.Text if l]

            self.text2d.string.setValues(_list)
            self.text3d.string.setValues(_list)
            self.onChanged(obj.ViewObject, "DisplayMode")

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""
        super(ViewProviderLabel, self).onChanged(vobj, prop)

        obj = vobj.Object
        properties = vobj.PropertiesList

        can_update_label = ("DisplayMode" in properties
                            and "LineSpacing" in properties
                            and "ScaleMultiplier" in properties
                            and "TextAlignment" in properties # Top, Middle or Bottom.
                            and "TextFont" in properties
                            and "TextSize" in properties)
        can_update_frame = (can_update_label
                            and "Frame" in properties)

        if prop == "ScaleMultiplier" and "ScaleMultiplier" in properties:
            if "ArrowSize" in properties:
                s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
                if s:
                    self.arrowpos.scaleFactor.setValue((s, s, s))
            if can_update_label:
                self.update_label(obj, vobj)
            if can_update_frame:
                self.update_frame(obj, vobj)

        elif prop == "LineColor" and "LineColor" in properties:
            col = vobj.LineColor
            self.matline.diffuseColor.setValue([col[0], col[1], col[2]])

        elif prop == "TextColor" and "TextColor" in properties:
            col = vobj.TextColor
            self.mattext.diffuseColor.setValue([col[0], col[1], col[2]])

        elif prop == "LineWidth" and "LineWidth" in properties:
            self.drawstyle.lineWidth = vobj.LineWidth

        elif prop == "TextFont" and "TextFont" in properties:
            self.font.name = vobj.TextFont.encode("utf8")
            if can_update_label:
                self.update_label(obj, vobj)
            if can_update_frame:
                self.update_frame(obj, vobj)

        elif prop in ["DisplayMode", "Frame", "TextAlignment", "TextSize"]:
            if can_update_label:
                self.update_label(obj, vobj)
            if can_update_frame:
                self.update_frame(obj, vobj)

        elif prop == "Line" and "Line" in properties:
            if vobj.Line:
                self.lineswitch.whichChild = 0
            else:
                self.lineswitch.whichChild = -1

        elif prop == "ArrowType" and "ArrowType" in properties:
            if len(obj.Points) > 1:
                self.update_arrow(obj, vobj)

        elif (prop == "ArrowSize"
              and "ArrowSize" in properties
              and "ScaleMultiplier" in properties):
            s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
            if s:
                self.arrowpos.scaleFactor.setValue((s, s, s))

        elif prop == "Justification" and "Justification" in properties:
            if vobj.Justification == "Left":
                self.text2d.justification = coin.SoText2.LEFT
                self.text3d.justification = coin.SoAsciiText.LEFT
            elif vobj.Justification == "Right":
                self.text2d.justification = coin.SoText2.RIGHT
                self.text3d.justification = coin.SoAsciiText.RIGHT
            else:
                self.text2d.justification = coin.SoText2.CENTER
                self.text3d.justification = coin.SoAsciiText.CENTER

        elif prop == "LineSpacing" and "LineSpacing" in properties:
            self.text2d.spacing = max(1, vobj.LineSpacing)
            self.text3d.spacing = max(1, vobj.LineSpacing)
            if can_update_label:
                self.update_label(obj, vobj)
            if can_update_frame:
                self.update_frame(obj, vobj)

    def get_text_size(self, vobj):
        """Return the bounding box of the text element."""
        if vobj.DisplayMode == "3D text":
            node = self.node3dtxt
        else:
            node = self.node2dtxt

        region = coin.SbViewportRegion()
        action = coin.SoGetBoundingBoxAction(region)
        node.getBoundingBox(action)

        return action.getBoundingBox().getSize().getValue()

    def update_label(self, obj, vobj):
        """Update the label including text size and multiplier."""
        size = vobj.TextSize.Value * vobj.ScaleMultiplier
        self.font.size = size

        if vobj.DisplayMode == "2D text":
            self.textpos.translation.setValue(obj.Placement.Base)
            return

        line_height = size * max(1, vobj.LineSpacing)
        if vobj.Frame == "None":
            margin = size * 0.1
            first_line_height = size
            # We need to calculate total_height without using get_text_size:
            # If StraightDirection = "Horizontal" and TextAlignment = "Bottom"
            # we want the horizontal line segment to be aligned with the
            # baseline of the bottom text even if there are descenders.
            total_height = first_line_height + (line_height * (len(obj.Text) - 1))
        else:
            margin = line_height * 0.25
            first_line_height = size + margin
            box = self.get_text_size(vobj)
            total_height = box[1] + (2 * margin)

        # Space between endpoint of line and text:
        v = App.Vector(margin, 0, 0)
        if obj.StraightDistance > 0:
            v = v.negative()

        if vobj.TextAlignment == "Top":
            v = v + App.Vector(0, -first_line_height, 0)
        elif vobj.TextAlignment == "Middle":
            v = v + App.Vector(0, -first_line_height + (total_height / 2), 0)
        elif vobj.TextAlignment == "Bottom":
            v = v + App.Vector(0, -first_line_height + total_height, 0)

        v = obj.Placement.Rotation.multVec(v)
        pos = v + obj.Placement.Base
        self.textpos.translation.setValue(pos)
        self.textpos.rotation.setValue(obj.Placement.Rotation.Q)

    def update_arrow(self, obj, vobj):
        """Update the arrow tip of the line."""
        if hasattr(self, "symbol"):
            if self.arrow.findChild(self.symbol) != -1:
                self.arrow.removeChild(self.symbol)

        s = utils.ARROW_TYPES.index(vobj.ArrowType)
        self.symbol = gui_utils.dim_symbol(s)
        self.arrow.addChild(self.symbol)

        prec = 10**(-utils.precision())
        x_axis = App.Vector(1,0,0)
        target_dir = None
        # search in Points to get first point != to TargetPoint and use it
        # to get the target line direction
        for pnt in obj.Points[-2::-1]:
            if not pnt.isEqual(obj.Points[-1],prec):
                target_dir = pnt.sub(obj.Points[-1])
                break
        if target_dir is None:
            target_dir = x_axis
        target_dir_xy = obj.Placement.Rotation.inverted()*target_dir
        angle = target_dir_xy.getAngle(x_axis)*App.Units.Radian
        axis = x_axis.cross(target_dir_xy)
        rot = App.Rotation(axis, angle)

        self.arrowpos.rotation.setValue((obj.Placement.Rotation*rot).Q)
        self.arrowpos.translation.setValue(obj.Points[-1])

    def update_frame(self, obj, vobj):
        """Update the frame around the text."""
        self.frame.coordIndex.deleteValues(0)

        if vobj.Frame == "None":
            return
        if vobj.DisplayMode == "2D text":
            return

        size = vobj.TextSize.Value * vobj.ScaleMultiplier
        self.font.size = size

        line_height = size * max(1, vobj.LineSpacing)
        margin = line_height * 0.25
        first_line_height = size + margin
        box = self.get_text_size(vobj)
        total_width = box[0] + (2 * margin)
        total_height = box[1] + (2 * margin)

        # Space between frame and text:
        v = App.Vector(-margin, 0, 0)
        if obj.StraightDistance > 0:
            v = v.negative()
            total_width = -total_width

        # Shape of the rectangle
        # (p5)p1 --------- p2
        #     |            |
        #     b            |
        #     |            |
        #     p4 --------- p3
        #
        pts = []
        pts.append(v + App.Vector(0, first_line_height, 0))
        pts.append(pts[-1] + App.Vector(total_width, 0, 0))
        pts.append(pts[-1] + App.Vector(0, -total_height, 0))
        pts.append(pts[-1] + App.Vector(-total_width, 0, 0))
        pts.append(pts[0])

        self.fcoords.point.setValues(pts)
        self.frame.coordIndex.setValues(0,
                                        len(pts),
                                        range(len(pts)))


# Alias for compatibility with v0.18 and earlier
ViewProviderDraftLabel = ViewProviderLabel

## @}
