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
"""Provides the object code for the dimension objects.

This includes the `LinearDimension` and `AgularDimension`.
The first one measures a distance between two points or vertices
in an object; it includes radial dimensions of circular arcs.
The second one creates an arc between two straight lines to measure
the angle between both.

To Do
-----
1. Currently the angular dimension does not use linked geometrical
elements, meaning that it cannot update its `Angle` by picking lines or edges
from objects. If fact, `LinkedGeometry` is hidden to prevent the user
from picking any object.

At the moment the user must manually modify `FirstAngle` and `LastAngle`
to obtain a new `Angle`, but since the values are manually entered
the result is not parametrically tied to any actual object in the document.

We introduced a function `measure_two_obj_angles` to calculate
the corresponding parameters from a pair of objects and their edges.
Currently this function is deactivated because we don't consider it
to be ready; it is there for testing purposes only.
This needs to be improved because at the moment it only gives
one possible angle. We should be able to get the four angles
of a two-line intersection. Maybe a new property is required
to indicate the quadrant to choose and display.

2. In general, the `LinkedGeometry` property must be changed in type,
as it does not need to be an `App::PropertyLinkSubList`.
A `LinkSubList` is to select multiple subelements (vertices, edges)
from multiple objects (two lines). However, since we typically measure
a single object, for example, a single line or circle, the subelements
that we can choose must belong to this object only.

Therefore, just like in the case of the `PathArray` class the best property
that could be used is `App::PropertyLinkSub`.
Then in the property editor we will be unable to select more than one object
thus preventing errors of the subelements not matching the measured object.

3. Currently the `LinearDimension` class is able to measure the distance
between two arbitrary vertices in two distinct objects.
For this case `App::PropertyLinkSubList` is in fact the right property
to use, however, neither the `make_dimension` functions
nor the Gui Command are set up to use this type of information.
This has to be done manually by picking the two objects and the two vertices
in the property editor. That is, this functionality is not entirely intuitive,
so it is somewhat 'hidden' from the user.

So, the make function and the Gui Command should be expanded to consider
this case.

Another possibility would be to use one property (LinkSub) for single-object
measurements (linear, radial), and a second property (LinkSubList)
for two-object measurements (linear, angular). This would require adjustments
to the `execute` method to handle both cases properly. It may be necessary
to have another property to control which type to use.

4. The `Support` property is not used at all, so it should be removed.
It is just set at creation time by the `make_dimension` function
but it actually isn't used in the `execute` code.

5. In fact the `DimensionBase` class is not the best base class
than can be used as parent for all dimensions because it defines `Normal`,
`Support`, and `LinkedGeometry`, which aren't used in all cases.
In some of the derived classes, these properties are hidden.

So, together with what is explained in point 3, we probably need to use
a more generic base class, while at the same time improve the way
the link properties are used.
"""
## @package dimension
# \ingroup draftobjects
# \brief Provides the object code for the dimension objects.

## \addtogroup draftobjects
# @{
import math
from PySide.QtCore import QT_TRANSLATE_NOOP

import FreeCAD as App
import DraftVecUtils
import DraftGeomUtils
import draftutils.utils as utils

from draftutils.messages import _wrn
from draftutils.translate import translate

from draftobjects.draft_annotation import DraftAnnotation


class DimensionBase(DraftAnnotation):
    """The base objects for dimension objects.

    This class inherits `DraftAnnotation` to define the basic properties
    of all annotation type objects, like a scaling multiplier.

    This class is not used directly, but inherited by all dimension
    objects.
    """

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
                                     "- An arc object, and its edge.")
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
        """Execute code when the document is restored."""
        super().onDocumentRestored(obj)

        if not hasattr(obj, "ViewObject"):
            return
        vobj = obj.ViewObject
        if not vobj:
            return
        if hasattr(vobj, "TextColor"):
            return
        self.update_properties_0v21(obj, vobj)

    def update_properties_0v21(self, obj, vobj):
        """Update view properties."""
        vobj.Proxy.set_text_properties(vobj, vobj.PropertiesList)
        vobj.TextColor = vobj.LineColor
        _wrn("v0.21, " + obj.Label + ", "
             + translate("draft", "added view property 'TextColor'"))
        _wrn("v0.21, " + obj.Label + ", "
             + translate("draft", "renamed 'DisplayMode' options to 'World/Screen'"))


