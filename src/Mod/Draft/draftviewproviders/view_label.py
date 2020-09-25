# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *   Copyright (c) 2020 Eliud Cabrera Castillo <e.cabrera-castillo@tum.de> *
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
import DraftVecUtils
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

        switchnode = coin.SoSeparator()
        switchnode.addChild(self.line)
        switchnode.addChild(self.arrow)
        self.lineswitch.addChild(switchnode)
        self.lineswitch.whichChild = 0

        self.node2d = coin.SoGroup()
        self.node2d.addChild(self.matline)
        self.node2d.addChild(self.arrow)
        self.node2d.addChild(self.drawstyle)
        self.node2d.addChild(self.lcoords)
        self.node2d.addChild(self.lineswitch)
        self.node2d.addChild(self.mattext)
        self.node2d.addChild(textdrawstyle)
        self.node2d.addChild(self.textpos)
        self.node2d.addChild(self.font)
        self.node2d.addChild(self.text2d)
        self.node2d.addChild(self.fcoords)
        self.node2d.addChild(self.frame)

        self.node3d = coin.SoGroup()
        self.node3d.addChild(self.matline)
        self.node3d.addChild(self.arrow)
        self.node3d.addChild(self.drawstyle)
        self.node3d.addChild(self.lcoords)
        self.node3d.addChild(self.lineswitch)
        self.node3d.addChild(self.mattext)
        self.node3d.addChild(textdrawstyle)
        self.node3d.addChild(self.textpos)
        self.node3d.addChild(self.font)
        self.node3d.addChild(self.text3d)
        self.node3d.addChild(self.fcoords)
        self.node3d.addChild(self.frame)

        vobj.addDisplayMode(self.node2d, "2D text")
        vobj.addDisplayMode(self.node3d, "3D text")
        self.onChanged(vobj, "LineColor")
        self.onChanged(vobj, "TextColor")
        self.onChanged(vobj, "ArrowSize")
        self.onChanged(vobj, "Line")
        # self.onChanged(vobj, "ScaleMultiplier")

    def getDisplayModes(self, vobj):
        """Return the display modes that this viewprovider supports."""
        return ["2D text", "3D text"]

    def getDefaultDisplayMode(self):
        """Return the default display mode."""
        return "3D text"

    def setDisplayMode(self, mode):
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
                self.onChanged(obj.ViewObject, "TextSize")
                self.onChanged(obj.ViewObject, "ArrowType")

            if obj.StraightDistance > 0:
                self.text2d.justification = coin.SoText2.RIGHT
                self.text3d.justification = coin.SoAsciiText.RIGHT
            else:
                self.text2d.justification = coin.SoText2.LEFT
                self.text3d.justification = coin.SoAsciiText.LEFT

            self.onChanged(obj.ViewObject, "TextAlignment")
            self.onChanged(obj.ViewObject, "Frame")

        elif prop == "Text" and obj.Text:
            self.text2d.string.setValue("")
            self.text3d.string.setValue("")

            if sys.version_info.major >= 3:
                _list = [l for l in obj.Text if l]
            else:
                _list = [l.encode("utf8") for l in obj.Text if l]

            self.text2d.string.setValues(_list)
            self.text3d.string.setValues(_list)
            self.onChanged(obj.ViewObject, "TextAlignment")
            self.onChanged(obj.ViewObject, "Frame")

    def onChanged(self, vobj, prop):
        """Execute when a view property is changed."""
        super(ViewProviderLabel, self).onChanged(vobj, prop)

        obj = vobj.Object
        properties = vobj.PropertiesList

        if prop == "ScaleMultiplier" and "ScaleMultiplier" in properties:
            if "TextSize" in properties and "TextAlignment" in properties:
                self.update_label(obj, vobj)
            if "ArrowSize" in properties:
                s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
                if s:
                    self.arrowpos.scaleFactor.setValue((s, s, s))

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

        elif (prop in ["TextSize", "TextAlignment"]
              and "ScaleMultiplier" in properties
              and "TextSize" in properties
              and "TextAlignment" in properties):
            self.update_label(obj, vobj)

        elif prop == "Line" and "Line" in properties:
            if vobj.Line:
                self.lineswitch.whichChild = 0
            else:
                self.lineswitch.whichChild = -1

        elif prop == "ArrowType" and "ArrowType" in properties:
            if len(obj.Points) > 1:
                self.update_arrow(obj, vobj)

        elif (prop == "ArrowSize" and "ArrowSize" in properties
              and "ScaleMultiplier" in properties):
            s = vobj.ArrowSize.Value * vobj.ScaleMultiplier
            if s:
                self.arrowpos.scaleFactor.setValue((s, s, s))

        elif prop == "Frame" and "Frame" in properties:
            self.frame.coordIndex.deleteValues(0)

            if vobj.Frame == "Rectangle":
                self.draw_frame(obj, vobj)

        elif prop in "Justification" and "Justification" in properties:
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
            self.text2d.spacing = vobj.LineSpacing
            self.text3d.spacing = vobj.LineSpacing

    def get_text_size(self, vobj):
        """Return the bunding box of the text element."""
        if vobj.DisplayMode == "3D text":
            text = self.text3d
        else:
            text = self.text2d

        view = Gui.ActiveDocument.ActiveView
        region = view.getViewer().getSoRenderManager().getViewportRegion()
        action = coin.SoGetBoundingBoxAction(region)
        text.getBoundingBox(action)

        return action.getBoundingBox().getSize().getValue()

    def update_label(self, obj, vobj):
        """Update the label including text size and multiplier."""
        self.font.size = vobj.TextSize.Value * vobj.ScaleMultiplier

        # Tiny additional space added to the label
        v = App.Vector(1, 0, 0)
        if obj.StraightDistance > 0:
            v = v.negative()

        v.multiply(vobj.TextSize/10)
        tsize = self.get_text_size(vobj)

        n_lines = len(obj.Text)
        total_h = tsize[1]
        height = total_h/(n_lines + 1)

        if vobj.TextAlignment == "Top":
            d = v + App.Vector(0, -height, 0)
        elif vobj.TextAlignment == "Middle":
            if n_lines == 1:
                d = v + App.Vector(0, -height/2, 0)
            else:
                d = v + App.Vector(0, -height + total_h/2, 0)
        elif vobj.TextAlignment == "Bottom":
            if n_lines == 1:
                d = v + App.Vector(0, 0, 0)
            else:
                d = v + App.Vector(0, -height + n_lines * height, 0)

        d = obj.Placement.Rotation.multVec(d)
        pos = d + obj.Placement.Base
        self.textpos.translation.setValue(pos)
        self.textpos.rotation.setValue(obj.Placement.Rotation.Q)

    def update_arrow(self, obj, vobj):
        """Update the arrow tip of the line."""
        if hasattr(self, "symbol"):
            if self.arrow.findChild(self.symbol) != -1:
                self.arrow.removeChild(self.symbol)

        s = utils.ARROW_TYPES.index(vobj.ArrowType)
        self.symbol = gui_utils.dim_symbol(s)

        if vobj.ArrowType == "Circle":
            # TODO: fix behavior of the 'Circle' marker.
            # Instead of appearing at the tip of the line
            # the 'Circle' marker appears displaced and duplicated
            # a certain distance from the tip, which is the `TargetPoint`.
            # Somehow the translation is added to the position of the tip
            # resulting in a wrong value.
            # So the arrow position is reset; nevertheless, this doesn't
            # entirely fix the issue.
            coords2 = coin.SoCoordinate3()
            coords2.point.setValues([obj.Points[-1]])
            self.arrow.addChild(coords2)
            self.arrowpos.translation.setValue((0, 0, 0))
        else:
            self.arrowpos.translation.setValue(obj.Points[-1])
        self.arrow.addChild(self.symbol)

        v1 = obj.Points[-2].sub(obj.Points[-1])

        if not DraftVecUtils.isNull(v1):
            v1.normalize()
            v2 = App.Vector(0, 0, 1)
            if round(v2.getAngle(v1), 4) in [0, round(math.pi, 4)]:
                v2 = App.Vector(0, 1, 0)

            v3 = v1.cross(v2).negative()

            _rot_mat = DraftVecUtils.getPlaneRotation(v1, v3, v2)
            q = App.Placement(_rot_mat).Rotation.Q
            self.arrowpos.rotation.setValue((q[0], q[1], q[2], q[3]))

    def draw_frame(self, obj, vobj):
        """Draw the frame around the text."""
        tsize = self.get_text_size(vobj)
        total_w = tsize[0]
        total_h = tsize[1]

        n_lines = len(obj.Text)
        height = total_h/(n_lines + 1)

        # Tiny additional space added to the label
        v = App.Vector(1, 0, 0)

        if obj.StraightDistance > 0:
            v = v.negative()
            total_w = -total_w

        v.multiply(vobj.TextSize/10)

        pts = []
        _base = obj.Placement.Base
        _pos = App.Vector(self.textpos.translation.getValue().getValue())

        # The original base position must be subtracted, otherwise the frame
        # node is displaced twice
        base = _pos - _base - v

        # Shape of the rectangle
        # (p5)p1 --------- p2
        #     |            |
        #     b            |
        #     |            |
        #     p4 --------- p3
        #
        pts.append(base + App.Vector(0, 1.07 * height, 0))
        pts.append(pts[-1] + App.Vector(total_w, 0, 0))
        pts.append(pts[-1] + App.Vector(0, -1.07 * total_h, 0))
        pts.append(pts[-1] + App.Vector(-total_w, 0, 0))
        pts.append(pts[0])

        self.fcoords.point.setValues(pts)
        self.frame.coordIndex.setValues(0,
                                        len(pts),
                                        range(len(pts)))


# Alias for compatibility with v0.18 and earlier
ViewProviderDraftLabel = ViewProviderLabel

## @}
