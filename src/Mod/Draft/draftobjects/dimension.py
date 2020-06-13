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

        Set start point and end point according to the linked geometry
        and the number of subelements.

        If it has one subelement, we assume a straight edge or a circular edge.
        If it has two subelements, we assume a straight edge (two vertices).
        """
        if obj.LinkedGeometry:
            if len(obj.LinkedGeometry) == 1:
                linked_obj = obj.LinkedGeometry[0][0]
                sub_list = obj.LinkedGeometry[0][1]

                if len(sub_list) == 1:
                    # If it has one subelement, we assume an edge
                    # that can be a straight line, or a circular edge
                    subelement = sub_list[0]
                    (obj.Start,
                     obj.End) = measure_one_obj_edge(linked_obj,
                                                     subelement,
                                                     obj.Dimline,
                                                     obj.Diameter)
                elif len(sub_list) == 2:
                    # If it has two subelements, we assume a straight edge
                    # that is measured by two vertices
                    (obj.Start,
                     obj.End) = measure_one_obj_vertices(linked_obj,
                                                         sub_list)

            elif len(obj.LinkedGeometry) == 2:
                # If the list has two objects, it measures the distance
                # between the two vertices in those two objects
                (obj.Start,
                 obj.End) = measure_two_objects(obj.LinkedGeometry[0],
                                                obj.LinkedGeometry[1])

        # Update the distance property by comparing the floats
        # with the precision
        net_length = (obj.Start.sub(obj.End)).Length
        rounded_1 = round(obj.Distance.Value, utils.precision())
        rounded_2 = round(net_length, utils.precision())

        if rounded_1 != rounded_2:
            obj.Distance = net_length

        # The lines and text are created in the viewprovider, so we should
        # update it whenever the object is recomputed
        if App.GuiUp and obj.ViewObject:
            obj.ViewObject.update()


def measure_one_obj_edge(obj, subelement, dim_point, diameter=False):
    """Measure one object with one subelement, a straight or circular edge.

    Parameters
    ----------
    obj: Part::Feature
        The object that is measured.

    subelement: str
        The subelement that is measured, for example, `'Edge1'`.

    dim_line: Base::Vector3
        A point through which the dimension goes through.
    """
    start = App.Vector()
    end = App.Vector()

    if "Edge" in subelement:
        n = int(subelement[4:]) - 1
        edge = obj.Shape.Edges[n]

        if DraftGeomUtils.geomType(edge) == "Line":
            start = edge.Vertexes[0].Point
            end = edge.Vertexes[-1].Point
        elif DraftGeomUtils.geomType(edge) == "Circle":
            center = edge.Curve.Center
            radius = edge.Curve.Radius
            axis = edge.Curve.Axis
            dim_line = dim_point.sub(center)

            # The ray is projected to the plane on which the circle lies,
            # but if the projection is not succesful, try in the other planes
            ray = dim_line.projectToPlane(App.Vector(0, 0, 0), axis)

            if ray.Length == 0:
                ray = axis.cross(App.Vector(1, 0, 0))
                if ray.Length == 0:
                    ray = axis.cross(App.Vector(0, 1, 0))

            # The ray is made as large as the arc's radius
            # and optionally the diameter
            ray = DraftVecUtils.scaleTo(ray, radius)

            if diameter:
                # The start and end points lie on the arc
                start = center.add(ray.negative())
                end = center.add(ray)
            else:
                # The start is th center and the end lies on the arc
                start = center
                end = center.add(ray)

    return start, end


def measure_one_obj_vertices(obj, subelements):
    """Measure two vertices in the same object."""
    start = App.Vector()
    end = App.Vector()

    subelement1 = subelements[0]
    subelement2 = subelements[1]

    if "Vertex" in subelement1 and "Vertex" in subelement2:
        n1 = int(subelement1[6:]) - 1
        n2 = int(subelement2[6:]) - 1
        start = obj.Shape.Vertexes[n1].Point
        end = obj.Shape.Vertexes[n2].Point

    return start, end


def measure_two_objects(link_sub_1, link_sub_2):
    """Measure two vertices from two different objects.

    Parameters
    ----------
    link_sub_1: tuple
        A tuple containing one object and a list of subelement strings,
        which may be empty. Only the first subelement is considered, which
        must be a vertex.
        ::
            link_sub_1 = (obj1, ['VertexN', ...])

    link_sub_2: tuple
        Same.
    """
    start = App.Vector()
    end = App.Vector()

    obj1 = link_sub_1[0]
    lsub1 = link_sub_1[1]

    obj2 = link_sub_2[0]
    lsub2 = link_sub_2[1]

    # The subelement list may be empty so we test it first
    # and pick only the first item
    if lsub1 and lsub2:
        subelement1 = lsub1[0]
        subelement2 = lsub2[0]

        if "Vertex" in subelement1 and "Vertex" in subelement2:
            n1 = int(subelement1[6:]) - 1
            n2 = int(subelement2[6:]) - 1
            start = obj1.Shape.Vertexes[n1].Point
            end = obj2.Shape.Vertexes[n2].Point

    return start, end


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
