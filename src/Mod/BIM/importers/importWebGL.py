# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2020 Travis Apple <travisapple@gmail.com>               *
# *   Copyright (c) 2025 baidakovil <baidakovil@icloud.com>                 *
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

# REFS:
# https://github.com/mrdoob/three.js/blob/master/examples/webgl_interactive_buffergeometry.html
# https://threejs.org/examples/#webgl_buffergeometry_lines
# https://forum.freecad.org/viewtopic.php?t=51245
# https://forum.freecad.org/viewtopic.php?t=29487
# https://threejs.org/examples/#webgl_raycast_sprite
#
# Params for export()
#    'colors' is of the form: {'Body': [1,0,0], 'Body001': [1,1,0], 'Body002': [1,0,1] }
#    'camera' is of the form: "PerspectiveCamera {\n  viewportMapping ADJUST_CAMERA\n  position 30.242626 -51.772324 85.63475\n  orientation -0.4146691 0.088459305 -0.90566254  4.7065201\nnearDistance 53.126431\n  farDistance 123.09125\n  aspectRatio 1\n  focalDistance 104.53851\n  heightAngle 0.78539819\n\n}"
#    The 'camera' string for the active document may be generated from: import OfflineRenderingUtils; OfflineRenderingUtils.getCamera(FreeCAD.ActiveDocument.FileName);
#
# Development reload oneliner:
# def re(): from importlib import reload;import importWebGL;reload(importWebGL);o=FreeCAD.getDocument("YourDocName");importWebGL.export([o.getObject("YourBodyName")],u"C:/path/to/your/file.htm");

## @package importWebGL
#  \ingroup ARCH
#  \brief FreeCAD WebGL Exporter
#
#  This module provides tools to export HTML files containing the
#  exported objects in WebGL format and a simple three.js-based viewer.
#  Tests are provided in src/Mod/BIM/bimtests/TestWebGLExport.py.
#  The template is provided in src/Mod/BIM/Resources/templates/webgl_export_template.html.

"""FreeCAD WebGL Exporter"""

import json
import os
import textwrap
from builtins import open as pyopen
from typing_extensions import NotRequired, TypedDict

import numpy as np

import FreeCAD
import Draft
import Mesh
import OfflineRenderingUtils
import Part
from draftutils import params

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtWidgets
    from draftutils.translate import translate
else:
    FreeCADGui = None

    def translate(ctxt, txt):
        return txt


disableCompression = False  # Compress object data before sending to JS
base = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!#$%&()*+-:;/=>?@[]^_,.{|}~`"  # safe str chars for js in all cases
baseFloat = ",.-0123456789"
threejs_version = "0.172.0"


def getHTMLTemplate():
    """Returns the HTML template from external file.
    The custom template path can be set in the Preferences.
    Returns None if no valid template is found.
    """

    def try_read_template(path, description):
        """Helper function to safely read a template file."""
        if not os.path.isfile(path):
            FreeCAD.Console.PrintWarning(
                f"{description.capitalize()} file '{path}' does not "
                "exist or is not a file.\n"
            )
            return None
        try:
            with open(path, "r", encoding="utf-8") as f:
                return f.read()
        except Exception as e:
            FreeCAD.Console.PrintWarning(
                f"Failed to read {description} file '{path}' "
                f"due to: {str(e)}\n"
            )
            return None

    using_custom_template = params.get_param(
        "useCustomWebGLExportTemplate", path="Mod/BIM"
    )

    # Try to use custom template if enabled
    if using_custom_template:
        custom_path = params.get_param(
            "WebGLTemplateCustomPath", path="Mod/BIM"
        )
        custom_content = try_read_template(
            custom_path, "custom WebGL template"
        )
        if custom_content:
            FreeCAD.Console.PrintMessage(
                f"Using custom template file '{custom_path}'.\n"
            )
            return custom_content
        else:
            # Custom template failed - ask user or auto-fallback
            if not FreeCADGui:
                # In non-GUI mode, cancel export when custom template fails
                FreeCAD.Console.PrintError(
                    f"Export cancelled: Custom template '{custom_path}' "
                    "not available.\n"
                )
                return None

            # In GUI mode, ask the user
            message = translate(
                "BIM",
                "Custom WebGL template file '{}' could not be read.\n\n"
                "Do you want to proceed using the default template?",
            ).format(custom_path)

            reply = QtWidgets.QMessageBox.question(
                FreeCADGui.getMainWindow(),
                translate("BIM", "WebGL Template Not Found"),
                message,
                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                QtWidgets.QMessageBox.Yes,
            )

            if reply != QtWidgets.QMessageBox.Yes:
                # User chose not to proceed - return None to indicate failure
                FreeCAD.Console.PrintError(
                    f"Export cancelled: Custom template '{custom_path}' "
                    "not available.\n"
                )
                return None

    # Try to use default template
    default_template_path = os.path.join(
        FreeCAD.getResourceDir(),
        "Mod",
        "BIM",
        "Resources",
        "templates",
        "webgl_export_template.html",
    )
    default_content = try_read_template(
        default_template_path, "default WebGL template"
    )
    if default_content:
        FreeCAD.Console.PrintMessage(
            f"Using default template file '{default_template_path}'.\n"
        )
        return default_content

    # No template available - export cannot proceed
    if FreeCADGui:
        # In GUI mode, show Qt error dialog
        message = translate(
            "BIM",
            "The default WebGL export template is not available at path:"
            " {}\n\nPlease check your FreeCAD installation or provide a "
            "custom template under menu Preferences -> Import-Export -> WebGL.",
        ).format(default_template_path)

        # Use getMainWindow() as parent following FreeCAD patterns
        parent = FreeCADGui.getMainWindow()
        title = translate("BIM", "WebGL Export Template Error")

        QtWidgets.QMessageBox.critical(parent, title, message)
    else:
        # In headless mode, print to console
        FreeCAD.Console.PrintError(
            "Default WebGL export template not available at"
            f"path: {default_template_path}\n"
        )

    return None