class LinearDimension(DimensionBase):
    """The linear dimension object.

    This inherits `DimensionBase` to provide the basic functionality of
    a dimension.

    This linear dimension includes measurements between two vertices,
    but also a radial dimension of a circular edge or arc.
    """

    def __init__(self, obj):
        obj.Proxy = self
        self.set_properties(obj)
        self.Type = "LinearDimension"

    def set_properties(self, obj):
        """Set basic properties only if they don't exist."""
        super().set_properties(obj)

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
        """Execute code when the document is restored."""
        super().onDocumentRestored(obj)
        self.Type = "LinearDimension"

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
            # but if the projection is not successful, try in the other planes
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
        obj.Proxy = self
        self.set_properties(obj)
        self.Type = "AngularDimension"

    def set_properties(self, obj):
        """Set basic properties only if they don't exist."""
        super().set_properties(obj)

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
        """Execute code when the document is restored."""
        super().onDocumentRestored(obj)
        self.Type = "AngularDimension"

    def execute(self, obj):
        """Execute when the object is created or recomputed.

        Nothing is actually done here, except update the viewprovider,
        as the lines and text are created in the viewprovider.
        """
        # TODO: introduce the calculation of 'Angle' by taking a pair of
        # objects and edges in the 'LinkedGeometry' property.
        #
        # We introduced a function `measure_two_obj_angles` to calculate
        # the corresponding parameters from a pair of objects and their edges.
        # Currently this function is deactivated because we don't consider it
        # to be ready; it is there for testing purposes only.
        #
        # if obj.LinkedGeometry and len(obj.LinkedGeometry) == 2:
        #     (obj.FirstAngle,
        #      obj.LastAngle) = measure_two_obj_angles(obj.LinkedGeometry[0],
        #                                              obj.LinkedGeometry[1])

        # TODO: move the calculation of 'Angle' from the viewprovider
        # to this object class.
        # The viewprovider should modify visual properties only, not real
        # properties. It can react to real properties by using the 'updateData'
        # method.
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


def measure_two_obj_angles(link_sub_1, link_sub_2):
    """Measure two edges from two different objects to measure the angle.

    This function is a prototype because it does not determine all possible
    starting and ending angles that could be used to draw the dimension line,
    which is a circular arc.

    Parameters
    ----------
    link_sub_1: tuple
        A tuple containing one object and a list of subelement strings,
        which may be empty. Only the first subelement is considered, which
        must be an edge.
        ::
            link_sub_1 = (obj1, ['EdgeN', ...])

    link_sub_2: tuple
        Same.
    """
    start = 0
    end = 0

    obj1 = link_sub_1[0]
    lsub1 = link_sub_1[1]

    obj2 = link_sub_2[0]
    lsub2 = link_sub_2[1]

    # The subelement list may be empty so we test it first
    # and pick only the first item
    if lsub1 and lsub2:
        subelement1 = lsub1[0]
        subelement2 = lsub2[0]

        if "Edge" in subelement1 and "Edge" in subelement2:
            n1 = int(subelement1[4:]) - 1
            n2 = int(subelement2[4:]) - 1
            start = obj1.Shape.Edges[n1].Curve.Direction
            end = obj2.Shape.Edges[n2].Curve.Direction

            # We get the angle from the direction of the line to the U axis
            # of the working plane; we should be able to also use the V axis
            start_r = DraftVecUtils.angle(start, App.DraftWorkingPlane.u)
            end_r = DraftVecUtils.angle(end, App.DraftWorkingPlane.u)
            start = math.degrees(start_r)
            end = math.degrees(end_r)

            # We make the angle positive because when tested, some errors
            # were produced in the code that calculates the 'Angle'.
            # This code is actually inside the viewprovider.
            if start < 0:
                start = abs(start)
            if end < 0:
                end = abs(end)

    return start, end


# Alias for compatibility with v0.18 and earlier
_AngularDimension = AngularDimension

## @}
