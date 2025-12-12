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

    def removeHoles(self, solid, face, tolerance=1e-6):
        """removeHoles(solid, face, tolerance) ... Remove hole wires from a face, keeping outer wire and boss wires.

        Uses a cross-section algorithm: sections the solid slightly above the face level.
        Wires that appear in the section are bosses (material above).
        Wires that don't appear are holes (voids).

        Args:
            solid: The parent solid object
            face: The face to process
            tolerance: Distance tolerance for comparisons

        Returns:
            New face with outer wire and boss wires only
        """
        outer_wire = face.OuterWire
        candidate_wires = [w for w in face.Wires if not w.isSame(outer_wire)]

        if not candidate_wires:
            return face

        boss_wires = []

        try:
            # Create cutting plane from outer wire, offset above face by tolerance
            cutting_plane = Part.Face(outer_wire)
            cutting_plane.translate(FreeCAD.Vector(0, 0, tolerance))

            # Section the solid
            section = solid.Shape.section(cutting_plane)

            if hasattr(section, "Edges") and section.Edges:
                # Translate section edges back to face level
                translated_edges = []
                for edge in section.Edges:
                    translated_edge = edge.copy()
                    translated_edge.translate(FreeCAD.Vector(0, 0, -tolerance))
                    translated_edges.append(translated_edge)

                # Build closed wires from edges
                edge_groups = Part.sortEdges(translated_edges)
                all_section_wires = []

                for edge_list in edge_groups:
                    try:
                        wire = Part.Wire(edge_list)
                        if wire.isClosed():
                            all_section_wires.append(wire)
                    except Exception:
                        # ignore any wires that can't be built
                        pass

                # Filter out outer wire, keep remaining as boss wires
                for wire in all_section_wires:
                    if not wire.isSame(outer_wire):
                        length_diff = abs(wire.Length - outer_wire.Length)
                        if length_diff > tolerance:
                            boss_wires.append(wire)

        except Exception as e:
            Path.Log.error("removeHoles: Section algorithm failed: {}".format(e))
            boss_wires = candidate_wires

        # Construct new face with outer wire and boss wires
        wire_compound = Part.makeCompound([outer_wire] + boss_wires)
        new_face = Part.makeFace(wire_compound, "Part::FaceMakerBullseye")

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

    def areaOpOnDocumentRestored(self, obj):
        """opOnDocumentRestored(obj) ... adds the UseOutline property if it doesn't exist."""
        self.initPocketOp(obj)

    def pocketInvertExtraOffset(self):
        return False

    def areaOpSetDefaultValues(self, obj, job):
        """areaOpSetDefaultValues(obj, job) ... set default values"""
        obj.ClearingPattern = "Offset"
        obj.StepOver = 50
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
            for base, subList in obj.Base:
                for sub in subList:
                    if "Face" in sub:
                        if sub not in avoidFeatures and not self.classifySub(base, sub):
                            Path.Log.error(
                                "Pocket does not support shape {}.{}".format(base.Label, sub)
                            )

            # Convert horizontal faces to use outline only if requested
            Path.Log.debug("UseOutline: {}".format(obj.UseOutline))
            Path.Log.debug("self.horiz: {}".format(self.horiz))
            if obj.UseOutline and self.horiz:
                horiz = [self.removeHoles(base, face) for (face, base) in self.horiz]
                self.horiz = horiz
            else:
                # Extract just the faces from the tuples for further processing
                self.horiz = [face for (face, base) in self.horiz]

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
            self.horizontal = Path.Geom.combineHorizontalFaces(self.horiz)

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

    def classifySub(self, bs, sub):
        """classifySub(bs, sub)...
        Given a base and a sub-feature name, returns True
        if the sub-feature is a horizontally or vertically oriented flat face.
        """
        face = bs.Shape.getElement(sub)

        if isinstance(face.Surface, Part.Plane):
            Path.Log.debug("type() == Part.Plane")
            if Path.Geom.isVertical(face.Surface.Axis):
                Path.Log.debug("  -isVertical()")
                # it's a flat horizontal face
                self.horiz.append((face, bs))
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
                self.horiz.append((face, bs))
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
                self.horiz.append((disk, bs))
                return True

            else:
                Path.Log.debug("  -none isClosed()")
                # partial cylinder wall
                self.vert.append(face)
                return True

        elif isinstance(face.Surface, Part.SurfaceOfExtrusion):
            # extrusion wall
            Path.Log.debug("type() == Part.SurfaceOfExtrusion")
            # Save face to self.horiz for processing or display error
            if self.isVerticalExtrusionFace(face):
                self.vert.append(face)
                return True
            else:
                Path.Log.error("Failed to identify vertical face from {}".format(sub))
                return False

        else:
            Path.Log.debug("  -type(face.Surface): {}".format(type(face.Surface)))
            return False


# Eclass


def SetupProperties():
    setup = PathPocketBase.SetupProperties()  # Add properties from PocketBase module
    setup.extend(FeatureExtensions.SetupProperties())  # Add properties from Extensions Feature

    # Add properties initialized here in PocketShape
    setup.append("UseOutline")
    return setup


def Create(name, obj=None, parentJob=None):
    """Create(name) ... Creates and returns a Pocket operation."""
    if obj is None:
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", name)
    obj.Proxy = ObjectPocket(obj, name, parentJob)
    return obj