def export(
    exportList, filename: str, colors: dict[str, str] | None = None, camera: str | None = None
) -> bool:
    """Exports objects to an html file.

    Returns:
        bool: True if export was successful, False if not (particularly,
        False if no template was available).
    """

    # Check template availability first, before any processing
    html = getHTMLTemplate()
    if html is None:
        # No template available - export failed
        return False

    global disableCompression, base, baseFloat

    data = {"camera": {}, "file": {}, "objects": []}

    populate_camera(data["camera"], camera)

    # Take the objects out of groups
    objectslist = Draft.get_group_contents(exportList, walls=True, addgroups=False)
    # objectslist = Arch.pruneIncluded(objectslist)

    for obj in objectslist:
        # Pull all obj data before we dig down the links
        label = obj.Label
        color, opacity = get_view_properties(obj, label, colors)

        validObject = False
        if obj.isDerivedFrom("Mesh::Feature"):
            mesh = obj.Mesh
            validObject = True
        if obj.isDerivedFrom("Part::Feature"):
            objShape = obj.Shape
            validObject = True
        if obj.isDerivedFrom("App::Link"):
            linkPlacement = obj.LinkPlacement
            while True:  # drill down to get to the actual obj
                if obj.isDerivedFrom("App::Link"):
                    if obj.ViewObject.OverrideMaterial:
                        color = Draft.getrgb(
                            obj.ViewObject.ShapeMaterial.DiffuseColor, testbw=False
                        )
                    obj = obj.LinkedObject
                    if hasattr(obj, "__len__"):
                        FreeCAD.Console.PrintMessage(f"{label}: Sub-Links are Unsupported.\n")
                        break
                elif obj.isDerivedFrom("Part::Feature"):
                    objShape = obj.Shape.copy(False)
                    objShape.Placement = linkPlacement
                    validObject = True
                    break
                elif obj.isDerivedFrom("Mesh::Feature"):
                    mesh = obj.Mesh.copy()
                    mesh.Placement = linkPlacement
                    validObject = True
                    break

        if not validObject:
            continue

        objdata = {
            "name": label,
            "color": color,
            "opacity": opacity,
            "verts": "",
            "facets": "",
            "wires": [],
            "faceColors": [],
            "facesToFacets": [],
            "floats": [],
        }

        if obj.isDerivedFrom("Part::Feature"):
            deviation = 0.5
            if FreeCADGui and hasattr(obj.ViewObject, "Deviation"):
                deviation = obj.ViewObject.Deviation

                # obj.ViewObject.DiffuseColor is length=1 when all faces are the same color, length=len(faces) for when they're not
                if len(obj.ViewObject.DiffuseColor) == len(objShape.Faces):
                    for fc in obj.ViewObject.DiffuseColor:
                        objdata["faceColors"].append(Draft.getrgb(fc, testbw=False))

            # get verts and facets for ENTIRE object
            shapeData = objShape.tessellate(deviation)
            mesh = Mesh.Mesh(shapeData)

            if len(objShape.Faces) > 1:
                # Map each Facet created by tessellate() to a Face so that it can be colored correctly using faceColors
                # This is done by matching the results of a tessellate() on EACH FACE to the overall tessellate stored in shapeData
                # if there is any error in matching these two then we display the whole object as one face and forgo the face colors
                for f in objShape.Faces:
                    faceData = f.tessellate(deviation)
                    found = True
                    # face verts. List of type Vector()
                    for fv in range(len(faceData[0])):
                        found = False
                        for sv in range(len(shapeData[0])):  # shape verts
                            # do not use isEqual() here
                            if faceData[0][fv] == shapeData[0][sv]:
                                # replace with the index of shapeData[0]
                                faceData[0][fv] = sv
                                found = True
                                break
                        if not found:
                            break
                    if not found:
                        FreeCAD.Console.PrintMessage("Facet to Face Mismatch.\n")
                        objdata["facesToFacets"] = []
                        break

                    # map each of the face facets to the shape facets and make a list of shape facet indices that belong to this face
                    facetList = []
                    for ff in faceData[1]:  # face facets
                        found = False
                        for sf in range(len(shapeData[1])):  # shape facets
                            if (
                                faceData[0][ff[0]] in shapeData[1][sf]
                                and faceData[0][ff[1]] in shapeData[1][sf]
                                and faceData[0][ff[2]] in shapeData[1][sf]
                            ):
                                facetList.append(sf)
                                found = True
                                break
                        if not found:
                            break
                    if not found:
                        FreeCAD.Console.PrintMessage("Facet List Mismatch.\n")
                        objdata["facesToFacets"] = []
                        break

                    if not disableCompression:
                        facetList = baseEncode(facetList)

                    objdata["facesToFacets"].append(facetList)

            wires = []  # Add wires
            for f in objShape.Faces:
                for w in f.Wires:
                    wo = Part.Wire(Part.__sortEdges__(w.Edges))
                    # use strings to avoid 0.00001 written as 1e-05
                    wire = []
                    for v in wo.discretize(QuasiDeflection=0.005):
                        wire.extend([f"{v.x:.5f}", f"{v.y:.5f}", f"{v.z:.5f}"])
                    wires.append(wire)

            if not disableCompression:
                wires, objdata["floats"] = compress_wires(wires, objdata["floats"])
            objdata["wires"] = wires

        vIndex = {}
        verts = []
        for p in mesh.Points:
            vIndex[p.Index] = p.Index
            verts.extend([f"{p.Vector.x:.5f}", f"{p.Vector.y:.5f}", f"{p.Vector.z:.5f}"])

        facets = [vIndex[i] for f in mesh.Facets for i in f.PointIndices]

        if not disableCompression:
            verts, objdata["floats"] = compress_verts(verts, objdata["floats"])
            objdata["floats"] = compress_floats(objdata["floats"])
            facets = baseEncode(facets)
            verts = baseEncode(verts)

        objdata["facets"] = facets
        objdata["verts"] = verts

        data["objects"].append(objdata)

    html = html.replace("$pagetitle", FreeCAD.ActiveDocument.Label)
    version = FreeCAD.Version()
    html = html.replace("$version", f"{version[0]}.{version[1]}.{version[2]}")

    # Remove data compression in JS
    data["compressed"] = not disableCompression
    data["base"] = base
    data["baseFloat"] = baseFloat

    html = html.replace("$data", json.dumps(data, separators=(",", ":")))  # Shape Data
    html = html.replace("$threejs_version", threejs_version)

    with pyopen(filename, "w", encoding="utf-8") as outfile:
        outfile.write(html)
    FreeCAD.Console.PrintMessage(translate("Arch", "Successfully written") + f" {filename}\n")
    return True


