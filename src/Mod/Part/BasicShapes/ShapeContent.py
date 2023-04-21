#! python
# -*- coding: utf-8 -*-
# ShapeContent.py
# 2021, by Mark Ganson <TheMarkster>
# LGPL 2.1 or later

# this file is called from c++ in TaskCheckGeometry.cpp
# from buildShapeContent()

import FreeCAD as App
import Part

translate = App.Qt.translate


def roundVector(v, dec):
    return str([round(v[0], dec), round(v[1], dec), round(v[2], dec)])


def buildShapeContent(objArg, decimals=2, advancedShapeContent=True):
    linkName = ""
    if objArg.isDerivedFrom("App::Link"):
        linkName = "<" + objArg.Name + "> "

    obj = objArg
    shp = Part.getShape(objArg)
    typeStr = str(shp.ShapeType)
    lbl = "" if obj.Name == obj.Label else "(" + obj.Label + ")"
    result = linkName + obj.Name + lbl + "\n"
    result += (
        translate("TaskCheckGeometryResults", "Shape type") + ":  " + typeStr + "\n"
    )
    result += (
        translate("TaskCheckGeometryResults", "Vertices")
        + ":  "
        + str(len(shp.Vertexes))
        + "\n"
    )
    result += (
        translate("TaskCheckGeometryResults", "Edges")
        + ":  "
        + str(len(shp.Edges))
        + "\n"
    )
    result += (
        translate("TaskCheckGeometryResults", "Wires")
        + ":  "
        + str(len(shp.Wires))
        + "\n"
    )
    result += (
        translate("TaskCheckGeometryResults", "Faces")
        + ":  "
        + str(len(shp.Faces))
        + "\n"
    )
    result += (
        translate("TaskCheckGeometryResults", "Shells")
        + ":  "
        + str(len(shp.Shells))
        + "\n"
    )
    result += (
        translate("TaskCheckGeometryResults", "Solids")
        + ":  "
        + str(len(shp.Solids))
        + "\n"
    )
    result += (
        translate("TaskCheckGeometryResults", "CompSolids")
        + ":  "
        + str(len(shp.CompSolids))
        + "\n"
    )
    result += (
        translate("TaskCheckGeometryResults", "Compounds")
        + ":  "
        + str(len(shp.Compounds))
        + "\n"
    )
    result += (
        translate("TaskCheckGeometryResults", "Shapes")
        + ":  "
        + str(
            len(
                shp.Vertexes
                + shp.Edges
                + shp.Wires
                + shp.Faces
                + shp.Shells
                + shp.Solids
                + shp.CompSolids
                + shp.Compounds
            )
        )
        + "\n"
    )
    if advancedShapeContent:
        result += "----------\n"
        if (
            hasattr(shp, "Area")
            and not "Wire" in typeStr
            and not "Edge" in typeStr
            and not "Vertex" in typeStr
        ):
            result += (
                translate("TaskCheckGeometryResults", "Area")
                + ":  "
                + str(round(shp.Area, decimals))
                + "\n"
            )
        if (
            hasattr(shp, "Volume")
            and not "Wire" in typeStr
            and not "Edge" in typeStr
            and not "Vertex" in typeStr
            and not "Face" in typeStr
        ):
            result += (
                translate("TaskCheckGeometryResults", "Volume")
                + ":  "
                + str(round(shp.Volume, decimals))
                + "\n"
            )
        if hasattr(shp, "Mass"):
            result += (
                translate("TaskCheckGeometryResults", "Mass")
                + ":  "
                + str(round(shp.Mass, decimals))
                + "\n"
            )
        if hasattr(shp, "Length"):
            result += (
                translate("TaskCheckGeometryResults", "Length")
                + ":  "
                + str(round(shp.Length, decimals))
                + "\n"
            )
        if hasattr(shp, "Curve") and hasattr(shp.Curve, "Radius"):
            result += (
                translate("TaskCheckGeometryResults", "Radius")
                + ":  "
                + str(round(shp.Curve.Radius, decimals))
                + "\n"
            )
        if hasattr(shp, "Curve") and hasattr(shp.Curve, "Center"):
            result += (
                translate("TaskCheckGeometryResults", "Curve center")
                + ":  "
                + str([round(vv, decimals) for vv in shp.Curve.Center])
                + "\n"
            )
        if hasattr(shp, "Curve") and hasattr(shp.Curve, "Continuity"):
            result += (
                translate("TaskCheckGeometryResults", "Continuity")
                + ":  "
                + str(shp.Curve.Continuity)
                + "\n"
            )
        if hasattr(shp, "CenterOfMass"):
            result += (
                translate("TaskCheckGeometryResults", "Center of mass")
                + ":  "
                + roundVector(shp.CenterOfMass, decimals)
                + "\n"
            )
        if hasattr(shp, "normalAt"):
            try:
                result += (
                    "normalAt(0):  "
                    + str([round(vv, decimals) for vv in shp.normalAt(0)])
                    + "\n"
                )
            except Exception:
                try:
                    result += (
                        "normalAt(0,0):  "
                        + str([round(vv, decimals) for vv in shp.normalAt(0, 0)])
                        + "\n"
                    )
                except Exception:
                    pass
        if hasattr(shp, "isClosed") and ("Wire" in typeStr or "Edge" in typeStr):
            result += (
                translate("TaskCheckGeometryResults", "Is closed")
                + "?  "
                + str(shp.isClosed())
                + "\n"
            )
        if hasattr(shp, "Orientation"):
            result += (
                translate("TaskCheckGeometryResults", "Orientation")
                + ":  "
                + str(shp.Orientation)
                + "\n"
            )
        if hasattr(shp, "PrincipalProperties"):
            props = shp.PrincipalProperties
            for p in props:
                if isinstance(props[p], App.Vector) or isinstance(props[p], tuple):
                    result += str(p) + ":  " + roundVector(props[p], decimals) + "\n"
                else:
                    result += str(p) + ":  " + str(props[p]) + "\n"
        if hasattr(obj, "getGlobalPlacement"):
            if obj.getGlobalPlacement() != obj.Placement:
                rpl = obj.getGlobalPlacement() * obj.Placement.inverse()
                rot = rpl.Rotation
                if hasattr(shp, "CenterOfMass"):
                    result += (
                        translate("TaskCheckGeometryResults", "Global center of mass")
                        + ":  "
                        + roundVector(rpl.multVec(shp.CenterOfMass), decimals)
                        + "\n"
                    )
                if hasattr(shp, "PrincipalProperties"):
                    props = shp.PrincipalProperties
                    for p in props:
                        if "AxisOfInertia" in p:
                            result += (
                                "Global "
                                + str(p)
                                + ":  "
                                + roundVector(rot.multVec(props[p]), decimals)
                                + "\n"
                            )
            else:
                result += (
                    translate("TaskCheckGeometryResults", "Global placement")
                    + " = "
                    + translate("TaskCheckGeometryResults", "Placement")
                )
    return result
