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
import Path.Base.Drillable as Drillable

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Draft = LazyLoader("Draft", globals(), "Draft")
Part = LazyLoader("Part", globals(), "Part")
DraftGeomUtils = LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")


__title__ = "CAM Circular Holes Base Operation"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Base class an implementation for operations on circular holes."


translate = FreeCAD.Qt.translate


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class ObjectOp(PathOp.ObjectOp):
    """Base class for proxy objects of all operations on circular holes."""

    def opFeatures(self, obj):
        """opFeatures(obj) ... calls circularHoleFeatures(obj) and ORs in the standard features required for processing circular holes.
        Do not overwrite, implement circularHoleFeatures(obj) instead"""
        return (
            PathOp.FeatureTool
            | PathOp.FeatureDepths
            | PathOp.FeatureHeights
            | PathOp.FeatureBaseFaces
            | self.circularHoleFeatures(obj)
            | PathOp.FeatureCoolant
        )

    def circularHoleFeatures(self, obj):
        """circularHoleFeatures(obj) ... overwrite to add operations specific features.
        Can safely be overwritten by subclasses."""
        return 0

    def initOperation(self, obj):
        """initOperation(obj) ... adds Disabled properties and calls initCircularHoleOperation(obj).
        Do not overwrite, implement initCircularHoleOperation(obj) instead."""
        obj.addProperty(
            "App::PropertyStringList",
            "Disabled",
            "Base",
            QT_TRANSLATE_NOOP("App::Property", "List of disabled features"),
        )
        self.initCircularHoleOperation(obj)

    def initCircularHoleOperation(self, obj):
        """initCircularHoleOperation(obj) ... overwrite if the subclass needs initialisation.
        Can safely be overwritten by subclasses."""
        pass

    def holeDiameter(self, obj, base, sub):
        """holeDiameter(obj, base, sub) ... returns the diameter of the specified hole."""
        try:
            shape = base.Shape.getElement(sub)
            if shape.ShapeType == "Vertex":
                return 0

            if shape.ShapeType == "Edge" and type(shape.Curve) == Part.Circle:
                return shape.Curve.Radius * 2

            if shape.ShapeType == "Face":
                for i in range(len(shape.Edges)):
                    if (
                        type(shape.Edges[i].Curve) == Part.Circle
                        and shape.Edges[i].Curve.Radius * 2
                        < shape.BoundBox.XLength * 1.1
                        and shape.Edges[i].Curve.Radius * 2
                        > shape.BoundBox.XLength * 0.9
                    ):
                        return shape.Edges[i].Curve.Radius * 2

            # for all other shapes the diameter is just the dimension in X.
            # This may be inaccurate as the BoundBox is calculated on the tessellated geometry
            Path.Log.warning(
                translate(
                    "CAM",
                    "Hole diameter may be inaccurate due to tessellation on face. Consider selecting hole edge.",
                )
            )
            return shape.BoundBox.XLength
        except Part.OCCError as e:
            Path.Log.error(e)

        return 0

    def holePosition(self, obj, base, sub):
        """holePosition(obj, base, sub) ... returns a Vector for the position defined by the given features.
        Note that the value for Z is set to 0."""

        try:
            shape = base.Shape.getElement(sub)
            if shape.ShapeType == "Vertex":
                return FreeCAD.Vector(shape.X, shape.Y, 0)

            if shape.ShapeType == "Edge" and hasattr(shape.Curve, "Center"):
                return FreeCAD.Vector(shape.Curve.Center.x, shape.Curve.Center.y, 0)

            if shape.ShapeType == "Face":
                if hasattr(shape.Surface, "Center"):
                    return FreeCAD.Vector(
                        shape.Surface.Center.x, shape.Surface.Center.y, 0
                    )
                if len(shape.Edges) == 1 and type(shape.Edges[0].Curve) == Part.Circle:
                    return shape.Edges[0].Curve.Center
        except Part.OCCError as e:
            Path.Log.error(e)

        Path.Log.error(
            translate(
                "CAM",
                "Feature %s.%s cannot be processed as a circular hole - please remove from Base geometry list.",
            )
            % (base.Label, sub)
        )
        return None

    def isHoleEnabled(self, obj, base, sub):
        """isHoleEnabled(obj, base, sub) ... return true if hole is enabled."""
        name = "%s.%s" % (base.Name, sub)
        return name not in obj.Disabled

    def opExecute(self, obj):
        """opExecute(obj) ... processes all Base features and Locations and collects
        them in a list of positions and radii which is then passed to circularHoleExecute(obj, holes).
        If no Base geometries and no Locations are present, the job's Base is inspected and all
        drillable features are added to Base. In this case appropriate values for depths are also
        calculated and assigned.
        Do not overwrite, implement circularHoleExecute(obj, holes) instead."""
        Path.Log.track()

        def haveLocations(self, obj):
            if PathOp.FeatureLocations & self.opFeatures(obj):
                return len(obj.Locations) != 0
            return False

        holes = []
        for base, subs in obj.Base:
            for sub in subs:
                Path.Log.debug("processing {} in {}".format(sub, base.Name))
                if self.isHoleEnabled(obj, base, sub):
                    pos = self.holePosition(obj, base, sub)
                    if pos:
                        holes.append(
                            {
                                "x": pos.x,
                                "y": pos.y,
                                "r": self.holeDiameter(obj, base, sub),
                            }
                        )

        if haveLocations(self, obj):
            for location in obj.Locations:
                holes.append({"x": location.x, "y": location.y, "r": 0})

        if len(holes) > 0:
            self.circularHoleExecute(obj, holes)

    def circularHoleExecute(self, obj, holes):
        """circularHoleExecute(obj, holes) ... implement processing of holes.
        holes is a list of dictionaries with 'x', 'y' and 'r' specified for each hole.
        Note that for Vertexes, non-circular Edges and Locations r=0.
        Must be overwritten by subclasses."""
        pass

    def findAllHoles(self, obj):
        """findAllHoles(obj) ... find all holes of all base models and assign as features."""
        Path.Log.track()
        job = self.getJob(obj)
        if not job:
            return

        matchvector = None if job.JobType == "Multiaxis" else FreeCAD.Vector(0, 0, 1)
        tooldiameter = obj.ToolController.Tool.Diameter

        features = []
        for base in self.model:
            features.extend(
                Drillable.getDrillableTargets(
                    base, ToolDiameter=tooldiameter, vector=matchvector
                )
            )
        obj.Base = features
        obj.Disabled = []
