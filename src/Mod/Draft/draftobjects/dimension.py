# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Provides the object code for the Draft Dimensions.

This includes the `LinearDimension` and `AgularDimension`.
The first one measures a distance between two points or vertices
in an object; it includes radial dimensions of circular arcs.
The second one creates an arc between two straight lines to measure
the angle between both.
"""
## @package dimension
# \ingroup DRAFT
# \brief Provides the object code for the Draft Dimensions.

from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils
import DraftGeomUtils
import draftutils.utils as utils

from draftobjects.draft_annotation import DraftAnnotation


class DimensionBase(DraftAnnotation):
    """The base objects for dimension objects.

    This class inherits `DraftAnnotation` to define the basic properties
    of all annotation type objects, like a scaling multiplier.

    This class is not used directly, but inherited by all dimension
    objects.
    """

    def __init__(self, obj, tp="Dimension"):
        super(DimensionBase, self).__init__(obj, tp)
        self.set_properties(obj)
        obj.Proxy = self

    def set_properties(self, obj):
        """Set basic properties only if they don't exist."""
        properties = obj.PropertiesList

        if "Normal" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The normal direction of the text "
                                     "of the dimension")
            obj.addProperty("App::PropertyVector",
                            "Normal",
                            "Dimension",
                            _tip)
            obj.Normal = App.Vector(0, 0, 1)

        # TODO: remove Support property as it is not used at all.
        # It is just set at creation time by the make_dimension function
        # but it is not used.
        if "Support" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The object measured by this dimension "
                                     "object")
            obj.addProperty("App::PropertyLink",
                            "Support",
                            "Dimension",
                            _tip)
            obj.Support = None

        if "LinkedGeometry" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The object, and specific subelements "
                                     "of it,\n"
                                     "that this dimension object "
                                     "is measuring.\n"
                                     "\n"
                                     "There are various possibilities:\n"
                                     "- An object, and one of its edges.\n"
                                     "- An object, and two of its vertices.\n"
                                     "- An arc object, and its edge.\n")
            obj.addProperty("App::PropertyLinkSubList",
                            "LinkedGeometry",
                            "Dimension",
                            _tip)
            obj.LinkedGeometry = []

        if "Dimline" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "A point through which the dimension "
                                     "line, or an extrapolation of it, "
                                     "will pass.\n"
                                     "\n"
                                     "- For linear dimensions, this property "
                                     "controls how close the dimension line\n"
                                     "is to the measured object.\n"
                                     "- For radial dimensions, this controls "
                                     "the direction of the dimension line\n"
                                     "that displays the measured radius or "
                                     "diameter.\n"
                                     "- For angular dimensions, "
                                     "this controls the radius of the "
                                     "dimension arc\n"
                                     "that displays the measured angle.")
            obj.addProperty("App::PropertyVectorDistance",
                            "Dimline",
                            "Dimension",
                            _tip)
            obj.Dimline = App.Vector(0, 1, 0)

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        It calls the parent class to add missing annotation properties.
        """
        super(DimensionBase, self).onDocumentRestored(obj)


class LinearDimension(DimensionBase):
    """The linear dimension object.

    This inherits `DimensionBase` to provide the basic functionality of
    a dimension.

    This linear dimension includes measurements between two vertices,
    but also a radial dimension of a circular edge or arc.
    """

    def __init__(self, obj):
        super(LinearDimension, self).__init__(obj, "LinearDimension")
        super(LinearDimension, self).set_properties(obj)
        self.set_properties(obj)
        obj.Proxy = self

    def set_properties(self, obj):
        """Set basic properties only if they don't exist."""
        properties = obj.PropertiesList

        if "Start" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Starting point of the dimension line.\n"
                                     "\n"
                                     "If it is a radius dimension it will be "
                                     "the center of the arc.\n"
                                     "If it is a diameter dimension "
                                     "it will be a point that lies "
                                     "on the arc.")
            obj.addProperty("App::PropertyVectorDistance",
                            "Start",
                            "Linear/radial dimension",
                            _tip)
            obj.Start = App.Vector(0, 0, 0)

        if "End" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Ending point of the dimension line.\n"
                                     "\n"
                                     "If it is a radius or diameter "
                                     "dimension\n"
                                     "it will be a point that lies "
                                     "on the arc.")
            obj.addProperty("App::PropertyVectorDistance",
                            "End",
                            "Linear/radial dimension",
                            _tip)
            obj.End = App.Vector(1, 0, 0)

        if "Direction" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The direction of the dimension line.\n"
                                     "If this remains '(0,0,0)', "
                                     "the direction will be calculated "
                                     "automatically.")
            obj.addProperty("App::PropertyVector",
                            "Direction",
                            "Linear/radial dimension",
                            _tip)

        if "Distance" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The value of the measurement.\n"
                                     "\n"
                                     "This property is read-only because "
                                     "the value is calculated\n"
                                     "from the 'Start' and 'End' properties.\n"
                                     "\n"
                                     "If the 'Linked Geometry' "
                                     "is an arc or circle, this 'Distance'\n"
                                     "is the radius or diameter, depending "
                                     "on the 'Diameter' property.")
            obj.addProperty("App::PropertyLength",
                            "Distance",
                            "Linear/radial dimension",
                            _tip)
            obj.Distance = 0

        if "Diameter" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "When measuring circular arcs, "
                                     "it determines whether to display\n"
                                     "the radius or the diameter value")
            obj.addProperty("App::PropertyBool",
                            "Diameter",
                            "Radial dimension",
                            _tip)
            obj.Diameter = False

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        It calls the parent class to add missing dimension properties.
        """
        super(LinearDimension, self).onDocumentRestored(obj)

    def onChanged(self, obj, prop):
        """Execute when a property is changed.

        It just sets some properties to be read-only or hidden,
        as they aren't used.
        """
        if hasattr(obj, "Distance"):
            obj.setPropertyStatus('Distance', 'ReadOnly')

        # if hasattr(obj, "Normal"):
        #    obj.setPropertyStatus('Normal', 'Hidden')
        if hasattr(obj, "Support"):
            obj.setPropertyStatus('Support', 'Hidden')

    def execute(self, obj):
        """Execute when the object is created or recomputed.

        Set start point and end point according to the linked geometry.
        """
        if obj.LinkedGeometry:
            if len(obj.LinkedGeometry) == 1:
                lobj = obj.LinkedGeometry[0][0]
                lsub = obj.LinkedGeometry[0][1]
                if len(lsub) == 1:
                    if "Edge" in lsub[0]:
                        n = int(lsub[0][4:]) - 1
                        edge = lobj.Shape.Edges[n]
                        if DraftGeomUtils.geomType(edge) == "Line":
                            obj.Start = edge.Vertexes[0].Point
                            obj.End = edge.Vertexes[-1].Point
                        elif DraftGeomUtils.geomType(edge) == "Circle":
                            c = edge.Curve.Center
                            r = edge.Curve.Radius
                            a = edge.Curve.Axis
                            ray = obj.Dimline.sub(c).projectToPlane(App.Vector(0, 0, 0), a)
                            if ray.Length == 0:
                                ray = a.cross(App.Vector(1, 0, 0))
                                if ray.Length == 0:
                                    ray = a.cross(App.Vector(0, 1, 0))
                            ray = DraftVecUtils.scaleTo(ray, r)
                            if hasattr(obj, "Diameter"):
                                if obj.Diameter:
                                    obj.Start = c.add(ray.negative())
                                    obj.End = c.add(ray)
                                else:
                                    obj.Start = c
                                    obj.End = c.add(ray)
                elif len(lsub) == 2:
                    if ("Vertex" in lsub[0]) and ("Vertex" in lsub[1]):
                        n1 = int(lsub[0][6:]) - 1
                        n2 = int(lsub[1][6:]) - 1
                        obj.Start = lobj.Shape.Vertexes[n1].Point
                        obj.End = lobj.Shape.Vertexes[n2].Point
            elif len(obj.LinkedGeometry) == 2:
                lobj1 = obj.LinkedGeometry[0][0]
                lobj2 = obj.LinkedGeometry[1][0]
                lsub1 = obj.LinkedGeometry[0][1]
                lsub2 = obj.LinkedGeometry[1][1]
                if (len(lsub1) == 1) and (len(lsub2) == 1):
                    if ("Vertex" in lsub1[0]) and ("Vertex" in lsub2[1]):
                        n1 = int(lsub1[0][6:]) - 1
                        n2 = int(lsub2[0][6:]) - 1
                        obj.Start = lobj1.Shape.Vertexes[n1].Point
                        obj.End = lobj2.Shape.Vertexes[n2].Point

        # set the distance property
        total_len = (obj.Start.sub(obj.End)).Length
        if round(obj.Distance.Value, utils.precision()) != round(total_len, utils.precision()):
            obj.Distance = total_len

        # The lines and text are created in the viewprovider, so we should
        # update it whenever the object is recomputed
        if App.GuiUp and obj.ViewObject:
            obj.ViewObject.update()


# Alias for compatibility with v0.18 and earlier
_Dimension = LinearDimension


class AngularDimension(DimensionBase):
    """The angular dimension object.

    This inherits `DimensionBase` to provide the basic functionality of
    a dimension.
    """

    def __init__(self, obj):
        super(AngularDimension, self).__init__(obj, "AngularDimension")
        super(AngularDimension, self).set_properties(obj)
        self.set_properties(obj)
        obj.Proxy = self

        # Inherited properties from the parent class
        obj.Normal = App.Vector(0, 0, 1)
        obj.Dimline = App.Vector(0, 1, 0)

    def set_properties(self, obj):
        """Set basic properties only if they don't exist."""
        properties = obj.PropertiesList

        if "FirstAngle" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Starting angle of the dimension line "
                                     "(circular arc).\n"
                                     "The arc is drawn counter-clockwise.")
            obj.addProperty("App::PropertyAngle",
                            "FirstAngle",
                            "Angular dimension",
                            _tip)
            obj.FirstAngle = 0

        if "LastAngle" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "Ending angle of the dimension line "
                                     "(circular arc).\n"
                                     "The arc is drawn counter-clockwise.")
            obj.addProperty("App::PropertyAngle",
                            "LastAngle",
                            "Angular dimension",
                            _tip)
            obj.LastAngle = 90

        if "Center" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The center point of the dimension "
                                     "line, which is a circular arc.\n"
                                     "\n"
                                     "This is normally the point where two "
                                     "line segments, or their extensions\n"
                                     "intersect, resulting in the "
                                     "measured 'Angle' between them.")
            obj.addProperty("App::PropertyVectorDistance",
                            "Center",
                            "Angular dimension",
                            _tip)
            obj.Center = App.Vector(0, 0, 0)

        if "Angle" not in properties:
            _tip = QT_TRANSLATE_NOOP("App::Property",
                                     "The value of the measurement.\n"
                                     "\n"
                                     "This property is read-only because "
                                     "the value is calculated from\n"
                                     "the 'First Angle' and "
                                     "'Last Angle' properties.")
            obj.addProperty("App::PropertyAngle",
                            "Angle",
                            "Angular dimension",
                            _tip)
            obj.Angle = 0

    def onDocumentRestored(self, obj):
        """Execute code when the document is restored.

        It calls the parent class to add missing dimension properties.
        """
        super(AngularDimension, self).onDocumentRestored(obj)

    def execute(self, obj):
        """Execute when the object is created or recomputed.

        Nothing is actually done here, except update the viewprovider,
        as the lines and text are created in the viewprovider.
        """
        if App.GuiUp and obj.ViewObject:
            obj.ViewObject.update()

    def onChanged(self, obj, prop):
        """Execute when a property is changed.

        It just sets some properties to be read-only or hidden,
        as they aren't used.
        """
        if hasattr(obj, "Angle"):
            obj.setPropertyStatus('Angle', 'ReadOnly')

        if hasattr(obj, "Normal"):
            obj.setPropertyStatus('Normal', 'Hidden')
        if hasattr(obj, "Support"):
            obj.setPropertyStatus('Support', 'Hidden')
        if hasattr(obj, "LinkedGeometry"):
            obj.setPropertyStatus('LinkedGeometry', 'Hidden')


# Alias for compatibility with v0.18 and earlier
_AngularDimension = AngularDimension
