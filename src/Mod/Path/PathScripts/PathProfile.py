# -*- coding: utf-8 -*-
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
import PathScripts.PathAreaOp as PathAreaOp
import PathScripts.PathLog as PathLog
import PathScripts.PathOp as PathOp
import PathScripts.PathUtils as PathUtils
import PathScripts.drillableLib as drillableLib
import math
import numpy
from PySide.QtCore import QT_TRANSLATE_NOOP

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")
DraftGeomUtils = LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")

translate = FreeCAD.Qt.translate

__title__ = "Path Profile Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = (
    "Path Profile operation based on entire model, selected faces or selected edges."
)
__contributors__ = "Schildkroet"

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


class ObjectProfile(PathAreaOp.ObjectOp):
    """Proxy object for Profile operations based on faces."""

    def areaOpFeatures(self, obj):
        """areaOpFeatures(obj) ... returns operation-specific features"""
        return PathOp.FeatureBaseFaces | PathOp.FeatureBaseEdges

    def initAreaOp(self, obj):
        """initAreaOp(obj) ... creates all profile specific properties."""
        self.propertiesReady = False
        self.initAreaOpProperties(obj)

        obj.setEditorMode("MiterLimit", 2)
        obj.setEditorMode("JoinType", 2)

    def initAreaOpProperties(self, obj, warn=False):
        """initAreaOpProperties(obj) ... create operation specific properties"""
        self.addNewProps = []

        for (propertytype, propertyname, grp, tt) in self.areaOpProperties():
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
                    "App::Property", "Maximum distance before a miter join is truncated"
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
                QT_TRANSLATE_NOOP(
                    "App::Property", "Profile holes as well as the outline"
                ),
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
                "App::PropertyFloat",
                "ExpandKerf",
                "Profile",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Expand the kerf by % of tool diameter"
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
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        PathLog.debug(enums)

        for k, v in enumerate(enums):
            # data[k] = [tup[idx] for tup in v]
            data.append((v, [tup[idx] for tup in enums[v]]))
        PathLog.debug(data)

        return data

    def areaOpPropertyDefaults(self, obj, job):
        """areaOpPropertyDefaults(obj, job) ... returns a dictionary of default values
        for the operation's properties."""
        return {
            "Direction": "CW",
            "HandleMultipleFeatures": "Collectively",
            "JoinType": "Round",
            "MiterLimit": 0.1,
            "OffsetExtra": 0.0,
            "Side": "Outside",
            "UseComp": True,
            "processCircles": False,
            "processHoles": False,
            "processPerimeter": True,
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

        obj.setEditorMode("JoinType", 2)
        obj.setEditorMode("MiterLimit", 2)  # ml
        obj.setEditorMode("Side", side)
        obj.setEditorMode("HandleMultipleFeatures", fc)
        obj.setEditorMode("processCircles", fc)
        obj.setEditorMode("processHoles", fc)
        obj.setEditorMode("processPerimeter", fc)

    def opOnChanged(self, obj, prop):
        PathLog.debug(prop)
        if prop == "ExpandKerf" and obj.ExpandKerf < 0:
            obj.ExpandKerf = 0

    def _getOperationType(self, obj):
        if len(obj.Base) == 0:
            return "Contour"

        # return first geometry type selected
        (_, subsList) = obj.Base[0]
        return subsList[0][:4]

    def areaOpOnDocumentRestored(self, obj):
        self.propertiesReady = False

        self.initAreaOpProperties(obj, warn=True)
        self.areaOpSetDefaultValues(obj, PathUtils.findParentJob(obj))
        self.setOpEditorProperties(obj)

    def areaOpOnChanged(self, obj, prop):
        """areaOpOnChanged(obj, prop) ... updates certain property visibilities depending on changed properties."""
        if prop in ["UseComp", "JoinType", "Base"]:
            if hasattr(self, "propertiesReady") and self.propertiesReady:
                self.setOpEditorProperties(obj)

    def areaOpAreaParams(self, obj, isHole):
        """areaOpAreaParams(obj, isHole) ... returns dictionary with area parameters.
        Do not overwrite."""
        params = {}
        params["Fill"] = 0
        params["Coplanar"] = 0
        params["SectionCount"] = -1

        if obj.ExpandKerf > 0:
            params.update(
                PathUtils.extraKerf(
                    tooldiameter=(self.radius * 2), extrakerf=obj.ExpandKerf
                )
            )

        PathLog.debug(params)

        offset = obj.OffsetExtra.Value  # 0.0
        if obj.UseComp:
            offset = self.radius + obj.OffsetExtra.Value
        if obj.Side == "Inside":
            offset = 0 - offset
        if isHole:
            offset = 0 - offset
        params["Offset"] = offset

        jointype = ["Round", "Square", "Miter"]
        params["JoinType"] = jointype.index(obj.JoinType)

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

        if not obj.UseComp:
            if direction == "CCW":
                params["orientation"] = 1
            else:
                params["orientation"] = 0

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
        remainingObjBaseFeatures = []
        self.isDebug = True if PathLog.getLevel(PathLog.thisModule()) == 4 else False
        self.inaccessibleMsg = translate(
            "PathProfile",
            "The selected edge(s) are inaccessible. If multiple, re-ordering selection might work.",
        )
        self.offsetExtra = obj.OffsetExtra.Value

        if self.isDebug:
            for grpNm in ["tmpDebugGrp", "tmpDebugGrp001"]:
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
                Path.Command(
                    "(Compensated Tool Path. Diameter: " + str(self.radius * 2) + ")"
                )
            )
        else:
            self.useComp = False
            self.ofstRadius = self.offsetExtra
            self.commandlist.append(Path.Command("(Uncompensated Tool Path)"))

        # Pre-process Base Geometry to process edges
        if (
            obj.Base and len(obj.Base) > 0
        ):  # The user has selected subobjects from the base.  Process each.
            shapes.extend(self._processEdges(obj, remainingObjBaseFeatures))
            PathLog.track("returned {} shapes".format(len(shapes)))

        PathLog.track(remainingObjBaseFeatures)
        if obj.Base and len(obj.Base) > 0 and not remainingObjBaseFeatures:
            # Edges were already processed, or whole model targeted.
            PathLog.track("remainingObjBaseFeatures is False")
        elif (
            remainingObjBaseFeatures and len(remainingObjBaseFeatures) > 0
        ):  # Process remaining features after edges processed above.
            for (base, subsList) in remainingObjBaseFeatures:
                holes = []
                faces = []
                faceDepths = []

                for sub in subsList:
                    shape = getattr(base.Shape, sub)
                    # only process faces here
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                        if numpy.isclose(
                            abs(shape.normalAt(0, 0).z), 1
                        ):  # horizontal face
                            PathLog.debug(abs(shape.normalAt(0, 0).z))
                            for wire in shape.Wires:
                                if wire.hashCode() == shape.OuterWire.hashCode():
                                    continue
                                holes.append((base.Shape, wire))

                        # Add face depth to list
                        faceDepths.append(shape.BoundBox.ZMin)
                    else:
                        PathLog.track()
                        ignoreSub = base.Name + "." + sub
                        msg = "Found a selected object which is not a face. Ignoring:"
                        PathLog.warning(msg + " {}".format(ignoreSub))

                for baseShape, wire in holes:
                    cont = False
                    f = Part.makeFace(wire, "Part::FaceMakerSimple")
                    drillable = drillableLib.isDrillable(baseShape, f, vector=None)
                    PathLog.debug(drillable)

                    if obj.processCircles:
                        if drillable:
                            cont = True
                    if obj.processHoles:
                        if not drillable:
                            cont = True

                    if cont:
                        shapeEnv = PathUtils.getEnvelope(
                            baseShape, subshape=f, depthparams=self.depthparams
                        )

                        if shapeEnv:
                            self._addDebugObject("HoleShapeEnvelope", shapeEnv)
                            tup = shapeEnv, True, "pathProfile"
                            shapes.append(tup)

                if faces and obj.processPerimeter:
                    if obj.HandleMultipleFeatures == "Collectively":
                        custDepthparams = self.depthparams
                        cont = True
                        profileshape = Part.makeCompound(faces)

                        try:
                            shapeEnv = PathUtils.getEnvelope(
                                profileshape, depthparams=custDepthparams
                            )
                        except Exception as ee:
                            # PathUtils.getEnvelope() failed to return an object.
                            msg = translate(
                                "PathProfile", "Unable to create path for face(s)."
                            )
                            PathLog.error(msg + "\n{}".format(ee))
                            cont = False

                        if cont:
                            self._addDebugObject("CollectCutShapeEnv", shapeEnv)
                            tup = shapeEnv, False, "pathProfile"
                            shapes.append(tup)

                    elif obj.HandleMultipleFeatures == "Individually":
                        for shape in faces:
                            custDepthparams = self.depthparams
                            self._addDebugObject("Indiv_Shp", shape)
                            shapeEnv = PathUtils.getEnvelope(
                                shape, depthparams=custDepthparams
                            )
                            if shapeEnv:
                                self._addDebugObject("IndivCutShapeEnv", shapeEnv)
                                tup = shapeEnv, False, "pathProfile"
                                shapes.append(tup)

        else:  # Try to build targets from the job models
            # No base geometry selected, so treating operation like a exterior contour operation
            PathLog.track()
            self.opUpdateDepths(obj)

            if 1 == len(self.model) and hasattr(self.model[0], "Proxy"):
                PathLog.debug("Single model processed.")
                shapes.extend(self._processEachModel(obj))
            else:
                shapes.extend(self._processEachModel(obj))

        self.removalshapes = shapes
        PathLog.debug("%d shapes" % len(shapes))

        # Delete the temporary objects
        if self.isDebug:
            if FreeCAD.GuiUp:
                import FreeCADGui

                FreeCADGui.ActiveDocument.getObject(tmpGrpNm).Visibility = False
            self.tmpGrp.purgeTouched()

        # for shape in shapes:
        #     Part.show(shape[0])
        #     print(shape)
        return shapes

    # Method to handle each model as a whole, when no faces are selected
    def _processEachModel(self, obj):
        shapeTups = []
        for base in self.model:
            if hasattr(base, "Shape"):
                env = PathUtils.getEnvelope(
                    partshape=base.Shape, subshape=None, depthparams=self.depthparams
                )
                if env:
                    shapeTups.append((env, False))
        return shapeTups

    # Edges pre-processing
    def _processEdges(self, obj, remainingObjBaseFeatures):
        PathLog.track("remainingObjBaseFeatures: {}".format(remainingObjBaseFeatures))
        shapes = []
        basewires = []
        ezMin = None
        self.cutOut = self.tool.Diameter

        for base, subsList in obj.Base:
            keepFaces = []
            edgelist = []
            for sub in subsList:
                shape = getattr(base.Shape, sub)
                # extract and process edges
                if isinstance(shape, Part.Edge):
                    edgelist.append(getattr(base.Shape, sub))
                # save faces for regular processing
                elif isinstance(shape, Part.Face):
                    keepFaces.append(sub)
            if len(edgelist) > 0:
                basewires.append((base, DraftGeomUtils.findWires(edgelist)))
                if ezMin is None or base.Shape.BoundBox.ZMin < ezMin:
                    ezMin = base.Shape.BoundBox.ZMin

            if len(keepFaces) > 0:  # save faces for returning and processing
                remainingObjBaseFeatures.append((base, keepFaces))

        PathLog.track(basewires)
        for base, wires in basewires:
            for wire in wires:
                if wire.isClosed():
                    # Attempt to profile a closed wire

                    # f = Part.makeFace(wire, 'Part::FaceMakerSimple')
                    # if planar error, Comment out previous line, uncomment the next two
                    (origWire, flatWire) = self._flattenWire(
                        obj, wire, obj.FinalDepth.Value
                    )
                    f = flatWire.Wires[0]
                    if f:
                        shapeEnv = PathUtils.getEnvelope(
                            Part.Face(f), depthparams=self.depthparams
                        )
                        if shapeEnv:
                            tup = shapeEnv, False, "pathProfile"
                            shapes.append(tup)
                    else:
                        PathLog.error(self.inaccessibleMsg)
                else:
                    # Attempt open-edges profile
                    if self.JOB.GeometryTolerance.Value == 0.0:
                        msg = self.JOB.Label + ".GeometryTolerance = 0.0. "
                        msg += "Please set to an acceptable value greater than zero."
                        PathLog.error(msg)
                    else:
                        flattened = self._flattenWire(obj, wire, obj.FinalDepth.Value)
                        zDiff = math.fabs(wire.BoundBox.ZMin - obj.FinalDepth.Value)
                        if flattened and zDiff >= self.JOB.GeometryTolerance.Value:
                            cutWireObjs = False
                            openEdges = []
                            passOffsets = [self.ofstRadius]
                            (origWire, flatWire) = flattened

                            self._addDebugObject("FlatWire", flatWire)

                            for po in passOffsets:
                                self.ofstRadius = po
                                cutShp = self._getCutAreaCrossSection(
                                    obj, base, origWire, flatWire
                                )
                                if cutShp:
                                    cutWireObjs = self._extractPathWire(
                                        obj, base, flatWire, cutShp
                                    )

                                if cutWireObjs:
                                    for cW in cutWireObjs:
                                        openEdges.append(cW)
                                else:
                                    PathLog.error(self.inaccessibleMsg)

                            if openEdges:
                                tup = openEdges, False, "OpenEdge"
                                shapes.append(tup)
                        else:
                            if zDiff < self.JOB.GeometryTolerance.Value:
                                msg = translate(
                                    "PathProfile",
                                    "Check edge selection and Final Depth requirements for profiling open edge(s).",
                                )
                                PathLog.error(msg)
                            else:
                                PathLog.error(self.inaccessibleMsg)
                    # Eif
                # Eif
            # Efor
        # Efor

        return shapes

    def _flattenWire(self, obj, wire, trgtDep):
        """_flattenWire(obj, wire)... Return a flattened version of the wire"""
        PathLog.debug("_flattenWire()")
        wBB = wire.BoundBox

        if wBB.ZLength > 0.0:
            PathLog.debug("Wire is not horizontally co-planar. Flattening it.")

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

    # Open-edges methods
    def _getCutAreaCrossSection(self, obj, base, origWire, flatWire):
        PathLog.debug("_getCutAreaCrossSection()")
        # FCAD = FreeCAD.ActiveDocument
        tolerance = self.JOB.GeometryTolerance.Value
        toolDiam = (
            2 * self.radius
        )  # self.radius defined in PathAreaOp or PathProfileBase modules
        minBfr = toolDiam * 1.25
        bbBfr = (self.ofstRadius * 2) * 1.25
        if bbBfr < minBfr:
            bbBfr = minBfr
        # fwBB = flatWire.BoundBox
        wBB = origWire.BoundBox
        minArea = (self.ofstRadius - tolerance) ** 2 * math.pi

        useWire = origWire.Wires[0]
        numOrigEdges = len(useWire.Edges)
        sdv = wBB.ZMax
        fdv = obj.FinalDepth.Value
        extLenFwd = sdv - fdv
        if extLenFwd <= 0.0:
            msg = "For open edges, verify Final Depth for this operation."
            FreeCAD.Console.PrintError(msg + "\n")
            # return False
            extLenFwd = 0.1
        WIRE = flatWire.Wires[0]
        numEdges = len(WIRE.Edges)

        # Identify first/last edges and first/last vertex on wire
        begE = WIRE.Edges[0]  # beginning edge
        endE = WIRE.Edges[numEdges - 1]  # ending edge
        blen = begE.Length
        elen = endE.Length
        Vb = begE.Vertexes[0]  # first vertex of wire
        Ve = endE.Vertexes[1]  # last vertex of wire
        pb = FreeCAD.Vector(Vb.X, Vb.Y, fdv)
        pe = FreeCAD.Vector(Ve.X, Ve.Y, fdv)

        # Obtain beginning point perpendicular points
        if blen > 0.1:
            bcp = begE.valueAt(
                begE.getParameterByLength(0.1)
            )  # point returned 0.1 mm along edge
        else:
            bcp = FreeCAD.Vector(begE.Vertexes[1].X, begE.Vertexes[1].Y, fdv)
        if elen > 0.1:
            ecp = endE.valueAt(
                endE.getParameterByLength(elen - 0.1)
            )  # point returned 0.1 mm along edge
        else:
            ecp = FreeCAD.Vector(endE.Vertexes[1].X, endE.Vertexes[1].Y, fdv)

        # Create intersection tags for determining which side of wire to cut
        (begInt, begExt, iTAG, eTAG) = self._makeIntersectionTags(
            useWire, numOrigEdges, fdv
        )
        if not begInt or not begExt:
            return False
        self.iTAG = iTAG
        self.eTAG = eTAG

        # Create extended wire boundbox, and extrude
        extBndbox = self._makeExtendedBoundBox(wBB, bbBfr, fdv)
        extBndboxEXT = extBndbox.extrude(FreeCAD.Vector(0, 0, extLenFwd))

        # Cut model(selected edges) from extended edges boundbox
        cutArea = extBndboxEXT.cut(base.Shape)
        self._addDebugObject("CutArea", cutArea)

        # Get top and bottom faces of cut area (CA), and combine faces when necessary
        topFc = []
        botFc = []
        bbZMax = cutArea.BoundBox.ZMax
        bbZMin = cutArea.BoundBox.ZMin
        for f in range(0, len(cutArea.Faces)):
            FcBB = cutArea.Faces[f].BoundBox
            if (
                abs(FcBB.ZMax - bbZMax) < tolerance
                and abs(FcBB.ZMin - bbZMax) < tolerance
            ):
                topFc.append(f)
            if (
                abs(FcBB.ZMax - bbZMin) < tolerance
                and abs(FcBB.ZMin - bbZMin) < tolerance
            ):
                botFc.append(f)
        if len(topFc) == 0:
            PathLog.error("Failed to identify top faces of cut area.")
            return False
        topComp = Part.makeCompound([cutArea.Faces[f] for f in topFc])
        topComp.translate(
            FreeCAD.Vector(0, 0, fdv - topComp.BoundBox.ZMin)
        )  # Translate face to final depth
        if len(botFc) > 1:
            # PathLog.debug('len(botFc) > 1')
            bndboxFace = Part.Face(extBndbox.Wires[0])
            tmpFace = Part.Face(extBndbox.Wires[0])
            for f in botFc:
                Q = tmpFace.cut(cutArea.Faces[f])
                tmpFace = Q
            botComp = bndboxFace.cut(tmpFace)
        else:
            botComp = Part.makeCompound(
                [cutArea.Faces[f] for f in botFc]
            )  # Part.makeCompound([CA.Shape.Faces[f] for f in botFc])
        botComp.translate(
            FreeCAD.Vector(0, 0, fdv - botComp.BoundBox.ZMin)
        )  # Translate face to final depth

        # Make common of the two
        comFC = topComp.common(botComp)

        # Determine with which set of intersection tags the model intersects
        (cmnIntArea, cmnExtArea) = self._checkTagIntersection(iTAG, eTAG, "QRY", comFC)
        if cmnExtArea > cmnIntArea:
            PathLog.debug("Cutting on Ext side.")
            self.cutSide = "E"
            self.cutSideTags = eTAG
            tagCOM = begExt.CenterOfMass
        else:
            PathLog.debug("Cutting on Int side.")
            self.cutSide = "I"
            self.cutSideTags = iTAG
            tagCOM = begInt.CenterOfMass

        # Make two beginning style(oriented) 'L' shape stops
        begStop = self._makeStop("BEG", bcp, pb, "BegStop")
        altBegStop = self._makeStop("END", bcp, pb, "BegStop")

        # Identify to which style 'L' stop the beginning intersection tag is closest,
        # and create partner end 'L' stop geometry, and save for application later
        lenBS_extETag = begStop.CenterOfMass.sub(tagCOM).Length
        lenABS_extETag = altBegStop.CenterOfMass.sub(tagCOM).Length
        if lenBS_extETag < lenABS_extETag:
            endStop = self._makeStop("END", ecp, pe, "EndStop")
            pathStops = Part.makeCompound([begStop, endStop])
        else:
            altEndStop = self._makeStop("BEG", ecp, pe, "EndStop")
            pathStops = Part.makeCompound([altBegStop, altEndStop])
        pathStops.translate(FreeCAD.Vector(0, 0, fdv - pathStops.BoundBox.ZMin))

        # Identify closed wire in cross-section that corresponds to user-selected edge(s)
        workShp = comFC
        wire = origWire
        WS = workShp.Wires
        lenWS = len(WS)
        wi = 0
        if lenWS < 3:
            # fcShp = workShp
            pass
        else:
            wi = None
            for wvt in wire.Vertexes:
                for w in range(0, lenWS):
                    twr = WS[w]
                    for v in range(0, len(twr.Vertexes)):
                        V = twr.Vertexes[v]
                        if abs(V.X - wvt.X) < tolerance:
                            if abs(V.Y - wvt.Y) < tolerance:
                                # Same vertex found.  This wire to be used for offset
                                wi = w
                                break
            # Efor

            if wi is None:
                PathLog.error(
                    "The cut area cross-section wire does not coincide with selected edge. Wires[] index is None."
                )
                return False
            else:
                PathLog.debug("Cross-section Wires[] index is {}.".format(wi))

            nWire = Part.Wire(Part.__sortEdges__(workShp.Wires[wi].Edges))
            fcShp = Part.Face(nWire)
            fcShp.translate(FreeCAD.Vector(0, 0, fdv - workShp.BoundBox.ZMin))
        # Eif

        # verify that wire chosen is not inside the physical model
        if wi > 0:  # and isInterior is False:
            PathLog.debug("Multiple wires in cut area. First choice is not 0. Testing.")
            testArea = fcShp.cut(base.Shape)

            isReady = self._checkTagIntersection(iTAG, eTAG, self.cutSide, testArea)
            PathLog.debug("isReady {}.".format(isReady))

            if isReady is False:
                PathLog.debug("Using wire index {}.".format(wi - 1))
                pWire = Part.Wire(Part.__sortEdges__(workShp.Wires[wi - 1].Edges))
                pfcShp = Part.Face(pWire)
                pfcShp.translate(FreeCAD.Vector(0, 0, fdv - workShp.BoundBox.ZMin))
                workShp = pfcShp.cut(fcShp)

            if testArea.Area < minArea:
                PathLog.debug("offset area is less than minArea of {}.".format(minArea))
                PathLog.debug("Using wire index {}.".format(wi - 1))
                pWire = Part.Wire(Part.__sortEdges__(workShp.Wires[wi - 1].Edges))
                pfcShp = Part.Face(pWire)
                pfcShp.translate(FreeCAD.Vector(0, 0, fdv - workShp.BoundBox.ZMin))
                workShp = pfcShp.cut(fcShp)
        # Eif

        # Add path stops at ends of wire
        cutShp = workShp.cut(pathStops)
        self._addDebugObject("CutShape", cutShp)

        return cutShp

    def _checkTagIntersection(self, iTAG, eTAG, cutSide, tstObj):
        PathLog.debug("_checkTagIntersection()")
        # Identify intersection of Common area and Interior Tags
        intCmn = tstObj.common(iTAG)

        # Identify intersection of Common area and Exterior Tags
        extCmn = tstObj.common(eTAG)

        # Calculate common intersection (solid model side, or the non-cut side) area with tags, to determine physical cut side
        cmnIntArea = intCmn.Area
        cmnExtArea = extCmn.Area
        if cutSide == "QRY":
            return (cmnIntArea, cmnExtArea)

        if cmnExtArea > cmnIntArea:
            PathLog.debug("Cutting on Ext side.")
            if cutSide == "E":
                return True
        else:
            PathLog.debug("Cutting on Int side.")
            if cutSide == "I":
                return True
        return False

    def _extractPathWire(self, obj, base, flatWire, cutShp):
        PathLog.debug("_extractPathWire()")

        subLoops = []
        rtnWIRES = []
        osWrIdxs = []
        subDistFactor = (
            1.0  # Raise to include sub wires at greater distance from original
        )
        fdv = obj.FinalDepth.Value
        wire = flatWire
        lstVrtIdx = len(wire.Vertexes) - 1
        lstVrt = wire.Vertexes[lstVrtIdx]
        frstVrt = wire.Vertexes[0]
        cent0 = FreeCAD.Vector(frstVrt.X, frstVrt.Y, fdv)
        cent1 = FreeCAD.Vector(lstVrt.X, lstVrt.Y, fdv)

        # Calculate offset shape, containing cut region
        ofstShp = self._getOffsetArea(obj, cutShp, False)

        # CHECK for ZERO area of offset shape
        try:
            if hasattr(ofstShp, "Area"):
                osArea = ofstShp.Area
                if osArea:  # Make LGTM parser happy
                    pass
            else:
                PathLog.error("No area to offset shape returned.")
                return []
        except Exception as ee:
            PathLog.error("No area to offset shape returned.\n{}".format(ee))
            return []

        self._addDebugObject("OffsetShape", ofstShp)

        numOSWires = len(ofstShp.Wires)
        for w in range(0, numOSWires):
            osWrIdxs.append(w)

        # Identify two vertexes for dividing offset loop
        NEAR0 = self._findNearestVertex(ofstShp, cent0)
        # min0i = 0
        min0 = NEAR0[0][4]
        for n in range(0, len(NEAR0)):
            N = NEAR0[n]
            if N[4] < min0:
                min0 = N[4]
                # min0i = n
        (w0, vi0, pnt0, _, _) = NEAR0[0]  # min0i
        near0Shp = Part.makeLine(cent0, pnt0)
        self._addDebugObject("Near0", near0Shp)

        NEAR1 = self._findNearestVertex(ofstShp, cent1)
        # min1i = 0
        min1 = NEAR1[0][4]
        for n in range(0, len(NEAR1)):
            N = NEAR1[n]
            if N[4] < min1:
                min1 = N[4]
                # min1i = n
        (w1, vi1, pnt1, _, _) = NEAR1[0]  # min1i
        near1Shp = Part.makeLine(cent1, pnt1)
        self._addDebugObject("Near1", near1Shp)

        if w0 != w1:
            PathLog.warning(
                "Offset wire endpoint indexes are not equal - w0, w1: {}, {}".format(
                    w0, w1
                )
            )

        # Debugging
        """
        if self.isDebug:
            PathLog.debug('min0i is {}.'.format(min0i))
            PathLog.debug('min1i is {}.'.format(min1i))
            PathLog.debug('NEAR0[{}] is {}.'.format(w0, NEAR0[w0]))
            PathLog.debug('NEAR1[{}] is {}.'.format(w1, NEAR1[w1]))
            PathLog.debug('NEAR0 is {}.'.format(NEAR0))
            PathLog.debug('NEAR1 is {}.'.format(NEAR1))
        """

        mainWire = ofstShp.Wires[w0]

        # Check for additional closed loops in offset wire by checking distance to iTAG or eTAG elements
        if numOSWires > 1:
            # check all wires for proximity(children) to intersection tags
            tagsComList = []
            for T in self.cutSideTags.Faces:
                tcom = T.CenterOfMass
                tv = FreeCAD.Vector(tcom.x, tcom.y, 0.0)
                tagsComList.append(tv)
            subDist = self.ofstRadius * subDistFactor
            for w in osWrIdxs:
                if w != w0:
                    cutSub = False
                    VTXS = ofstShp.Wires[w].Vertexes
                    for V in VTXS:
                        v = FreeCAD.Vector(V.X, V.Y, 0.0)
                        for t in tagsComList:
                            if t.sub(v).Length < subDist:
                                cutSub = True
                                break
                        if cutSub is True:
                            break
                    if cutSub is True:
                        sub = Part.Wire(Part.__sortEdges__(ofstShp.Wires[w].Edges))
                        subLoops.append(sub)
                # Eif

        # Break offset loop into two wires - one of which is the desired profile path wire.
        try:
            (edgeIdxs0, edgeIdxs1) = self._separateWireAtVertexes(
                mainWire, mainWire.Vertexes[vi0], mainWire.Vertexes[vi1]
            )
        except Exception as ee:
            PathLog.error("Failed to identify offset edge.\n{}".format(ee))
            return False
        edgs0 = []
        edgs1 = []
        for e in edgeIdxs0:
            edgs0.append(mainWire.Edges[e])
        for e in edgeIdxs1:
            edgs1.append(mainWire.Edges[e])
        part0 = Part.Wire(Part.__sortEdges__(edgs0))
        part1 = Part.Wire(Part.__sortEdges__(edgs1))

        # Determine which part is nearest original edge(s)
        distToPart0 = self._distMidToMid(wire.Wires[0], part0.Wires[0])
        distToPart1 = self._distMidToMid(wire.Wires[0], part1.Wires[0])
        if distToPart0 < distToPart1:
            rtnWIRES.append(part0)
        else:
            rtnWIRES.append(part1)
        rtnWIRES.extend(subLoops)

        return rtnWIRES

    def _getOffsetArea(self, obj, fcShape, isHole):
        """Get an offset area for a shape. Wrapper around
        PathUtils.getOffsetArea."""
        PathLog.debug("_getOffsetArea()")

        JOB = PathUtils.findParentJob(obj)
        tolerance = JOB.GeometryTolerance.Value
        offset = self.ofstRadius

        if isHole is False:
            offset = 0 - offset

        return PathUtils.getOffsetArea(
            fcShape, offset, plane=fcShape, tolerance=tolerance
        )

    def _findNearestVertex(self, shape, point):
        PathLog.debug("_findNearestVertex()")
        PT = FreeCAD.Vector(point.x, point.y, 0.0)

        def sortDist(tup):
            return tup[4]

        PNTS = []
        for w in range(0, len(shape.Wires)):
            WR = shape.Wires[w]
            V = WR.Vertexes[0]
            P = FreeCAD.Vector(V.X, V.Y, 0.0)
            dist = P.sub(PT).Length
            vi = 0
            pnt = P
            vrt = V
            for v in range(0, len(WR.Vertexes)):
                V = WR.Vertexes[v]
                P = FreeCAD.Vector(V.X, V.Y, 0.0)
                d = P.sub(PT).Length
                if d < dist:
                    dist = d
                    vi = v
                    pnt = P
                    vrt = V
            PNTS.append((w, vi, pnt, vrt, dist))
        PNTS.sort(key=sortDist)
        return PNTS

    def _separateWireAtVertexes(self, wire, VV1, VV2):
        PathLog.debug("_separateWireAtVertexes()")
        tolerance = self.JOB.GeometryTolerance.Value
        grps = [[], []]
        wireIdxs = [[], []]
        V1 = FreeCAD.Vector(VV1.X, VV1.Y, VV1.Z)
        V2 = FreeCAD.Vector(VV2.X, VV2.Y, VV2.Z)

        lenE = len(wire.Edges)
        FLGS = []
        for e in range(0, lenE):
            FLGS.append(0)

        chk4 = False
        for e in range(0, lenE):
            v = 0
            E = wire.Edges[e]
            fv0 = FreeCAD.Vector(E.Vertexes[0].X, E.Vertexes[0].Y, E.Vertexes[0].Z)
            fv1 = FreeCAD.Vector(E.Vertexes[1].X, E.Vertexes[1].Y, E.Vertexes[1].Z)

            if fv0.sub(V1).Length < tolerance:
                v = 1
                if fv1.sub(V2).Length < tolerance:
                    v += 3
                    chk4 = True
            elif fv1.sub(V1).Length < tolerance:
                v = 1
                if fv0.sub(V2).Length < tolerance:
                    v += 3
                    chk4 = True

            if fv0.sub(V2).Length < tolerance:
                v = 3
                if fv1.sub(V1).Length < tolerance:
                    v += 1
                    chk4 = True
            elif fv1.sub(V2).Length < tolerance:
                v = 3
                if fv0.sub(V1).Length < tolerance:
                    v += 1
                    chk4 = True
            FLGS[e] += v
        # Efor

        # PathLog.debug('_separateWireAtVertexes() FLGS: {}'.format(FLGS))

        PRE = []
        POST = []
        IDXS = []
        IDX1 = []
        IDX2 = []
        for e in range(0, lenE):
            f = FLGS[e]
            PRE.append(f)
            POST.append(f)
            IDXS.append(e)
            IDX1.append(e)
            IDX2.append(e)

        PRE.extend(FLGS)
        PRE.extend(POST)
        lenFULL = len(PRE)
        IDXS.extend(IDX1)
        IDXS.extend(IDX2)

        if chk4 is True:
            # find beginning 1 edge
            begIdx = None
            for e in range(0, lenFULL):
                f = PRE[e]
                i = IDXS[e]
                if f == 4:
                    begIdx = e
                    grps[0].append(f)
                    wireIdxs[0].append(i)
                    break
            # find first 3 edge
            for e in range(begIdx + 1, lenE + begIdx):
                f = PRE[e]
                i = IDXS[e]
                grps[1].append(f)
                wireIdxs[1].append(i)
        else:
            # find beginning 1 edge
            begIdx = None
            begFlg = False
            for e in range(0, lenFULL):
                f = PRE[e]
                if f == 1:
                    if not begFlg:
                        begFlg = True
                    else:
                        begIdx = e
                        break
            # find first 3 edge and group all first wire edges
            endIdx = None
            for e in range(begIdx, lenE + begIdx):
                f = PRE[e]
                i = IDXS[e]
                if f == 3:
                    grps[0].append(f)
                    wireIdxs[0].append(i)
                    endIdx = e
                    break
                else:
                    grps[0].append(f)
                    wireIdxs[0].append(i)
            # Collect remaining edges
            for e in range(endIdx + 1, lenFULL):
                f = PRE[e]
                i = IDXS[e]
                if f == 1:
                    grps[1].append(f)
                    wireIdxs[1].append(i)
                    break
                else:
                    wireIdxs[1].append(i)
                    grps[1].append(f)
            # Efor
        # Eif

        # Debugging
        """
        if self.isDebug:
            PathLog.debug('grps[0]: {}'.format(grps[0]))
            PathLog.debug('grps[1]: {}'.format(grps[1]))
            PathLog.debug('wireIdxs[0]: {}'.format(wireIdxs[0]))
            PathLog.debug('wireIdxs[1]: {}'.format(wireIdxs[1]))
            PathLog.debug('PRE: {}'.format(PRE))
            PathLog.debug('IDXS: {}'.format(IDXS))
        """
        return (wireIdxs[0], wireIdxs[1])

    def _makeCrossSection(self, shape, sliceZ, zHghtTrgt=False):
        """_makeCrossSection(shape, sliceZ, zHghtTrgt=None)...
        Creates cross-section objectc from shape.  Translates cross-section to zHghtTrgt if available.
        Makes face shape from cross-section object. Returns face shape at zHghtTrgt."""
        PathLog.debug("_makeCrossSection()")
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

    def _makeExtendedBoundBox(self, wBB, bbBfr, zDep):
        PathLog.debug("_makeExtendedBoundBox()")
        p1 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMin - bbBfr, zDep)
        p2 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMin - bbBfr, zDep)
        p3 = FreeCAD.Vector(wBB.XMax + bbBfr, wBB.YMax + bbBfr, zDep)
        p4 = FreeCAD.Vector(wBB.XMin - bbBfr, wBB.YMax + bbBfr, zDep)

        L1 = Part.makeLine(p1, p2)
        L2 = Part.makeLine(p2, p3)
        L3 = Part.makeLine(p3, p4)
        L4 = Part.makeLine(p4, p1)

        return Part.Face(Part.Wire([L1, L2, L3, L4]))

    def _makeIntersectionTags(self, useWire, numOrigEdges, fdv):
        PathLog.debug("_makeIntersectionTags()")
        # Create circular probe tags around perimiter of wire
        extTags = []
        intTags = []
        tagRad = self.radius / 2
        tagCnt = 0
        begInt = False
        begExt = False
        for e in range(0, numOrigEdges):
            E = useWire.Edges[e]
            LE = E.Length
            if LE > (self.radius * 2):
                nt = math.ceil(
                    LE / (tagRad * math.pi)
                )  # (tagRad * 2 * math.pi) is circumference
            else:
                nt = 4  # desired + 1
            mid = LE / nt
            spc = self.radius / 10
            for i in range(0, int(nt)):
                if i == 0:
                    if e == 0:
                        if LE > 0.2:
                            aspc = 0.1
                        else:
                            aspc = LE * 0.75
                        cp1 = E.valueAt(E.getParameterByLength(0))
                        cp2 = E.valueAt(E.getParameterByLength(aspc))
                        (intTObj, extTObj) = self._makeOffsetCircleTag(
                            cp1, cp2, tagRad, fdv, "BeginEdge[{}]_".format(e)
                        )
                        if intTObj and extTObj:
                            begInt = intTObj
                            begExt = extTObj
                else:
                    d = i * mid
                    negTestLen = d - spc
                    if negTestLen < 0:
                        negTestLen = d - (LE * 0.25)
                    posTestLen = d + spc
                    if posTestLen > LE:
                        posTestLen = d + (LE * 0.25)
                    cp1 = E.valueAt(E.getParameterByLength(negTestLen))
                    cp2 = E.valueAt(E.getParameterByLength(posTestLen))
                    (intTObj, extTObj) = self._makeOffsetCircleTag(
                        cp1, cp2, tagRad, fdv, "Edge[{}]_".format(e)
                    )
                    if intTObj and extTObj:
                        tagCnt += nt
                        intTags.append(intTObj)
                        extTags.append(extTObj)
        # tagArea = math.pi * tagRad**2 * tagCnt
        iTAG = Part.makeCompound(intTags)
        eTAG = Part.makeCompound(extTags)

        return (begInt, begExt, iTAG, eTAG)

    def _makeOffsetCircleTag(self, p1, p2, cutterRad, depth, lbl, reverse=False):
        # PathLog.debug('_makeOffsetCircleTag()')
        pb = FreeCAD.Vector(p1.x, p1.y, 0.0)
        pe = FreeCAD.Vector(p2.x, p2.y, 0.0)

        toMid = pe.sub(pb).multiply(0.5)
        lenToMid = toMid.Length
        if lenToMid == 0.0:
            # Probably a vertical line segment
            return (False, False)

        cutFactor = (
            cutterRad / 2.1
        ) / lenToMid  # = 2 is tangent to wire; > 2 allows tag to overlap wire; < 2 pulls tag away from wire
        perpE = FreeCAD.Vector(-1 * toMid.y, toMid.x, 0.0).multiply(
            -1 * cutFactor
        )  # exterior tag
        extPnt = pb.add(toMid.add(perpE))

        # make exterior tag
        eCntr = extPnt.add(FreeCAD.Vector(0, 0, depth))
        ecw = Part.Wire(Part.makeCircle((cutterRad / 2), eCntr).Edges[0])
        extTag = Part.Face(ecw)

        # make interior tag
        perpI = FreeCAD.Vector(-1 * toMid.y, toMid.x, 0.0).multiply(
            cutFactor
        )  # interior tag
        intPnt = pb.add(toMid.add(perpI))
        iCntr = intPnt.add(FreeCAD.Vector(0, 0, depth))
        icw = Part.Wire(Part.makeCircle((cutterRad / 2), iCntr).Edges[0])
        intTag = Part.Face(icw)

        return (intTag, extTag)

    def _makeStop(self, sType, pA, pB, lbl):
        # PathLog.debug('_makeStop()')
        ofstRad = self.ofstRadius
        extra = self.radius / 5.0
        lng = 0.05
        med = lng / 2.0
        shrt = lng / 5.0

        E = FreeCAD.Vector(pB.x, pB.y, 0)  # endpoint
        C = FreeCAD.Vector(pA.x, pA.y, 0)  # checkpoint

        if self.useComp is True or (self.useComp is False and self.offsetExtra != 0):
            # 'L' stop shape and edge map
            # --1--
            # |   |
            # 2   6
            # |   |
            # |   ----5----|
            # |            4
            # -----3-------|
            # positive dist in _makePerp2DVector() is CCW rotation
            p1 = E
            if sType == "BEG":
                p2 = self._makePerp2DVector(C, E, -1 * shrt)  # E1
                p3 = self._makePerp2DVector(p1, p2, ofstRad + lng + extra)  # E2
                p4 = self._makePerp2DVector(p2, p3, shrt + ofstRad + extra)  # E3
                p5 = self._makePerp2DVector(p3, p4, lng + extra)  # E4
                p6 = self._makePerp2DVector(p4, p5, ofstRad + extra)  # E5
            elif sType == "END":
                p2 = self._makePerp2DVector(C, E, shrt)  # E1
                p3 = self._makePerp2DVector(p1, p2, -1 * (ofstRad + lng + extra))  # E2
                p4 = self._makePerp2DVector(p2, p3, -1 * (shrt + ofstRad + extra))  # E3
                p5 = self._makePerp2DVector(p3, p4, -1 * (lng + extra))  # E4
                p6 = self._makePerp2DVector(p4, p5, -1 * (ofstRad + extra))  # E5
            p7 = E  # E6
            L1 = Part.makeLine(p1, p2)
            L2 = Part.makeLine(p2, p3)
            L3 = Part.makeLine(p3, p4)
            L4 = Part.makeLine(p4, p5)
            L5 = Part.makeLine(p5, p6)
            L6 = Part.makeLine(p6, p7)
            wire = Part.Wire([L1, L2, L3, L4, L5, L6])
        else:
            # 'L' stop shape and edge map
            # :
            # |----2-------|
            # 3            1
            # |-----4------|
            # positive dist in _makePerp2DVector() is CCW rotation
            p1 = E
            if sType == "BEG":
                p2 = self._makePerp2DVector(
                    C, E, -1 * (shrt + abs(self.offsetExtra))
                )  # left, shrt
                p3 = self._makePerp2DVector(p1, p2, shrt + abs(self.offsetExtra))
                p4 = self._makePerp2DVector(
                    p2, p3, (med + abs(self.offsetExtra))
                )  # FIRST POINT
                p5 = self._makePerp2DVector(
                    p3, p4, shrt + abs(self.offsetExtra)
                )  # E1 SECOND
            elif sType == "END":
                p2 = self._makePerp2DVector(
                    C, E, (shrt + abs(self.offsetExtra))
                )  # left, shrt
                p3 = self._makePerp2DVector(p1, p2, -1 * (shrt + abs(self.offsetExtra)))
                p4 = self._makePerp2DVector(
                    p2, p3, -1 * (med + abs(self.offsetExtra))
                )  # FIRST POINT
                p5 = self._makePerp2DVector(
                    p3, p4, -1 * (shrt + abs(self.offsetExtra))
                )  # E1 SECOND
            p6 = p1  # E4
            L1 = Part.makeLine(p1, p2)
            L2 = Part.makeLine(p2, p3)
            L3 = Part.makeLine(p3, p4)
            L4 = Part.makeLine(p4, p5)
            L5 = Part.makeLine(p5, p6)
            wire = Part.Wire([L1, L2, L3, L4, L5])
        # Eif
        face = Part.Face(wire)
        self._addDebugObject(lbl, face)

        return face

    def _makePerp2DVector(self, v1, v2, dist):
        p1 = FreeCAD.Vector(v1.x, v1.y, 0.0)
        p2 = FreeCAD.Vector(v2.x, v2.y, 0.0)
        toEnd = p2.sub(p1)
        factor = dist / toEnd.Length
        perp = FreeCAD.Vector(-1 * toEnd.y, toEnd.x, 0.0).multiply(factor)
        return p1.add(toEnd.add(perp))

    def _distMidToMid(self, wireA, wireB):
        mpA = self._findWireMidpoint(wireA)
        mpB = self._findWireMidpoint(wireB)
        return mpA.sub(mpB).Length

    def _findWireMidpoint(self, wire):
        midPnt = None
        dist = 0.0
        wL = wire.Length
        midW = wL / 2

        for e in range(0, len(wire.Edges)):
            E = wire.Edges[e]
            elen = E.Length
            d_ = dist + elen
            if dist < midW and midW <= d_:
                dtm = midW - dist
                midPnt = E.valueAt(E.getParameterByLength(dtm))
                break
            else:
                dist += elen
        return midPnt

    # Method to add temporary debug object
    def _addDebugObject(self, objName, objShape):
        if self.isDebug:
            newDocObj = FreeCAD.ActiveDocument.addObject(
                "Part::Feature", "tmp_" + objName
            )
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
