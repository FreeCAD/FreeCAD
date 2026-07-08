# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 Schildkroet                                        *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path
import Path.Base.Drillable as Drillable
import Path.Op.Area as PathAreaOp
import Path.Op.Base as PathOp
import PathScripts.PathUtils as PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

translate = FreeCAD.Qt.translate

__title__ = "CAM Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Create a profile toolpath based on entire model, selected faces or selected edges."
__contributors__ = "Schildkroet"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectProfile(PathAreaOp.ObjectOp):
    """Proxy object for Profile operations based on faces."""

    def areaOpFeatures(self, obj):
        """areaOpFeatures(obj) ... returns operation-specific features"""
        return PathOp.FeatureBaseFaces | PathOp.FeatureBaseEdges | PathOp.FeatureBaseModels

    def initAreaOp(self, obj):
        """initAreaOp(obj) ... creates all profile specific properties."""
        self.propertiesReady = False
        self.initAreaOpProperties(obj)

        obj.setEditorMode("MiterLimit", 2)
        obj.setEditorMode("JoinType", 2)

    def execute(self, obj):
        """execute(obj) ... override to handle 3+2 transformation for Area-based operations."""
        # Call the base class execute() method which handles 3+2 transformation
        return PathOp.ObjectOp.execute(self, obj)

    def initAreaOpProperties(self, obj, warn=False):
        """initAreaOpProperties(obj) ... create operation specific properties"""
        self.addNewProps = []

        for propertytype, propertyname, grp, tt in self.areaOpProperties():
            if not hasattr(obj, propertyname):
                obj.addProperty(propertytype, propertyname, grp, tt)
                self.addNewProps.append(propertyname)

        if len(self.addNewProps) > 0:
            # Set enumeration lists for enumeration properties
            ENUMS = self.areaOpPropertyEnumerations()
            for n in ENUMS:
                if n[0] in self.addNewProps:
                    setattr(obj, n[0], n[1])
            if warn:
                newPropMsg = "New property added to"
                newPropMsg += ' "{}": {}'.format(obj.Label, self.addNewProps) + ". "
                newPropMsg += "Check its default value." + "\n"
                FreeCAD.Console.PrintWarning(newPropMsg)

        self.propertiesReady = True

    def areaOpProperties(self):
        """areaOpProperties(obj) ... returns a tuples.
        Each tuple contains property declaration information in the
        form of (prototype, name, section, tooltip)."""
        return [
            (
                "App::PropertyEnumeration",
                "Direction",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The direction that the toolpath should go around the part ClockWise (CW) or CounterClockWise (CCW)",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "HandleMultipleFeatures",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Choose how to process multiple Base Geometry features.",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "JoinType",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Controls how tool moves around corners. Default=Round",
                ),
            ),
            (
                "App::PropertyFloat",
                "MiterLimit",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Maximum distance before a miter joint is truncated"
                ),
            ),
            (
                "App::PropertyDistance",
                "OffsetExtra",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Extra value to stay away from final profile- good for roughing toolpath",
                ),
            ),
            (
                "App::PropertyBool",
                "processHoles",
                "Profile",
                QT_TRANSLATE_NOOP("App::Property", "Profile holes as well as the outline"),
            ),
            (
                "App::PropertyBool",
                "processPerimeter",
                "Profile",
                QT_TRANSLATE_NOOP("App::Property", "Profile the outline"),
            ),
            (
                "App::PropertyBool",
                "processCircles",
                "Profile",
                QT_TRANSLATE_NOOP("App::Property", "Profile round holes"),
            ),
            (
                "App::PropertyEnumeration",
                "Side",
                "Profile",
                QT_TRANSLATE_NOOP("App::Property", "Side of edge that tool should cut"),
            ),
            (
                "App::PropertyBool",
                "UseComp",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Make True, if using Cutter Radius Compensation"
                ),
            ),
            (
                "App::PropertyIntegerConstraint",
                "NumPasses",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "The number of passes to do. If more than one, requires a non-zero value for Stepover",
                ),
            ),
            (
                "App::PropertyDistance",
                "Stepover",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "If doing multiple passes, the extra offset of each additional pass",
                ),
            ),
            (
                "App::PropertyBool",
                "UseLongestEdge",
                "Start Point",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Override start point"
                    "\nShoud be used only with Individually HandleMultipleFeatures"
                    "and disabled UseStartPoint",
                ),
            ),
            (
                "App::PropertyLength",
                "RetractThreshold",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Set distance which will attempts to avoid unnecessary retractions",
                ),
            ),
            (
                "App::PropertyEnumeration",
                "SortingMode",
                "Path",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Order processing of the shapes"
                    "\nAutomatic: uses nearest neighbour algorithm to sort shapes"
                    "\nManual: uses order of shapes selection",
                ),
            ),
        ]

    @classmethod
    def areaOpPropertyEnumerations(self, dataType="data"):
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
                (translate("PathProfile", "CW"), "CW"),
                (translate("PathProfile", "CCW"), "CCW"),
            ],  # this is the direction that the profile runs
            "HandleMultipleFeatures": [
                (translate("PathProfile", "Collectively"), "Collectively"),
                (translate("PathProfile", "Individually"), "Individually"),
            ],
            "JoinType": [
                (translate("PathProfile", "Round"), "Round"),
                (translate("PathProfile", "Square"), "Square"),
                (translate("PathProfile", "Miter"), "Miter"),
            ],  # this is the direction that the Profile runs
            "Side": [
                (translate("PathProfile", "Outside"), "Outside"),
                (translate("PathProfile", "Inside"), "Inside"),
            ],  # side of profile that cutter is on in relation to direction of profile
            "SortingMode": [
                (translate("PathProfile", "Automatic"), "Automatic"),
                (translate("PathProfile", "Manual"), "Manual"),
            ],
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            # data[k] = [tup[idx] for tup in v]
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def areaOpPropertyDefaults(self, obj, job):
        """areaOpPropertyDefaults(obj, job) ... returns a dictionary of default values
        for the operation's properties."""
        return {
            "Direction": "CW",
            "HandleMultipleFeatures": "Individually",
            "JoinType": "Round",
            "MiterLimit": 0.1,
            "OffsetExtra": 0.0,
            "Side": "Outside",
            "UseComp": True,
            "processCircles": False,
            "processHoles": False,
            "processPerimeter": True,
            "Stepover": 0,
            "NumPasses": (1, 1, 99999, 1),
        }

    def areaOpApplyPropertyDefaults(self, obj, job, propList):
        # Set standard property defaults
        PROP_DFLTS = self.areaOpPropertyDefaults(obj, job)
        for n in PROP_DFLTS:
            if n in propList:
                prop = getattr(obj, n)
                val = PROP_DFLTS[n]
                setVal = False
                if hasattr(prop, "Value"):
                    if isinstance(val, int) or isinstance(val, float):
                        setVal = True
                if setVal:
                    # propVal = getattr(prop, 'Value')
                    # Need to check if `val` below should be `propVal` commented out above
                    setattr(prop, "Value", val)
                else:
                    setattr(obj, n, val)

    def areaOpSetDefaultValues(self, obj, job):
        if self.addNewProps and self.addNewProps.__len__() > 0:
            self.areaOpApplyPropertyDefaults(obj, job, self.addNewProps)

    def setOpEditorProperties(self, obj):
        """setOpEditorProperties(obj, porp) ... Process operation-specific changes to properties visibility."""
        fc = 2
        # ml = 0 if obj.JoinType == 'Miter' else 2
        side = 0 if obj.UseComp else 2
        opType = self._getOperationType(obj)

        if opType == "Contour":
            side = 2
        elif opType == "Face":
            fc = 0
        elif opType == "Edge":
            pass

        useLongestEdgeMode = (
            0 if obj.HandleMultipleFeatures == "Individually" and not obj.UseStartPoint else 2
        )
        sortingMode = 0 if obj.HandleMultipleFeatures == "Individually" else 2
        multiPassMode = 0 if obj.NumPasses > 1 else 2

        obj.setEditorMode("Stepover", multiPassMode)
        obj.setEditorMode("JoinType", 2)
        obj.setEditorMode("MiterLimit", 2)  # ml
        obj.setEditorMode("Side", side)
        obj.setEditorMode("HandleMultipleFeatures", 0)
        obj.setEditorMode("processCircles", fc)
        obj.setEditorMode("processHoles", fc)
        obj.setEditorMode("processPerimeter", fc)
        obj.setEditorMode("UseLongestEdge", useLongestEdgeMode)
        obj.setEditorMode("SortingMode", sortingMode)

    def _getOperationType(self, obj):
        if len(obj.Base) == 0:
            return "Contour"

        # return first geometry type selected
        _, subsList = obj.Base[0]
        return subsList[0][:4]

    def areaOpOnDocumentRestored(self, obj):
        self.propertiesReady = False
        self.initAreaOpProperties(obj, warn=True)
        self.areaOpSetDefaultValues(obj, PathUtils.findParentJob(obj))
        self.setOpEditorProperties(obj)

    def areaOpOnChanged(self, obj, prop):
        """areaOpOnChanged(obj, prop) ... updates certain property visibilities depending on changed properties."""
        if hasattr(self, "propertiesReady") and self.propertiesReady:
            self.setOpEditorProperties(obj)

    def areaOpAreaParams(self, obj, isHole):
        """areaOpAreaParams(obj, isHole) ... returns dictionary with area parameters.
        Do not overwrite."""
        params = {}
        params["Fill"] = 0
        params["Coplanar"] = 0
        params["SectionCount"] = -1

        offset = obj.OffsetExtra.Value  # 0.0
        num_passes = max(1, obj.NumPasses)
        stepover = obj.Stepover.Value
        if num_passes > 1 and stepover == 0:
            # This check is important because C++ code has a default value for stepover
            # if it's 0 and extra passes are requested
            num_passes = 1
            Path.Log.warning(
                "Multipass profile requires a non-zero stepover. Reducing to a single pass."
            )

        if obj.UseComp:
            offset = self.radius + obj.OffsetExtra.Value
        if obj.Side == "Inside":
            offset = 0 - offset
            stepover = -stepover
        if isHole:
            offset = 0 - offset
            stepover = -stepover

        # Modify offset and stepover to do passes from most-offset to least
        offset += stepover * (num_passes - 1)
        stepover = -stepover

        params["Offset"] = offset
        params["ExtraPass"] = num_passes - 1
        params["Stepover"] = stepover

        # Map JoinType string to AreaParams enum value
        jointype_map = {
            "Round": Path.ClipperJoinTypeRound,
            "Square": Path.ClipperJoinTypeSquare,
            "Miter": Path.ClipperJoinTypeMiter,
        }
        params["JoinType"] = jointype_map.get(obj.JoinType, Path.ClipperJoinTypeRound)

        if obj.JoinType == "Miter":
            params["MiterLimit"] = obj.MiterLimit

        if obj.SplitArcs:
            params["Explode"] = True
            params["FitArcs"] = False

        return params

    def areaOpPathParams(self, obj, isHole):
        """areaOpPathParams(obj, isHole) ... returns dictionary with path parameters.
        Do not overwrite."""
        params = {}

        # Reverse the direction for holes
        if isHole:
            direction = "CW" if obj.Direction == "CCW" else "CCW"
        else:
            direction = obj.Direction

        if direction == "CCW":
            params["orientation"] = 0
        else:
            params["orientation"] = 1

        offset = obj.OffsetExtra.Value
        if obj.UseComp:
            offset = self.radius + obj.OffsetExtra.Value
        if offset == 0.0:
            if direction == "CCW":
                params["orientation"] = 1
            else:
                params["orientation"] = 0

        if obj.NumPasses > 1:
            # Disable path sorting to ensure that offsets appear in order, from farthest offset to closest, on all layers
            params["sort_mode"] = 0

        return params

    def areaOpUseProjection(self, obj):
        """areaOpUseProjection(obj) ... returns True"""
        return True

    def opUpdateDepths(self, obj):
        if hasattr(obj, "Base") and obj.Base.__len__() == 0:
            obj.OpStartDepth = obj.OpStockZMax
            obj.OpFinalDepth = obj.OpStockZMin

    def areaOpShapes(self, obj):
        """areaOpShapes(obj) ... returns envelope for all base shapes or wires"""

        shapes = []
        self.isDebug = True if Path.Log.getLevel(Path.Log.thisModule()) == 4 else False
        self.inaccessibleMsg = translate(
            "PathProfile",
            "The selected edge(s) are inaccessible. If multiple, re-ordering selection might work.",
        )
        self.offsetExtra = obj.OffsetExtra.Value

        if self.isDebug:
            for grpNm in ("tmpDebugGrp", "tmpDebugGrp001"):
                if hasattr(FreeCAD.ActiveDocument, grpNm):
                    for go in FreeCAD.ActiveDocument.getObject(grpNm).Group:
                        FreeCAD.ActiveDocument.removeObject(go.Name)
                    FreeCAD.ActiveDocument.removeObject(grpNm)
            self.tmpGrp = FreeCAD.ActiveDocument.addObject(
                "App::DocumentObjectGroup", "tmpDebugGrp"
            )
            tmpGrpNm = self.tmpGrp.Name
        self.JOB = PathUtils.findParentJob(obj)

        if obj.UseComp:
            self.useComp = True
            self.ofstRadius = self.radius + self.offsetExtra
            self.commandlist.append(
                Path.Command("(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")")
            )
        else:
            self.useComp = False
            self.ofstRadius = self.offsetExtra
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        if obj.Base:
            # process selection
            shapes.extend(self._processBase(obj))
            Path.Log.track("returned {} shapes".format(len(shapes)))
        else:
            # no base geometry selected, so treating operation like a exterior contour operation
            Path.Log.track()
            self.opUpdateDepths(obj)
            shapes.extend(self._processEachModel())

        self.removalshapes = shapes
        Path.Log.debug("%d shapes" % len(shapes))

        # Delete the temporary objects
        if self.isDebug:
            if FreeCAD.GuiUp:
                import FreeCADGui

                FreeCADGui.ActiveDocument.getObject(tmpGrpNm).Visibility = False
            self.tmpGrp.purgeTouched()

        return shapes

    def _processEachModel(self, base=None):
        """_processEachModel() ... returns envelope of shapes without sub selection"""
        shapeTups = []
        if base:
            models = [base]
        else:
            models = self.model
        for base in models:
            if not hasattr(base, "Shape"):
                continue
            if isinstance(base.Shape, Part.Compound):
                shapes = [shape for shape in base.Shape.SubShapes]
            else:
                shapes = [base.Shape]
            for shape in shapes:
                env = PathUtils.getEnvelope(
                    partshape=shape, subshape=None, depthparams=self.depthparams
                )
                if env:
                    shapeTups.append((env, False))
        return shapeTups

    def _processBase(self, obj):
        """_preprocessBase(obj) ... returns envelope of selected shapes"""
        shapeTups = []

        self.solids = [base.Shape for base in self.model]
        self.tol = self.job.GeometryTolerance.Value or 0.01

        bases = []
        edgeslist = []
        horFaces = []
        vertFaces = []
        for base, subsList in self.baseShapes(obj):
            if subsList == ("",):
                shapeTups.extend(self._processEachModel(base))
                continue
            if base.Shape.Faces and base not in bases:
                bases.append(base)
            for subName in subsList:
                sub = getattr(base.Shape, subName)
                if isinstance(sub, Part.Edge):
                    edgeslist.append(sub)
                elif isinstance(sub, Part.Face):
                    if Path.Geom.isHorizontal(sub):
                        horFaces.append(sub)
                    else:
                        vertFaces.append(sub)

        for face in horFaces:
            for base in bases:
                if base.Shape.isInside(face.Vertexes[0].Point, self.tol, True):
                    shapeTups.extend(self._processHorFace(obj, base, face))
                    break

        # extend list of selected edges by bottom edges from vertical faces
        for face in vertFaces:
            fzMin = min(e.BoundBox.ZMin for e in face.Edges)
            bEs = [e for e in face.Edges if Path.Geom.isRoughly(e.BoundBox.ZMax, fzMin)]
            edgeslist.extend(bEs)

        for se in Part.getSortedClusters(edgeslist):
            wire = Part.Wire(Part.__sortEdges__(se))
            if wire.isClosed():
                shapeTups.extend(self._processClosedWire(obj, None, wire))
            else:
                for base in bases:
                    if any(base.Shape.isInside(e.Vertexes[0].Point, self.tol, True) for e in se):
                        shapeTups.extend(self._processOpenWireNG(obj, base, wire))
                        break
                else:
                    Path.Log.warning("Skipped open wire without base solid model")

        return shapeTups

    def _processHorFace(self, obj, base, face):
        """_processHorFace(obj, base, face) ... returns envelope of horizontal face"""
        shapeTups = []

        ohash = face.OuterWire.hashCode()
        holes = [wire for wire in face.Wires if wire.hashCode() != ohash]

        for wire in holes:
            f = Part.makeFace(wire, "Part::FaceMakerSimple")
            drillable = Drillable.isDrillable(base.Shape, f, vector=None)
            Path.Log.debug(drillable)
            if (obj.processCircles and drillable) or (obj.processHoles and not drillable):
                shapeEnv = PathUtils.getEnvelope(
                    base.Shape, subshape=f, depthparams=self.depthparams
                )
                if shapeEnv:
                    self._addDebugObject("HoleShapeEnvelope", shapeEnv)
                    shapeTups.append((shapeEnv, True, "pathProfile"))

        if obj.processPerimeter:
            try:
                shapeEnv = PathUtils.getEnvelope(face, depthparams=self.depthparams)
            except Exception as ee:
                # PathUtils.getEnvelope() failed to return an object.
                msg = translate("PathProfile", "Unable to create path for face(s).")
                Path.Log.error(msg + "\n{}".format(ee))
                shapeEnv = None

            if shapeEnv:
                for shEnv in shapeEnv.Solids:
                    # divide solids after for 'Individually'
                    self._addDebugObject("CutShapeEnv", shEnv)
                    shapeTups.append((shEnv, False, "pathProfile"))

    def _processClosedWire(self, obj, base, wire):
        """_processClosedWire(obj, base, edges) ... returns envelope of the closed wire"""
        Path.Log.track(base)
        shapeTups = []
        origWire, flatWire = self._flattenWire(obj, wire, obj.FinalDepth.Value)
        if f := flatWire.Wires[0]:
            shape = Part.Face(f)
            if obj.ExtensionOffset:
                ext = Path.Op.Util.getExtendedFaces(
                    shape, obj.ExtensionOffset.Value, self.solids, self.tol
                )
                shape = Part.Compound(ext)
            if shapeEnv := PathUtils.getEnvelope(shape, depthparams=self.depthparams):
                shapeTups.append((shapeEnv, False, "pathProfile"))
        if not shapeTups:
            Path.Log.error(self.inaccessibleMsg)

        return shapeTups

    def _processOpenWire(self, obj, base, wire):
        """_processOpenWire(obj, wire) ... returns envelope of edges forms the open wire"""
        shapeTups = []

        # remove horiontal faces for checks collisions
        shape = Part.Compound([f for s in self.solids for f in s.Faces if Path.Geom.isVertical(f)])

        goodWires = []
        passOffsets = self.areaOpAreaParams(obj, False)["Offset"]
        for po in passOffsets:
            # put here offset open wire TODO
            owire = Path.Op.Util.offsetWire(wire, base.Shape, po, forward=True, tolerance=self.tol)
            rawWires = [owire]
            while rawWires:
                wire = rawWires.pop()
                distData = wire.distToShape(shape)
                dist = distData[0]
                if dist > self.radius or Path.Geom.isRoughly(dist, self.radius, self.tol):
                    goodWires.append(wire)
                else:
                    point = distData[1][0][1]
                    circle = Part.makeCircle(self.radius, point)
                    face = Part.makeFace(circle)
                    cut = wire.cut(face)
                    ws = [Part.Wire(se) for se in Part.sortEdges(cut.Edges)]
                    rawWires.extend(ws)

        if goodWires:
            shapeTups.append((goodWires[0], goodWires, "OpenEdge"))
        else:
            Path.Log.error(self.inaccessibleMsg)

        return shapeTups

    def _flattenWire(self, obj, wire, trgtDep):
        """_flattenWire(obj, wire)... Returns a flattened version of the wire"""
        Path.Log.debug("_flattenWire()")
        wBB = wire.BoundBox

        if not Path.Geom.isRoughly(wBB.ZLength, 0):
            Path.Log.debug("Wire is not horizontally co-planar. Flattening it.")

            # Extrude non-horizontal wire
            extFwdLen = (wBB.ZLength + 2.0) * 2.0
            mbbEXT = wire.extrude(FreeCAD.Vector(0, 0, extFwdLen))

            # Create cross-section of shape and translate
            sliceZ = wire.BoundBox.ZMin + (extFwdLen / 2)
            crsectFaceShp = self._makeCrossSection(mbbEXT, sliceZ, trgtDep)
            if crsectFaceShp is not False:
                return (wire, crsectFaceShp)
            else:
                return False
        else:
            srtWire = Part.Wire(Part.__sortEdges__(wire.Edges))
            srtWire.translate(FreeCAD.Vector(0, 0, trgtDep - srtWire.BoundBox.ZMin))

        return (wire, srtWire)

    def _makeCrossSection(self, shape, sliceZ, zHghtTrgt=False):
        """_makeCrossSection(shape, sliceZ, zHghtTrgt=None)...
        Creates cross-section objectc from shape.  Translates cross-section to zHghtTrgt if available.
        Makes face shape from cross-section object. Returns face shape at zHghtTrgt."""
        Path.Log.debug("_makeCrossSection()")
        # Create cross-section of shape and translate
        wires = []
        slcs = shape.slice(FreeCAD.Vector(0, 0, 1), sliceZ)
        if len(slcs) > 0:
            for i in slcs:
                wires.append(i)
            comp = Part.Compound(wires)
            if zHghtTrgt is not False:
                comp.translate(FreeCAD.Vector(0, 0, zHghtTrgt - comp.BoundBox.ZMin))
            return comp

        return False

    # Method to add temporary debug object
    def _addDebugObject(self, objName, objShape):
        if self.isDebug:
            newDocObj = FreeCAD.ActiveDocument.addObject("Part::Feature", "tmp_" + objName)
            newDocObj.Shape = objShape
            newDocObj.purgeTouched()
            self.tmpGrp.addObject(newDocObj)


def SetupProperties():
    setup = PathAreaOp.SetupProperties()
    setup.extend([tup[1] for tup in ObjectProfile.areaOpProperties(False)])
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Profile based on faces operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectProfile(obj, name, parentJob)
    return obj