def get_view_properties(obj, label: str, colors: dict[str, str] | None) -> tuple[str, float]:
    """Get the color and opacity of the object"""
    color = "#cccccc"
    opacity = 1.0
    if FreeCADGui and hasattr(obj.ViewObject, "ShapeColor"):
        color = Draft.getrgb(obj.ViewObject.ShapeColor, testbw=False)
        opacity = int((100 - obj.ViewObject.Transparency) / 5) / 20  # 0>>1 with step of 0.05
    elif colors:
        if label in colors:
            color = Draft.getrgb(colors[label], testbw=False)
    return color, opacity


class CameraDict(TypedDict):
    """Dictionary for camera contents"""

    type: NotRequired[str]
    focalDistance: NotRequired[str]
    position_x: NotRequired[str]
    position_y: NotRequired[str]
    position_z: NotRequired[str]


def populate_camera(data: CameraDict, camera: str | None):
    if not FreeCADGui and not camera:
        camera = OfflineRenderingUtils.getCamera(FreeCAD.ActiveDocument.FileName)

    if camera:
        # REF: src/Mod/BIM/OfflineRenderingUtils.py
        camnode = OfflineRenderingUtils.getCoinCamera(camera)
        cameraPosition = camnode.position.getValue().getValue()
        data["type"] = "Orthographic"
        if "PerspectiveCamera" in camera:
            data["type"] = "Perspective"
        data["focalDistance"] = camnode.focalDistance.getValue()
        data["position_x"] = cameraPosition[0]
        data["position_y"] = cameraPosition[1]
        data["position_z"] = cameraPosition[2]
    else:
        v = FreeCADGui.ActiveDocument.ActiveView
        data["type"] = v.getCameraType()
        data["focalDistance"] = v.getCameraNode().focalDistance.getValue()
        data["position_x"] = v.viewPosition().Base.x
        data["position_y"] = v.viewPosition().Base.y
        data["position_z"] = v.viewPosition().Base.z


