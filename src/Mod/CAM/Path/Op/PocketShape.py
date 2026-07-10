# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2017 sliptonic <shopinthewoods@gmail.com>               *
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

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import Path.Op.Base as PathOp
import Path.Op.PocketBase as PathPocketBase

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")
TechDraw = LazyLoader("TechDraw", globals(), "TechDraw")
math = LazyLoader("math", globals(), "math")
PathUtils = LazyLoader("PathScripts.PathUtils", globals(), "PathScripts.PathUtils")
FeatureExtensions = LazyLoader("Path.Op.FeatureExtension", globals(), "Path.Op.FeatureExtension")

translate = FreeCAD.Qt.translate

__title__ = "CAM Pocket Shape Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Class and implementation of shape based Pocket operation."


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectPocket(PathPocketBase.ObjectPocket):
    """Proxy object for Pocket operation."""

    def areaOpFeatures(self, obj):
        return (
            super(self.__class__, self).areaOpFeatures(obj)
            | PathOp.FeatureLocations
            | PathOp.FeatureBaseEdges
        )

    def removeHoles(self, solids, face):
        """Create face from outer wire and remove collisions with solids"""
        outer_wire = face.OuterWire
        outer_face = Part.Face(outer_wire)
        translate_dist = face.BoundBox.ZLength + self.tol
        outer_face.translate(FreeCAD.Vector(0, 0, translate_dist))
        new_face = outer_face.cut(solids)
        new_face.translate(FreeCAD.Vector(0, 0, -translate_dist))

        return new_face

    def initPocketOp(self, obj):
        """initPocketOp(obj) ... setup receiver"""
        if not hasattr(obj, "UseOutline"):
            obj.addProperty(
                "App::PropertyBool",
                "UseOutline",
                "Pocket",
                QT_TRANSLATE_NOOP("App::Property", "Uses the outline of the base geometry."),
            )

        FeatureExtensions.initialize_properties(obj)
        if not hasattr(obj, "CloseOpenPaths"):
            obj.addProperty(
                "App::PropertyBool",
                "CloseOpenPaths",
                "Pocket",
                QT_TRANSLATE_NOOP(
                    "App::Property",
                    "Close open area formed by edges or vertical faces by straight line.",
                ),
            )

    def areaOpOnDocumentRestored(self, obj):
        """opOnDocumentRestored(obj) ... adds the UseOutline property if it doesn't exist."""
        self.initPocketOp(obj)

    def pocketInvertExtraOffset(self):
        return False

    def areaOpSetDefaultValues(self, obj, job):
        """areaOpSetDefaultValues(obj, job) ... set default values"""
        obj.ClearingPattern = "Offset"
        obj.StepOver = 50
        obj.Angle = 45
        obj.setEditorMode("Angle", 2)  # hide for default Offset pattern
        obj.UseOutline = False
        FeatureExtensions.set_default_property_values(obj, job)

    def areaOpShapes(self, obj):
        """areaOpShapes(obj) ... return shapes representing the solids to be removed."""
        Path.Log.track()
        # self.isDebug = True if Path.Log.getLevel(Path.Log.thisModule()) == 4 else False
        self.removalshapes = []
        avoidFeatures = list()
        self.tol = self.job.GeometryTolerance.Value or 0.01
        solids = [base.Shape for base in self.model if base.Shape.Faces]

        # Get extensions and identify faces to avoid
        extensions = FeatureExtensions.getExtensions(obj)
        for e in extensions:
            if e.avoid:
                avoidFeatures.append(e.feature)

        if obj.Base:
            Path.Log.debug("base items exist.  Processing...")
            self.horiz = []
            self.vert = []
            self.edges = []
            for base, subList in self.baseShapes(obj):
                for sub in subList:
                    if sub in avoidFeatures:
                        # skip this sub shape
                        continue
                    if "Edge" in sub and self.classifySubEdge(base, sub):
                        # edge added to list
                        continue
                    if "Face" in sub and self.classifySubFace(base, sub):
                        # face added to list
                        continue
                    Path.Log.error("Pocket does not support shape {}.{}".format(base.Label, sub))

            if self.vert:
                self.processVerticalFaces(obj, self.vert)

            # Create horizonatal faces from closed wires
            for sortEdges in Part.sortEdges(self.edges):
                wire = Part.Wire(sortEdges)
                if not wire.isClosed():
                    if obj.CloseOpenPaths:  # add staright line to close wire
                        vertexes = wire.OrderedVertexes[0].Point, wire.OrderedVertexes[-1].Point
                        e = Part.makeLine(*vertexes)
                        wire = Part.Wire(sortEdges + [e])
                    else:
                        Path.Log.error(
                            translate(
                                "Pocket_Shape",
                                "Pocke_Shape can not process open wire.\nYou can enable feature Allow Open Area",
                            )
                        )
                        continue
                self.horiz.append(Part.Face(wire))

            # Convert horizontal faces to use outline only if requested
            Path.Log.debug("UseOutline: {}".format(obj.UseOutline))
            Path.Log.debug("self.horiz: {}".format(self.horiz))
            if obj.UseOutline and self.horiz:
                self.horiz = [self.removeHoles(solids, face) for face in self.horiz]

            # Add faces for extensions
            # Note: Extension faces don't have a parent base object, so we append them directly
            self.exts = []
            for ext in extensions:
                if not ext.avoid:
                    wire = ext.getWire()
                    if wire:
                        faces = ext.getExtensionFaces(wire)
                        for f in faces:
                            self.horiz.append(f)
                            self.exts.append(f)

            # check all faces and see if they are touching/overlapping and combine and simplify
            keepOrder = getattr(obj, "SortingMode", None) == "Manual"
            self.horizontal = Path.Geom.combineHorizontalFaces(self.horiz, keepOrder=keepOrder)

            # Move all faces to final depth less buffer before extrusion
            # Small negative buffer is applied to compensate for internal significant digits/rounding issue
            if self.job.GeometryTolerance.Value == 0.0:
                buffer = 0.000001
            else:
                buffer = self.job.GeometryTolerance.Value / 10.0
            for h in self.horizontal:
                h.translate(
                    FreeCAD.Vector(0.0, 0.0, obj.FinalDepth.Value - h.BoundBox.ZMin - buffer)
                )

            # extrude all faces up to StartDepth plus buffer and those are the removal shapes
            extent = FreeCAD.Vector(0, 0, obj.StartDepth.Value - obj.FinalDepth.Value + buffer)
            self.removalshapes = [
                (face.removeSplitter().extrude(extent), False) for face in self.horizontal
            ]

        else:  # process the job base object as a whole
            Path.Log.debug("processing the whole job base object")
            self.outlines = [
                Part.Face(TechDraw.findShapeOutline(base.Shape, 1, FreeCAD.Vector(0, 0, 1)))
                for base in self.model
            ]
            stockBB = self.stock.Shape.BoundBox

            self.bodies = []
            for outline in self.outlines:
                outline.translate(FreeCAD.Vector(0, 0, stockBB.ZMin - 1))
                body = outline.extrude(FreeCAD.Vector(0, 0, stockBB.ZLength + 2))
                self.bodies.append(body)
                self.removalshapes.append((self.stock.Shape.cut(body), False))

        # Tessellate all working faces
        # for (shape, hole) in self.removalshapes:
        #    shape.tessellate(0.05)  # originally 0.1

        if self.removalshapes:
            obj.removalshape = Part.makeCompound([tup[0] for tup in self.removalshapes])

        return self.removalshapes

    def classifySubEdge(self, bs, sub):
        """classifySubFace(bs, sub)...
        Given a base and a sub-feature name, returns True
        if the sub-feature is a horizontal edge.
        """
        edge = bs.Shape.getElement(sub)
        if Path.Geom.isHorizontal(edge):
            self.edges.append(edge)
            return True
        return False

    def classifySubFace(self, bs, sub):
        """classifySubFace(bs, sub)...
        Given a base and a sub-feature name, returns True
        if the sub-feature is a horizontally or vertically oriented flat face.
        """
        face = bs.Shape.getElement(sub)

        if isinstance(face.Surface, Part.Plane):
            Path.Log.debug("type() == Part.Plane")
            if Path.Geom.isVertical(face.Surface.Axis):
                Path.Log.debug("  -isVertical()")
                # it's a flat horizontal face
                self.horiz.append(face)
                return True

            elif Path.Geom.isHorizontal(face.Surface.Axis):
                Path.Log.debug("  -isHorizontal()")
                self.vert.append(face)
                return True
            else:
                return False

        elif isinstance(face.Surface, Part.BSplineSurface):
            Path.Log.debug("face Part.BSplineSurface")
            if Path.Geom.isRoughly(face.BoundBox.ZLength, 0):
                Path.Log.debug("  flat horizontal or almost flat horizontal")
                self.horiz.append(face)
                return True

        elif isinstance(face.Surface, Part.Cylinder) and Path.Geom.isVertical(face.Surface.Axis):
            Path.Log.debug("type() == Part.Cylinder")
            # vertical cylinder wall
            if any(e.isClosed() for e in face.Edges):
                Path.Log.debug("  -e.isClosed()")
                # complete cylinder
                circle = Part.makeCircle(face.Surface.Radius, face.Surface.Center)
                disk = Part.Face(Part.Wire(circle))
                disk.translate(FreeCAD.Vector(0, 0, face.BoundBox.ZMin - disk.BoundBox.ZMin))
                self.horiz.append(disk)
                return True

            else:
                Path.Log.debug("  -none isClosed()")
                # partial cylinder wall
                self.vert.append(face)
                return True

        elif isinstance(face.Surface, Part.SurfaceOfExtrusion):
            # extrusion wall
            Path.Log.debug("type() == Part.SurfaceOfExtrusion")
            if Path.Geom.isRoughly(abs(face.Surface.Direction.z), 1.0):
                # it's a vertical face
                self.vert.append(face)
                return True
            else:
                Path.Log.error("Failed to identify vertical face from {}".format(sub))
                return False

        else:
            Path.Log.debug("  -type(face.Surface): {}".format(type(face.Surface)))
            return False

    def processVerticalFaces(self, obj, faces):
        """processVerticalFaces(self, faces) ... create horizonatal face from wall"""
        z = obj.FinalDepth.Value
        depthparams = PathUtils.depth_params(0, z, z, 0, 0, z)
        for vertCon in Path.Geom.combineConnectedShapes(faces):
            try:
                if shapeEnv := PathUtils.getEnvelope(vertCon, depthparams=depthparams):
                    self.horiz.append(shapeEnv)
            except Exception:
                # getEnvelope failed, probably this is open wall

                # try to add edge which will close area
                if obj.CloseOpenPaths:
                    # Find faces which placed on ends of the wall
                    endFaces = []
                    candidates = vertCon.Faces[:]
                    for face in candidates:
                        c = 0
                        for candidate in candidates:
                            if face == candidate:
                                continue
                            if face.BoundBox.intersect(candidate.BoundBox) and Path.Geom.isRoughly(
                                face.distToShape(candidate)[0], 0
                            ):
                                c += 1
                            if c > 1:  # face should touch only one another face
                                break
                        else:
                            endFaces.append(face)

                    # Add helper edge and try getEnvelope again
                    if len(endFaces) == 2:  # should be only two end faces
                        points = []  # farest points which should be connected
                        for face in endFaces:
                            candidates.remove(face)
                            comp = Part.Compound(candidates)
                            tPoint = face.distToShape(comp)[1][0][0]  # face touched compound here
                            ps = [(v.Point.distanceToPoint(tPoint), v.Point) for v in face.Vertexes]
                            p = sorted(ps, key=lambda tup: tup[0])[-1][1]  # farest point
                            points.append(p)

                        edge = Part.makeLine(*points)
                        newComp = Part.Compound([vertCon, edge])
                        try:
                            if shapeEnv := PathUtils.getEnvelope(newComp, depthparams=depthparams):
                                self.horiz.append(shapeEnv)
                                continue
                        except Exception:
                            Path.Log.error(
                                translate("Pocket_Shape", "Processing vertical faces was failed")
                            )
                            continue

                Path.Log.error(
                    translate(
                        "Pocket_Shape",
                        "Processing vertical faces was failed.\nYou can enable feature Allow Open Area",
                    )
                )


# Eclass


def SetupProperties():
    setup = PathPocketBase.SetupProperties()  # Add properties from PocketBase module
    setup.extend(FeatureExtensions.SetupProperties())  # Add properties from Extensions Feature

    # Add properties initialized here in PocketShape
    setup.append("UseOutline")
    setup.append("CloseOpenPaths")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Pocket operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectPocket(obj, name, parentJob)
    return obj
