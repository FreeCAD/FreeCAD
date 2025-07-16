# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""This module contains geometry editing and geometry properties-related tools"""

import ifcopenshell
import ifcopenshell.util.unit

import FreeCAD

from . import ifc_tools


def add_geom_properties(obj):
    """Adds geometry properties to a FreeCAD object"""

    element = ifc_tools.get_ifc_element(obj)
    if not ifc_tools.has_representation(element):
        return
    ifcfile = ifc_tools.get_ifcfile(obj)
    scaling = ifcopenshell.util.unit.calculate_unit_scale(ifcfile)
    scaling = scaling * 1000  # given scale is for m, we work in mm
    for rep in element.Representation.Representations:
        if rep.RepresentationIdentifier == "Body":
            if len(rep.Items) == 1:
                # Extrusions
                if rep.Items[0].is_a("IfcExtrudedAreaSolid"):
                    ext = rep.Items[0]
                    if "ExtrusionDepth" not in obj.PropertiesList:
                        obj.addProperty(
                            "App::PropertyLength", "ExtrusionDepth", "Geometry", locked=True
                        )
                    obj.ExtrusionDepth = ext.Depth * scaling
                    if "ExtrusionDirection" not in obj.PropertiesList:
                        obj.addProperty(
                            "App::PropertyVector", "ExtrusionDirection", "Geometry", locked=True
                        )
                    obj.ExtrusionDirection = FreeCAD.Vector(
                        ext.ExtrudedDirection.DirectionRatios
                    )

                    # Extrusion of a rectangle
                    if ext.SweptArea.is_a("IfcRectangleProfileDef"):
                        if "RectangleLength" not in obj.PropertiesList:
                            obj.addProperty(
                                "App::PropertyLength", "RectangleLength", "Geometry", locked=True
                            )
                        obj.RectangleLength = ext.SweptArea.XDim * scaling
                        if "RectangleWidth" not in obj.PropertiesList:
                            obj.addProperty(
                                "App::PropertyLength", "RectangleWidth", "Geometry", locked=True
                            )
                        obj.RectangleWidth = ext.SweptArea.YDim * scaling

                    # Extrusion of a polyline
                    elif ext.SweptArea.is_a("IfcArbitraryClosedProfileDef"):
                        if ext.SweptArea.OuterCurve.is_a("IfcPolyline"):
                            if "PolylinePoints" not in obj.PropertiesList:
                                obj.addProperty(
                                    "App::PropertyVectorList",
                                    "PolylinePoints",
                                    "Geometry",
                                    locked=True,
                                )
                            points = [
                                p.Coordinates for p in ext.SweptArea.OuterCurve.Points
                            ]
                            points = [p + (0,) for p in points if len(p) < 3]
                            points = [
                                FreeCAD.Vector(p).multiply(scaling) for p in points
                            ]
                            obj.PolylinePoints = points

                    # I profile
                    elif ext.SweptArea.is_a("IfcIShapeProfileDef"):
                        for p in [
                            "FilletRadius",
                            "FlangeEdgeRadius",
                            "FlangeSlope",
                            "FlangeThickness",
                            "OverallDepth",
                            "OverallWidth",
                            "WebThickness",
                        ]:
                            if hasattr(ext.SweptArea, p):
                                obj.addProperty("App::PropertyLength", p, "Geometry", locked=True)
                                value = getattr(ext.SweptArea, p)
                                if not value:
                                    value = 0
                                value = value * scaling
                                setattr(obj, p, value)
                            obj.addProperty(
                                "App::PropertyString", "ProfileName", "Geometry", locked=True
                            )
                            obj.ProfileName = ext.SweptArea.ProfileName

        # below is disabled for now... Don't know if it's useful to expose to the user
        elif False:  # rep.RepresentationIdentifier == "Axis":
            # Wall axis consisting on a single line
            if len(rep.Items) == 1:
                if rep.Items[0].is_a("IfcCompositeCurve"):
                    if len(rep.Items[0].Segments) == 1:
                        if rep.Items[0].Segments[0].is_a("IfcCompositeCurveSegment"):
                            if rep.Items[0].Segments[0].ParentCurve.is_a("IfcPolyline"):
                                pol = rep.Items[0].Segments[0].ParentCurve
                                if len(pol.Points) == 2:
                                    if "AxisStart" not in obj.PropertiesList:
                                        obj.addProperty(
                                            "App::PropertyPosition",
                                            "AxisStart",
                                            "Geometry",
                                            locked=True,
                                        )
                                    obj.AxisStart = FreeCAD.Vector(
                                        pol.Points[0].Coordinates
                                    ).multiply(scaling)
                                    if "AxisEnd" not in obj.PropertiesList:
                                        obj.addProperty(
                                            "App::PropertyPosition",
                                            "AxisEnd",
                                            "Geometry",
                                            locked=True,
                                        )
                                    obj.AxisEnd = FreeCAD.Vector(
                                        pol.Points[1].Coordinates
                                    ).multiply(scaling)


def set_attribute(ifcfile, element, prop, value):
    """Sets an attribute. Returns True if the attribute was changed"""

    if value != getattr(element, prop):
        ifc_tools.api_run(
            "attribute.edit_attributes",
            ifcfile,
            product=element,
            attributes={prop: value},
        )
        return True
    return False