def compress_floats(floats: list[str]) -> str:
    """Compress floats to base 90

    Use ratio of 7x base13 to 4x base90 because 13^7 ~ 90^4
    """
    fullstr = json.dumps(floats, separators=(",", ":"))
    fullstr = fullstr.replace("[", "").replace("]", "").replace('"', "")
    floatStr = ""
    baseFloatCt = len(baseFloat)
    baseCt = len(base)
    for fs in range(0, len(fullstr), 7):  # chunks of 7 chars, skip the first one
        str7 = fullstr[fs : (fs + 7)]
        quotient = 0
        for s in range(len(str7)):
            quotient += baseFloat.find(str7[s]) * pow(baseFloatCt, (6 - s))
        for v in range(4):
            floatStr += base[quotient % baseCt]
            quotient = int(quotient / baseCt)
    return floatStr


def compress_wires(wires: list[list[str]], floats: list[str]) -> tuple[list[list[str]], list[str]]:
    """
    Create floats list to compress wires being written into the JS
    """
    lengths = []
    for w in wires:
        lengths.append(len(w))
        floats.extend(w)

    float_arr, all_wires = np.unique(floats, return_inverse=True)
    wire_arrays = np.array_split(all_wires, np.cumsum(lengths[:-1]))
    return [baseEncode(w.tolist()) for w in wire_arrays], float_arr.tolist()


def compress_verts(verts: list[str], floats: list[str]) -> tuple[list[int], list[str]]:
    """
    Create floats list to compress verts and wires being written into the JS
    """
    floats_v, ind, verts_v = np.unique(verts, return_index=True, return_inverse=True)

    # Reorder as np.unique orders the resulting array (needed for facet matching)
    floats_v = floats_v[ind.argsort()]
    reindex = dict(zip(ind.argsort(), np.arange(ind.size)))
    verts_v = np.vectorize(lambda entry: reindex[entry])(verts_v)

    # Get repeated indexes already existing from previous steps
    v_in_w = np.nonzero(np.isin(floats_v, floats))[0]
    w_in_v = np.nonzero(np.isin(floats, floats_v))[0]
    v_in_w2 = np.where(~np.isin(floats_v, floats))

    # Order values the same
    v_in_w = v_in_w[floats_v[v_in_w].argsort()]
    w_in_v = w_in_v[np.array(floats)[w_in_v].argsort()]

    # Replace repeated indexes that exist in floats
    new_index = len(floats)
    verts_v += new_index
    for vw, wv in zip(v_in_w + new_index, w_in_v):
        verts_v[verts_v == vw] = wv

    # Remove indexes of repeated entries in floats_v
    for vw in (v_in_w + new_index)[v_in_w.argsort()][::-1]:
        verts_v[verts_v > vw] -= 1

    return verts_v.tolist(), np.concatenate([floats, floats_v[v_in_w2]]).tolist()


def baseEncode(arr: list[int]) -> str:
    """Compresses an array of ints into a base90 string"""

    global base
    if len(arr) == 0:
        return ""

    longest = 0
    output = []
    baseCt = len(base)
    for v in range(len(arr)):
        buffer = ""
        quotient = arr[v]
        while True:
            buffer += base[quotient % baseCt]
            quotient = int(quotient / baseCt)
            if quotient == 0:
                break
        output.append(buffer)
        if len(buffer) > longest:
            longest = len(buffer)
    output = [("{:>" + str(longest) + "}").format(x) for x in output]  # pad each element
    return str(longest) + ("").join(output)
