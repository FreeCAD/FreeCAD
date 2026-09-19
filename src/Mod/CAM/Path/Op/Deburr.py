# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2018 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileCopyrightText: 2020-2021 Schildkroet
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import Part
import Path
from Path.Base.Drillable import isDrillableFace
import Path.Op.Base as PathOp
import Path.Op.EngraveBase as PathEngraveBase
import Path.Op.Util as PathOpUtil
from PathScripts import PathUtils
import math

from PySide.QtCore import QT_TRANSLATE_NOOP

__title__ = "CAM Deburr Operation"
__author__ = "sliptonic (Brad Collette), Schildkroet"
__url__ = "https://www.freecad.org"
__doc__ = "Deburr operation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ObjectDeburr(PathEngraveBase.ObjectOp):
    """Proxy class for Deburr operation."""

    def opFeatures(self, obj):
        return (
            PathOp.FeatureTool
            | PathOp.FeatureHeights
            | PathOp.FeatureStepDown
            | PathOp.FeatureBaseEdges
            | PathOp.FeatureBaseFaces
            | PathOp.FeatureCoolant
            | PathOp.FeatureBaseGeometry
            | PathOp.FeatureLinking
        )

    def initOperation(self, obj):
        """initOperation(obj) ... Initialize the operation by
        managing property creation and property editor status."""
        Path.Log.track(obj.Label)
        self.propertiesReady = False
        self.initOpProperties(obj)  # Initialize operation-specific properties

    def initOpProperties(self, obj, warn=False):
        """initOpProperties(obj) ... create operation specific properties"""
        Path.Log.track()
        self.addNewProps = []

        for prtyp, nm, grp, tt in self.opPropertyDefinitions():
            if not hasattr(obj, nm):
                obj.addProperty(prtyp, nm, grp, tt)
                self.addNewProps.append(nm)

        # Set enumeration lists for enumeration properties
        if len(self.addNewProps) > 0:
            ENUMS = self.propertyEnumerations()
            for n in ENUMS:
                if n[0] in self.addNewProps:
                    setattr(obj, n[0], n[1])
            if warn:
                newPropMsg = translate("CAM_Deburr", "New property added to")
                newPropMsg += ' "{}": {}'.format(obj.Label, self.addNewProps) + ". "
                newPropMsg += translate("CAM_Deburr", "Check default value(s).")
                FreeCAD.Console.PrintWarning(newPropMsg + "\n")

        self.propertiesReady = True

    def opPropertyDefinitions(self):
        """opPropertyDefinitions(obj) ... Store operation specific properties"""
        return [
            (
                "App::PropertyDistance",
                "Width",
                "Deburr",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "V-Bit:"
                    "\n  Control desired width of the chamfer"
                    "\n\nOther tools:"
                    "\n  Control horizontal offset"
                    "\n\nBall End and Bull Nose:"
                    "\n  Zero values of Width and ExtraDepth provides touching the shape"
                    "\n\nSet zero for horizontal face, which already have a chamfer",
                ),
            ),
            (
                "App::PropertyDistance",
                "ExtraDepth",
                "Deburr",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "V-Bit:"
                    "\n  Control additional depth with ensuring the desired width"
                    "\n\nOther tools:"
                    "\n  Control vertical offset"
                    "\n\nBall End and Bull Nose:"
                    "\n  Zero values of Width and ExtraDepth provides touching the shape",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "Direction",
                "Deburr",
                QT_TRANSLATE_NOOP("App::Property", "Direction of toolpath"),
            ),
            (
                "App::PropertyEnumeration",
                "Side",
                "Deburr",
                QT_TRANSLATE_NOOP("App::Property", "Side of base object"),
            ),
            (
                "App::PropertyIntegerConstraint",
                "EntryPoint",
                "Deburr",
                QT_TRANSLATE_NOOP("App::Property", "The segment where the toolpath starts"),
            ),
            (
                "App::PropertyBool",
                "ProcessCircles",
                "Deburr",
                QT_TRANSLATE_NOOP("App::Property", "Process round holes of horizontal faces"),
            ),
            (
                "App::PropertyBool",
                "ProcessHoles",
                "Deburr",
                QT_TRANSLATE_NOOP("App::Property", "Process holes of horizontal faces"),
            ),
            (
                "App::PropertyBool",
                "ProcessPerimeter",
                "Deburr",
                QT_TRANSLATE_NOOP("App::Property", "Process the outline of horizontal faces"),
            ),
            (
                "App::PropertyEnumeration",
                "SortingMode",
                "Sorting",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Order processing of the wires\n"
                    "\nManual - Using order from selection without sorting"
                    "\nAutomatic - Sorting wires by the nearest neighbour method, further improved with 2-opt",
                ),
            ),
            (
                "App::PropertyVectorDistance",
                "StartPoint",
                "Sorting",
                QT_TRANSLATE_NOOP("App::Property", "The start point for sorting"),
            ),
            (
                "App::PropertyVectorDistance",
                "EndPoint",
                "Sorting",
                QT_TRANSLATE_NOOP("App::Property", "The end point for sorting"),
            ),
            (
                "App::PropertyBool",
                "UseEndPoint",
                "Sorting",
                QT_TRANSLATE_NOOP("App::Property", "Use end point for sorting"),
            ),
        ]

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """opPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        # Enumeration lists for App::PropertyEnumeration properties
        enums = {
            "Direction": [
                (translate("CAM_Deburr", "CW"), "CW"),
                (translate("CAM_Deburr", "CCW"), "CCW"),
            ],  # this is the direction that the profile runs
            "Side": [
                (translate("CAM_Deburr", "Outside"), "Outside"),
                (translate("CAM_Deburr", "Inside"), "Inside"),
            ],  # this is the direction that the profile runs
            "SortingMode": [
                (translate("CAM_Deburr", "Automatic"), "Automatic"),
                (translate("CAM_Deburr", "Manual"), "Manual"),
            ],  # sorting wires
        }

        if dataType == "raw":
            return enums

        data = []
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            # data[k] = [tup[idx] for tup in v]
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def opPropertyDefaults(self, obj, job):
        """opPropertyDefaults(obj, job) ... returns a dictionary of default values
        for the operation's properties."""
        defaults = {
            "EntryPoint": (0, 0, 999999, 1),
            "ExtraDepth": 1.0,
            "Direction": "CW",
            "ProcessPerimeter": True,
            "SortingMode": "Automatic",
            "StepDown": 0.0,
            "Side": "Outside",
            "Width": 1.0,
        }

        return defaults

    def opApplyPropertyDefaults(self, obj, job, propList):
        # Set standard property defaults
        PROP_DFLTS = self.opPropertyDefaults(obj, job)
        for name in PROP_DFLTS:
            if name in propList:
                obj.clearExpression(name)
                val = PROP_DFLTS[name]
                setattr(obj, name, val)

    def opSetDefaultValues(self, obj, job):
        if self.addNewProps and self.addNewProps.__len__() > 0:
            self.opApplyPropertyDefaults(obj, job, self.addNewProps)

    def setOpEditorProperties(self, obj):
        SortingMode = 0 if obj.SortingMode == "Automatic" else 2
        obj.setEditorMode("StartPoint", SortingMode)
        obj.setEditorMode("EndPoint", SortingMode)
        obj.setEditorMode("UseEndPoint", SortingMode)

    def opOnDocumentRestored(self, obj):
        self.propertiesReady = False
        self.initOpProperties(obj, warn=True)
        self.opSetDefaultValues(obj, PathUtils.findParentJob(obj))
        self.setOpEditorProperties(obj)

    def opOnChanged(self, obj, prop):
        """opOnChanged(obj, prop) ... Called when a property changes"""
        if hasattr(self, "propertiesReady") and self.propertiesReady:
            self.setOpEditorProperties(obj)

        super().opOnChanged(obj, prop)

    def opExecute(self, obj):
        Path.Log.track(obj.Label)

        if not obj.Base:
            return

        tol = self.job.GeometryTolerance.Value or 0.01
        solids = [base.Shape for base in self.model if base.Shape.Faces]
        depth, offset = self.toolDepthAndOffset(obj.Width.Value, obj.ExtraDepth.Value, self.tool)

        Path.Log.track(obj.Label, depth, offset)

        edges = []
        wires = []
        faces = []
        for base, subsList in self.baseShapes(obj):
            for subName in subsList:
                sub = getattr(base.Shape, subName)
                if isinstance(sub, Part.Edge):
                    edges.append(sub)
                elif isinstance(sub, Part.Face):
                    faces.append(sub)

        holes = []  # inner wires of horizontal faces
        for face in faces:
            if Path.Geom.isHorizontal(face):
                outerWire, innerHoles, innerCircles = self.separateFaceWires(face, offset)
                if obj.ProcessPerimeter:
                    wires.append(outerWire)
                if obj.ProcessHoles:
                    holes.extend(innerHoles)
                if obj.ProcessCircles:
                    holes.extend(innerCircles)

            else:  # angled face
                fedges = [e for e in face.Edges if Path.Geom.isHorizontal(e)]
                wires = [Part.Wire(se) for se in Part.sortEdges(fedges)]
                wire = min(wires, key=lambda w: w.BoundBox.ZMax)
                diffz = face.BoundBox.ZMax - wire.BoundBox.ZMax
                wire.translate(FreeCAD.Vector(0, 0, diffz))
                edges.extend(wire.Edges)

        wires3d = []
        for se in Part.sortEdges(edges):
            wire = Part.Wire(se)
            if all(Path.Geom.isHorizontal(e) for e in wire.Edges):
                wires.append(wire)
            else:
                wires3d.append(wire)

        if not wires and not holes and not wires3d:
            return

        index = obj.Side == "Inside"
        owires = []
        for wire in wires:
            owires.extend(PathOpUtil.offsetWire(wire, solids, offset, tol)[index])

        for wire in holes:
            # inner wires of horizontal faces should be processed at opposite side
            candidates = PathOpUtil.offsetWire(wire, None, offset, tol)
            owires.extend(candidates[not index])

        zValues = []
        z = 0
        if obj.StepDown.Value != 0:
            while z + obj.StepDown.Value < depth:
                z += obj.StepDown.Value
                zValues.append(z)
        zValues.append(depth)
        Path.Log.track(obj.Label, depth, zValues)

        forward = obj.Direction == "CW"
        start_idx = max(0, obj.EntryPoint)

        self.buildpathocc(obj, owires, zValues, relZ=True, forward=forward, start_idx=start_idx)

        if not wires3d:
            return

        # experimental way for wires not in XY plane
        pathParams = {
            "shapes": None,
            "start": None,
            "return_end": True,
            "sort_mode": 3,
            "min_dist": 0,
            "orientation": obj.getEnumerationsOfProperty("Direction").index(obj.Direction),
            "threshold": 0,
            "retraction": obj.ClearanceHeight.Value,
            "resume_height": obj.SafeHeight.Value,
            "feedrate": self.horizFeed,
            "feedrate_v": self.vertFeed,
            "verbose": True,
            "preamble": False,
        }
        startPoint = FreeCAD.Vector()
        walloffset = offset if obj.Side == "Outside" else -offset
        for wire3d in wires3d:
            dwire3d = PathOpUtil.discretizeWire(wire3d)
            dwire3d = PathOpUtil.orientWire(dwire3d, True)
            wall = dwire3d.extrude(FreeCAD.Vector(0, 0, 10))
            owall = wall.makeOffsetShape(walloffset, tolerance=tol, join=2)

            edges = [e for e in owall.Edges if not Path.Geom.isVertical(e)]
            owire3d = Part.Wire(Part.__sortEdges__(edges))
            diffz = wire3d.BoundBox.ZMax - owire3d.BoundBox.ZMax - depth
            owire3d.translate(FreeCAD.Vector(0, 0, diffz))

            pathParams["shapes"] = [owire3d]
            pathParams["start"] = startPoint
            pp, startPoint = Path.fromShapes(**pathParams)
            self.commandlist.extend(pp.Commands)

    def toolDepthAndOffset(self, width, extraDepth, tool):
        """getOffset(width, extraDepth, tool)
        Returns offset and depth for given tool and chamfer width

        width: needed chamfer width
        extraDepth: place tool tip lower than chamfer bottom edge, but keep needed chamfer width
        tool: Part::Feature object with Proxy Path.Tool.toolbit"""

        if not hasattr(tool, "Diameter"):
            raise ValueError("Deburr requires tool with diameter\n")

        rextradepth = 0  # extra depth for ball/bull tool
        if hasattr(tool, "TipDiameter"):  # V-Bit
            toolOffset = float(tool.TipDiameter) / 2
        elif hasattr(tool, "CornerRadius"):  # Bull Nose
            radius = float(tool.CornerRadius)
            hypot = math.hypot(radius, radius)
            r = hypot - radius
            rextradepth = r / math.sqrt(2)
            toolOffset = float(tool.Diameter) / 2 - rextradepth
        elif tool.ShapeID.casefold() == "ballend":  # Ball End
            radius = float(tool.Diameter) / 2
            hypot = math.hypot(radius, radius)
            r = hypot - radius
            rextradepth = r / math.sqrt(2)
            toolOffset = float(tool.Diameter) / 2 - rextradepth
        else:  # Endmill
            toolOffset = float(tool.Diameter) / 2

        angle = float(getattr(tool, "CuttingEdgeAngle", 180))
        if Path.Geom.isRoughly(angle, 180) or Path.Geom.isRoughly(angle, 0):
            angle = 180

        tan = math.tan(math.radians(angle / 2))
        toolDepth = 0 if Path.Geom.isRoughly(tan, 0) else width / tan
        depth = toolDepth + extraDepth + rextradepth
        extraOffset = -width if angle == 180 else (extraDepth * tan)
        offset = toolOffset + extraOffset

        return depth, offset

    def separateFaceWires(self, face, offset):
        """separateFaceWires(face) ... return outerWire, innerHoles and innerCircles of face"""
        outerWire = face.OuterWire
        outerIndex = [w.hashCode() for w in face.Wires].index(outerWire.hashCode())

        innerWires = face.Wires
        del innerWires[outerIndex]

        innerHoles = []
        innerCircles = []
        for w in innerWires:
            f = Part.makeFace(w, "Part::FaceMakerSimple")
            if isDrillableFace(f, tooldiameter=2 * offset, vector=None):
                innerCircles.append(w)
            else:
                innerHoles.append(w)

        return outerWire, innerHoles, innerCircles


def SetupProperties():
    setup = []
    setup.append("Direction")
    setup.append("EntryPoint")
    setup.append("ExtraDepth")
    setup.append("ProcessHoles")
    setup.append("ProcessPerimeter")
    setup.append("Side")
    setup.append("SortingMode")
    setup.append("Width")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Deburr operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectDeburr(obj, name, parentJob)
    return obj