def set_geom_property(obj, prop):
    """Updates the internal IFC file with the given value"""

    # TODO verify if values are different than the stored value before changing!

    element = ifc_tools.get_ifc_element(obj)
    if not ifc_tools.has_representation(element):
        return False
    ifcfile = ifc_tools.get_ifcfile(obj)
    scaling = ifcopenshell.util.unit.calculate_unit_scale(ifcfile)
    scaling = 0.001 / scaling
    changed = False

    if prop == "ExtrusionDepth":
        for rep in element.Representation.Representations:
            if rep.RepresentationIdentifier == "Body":
                if len(rep.Items) == 1:
                    if rep.Items[0].is_a("IfcExtrudedAreaSolid"):
                        elem = rep.Items[0]
                        value = getattr(obj, prop).Value * scaling
                        changed = set_attribute(ifcfile, elem, "Depth", value)

    elif prop == "ExtrusionDirection":
        for rep in element.Representation.Representations:
            if rep.RepresentationIdentifier == "Body":
                if len(rep.Items) == 1:
                    if rep.Items[0].is_a("IfcExtrudedAreaSolid"):
                        elem = rep.Items[0].ExtrudedDirection
                        value = tuple(getattr(obj, prop))
                        changed = set_attribute(ifcfile, elem, "DirectionRatios", value)

    elif prop == "RectangleLength":
        for rep in element.Representation.Representations:
            if rep.RepresentationIdentifier == "Body":
                if len(rep.Items) == 1:
                    if rep.Items[0].is_a("IfcExtrudedAreaSolid"):
                        elem = rep.Items[0].SweptArea
                        if elem.is_a("IfcRectangleProfileDef"):
                            value = getattr(obj, prop).Value * scaling
                            changed = set_attribute(ifcfile, elem, "XDim", value)

    elif prop == "RectangleWidth":
        for rep in element.Representation.Representations:
            if rep.RepresentationIdentifier == "Body":
                if len(rep.Items) == 1:
                    if rep.Items[0].is_a("IfcExtrudedAreaSolid"):
                        elem = rep.Items[0].SweptArea
                        if elem.is_a("IfcRectangleProfileDef"):
                            value = getattr(obj, prop).Value * scaling
                            changed = set_attribute(ifcfile, elem, "YDim", value)

    elif prop == "PolylinePoints":
        # TODO check values against existing
        for rep in element.Representation.Representations:
            if rep.RepresentationIdentifier == "Body":
                if len(rep.Items) == 1:
                    if rep.Items[0].is_a("IfcExtrudedAreaSolid"):
                        if rep.Items[0].SweptArea.is_a("IfcArbitraryClosedProfileDef"):
                            if rep.Items[0].SweptArea.OuterCurve.is_a("IfcPolyline"):
                                elem = rep.Items[0].SweptArea.OuterCurve
                                elem_points = elem.Points
                                psize = elem_points[0].Dim
                                points = getattr(obj, prop)
                                if len(points) > len(elem_points):
                                    for i in range(len(points) - len(elem_points)):
                                        p = ifc_tools.api_run(
                                            "root.create_entity",
                                            ifcfile,
                                            ifc_class="IfcCartesianPoint",
                                        )
                                        elem_points.append(p)
                                    elem.Points = elem_points
                                elif len(points) < len(elem_points):
                                    rest = []
                                    for i in range(len(elem_points) - len(points)):
                                        rest.append(elem_points.pop())
                                    elem.Points = elem_points
                                    for r in rest:
                                        ifc_tools.api_run(
                                            "root.remove_product", ifcfile, product=r
                                        )
                                if len(points) == len(elem_points):
                                    for i in range(len(points)):
                                        v = FreeCAD.Vector(points[i]).multiply(scaling)
                                        coord = tuple(v)[:psize]
                                        ifc_tools.api_run(
                                            "attribute.edit_attributes",
                                            ifcfile,
                                            product=elem_points[i],
                                            attributes={"Coordinates": coord},
                                        )
                                    changed = True

    elif prop in [
        "FilletRadius",
        "FlangeEdgeRadius",
        "FlangeSlope",
        "FlangeThickness",
        "OverallDepth",
        "OverallWidth",
        "WebThickness",
    ]:
        for rep in element.Representation.Representations:
            if rep.RepresentationIdentifier == "Body":
                if len(rep.Items) == 1:
                    if rep.Items[0].SweptArea.is_a("IfcIShapeProfileDef"):
                        elem = rep.Items[0].SweptArea
                        value = getattr(obj, prop).Value * scaling
                        if value == 0 and getattr(elem, prop) is None:
                            value = None
                        changed = set_attribute(ifcfile, elem, prop, value)

    elif prop in ["ProfileName"]:
        for rep in element.Representation.Representations:
            if rep.RepresentationIdentifier == "Body":
                if len(rep.Items) == 1:
                    if rep.Items[0].SweptArea.is_a("IfcIShapeProfileDef"):
                        elem = rep.Items[0].SweptArea
                        value = obj.ProfileName
                        changed = set_attribute(ifcfile, elem, prop, value)

    if changed:
        FreeCAD.Console.PrintLog(
            "DEBUG: Changing prop"
            + obj.Label
            + ":"
            + str(prop)
            + "to"
            + str(getattr(obj, prop))
            + "\n"
        )
    return changed
