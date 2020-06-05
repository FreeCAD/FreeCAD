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
"""Provide the object code for Draft Label objects."""
## @package label
# \ingroup DRAFT
# \brief Provide the object code for Draft Label objects.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App

from FreeCAD import Units as U
from draftobjects.draft_annotation import DraftAnnotation


class Label(DraftAnnotation):
    """The Draft Label object."""

    def __init__(self, obj):
        super(Label, self).__init__(obj, "Label")
        self.set_properties(obj)
        obj.Proxy = self

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
            obj.LabelType = ["Custom", "Name", "Label", "Position",
                             "Length", "Area", "Volume", "Tag", "Material"]

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        It calls the parent class to add missing annotation properties.
        """
        super(Label, self).onDocumentRestored(obj)

    def execute(self, obj):
        """Execute when the object is created or recomputed."""
        if obj.StraightDirection != "Custom":
            p1 = obj.Placement.Base
            if obj.StraightDirection == "Horizontal":
                p2 = App.Vector(obj.StraightDistance.Value, 0, 0)
            else:
                p2 = App.Vector(0, obj.StraightDistance.Value, 0)

            p2 = obj.Placement.multVec(p2)
            # p3 = obj.Placement.multVec(obj.TargetPoint)
            p3 = obj.TargetPoint
            obj.Points = [p1, p2, p3]

        if obj.LabelType == "Custom":
            if obj.CustomText:
                obj.Text = obj.CustomText

        elif obj.Target and obj.Target[0]:
            if obj.LabelType == "Name":
                obj.Text = [obj.Target[0].Name]
            elif obj.LabelType == "Label":
                obj.Text = [obj.Target[0].Label]
            elif obj.LabelType == "Tag":
                if hasattr(obj.Target[0], "Tag"):
                    obj.Text = [obj.Target[0].Tag]
            elif obj.LabelType == "Material":
                if hasattr(obj.Target[0], "Material"):
                    if hasattr(obj.Target[0].Material, "Label"):
                        obj.Text = [obj.Target[0].Material.Label]
            elif obj.LabelType == "Position":
                p = obj.Target[0].Placement.Base
                if obj.Target[1]:
                    if "Vertex" in obj.Target[1][0]:
                        p = obj.Target[0].Shape.Vertexes[int(obj.Target[1][0][6:])-1].Point
                obj.Text = [U.Quantity(x, U.Length).UserString for x in tuple(p)]
            elif obj.LabelType == "Length":
                if hasattr(obj.Target[0], 'Shape'):
                    if hasattr(obj.Target[0].Shape, "Length"):
                        obj.Text = [U.Quantity(obj.Target[0].Shape.Length, U.Length).UserString]
                    if obj.Target[1] and ("Edge" in obj.Target[1][0]):
                        obj.Text = [U.Quantity(obj.Target[0].Shape.Edges[int(obj.Target[1][0][4:]) - 1].Length, U.Length).UserString]
            elif obj.LabelType == "Area":
                if hasattr(obj.Target[0], 'Shape'):
                    if hasattr(obj.Target[0].Shape, "Area"):
                        obj.Text = [U.Quantity(obj.Target[0].Shape.Area, U.Area).UserString.replace("^2", "²")]
                    if obj.Target[1] and ("Face" in obj.Target[1][0]):
                        obj.Text = [U.Quantity(obj.Target[0].Shape.Faces[int(obj.Target[1][0][4:])-1].Area, U.Area).UserString]
            elif obj.LabelType == "Volume":
                if hasattr(obj.Target[0], 'Shape'):
                    if hasattr(obj.Target[0].Shape, "Volume"):
                        obj.Text = [U.Quantity(obj.Target[0].Shape.Volume, U.Volume).UserString.replace("^3", "³")]


# Alias for compatibility with v0.18 and earlier
DraftLabel = Label
