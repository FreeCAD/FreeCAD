# -*- coding: utf-8 -*-
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
"""Provides the object code for the Label object."""
## @package label
# \ingroup draftobjects
# \brief Provides the object code for the Label object.

## \addtogroup draftobjects
# @{
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
from FreeCAD import Units as U
from draftobjects.draft_annotation import DraftAnnotation
from draftutils import gui_utils
from draftutils.messages import _wrn
from draftutils.translate import translate


class Label(DraftAnnotation):
    """The Draft Label object."""

    def __init__(self, obj):
        obj.Proxy = self
        self.set_properties(obj)
        self.Type = "Label"

    def set_properties(self, obj):
        """Set properties only if they don't exist."""
        self.set_target_properties(obj)
        self.set_leader_properties(obj)
        self.set_label_properties(obj)

    def set_target_properties(self, obj):
        """Set position properties only if they don't exist."""
        properties = obj.PropertiesList

        if "TargetPoint" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The position of the tip of the leader "
                                     "line.\n"
                                     "This point can be decorated "
                                     "with an arrow or another symbol.")
            obj.addProperty("App::PropertyVector",
                            "TargetPoint",
                            "Target",
                            _tip)
            obj.TargetPoint = App.Vector(2, -1, 0)

        if "Target" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Object, and optionally subelement, "
                                     "whose properties will be displayed\n"
                                     "as 'Text', depending on 'Label Type'.\n"
                                     "\n"
                                     "'Target' won't be used "
                                     "if 'Label Type' is set to 'Custom'.")
            obj.addProperty("App::PropertyLinkSub",
                            "Target",
                            "Target",
                            _tip)
            obj.Target = None

    def set_leader_properties(self, obj):
        """Set leader properties only if they don't exist."""
        properties = obj.PropertiesList

        if "Points" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The list of points defining the leader "
                                     "line; normally a list of three points.\n"
                                     "\n"
                                     "The first point should be the position "
                                     "of the text, that is, the 'Placement',\n"
                                     "and the last point should be "
                                     "the tip of the line, that is, "
                                     "the 'Target Point'.\n"
                                     "The middle point is calculated "
                                     "automatically depending on the chosen\n"
                                     "'Straight Direction' "
                                     "and the 'Straight Distance' value "
                                     "and sign.\n"
                                     "\n"
                                     "If 'Straight Direction' is set to "
                                     "'Custom', the 'Points' property\n"
                                     "can be set as a list "
                                     "of arbitrary points.")
            obj.addProperty("App::PropertyVectorList",
                            "Points",
                            "Leader",
                            _tip)
            obj.Points = []

        if "StraightDirection" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The direction of the straight segment "
                                     "of the leader line.\n"
                                     "\n"
                                     "If 'Custom' is chosen, the points "
                                     "of the leader can be specified by\n"
                                     "assigning a custom list "
                                     "to the 'Points' attribute.")
            obj.addProperty("App::PropertyEnumeration",
                            "StraightDirection",
                            "Leader",
                            _tip)
            obj.StraightDirection = ["Horizontal", "Vertical", "Custom"]

        if "StraightDistance" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The length of the straight segment "
                                     "of the leader line.\n"
                                     "\n"
                                     "This is an oriented distance; "
                                     "if it is negative, the line will "
                                     "be drawn\n"
                                     "to the left or below the 'Text', "
                                     "otherwise to the right or above it,\n"
                                     "depending on the value of "
                                     "'Straight Direction'.")
            obj.addProperty("App::PropertyDistance",
                            "StraightDistance",
                            "Leader",
                            _tip)
            obj.StraightDistance = 1

    def set_label_properties(self, obj):
        """Set label properties only if they don't exist."""
        properties = obj.PropertiesList

        if "Placement" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The placement of the 'Text' element "
                                     "in 3D space")
            obj.addProperty("App::PropertyPlacement",
                            "Placement",
                            "Label",
                            _tip)
            obj.Placement = App.Placement()

        if "CustomText" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The text to display when 'Label Type' "
                                     "is set to 'Custom'")
            obj.addProperty("App::PropertyStringList",
                            "CustomText",
                            "Label",
                            _tip)
            obj.CustomText = "Label"

        if "Text" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The text displayed by this label.\n"
                                     "\n"
                                     "This property is read-only, as the "
                                     "final text depends on 'Label Type',\n"
                                     "and the object defined in 'Target'.\n"
                                     "The 'Custom Text' is displayed only "
                                     "if 'Label Type' is set to 'Custom'.")
            obj.addProperty("App::PropertyStringList",
                            "Text",
                            "Label",
                            _tip)
            obj.setEditorMode("Text", 1)  # Read only

        # TODO: maybe here we can define a second and third 'label type'
        # properties, so that the final displayed text is either
        # the first type, or the combination of two or three types,
        # if they are available.
        # The current system has some labels combined, but these combinations
        # are hard coded. By considering multiple properties, we could produce
        # arbitrary combinations of labels.
        # This would also require updating the `return_info` function
        # to handle any combination that we want.
        if "LabelType" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The type of information displayed "
                                     "by this label.\n"
                                     "\n"
                                     "If 'Custom' is chosen, the contents of "
                                     "'Custom Text' will be used.\n"
                                     "For other types, the string will be "
                                     "calculated automatically from the "
                                     "object defined in 'Target'.\n"
                                     "'Tag' and 'Material' only work "
                                     "for objects that have these properties, "
                                     "like Arch objects.\n"
                                     "\n"
                                     "For 'Position', 'Length', and 'Area' "
                                     "these properties will be extracted "
                                     "from the main object in 'Target',\n"
                                     "or from the subelement "
                                     "'VertexN', 'EdgeN', or 'FaceN', "
                                     "respectively, if it is specified.")
            obj.addProperty("App::PropertyEnumeration",
                            "LabelType",
                            "Label",
                            _tip)
            obj.LabelType = get_label_types()

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored."""
        super().onDocumentRestored(obj)
        gui_utils.restore_view_object(obj, vp_module="view_label", vp_class="ViewProviderLabel")
        self.Type = "Label"

        if not getattr(obj, "ViewObject", None):
            return
        vobj = obj.ViewObject
        if hasattr(vobj, "FontName") and hasattr(vobj, "FontSize"):
            return
        self.update_properties_0v21(obj, vobj)

    def update_properties_0v21(self, obj, vobj):
        """Update view properties."""
        old_fontname = vobj.TextFont
        old_fontsize = vobj.TextSize
        vobj.removeProperty("TextFont")
        vobj.removeProperty("TextSize")

        vobj.Proxy.set_text_properties(vobj, vobj.PropertiesList)
        vobj.FontName = old_fontname
        vobj.FontSize = old_fontsize
        # The DisplayMode is updated automatically but the new values are
        # switched: "2D text" becomes "World" and "3D text" becomes "Screen".
        # It should be the other way around:
        vobj.DisplayMode = "World" if vobj.DisplayMode == "Screen" else "Screen"
        _wrn("v0.21, " + obj.Label + ", "
             + translate("draft", "renamed view property 'TextFont' to 'FontName'"))
        _wrn("v0.21, " + obj.Label + ", "
             + translate("draft", "renamed view property 'TextSize' to 'FontSize'"))
        _wrn("v0.21, " + obj.Label + ", "
             + translate("draft", "renamed 'DisplayMode' options to 'World/Screen'"))

    def onChanged(self, obj, prop):
        """Execute when a property is changed."""
        self.show_and_hide(obj, prop)

    def show_and_hide(self, obj, prop):
        """Show and hide the properties depending on the touched property."""
        # The minus sign removes the Hidden property (show)
        if prop == "LabelType":
            if obj.LabelType != "Custom":
                obj.setPropertyStatus("CustomText", "Hidden")
                obj.setPropertyStatus("Target", "-Hidden")
            else:
                obj.setPropertyStatus("CustomText", "-Hidden")
                obj.setPropertyStatus("Target", "Hidden")

    def execute(self, obj):
        """Execute when the object is created or recomputed."""
        if obj.StraightDirection != "Custom":
            p1 = obj.Placement.Base
            if obj.StraightDirection == "Horizontal":
                p2 = App.Vector(obj.StraightDistance.Value, 0, 0)
            elif obj.StraightDirection == "Vertical":
                p2 = App.Vector(0, obj.StraightDistance.Value, 0)

            p2 = obj.Placement.multVec(p2)
            # p3 = obj.Placement.multVec(obj.TargetPoint)
            p3 = obj.TargetPoint
            obj.Points = [p1, p2, p3]
        else:
            # If StraightDirection is 'Custom'
            # we can draw the leader line manually by specifying
            # any number of vectors in the Points property.
            # The first point should indicate the position of the text label,
            # while the last one should be 'TargetPoint'
            # obj.Points = [p1, p2, p3, p4, ...]
            #
            # The drawing of the line is done in the viewprovider
            #
            # However, as soon as StraightDirection is changed to
            # 'Horizontal' or 'Vertical' this custom list of points
            # will be overwritten
            pass

        if obj.LabelType == "Custom":
            if obj.CustomText:
                obj.Text = obj.CustomText

        elif obj.Target and obj.Target[0]:
            target = obj.Target[0]
            sub_list = obj.Target[1]
            typ = obj.LabelType

            # The sublist may be empty so we test it first
            subelement = sub_list[0] if sub_list else None
            obj.Text = return_info(target, typ, subelement)

        else:
            obj.Text = [translate("draft", "No Target")]


# Alias for compatibility with v0.18 and earlier
DraftLabel = Label


def get_label_types():
    return [QT_TRANSLATE_NOOP("Draft","Custom"),
            QT_TRANSLATE_NOOP("Draft","Name"),
            QT_TRANSLATE_NOOP("Draft","Label"),
            QT_TRANSLATE_NOOP("Draft","Position"),
            QT_TRANSLATE_NOOP("Draft","Length"),
            QT_TRANSLATE_NOOP("Draft","Area"),
            QT_TRANSLATE_NOOP("Draft","Volume"),
            QT_TRANSLATE_NOOP("Draft","Tag"),
            QT_TRANSLATE_NOOP("Draft","Material"),
            QT_TRANSLATE_NOOP("Draft","Label + Position"),
            QT_TRANSLATE_NOOP("Draft","Label + Length"),
            QT_TRANSLATE_NOOP("Draft","Label + Area"),
            QT_TRANSLATE_NOOP("Draft","Label + Volume"),
            QT_TRANSLATE_NOOP("Draft","Label + Material")]


def return_info(target, typ, subelement=None):
    """Return the text list from the target and the given type.

    Parameters
    ----------
    target: Part::Feature
        The object targeted by the label.

    typ: str
        It is the type of information that we want to extract.

    subelement: str, optional
        A string indicating a subelement of the `target`;
        it could be `'VertexN'`, `'EdgeN'`, or `'FaceN'`,
        where `'N'` is a number that starts from `1` up to the maximum
        number of subelements in that target.
    """
    # print(obj, target, typ, subelement)

    if typ == "Name":
        return _get_name(target)

    if typ == "Label":
        return _get_label(target)

    if typ == "Tag":
        return _get_tag(target)

    if typ == "Material":
        return _get_material(target)

    if typ == "Label + Material":
        return _get_label(target) + _get_material(target)

    if typ == "Position":
        return _get_position(target, subelement)

    if typ == "Label + Position":
        return _get_label(target) + _get_position(target, subelement)

    if typ == "Length":
        return _get_length(target, subelement)

    if typ == "Label + Length":
        return _get_label(target) + _get_length(target, subelement)

    if typ == "Area":
        return _get_area(target, subelement)

    if typ == "Label + Area":
        return _get_label(target) + _get_area(target, subelement)

    if typ == "Volume":
        return _get_volume(target, subelement)

    if typ == "Label + Volume":
        return _get_label(target) + _get_volume(target, subelement)

    return [translate("draft", "Invalid label type")]


def _get_name(target):
    return [target.Name]


def _get_label(target):
    return [target.Label]


def _get_tag(target):
    if hasattr(target, "Tag"):
        return [target.Tag]
    else:
        return [translate("draft", "Tag not available for object")]


def _get_material(target):
    if (hasattr(target, "Material") and hasattr(target.Material, "Label")):
        return [target.Material.Label]
    else:
        return [translate("draft", "Material not available for object")]


def _get_position(target, subelement):
    point = None
    if subelement is not None:
        if "Vertex" in subelement:
            point = target.Shape.Vertexes[int(subelement[6:]) - 1].Point
    else:
        point = target.Placement.Base
    if point is None:
        return [translate("draft", "Position not available for (sub)object")]
    return [U.Quantity(x, U.Length).UserString for x in tuple(point)]


def _get_length(target, subelement):
    length = None
    if subelement is not None:
        if "Edge" in subelement:
            length = target.Shape.Edges[int(subelement[4:]) - 1].Length
        elif "Face" in subelement:
            length = target.Shape.Faces[int(subelement[4:]) - 1].Length
    elif hasattr(target, "Length"):
        length = target.Length
    elif hasattr(target, "Shape") and hasattr(target.Shape, "Length"):
        length = target.Shape.Length
    if length is None:
        return [translate("draft", "Length not available for (sub)object")]
    return [U.Quantity(length, U.Length).UserString]


def _get_area(target, subelement):
    area = None
    if subelement is not None:
        if "Face" in subelement:
            area = target.Shape.Faces[int(subelement[4:]) - 1].Area
    elif hasattr(target, "Area"):
        area = target.Area
    elif hasattr(target, "Shape") and hasattr(target.Shape, "Area"):
        area = target.Shape.Area
    if area is None:
        return [translate("draft", "Area not available for (sub)object")]
    return [U.Quantity(area, U.Area).UserString.replace("^2", "²")]


def _get_volume(target, subelement):
    volume = None
    if subelement is not None:
        pass
    elif hasattr(target, "Volume"):
        volume = target.Volume
    elif hasattr(target, "Shape") and hasattr(target.Shape, "Volume"):
        volume = target.Shape.Volume
    if volume is None:
        return [translate("draft", "Volume not available for (sub)object")]
    return [U.Quantity(volume, U.Volume).UserString.replace("^3", "³")]


## @}
