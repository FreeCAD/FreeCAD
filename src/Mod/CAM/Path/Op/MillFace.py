# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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
import Path.Op.PocketBase as PathPocketBase
import PathScripts.PathUtils as PathUtils
from PySide.QtCore import QT_TRANSLATE_NOOP
import numpy

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

__title__ = "CAM Mill Face Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of Mill Facing operation."
__contributors__ = "russ4262 (Russell Johnson)"


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


class ObjectFace(PathPocketBase.ObjectPocket):
    """Proxy object for Mill Facing operation."""

    @classmethod
    def propertyEnumerations(self, dataType="data"):
        """helixOpPropertyEnumerations(dataType="data")... return property enumeration lists of specified dataType.
        Args:
            dataType = 'data', 'raw', 'translated'
        Notes:
        'data' is list of internal string literals used in code
        'raw' is list of (translated_text, data_string) tuples
        'translated' is list of translated string literals
        """

        enums = {
            "BoundaryShape": [
                (translate("CAM_Pocket", "Boundbox"), "Boundbox"),
                (translate("CAM_Pocket", "Face Region"), "Face Region"),
                (translate("CAM_Pocket", "Perimeter"), "Perimeter"),
                (translate("CAM_Pocket", "Stock"), "Stock"),
            ],
        }

        if dataType == "raw":
            return enums

        data = list()
        idx = 0 if dataType == "translated" else 1

        Path.Log.debug(enums)

        for k, v in enumerate(enums):
            data.append((v, [tup[idx] for tup in enums[v]]))
        Path.Log.debug(data)

        return data

    def initPocketOp(self, obj):
        Path.Log.track()
        """initPocketOp(obj) ... create facing specific properties"""
        obj.addProperty(
            "App::PropertyEnumeration",
            "BoundaryShape",
            "Face",
            QT_TRANSLATE_NOOP("App::Property", "Shape to use for calculating Boundary"),
        )
        obj.addProperty(
            "App::PropertyBool",
            "ClearEdges",
            "Face",
            QT_TRANSLATE_NOOP(
                "App::Property", "Clear edges of surface (Only applicable to BoundBox)"
            ),
        )
        if not hasattr(obj, "ExcludeRaisedAreas"):
            obj.addProperty(
                "App::PropertyBool",
                "ExcludeRaisedAreas",
                "Face",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Exclude milling raised areas inside the face."
                ),
            )

        for n in self.propertyEnumerations():
            setattr(obj, n[0], n[1])

    def pocketInvertExtraOffset(self):
        return True

    def areaOpOnChanged(self, obj, prop):
        """areaOpOnChanged(obj, prop) ... facing specific depths calculation."""
        Path.Log.track(prop)
        if prop == "StepOver" and obj.StepOver == 0:
            obj.StepOver = 1

        # default depths calculation not correct for facing
        if prop == "Base":
            job = PathUtils.findParentJob(obj)
            if job:
                obj.OpStartDepth = job.Stock.Shape.BoundBox.ZMax

            if len(obj.Base) >= 1:
                Path.Log.debug("processing")
                sublist = []
                for i in obj.Base:
                    o = i[0]
                    for s in i[1]:
                        sublist.append(o.Shape.getElement(s))

                # If the operation has a geometry identified the Finaldepth
                # is the top of the bboundbox which includes all features.
                # Otherwise, top of part.

                obj.OpFinalDepth = Part.makeCompound(sublist).BoundBox.ZMax
            elif job:
                obj.OpFinalDepth = job.Proxy.modelBoundBox(job).ZMax

    def areaOpShapes(self, obj):
        """areaOpShapes(obj) ... return top face"""
        # Facing is done either against base objects
        self.removalshapes = []
        holeShape = None

        Path.Log.debug("depthparams: {}".format([i for i in self.depthparams]))

        if obj.Base:
            Path.Log.debug("obj.Base: {}".format(obj.Base))
            faces = []
            holes = []
            holeEnvs = []
            oneBase = [obj.Base[0][0], True]
            sub0 = getattr(obj.Base[0][0].Shape, obj.Base[0][1][0])
            minHeight = sub0.BoundBox.ZMax

            for b in obj.Base:
                for sub in b[1]:
                    shape = getattr(b[0].Shape, sub)
                    if isinstance(shape, Part.Face):
                        faces.append(shape)
                        if shape.BoundBox.ZMin < minHeight:
                            minHeight = shape.BoundBox.ZMin
                        # Limit to one model base per operation
                        if oneBase[0] is not b[0]:
                            oneBase[1] = False
                        if numpy.isclose(
                            abs(shape.normalAt(0, 0).z), 1
                        ):  # horizontal face
                            # Analyze internal closed wires to determine if raised or a recess
                            for wire in shape.Wires[1:]:
                                if obj.ExcludeRaisedAreas:
                                    ip = self.isPocket(b[0], shape, wire)
                                    if ip is False:
                                        holes.append((b[0].Shape, wire))
                                else:
                                    holes.append((b[0].Shape, wire))
                    else:
                        Path.Log.warning(
                            'The base subobject, "{0}," is not a face. Ignoring "{0}."'.format(
                                sub
                            )
                        )

            if obj.ExcludeRaisedAreas and len(holes) > 0:
                for shape, wire in holes:
                    f = Part.makeFace(wire, "Part::FaceMakerSimple")
                    env = PathUtils.getEnvelope(
                        shape, subshape=f, depthparams=self.depthparams
                    )
                    holeEnvs.append(env)
                    holeShape = Part.makeCompound(holeEnvs)

            Path.Log.debug("Working on a collection of faces {}".format(faces))
            planeshape = Part.makeCompound(faces)

        # If no base object, do planing of top surface of entire model
        else:
            planeshape = Part.makeCompound([base.Shape for base in self.model])
            Path.Log.debug("Working on a shape {}".format(obj.Label))

        # Find the correct shape depending on Boundary shape.
        Path.Log.debug("Boundary Shape: {}".format(obj.BoundaryShape))
        bb = planeshape.BoundBox

        # Apply offset for clearing edges
        offset = 0
        if obj.ClearEdges:
            offset = self.radius + 0.1

        bb.XMin = bb.XMin - offset
        bb.YMin = bb.YMin - offset
        bb.XMax = bb.XMax + offset
        bb.YMax = bb.YMax + offset

        if obj.BoundaryShape == "Boundbox":
            bbperim = Part.makeBox(
                bb.XLength,
                bb.YLength,
                1,
                FreeCAD.Vector(bb.XMin, bb.YMin, bb.ZMin),
                FreeCAD.Vector(0, 0, 1),
            )
            env = PathUtils.getEnvelope(partshape=bbperim, depthparams=self.depthparams)
            if obj.ExcludeRaisedAreas and oneBase[1]:
                includedFaces = self.getAllIncludedFaces(
                    oneBase[0], env, faceZ=minHeight
                )
                if len(includedFaces) > 0:
                    includedShape = Part.makeCompound(includedFaces)
                    includedEnv = PathUtils.getEnvelope(
                        oneBase[0].Shape,
                        subshape=includedShape,
                        depthparams=self.depthparams,
                    )
                    env = env.cut(includedEnv)
        elif obj.BoundaryShape == "Stock":
            stock = PathUtils.findParentJob(obj).Stock.Shape
            env = stock

            if obj.ExcludeRaisedAreas and oneBase[1]:
                includedFaces = self.getAllIncludedFaces(
                    oneBase[0], stock, faceZ=minHeight
                )
                if len(includedFaces) > 0:
                    stockEnv = PathUtils.getEnvelope(
                        partshape=stock, depthparams=self.depthparams
                    )
                    includedShape = Part.makeCompound(includedFaces)
                    includedEnv = PathUtils.getEnvelope(
                        oneBase[0].Shape,
                        subshape=includedShape,
                        depthparams=self.depthparams,
                    )
                    env = stockEnv.cut(includedEnv)
        elif obj.BoundaryShape == "Perimeter":
            if obj.ClearEdges:
                psZMin = planeshape.BoundBox.ZMin
                ofstShape = PathUtils.getOffsetArea(
                    planeshape, self.radius * 1.25, plane=planeshape
                )
                ofstShape.translate(
                    FreeCAD.Vector(0.0, 0.0, psZMin - ofstShape.BoundBox.ZMin)
                )
                env = PathUtils.getEnvelope(
                    partshape=ofstShape, depthparams=self.depthparams
                )
            else:
                env = PathUtils.getEnvelope(
                    partshape=planeshape, depthparams=self.depthparams
                )
        elif obj.BoundaryShape == "Face Region":
            baseShape = planeshape  # oneBase[0].Shape
            psZMin = planeshape.BoundBox.ZMin
            ofst = 0.0
            if obj.ClearEdges:
                ofst = self.tool.Diameter * 0.51
            ofstShape = PathUtils.getOffsetArea(planeshape, ofst, plane=planeshape)
            ofstShape.translate(
                FreeCAD.Vector(0.0, 0.0, psZMin - ofstShape.BoundBox.ZMin)
            )

            # Calculate custom depth params for removal shape envelope, with start and final depth buffers
            custDepthparams = self._customDepthParams(
                obj, obj.StartDepth.Value + 0.2, obj.FinalDepth.Value - 0.1
            )  # only an envelope
            ofstShapeEnv = PathUtils.getEnvelope(
                partshape=ofstShape, depthparams=custDepthparams
            )
            if obj.ExcludeRaisedAreas:
                env = ofstShapeEnv.cut(baseShape)
                env.translate(
                    FreeCAD.Vector(0.0, 0.0, -0.00001)
                )  # lower removal shape into buffer zone
            else:
                env = ofstShapeEnv

        if holeShape:
            Path.Log.debug("Processing holes and face ...")
            holeEnv = PathUtils.getEnvelope(
                partshape=holeShape, depthparams=self.depthparams
            )
            newEnv = env.cut(holeEnv)
            tup = newEnv, False, "pathMillFace"
        else:
            Path.Log.debug("Processing solid face ...")
            tup = env, False, "pathMillFace"

        self.removalshapes.append(tup)
        obj.removalshape = self.removalshapes[0][0]  # save removal shape

        return self.removalshapes

    def areaOpSetDefaultValues(self, obj, job):
        """areaOpSetDefaultValues(obj, job) ... initialize mill facing properties"""
        obj.StepOver = 50
        obj.ZigZagAngle = 45.0
        obj.ExcludeRaisedAreas = False
        obj.ClearEdges = False

        # need to overwrite the default depth calculations for facing
        if job and len(job.Model.Group) > 0:
            obj.OpStartDepth = job.Stock.Shape.BoundBox.ZMax
            obj.OpFinalDepth = job.Proxy.modelBoundBox(job).ZMax

            # If the operation has a geometry identified the Finaldepth
            # is the top of the boundbox which includes all features.
            if len(obj.Base) >= 1:
                shapes = []
                for base, subs in obj.Base:
                    for s in subs:
                        shapes.append(getattr(base.Shape, s))
                obj.OpFinalDepth = Part.makeCompound(shapes).BoundBox.ZMax

    def isPocket(self, b, f, w):
        e = w.Edges[0]
        for fi in range(0, len(b.Shape.Faces)):
            face = b.Shape.Faces[fi]
            for ei in range(0, len(face.Edges)):
                edge = face.Edges[ei]
                if e.isSame(edge):
                    if f is face:
                        # Alternative: run loop to see if all edges are same
                        pass  # same source face, look for another
                    else:
                        if face.CenterOfMass.z < f.CenterOfMass.z:
                            return True
        return False

    def getAllIncludedFaces(self, base, env, faceZ):
        """getAllIncludedFaces(base, env, faceZ)...
        Return all `base` faces extending above `faceZ` whose boundboxes overlap with the `env` boundbox."""
        included = []

        eXMin = env.BoundBox.XMin
        eXMax = env.BoundBox.XMax
        eYMin = env.BoundBox.YMin
        eYMax = env.BoundBox.YMax
        eZMin = faceZ

        def isOverlap(faceMin, faceMax, envMin, envMax):
            if faceMax > envMin:
                if faceMin <= envMax or faceMin == envMin:
                    return True
            return False

        for fi in range(0, len(base.Shape.Faces)):
            # Check all faces of `base` shape
            incl = False
            face = base.Shape.Faces[fi]
            fXMin = face.BoundBox.XMin
            fXMax = face.BoundBox.XMax
            fYMin = face.BoundBox.YMin
            fYMax = face.BoundBox.YMax
            fZMax = face.BoundBox.ZMax

            if fZMax > eZMin:
                # Include face if its boundbox overlaps envelope boundbox
                if isOverlap(fXMin, fXMax, eXMin, eXMax):  # check X values
                    if isOverlap(fYMin, fYMax, eYMin, eYMax):  # check Y values
                        incl = True
            if incl:
                included.append(face)
        return included


def SetupProperties():
    setup = PathPocketBase.SetupProperties()
    setup.append("BoundaryShape")
    setup.append("ExcludeRaisedAreas")
    setup.append("ClearEdges")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Mill Facing operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectFace(obj, name, parentJob)
    return obj
