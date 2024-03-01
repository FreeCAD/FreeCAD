# -*- coding: utf-8 -*-
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
FeatureExtensions = LazyLoader(
    "Path.Op.FeatureExtension", globals(), "Path.Op.FeatureExtension"
)


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
        return super(self.__class__, self).areaOpFeatures(obj) | PathOp.FeatureLocations

    def initPocketOp(self, obj):
        """initPocketOp(obj) ... setup receiver"""
        if not hasattr(obj, "UseOutline"):
            obj.addProperty(
                "App::PropertyBool",
                "UseOutline",
                "Pocket",
                QT_TRANSLATE_NOOP(
                    "App::Property", "Uses the outline of the base geometry."
                ),
            )

        FeatureExtensions.initialize_properties(obj)

    def areaOpOnDocumentRestored(self, obj):
        """opOnDocumentRestored(obj) ... adds the UseOutline property if it doesn't exist."""
        self.initPocketOp(obj)

    def pocketInvertExtraOffset(self):
        return False

    def areaOpSetDefaultValues(self, obj, job):
        """areaOpSetDefaultValues(obj, job) ... set default values"""
        obj.StepOver = 100
        obj.ZigZagAngle = 45
        obj.UseOutline = False
        FeatureExtensions.set_default_property_values(obj, job)

    def areaOpShapes(self, obj):
        """areaOpShapes(obj) ... return shapes representing the solids to be removed."""
        Path.Log.track()
        self.removalshapes = []

        # self.isDebug = True if Path.Log.getLevel(Path.Log.thisModule()) == 4 else False
        self.removalshapes = []
        avoidFeatures = list()

        # Get extensions and identify faces to avoid
        extensions = FeatureExtensions.getExtensions(obj)
        for e in extensions:
            if e.avoid:
                avoidFeatures.append(e.feature)

        if obj.Base:
            Path.Log.debug("base items exist.  Processing...")
            self.horiz = []
            self.vert = []
            for (base, subList) in obj.Base:
                for sub in subList:
                    if "Face" in sub:
                        if sub not in avoidFeatures and not self.clasifySub(base, sub):
                            Path.Log.error(
                                "Pocket does not support shape {}.{}".format(
                                    base.Label, sub
                                )
                            )

            # Convert horizontal faces to use outline only if requested
            if obj.UseOutline and self.horiz:
                horiz = [Part.Face(f.Wire1) for f in self.horiz]
                self.horiz = horiz

            # Check if selected vertical faces form a loop
            if len(self.vert) > 0:
                self.vertical = Path.Geom.combineConnectedShapes(self.vert)
                self.vWires = [
                    TechDraw.findShapeOutline(shape, 1, FreeCAD.Vector(0, 0, 1))
                    for shape in self.vertical
                ]
                for wire in self.vWires:
                    w = Path.Geom.removeDuplicateEdges(wire)
                    face = Part.Face(w)
                    # face.tessellate(0.1)
                    if Path.Geom.isRoughly(face.Area, 0):
                        Path.Log.error("Vertical faces do not form a loop - ignoring")
                    else:
                        self.horiz.append(face)

            # Add faces for extensions
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
            self.horizontal = Path.Geom.combineHorizontalFaces(self.horiz)

            # Move all faces to final depth less buffer before extrusion
            # Small negative buffer is applied to compensate for internal significant digits/rounding issue
            if self.job.GeometryTolerance.Value == 0.0:
                buffer = 0.000001
            else:
                buffer = self.job.GeometryTolerance.Value / 10.0
            for h in self.horizontal:
                h.translate(
                    FreeCAD.Vector(
                        0.0, 0.0, obj.FinalDepth.Value - h.BoundBox.ZMin - buffer
                    )
                )

            # extrude all faces up to StartDepth plus buffer and those are the removal shapes
            extent = FreeCAD.Vector(
                0, 0, obj.StartDepth.Value - obj.FinalDepth.Value + buffer
            )
            self.removalshapes = [
                (face.removeSplitter().extrude(extent), False)
                for face in self.horizontal
            ]

        else:  # process the job base object as a whole
            Path.Log.debug("processing the whole job base object")
            self.outlines = [
                Part.Face(
                    TechDraw.findShapeOutline(base.Shape, 1, FreeCAD.Vector(0, 0, 1))
                )
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

    # Support methods
    def isVerticalExtrusionFace(self, face):
        fBB = face.BoundBox
        if Path.Geom.isRoughly(fBB.ZLength, 0.0):
            return False
        extr = face.extrude(FreeCAD.Vector(0.0, 0.0, fBB.ZLength))
        if hasattr(extr, "Volume"):
            if Path.Geom.isRoughly(extr.Volume, 0.0):
                return True
        return False

    def clasifySub(self, bs, sub):
        """clasifySub(bs, sub)...
        Given a base and a sub-feature name, returns True
        if the sub-feature is a horizontally oriented flat face.
        """
        face = bs.Shape.getElement(sub)

        if type(face.Surface) == Part.Plane:
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

        elif type(face.Surface) == Part.Cylinder and Path.Geom.isVertical(
            face.Surface.Axis
        ):
            Path.Log.debug("type() == Part.Cylinder")
            # vertical cylinder wall
            if any(e.isClosed() for e in face.Edges):
                Path.Log.debug("  -e.isClosed()")
                # complete cylinder
                circle = Part.makeCircle(face.Surface.Radius, face.Surface.Center)
                disk = Part.Face(Part.Wire(circle))
                disk.translate(
                    FreeCAD.Vector(0, 0, face.BoundBox.ZMin - disk.BoundBox.ZMin)
                )
                self.horiz.append(disk)
                return True

            else:
                Path.Log.debug("  -none isClosed()")
                # partial cylinder wall
                self.vert.append(face)
                return True

        elif type(face.Surface) == Part.SurfaceOfExtrusion:
            # extrusion wall
            Path.Log.debug("type() == Part.SurfaceOfExtrusion")
            # Save face to self.horiz for processing or display error
            if self.isVerticalExtrusionFace(face):
                self.vert.append(face)
                return True
            else:
                Path.Log.error("Failed to identify vertical face from {}".format(sub))

        else:
            Path.Log.debug("  -type(face.Surface): {}".format(type(face.Surface)))
            return False


# Eclass


def SetupProperties():
    setup = PathPocketBase.SetupProperties()  # Add properties from PocketBase module
    setup.extend(
        FeatureExtensions.SetupProperties()
    )  # Add properties from Extensions Feature

    # Add properties initialized here in PocketShape
    setup.append("UseOutline")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Pocket operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectPocket(obj, name, parentJob)
    return obj

    return obj
