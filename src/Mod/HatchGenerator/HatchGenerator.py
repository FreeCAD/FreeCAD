#!/usr/bin/env python
# -*- coding: utf-8 -*-

# ============================================================================
# FreeCAD Hatch Generator Addon
# ============================================================================
# Filename: HatchGenerator.py
# Description: A parametric hatch generator for 3D surfaces, walls, and roofs.
# Author: Regis Benoit Brice Nde Tene
# Version: 2.0 (Fixed: Edit mode, double-click, advanced tab layout)
# ============================================================================

import FreeCAD
import FreeCADGui
import Part
import math, random
from PySide import QtCore, QtGui, QtWidgets
import os
import json
import datetime

# ============================================================================
# Imports from split modules
# ============================================================================
from HatchPatterns import generateBuiltInPatternShape
from HatchCore import (
    buildHatchShape,
    normalizePatternShape,
    clipShapeToBase,
    makeTileAndClip,
    separateFacesAndEdges,
    shapeToEdges
)
from FaceExtractor import makeFaceExtractor, makeFaceExtractorFromSelection

# ================================
# Helper Functions
# ================================
def convertBaseSpacingValue(spacingValue, useUnits, selectedUnitSystem):
    """
    Converts the user-entered spacingValue into millimeters (or an internal unit).
    If useUnits=False, just return spacingValue as-is.
    If useUnits=True, interpret based on selectedUnitSystem.
    """
    if not useUnits:
        return spacingValue

    if selectedUnitSystem == "FreeCAD Default":
        return spacingValue
    elif selectedUnitSystem == "Metric (m)":
        return spacingValue * 1000.0
    elif selectedUnitSystem == "Imperial (ft)":
        return spacingValue * 304.8
    elif selectedUnitSystem == "BIM Workbench Unit":
        return spacingValue * 25.4
    else:
        return spacingValue

def getFaceLocalFrame(face):
    """
    Returns (origin, x_axis, y_axis, normal) for the given face.
    These are all FreeCAD.Vector in world coordinates.
    """
    # Surface normal at parametric center
    try:
        normal = face.normalAt(0, 0).normalize()
    except:
        # Fallback for faces that might not have proper parameterization
        normal = face.normalAt(face.Surface.parameter(face.CenterOfMass)[0], 
                               face.Surface.parameter(face.CenterOfMass)[1]).normalize()

    # Pick a reference up-vector that is not parallel to normal
    up = FreeCAD.Vector(0, 0, 1)
    if abs(normal.dot(up)) > 0.99:
        up = FreeCAD.Vector(0, 1, 0)

    # Build orthonormal local frame
    x_axis = up.cross(normal).normalize()
    y_axis = normal.cross(x_axis).normalize()

    # Origin = center of mass of the face
    origin = face.CenterOfMass

    return origin, x_axis, y_axis, normal

def buildSurfaceTransforms(face):
    """
    Returns a FreeCAD.Matrix that transforms world XYZ → local UV (2D on face),
    and its inverse that transforms local UV → world XYZ.
    """
    origin, x_axis, y_axis, normal = getFaceLocalFrame(face)

    # world_to_local: rotate world coords so that x_axis→X, y_axis→Y, normal→Z
    # then subtract origin
    to_local = FreeCAD.Matrix()
    # Column vectors of the rotation are the local axes
    to_local.A11 = x_axis.x;  to_local.A12 = x_axis.y;  to_local.A13 = x_axis.z
    to_local.A21 = y_axis.x;  to_local.A22 = y_axis.y;  to_local.A23 = y_axis.z
    to_local.A31 = normal.x;  to_local.A32 = normal.y;  to_local.A33 = normal.z
    
    # Create translation matrix
    translation = FreeCAD.Matrix()
    translation.move(FreeCAD.Vector(-origin.x, -origin.y, -origin.z))
    
    # Combine: first translate, then rotate
    to_local = to_local.multiply(translation)

    # local_to_world is the inverse
    to_world = to_local.inverse()

    return to_local, to_world

def projectShapeToLocal(shape, to_local_matrix):
    """Transform shape into the face's local coordinate system."""
    projected = shape.copy()
    projected.transformShape(to_local_matrix)
    return projected

def unprojectShapeToWorld(shape, to_world_matrix):
    """Transform shape from local back to world coordinates."""
    worldShape = shape.copy()
    worldShape.transformShape(to_world_matrix)
    return worldShape

def getShapeFaceNormal(shape):
    """Get the normal of the first face in a shape, or None if no faces."""
    if shape and shape.Faces:
        try:
            return shape.Faces[0].normalAt(0, 0).normalize()
        except:
            pass
    return None

def applyTileViewOverrides(tileObj, vp, overrideColor=None):
    """
    Ensures the tile rep is displayed in a certain style
    without triggering recomputes.
    """
    if not tileObj or not hasattr(tileObj, 'ViewObject'):
        return

    # If overrideColor was passed in, use it; otherwise use the tileObj's property
    if overrideColor is not None:
        color = overrideColor
    else:
        # If the tile's parent is our hatch object, we can read .TileRepColor
        hatchObj = tileObj.Document.getObject(tileObj.Name.replace("_TileRep",""))
        if hatchObj and hasattr(hatchObj, "TileRepColor"):
            color = hatchObj.TileRepColor
        else:
            color = (1.0, 0.0, 0.0)  # fallback color

    vp.ShapeColor = color
    vp.DisplayMode = "Wireframe"  # or "Flat Lines"
    vp.LineWidth = 2.0

def safeSetDisplayMode(vobj, desiredMode="Flat Lines"):
    modes = []
    try:
        modes = vobj.getDisplayModes()
    except Exception:
        pass
    if desiredMode in modes:
        vobj.DisplayMode = desiredMode
    elif modes:
        vobj.DisplayMode = modes[0]

def makeRectangle(width, height):
    """Creates a Face covering 'width' x 'height' in the XY plane."""
    p0 = FreeCAD.Vector(0,0,0)
    p1 = FreeCAD.Vector(width,0,0)
    p2 = FreeCAD.Vector(width,height,0)
    p3 = FreeCAD.Vector(0,height,0)
    poly = Part.makePolygon([p0, p1, p2, p3, p0])
    face = Part.Face(poly)
    return face

def getBaseShapeFromSketchOrFeature(obj):
    """
    If 'obj' is a Sketch, or a Draft object with MakeFace=False, 
    we try to build faces for any closed wires. Otherwise just return obj.Shape.
    """
    if not obj or not hasattr(obj, "Shape"):
        return None

    if obj.isDerivedFrom("Sketcher::SketchObject"):
        sketchShape = obj.Shape
        closed_faces = []
        for wire in sketchShape.Wires:
            if wire.isClosed():
                try:
                    face = Part.Face(wire)
                    closed_faces.append(face)
                except Exception as e:
                    FreeCAD.Console.PrintWarning(f"Failed to create face from wire: {e}\n")
        if closed_faces:
            return Part.makeCompound(closed_faces)
        else:
            return sketchShape
    else:
        if hasattr(obj, "ViewObject") and hasattr(obj.ViewObject, "MakeFace"):
            if obj.ViewObject.MakeFace is False:
                shape = obj.Shape
                closed_faces = []
                for wire in shape.Wires:
                    if wire.isClosed():
                        try:
                            face = Part.Face(wire)
                            closed_faces.append(face)
                        except Exception as e:
                            FreeCAD.Console.PrintWarning(f"Failed to create face: {e}\n")
                if closed_faces:
                    return Part.makeCompound(closed_faces)
                else:
                    return shape
        return obj.Shape

def getClosedWiresAsFaces(obj):
    """
    Return a shape that has faces. If the object's shape already has faces
    (e.g., Draft object with MakeFace=True), just return it as is.
    Otherwise, attempt to build faces from closed wires.
    """
    if not obj or not hasattr(obj, "Shape"):
        return None

    shape = obj.Shape
    if shape.isNull():
        return None

    if shape.Faces:
        return shape

    faces = []
    for wire in shape.Wires:
        if wire.isClosed():
            try:
                f = Part.Face(wire)
                faces.append(f)
            except Exception as e:
                FreeCAD.Console.PrintWarning(f"Cannot make face from wire: {e}\n")

    if faces:
        return Part.makeCompound(faces)
    else:
        return shape

# ================================
# Unified Bounding Box Helper
# ================================
def _compute_unified_bb(validBaseShapes):
    """
    Compute a single BoundBox that covers ALL base shapes.
    This is passed to buildHatchShape as overrideBB so the tile grid
    is positioned consistently across all shapes, avoiding the offset bug
    when BaseObject + BaseObjects are used together.
    """
    if len(validBaseShapes) == 1:
        return validBaseShapes[0].BoundBox
    compound = Part.makeCompound(validBaseShapes)
    return compound.BoundBox

# ================================
# Parametric Hatch Feature
# ================================
class CustomHatchFeature:
    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "CustomHatchFP"
        self._is_recomputing = False

        # Base
        obj.addProperty("App::PropertyLink", "BaseObject", "Hatch", "Object with the base shape.")
        # === We also have "BaseObjects" here: multiple base shapes
        obj.addProperty("App::PropertyLinkList", "BaseObjects", "Hatch", 
                        "Optional list of multiple base objects.")

        # Pattern
        obj.addProperty("App::PropertyLink", "PatternObject", "Hatch", "Object with the pattern shape.")
        # === PatternObjects for multiple patterns
        obj.addProperty("App::PropertyLinkList", "PatternObjects", "Hatch", 
                        "Optional list of multiple pattern objects (fused).")

        # Base Tile
        obj.addProperty("App::PropertyLink", "BaseTileObject", "Hatch",
                        "Optional tile shape object (if set, used to define bounding box).")
        obj.addProperty("App::PropertyBool", "TileVisibility", "Hatch",
                        "Toggle tile shape visibility as a separate object.")
        obj.TileVisibility = True
        obj.addProperty("App::PropertyBool", "UpdateTileOnChange", "Hatch",
                        "Update tile shapes automatically when Base Tile Object changes.")
        obj.UpdateTileOnChange = True

        # === Subtractions
        obj.addProperty("App::PropertyLinkList", "Subtractions", "Hatch",
                        "List of objects to subtract from the hatch pattern.")

        # Distribution
        obj.addProperty("App::PropertyEnumeration", "DistributionMode", "Hatch",
                        "How to distribute the pattern.")
        obj.DistributionMode = [
            "CenteredTiling",
            "RelativeSpacing",
            "SeamlessTiling",
            "LinearGrid",
            "RadialDistribution",
            "ConcentricDistribution",
            "RandomDistribution",
            "AdaptiveDistribution"
        ]
        obj.DistributionMode = "SeamlessTiling"

        # Pattern Type
        obj.addProperty("App::PropertyEnumeration", "PatternType", "Hatch",
                        "Choose built-in patterns or custom PatternObject.")
        obj.PatternType = [
            "CustomObject",
            "SolidFill",
            "HorizontalLines",
            "VerticalLines",
            "Crosshatch",
            "Herringbone",
            "BrickPattern",
            "RandomDots",
            "OverlappingSquares",
            "Checkerboard",
            "CheckerboardCircles",
            "RotatingHexagons",
            "NestedTriangles",
            "InterlockingCircles",
            "RecursiveSquares",
            "FlowerOfLife",
            "VoronoiMesh",
            "OffsetChecker",
            "ZigZag",
            "HexagonalHoriz",
            "HexagonalVerti",
            "HexagonalPattern",
            "TrianglesGrid",
            "MidEastMosaic",
            "StarGridPattern",
            "BasketWeave",
            "Honeycomb",
            "SineWave",
            "SpaceFrame",
            "HoneycombDual",
            "ArtDeco",
            "StainedGlass",
            "PenroseTriangle",
            "GreekKey",
            "ChainLinks",
            "TriangleForest",
            "CeramicTile",
            "CirclesGrid",
            "PlusSigns",
            "WavesPattern",
            "GalaxyStarsPattern",
            "GridDots",
            "InterlockingCircles",
            "HexDots",
            "FractalTree",
            "Voronoi",
            "FractalBranches",
            "OrganicMaze",
            "BiomorphicCells",
            "RadialSunburst",
            "Sunburst",
            "Ziggurat",
            "SpiralPattern",
            "PentaflakeFractal",
            "HilbertCurve",
            "SierpinskiTriangle",
            "PenroseTiling",
            "EinsteinMonotile",
            "LeafVeins",
            "WoodPlanks",
            "ParquetHerringbone",
            "WoodGrain",
            "DrywallOrangePeel",
            "DrywallKnockdown",
            "StuccoSandFloat",
            "StuccoDash",
            "DrywallSkipTrowel",
            "Concrete",
            "ConcreteStampedPattern",
            "ConcreteSaltFinish",
            "ConcreteFormTiePattern",
            "ConcreteSandblastPattern",
            "ConcreteControlJoint",
            "ConcreteGridPattern",
            "WoodKnotPattern",
            "ConcreteAggregatePattern",
            "BrushedConcrete",
            "PebbleConcrete",
            "CrackedConcrete",
            "AggregateConcrete",
            "StampedConcrete",
            "Insulation",
            "Rebar",
            "RoofTiles",
        ]
        obj.PatternType = "CustomObject"

        # Scale / Rotation / Spacing
        obj.addProperty("App::PropertyBool", "AutoScaleToFitBase", "Scaling", 
                        "If true, automatically compute scale.")
        obj.AutoScaleToFitBase = False
        obj.addProperty("App::PropertyFloat", "PatternScale", "Scaling", "Manual scale factor.")
        obj.PatternScale = 1.0
        obj.addProperty("App::PropertyFloat", "RotationDeg", "Hatch", "Rotation around Z (degrees).")
        obj.RotationDeg = 0.0
        obj.addProperty("App::PropertyFloat", "BaseSpacing", "Hatch", "Base spacing (mm or %).")
        obj.BaseSpacing = 10.0
        # Unit system properties
        obj.addProperty("App::PropertyBool", "UseUnits", "Hatch", 
                        "If true, interpret BaseSpacing with a chosen unit system.")
        obj.UseUnits = False
        obj.addProperty("App::PropertyEnumeration", "SelectedUnitSystem", "Hatch",
                        "User-chosen unit system for BaseSpacing.")
        obj.SelectedUnitSystem = ["FreeCAD Default", "Metric (m)", "Imperial (ft)", "BIM Workbench Unit"]
        obj.SelectedUnitSystem = "FreeCAD Default"
        obj.addProperty("App::PropertyInteger", "RepetitionsX", "Hatch", "Number of repeats in X.")
        obj.RepetitionsX = 5
        obj.addProperty("App::PropertyInteger", "RepetitionsY", "Hatch", "Number of repeats in Y.")
        obj.RepetitionsY = 5

        # Pattern offset
        obj.addProperty("App::PropertyFloat", "PatternOffsetX", "Hatch", 
                        "Offset the pattern in X direction.")
        obj.PatternOffsetX = 0.0
        obj.addProperty("App::PropertyFloat", "PatternOffsetY", "Hatch", 
                        "Offset the pattern in Y direction.")
        obj.PatternOffsetY = 0.0

        # Scale Mode
        obj.addProperty("App::PropertyEnumeration", "ScaleMode", "Scaling", 
                        "How PatternScale is interpreted.")
        obj.ScaleMode = ["Absolute", "FitWidth", "FitHeight", "FitMinDim", "FitMaxDim"]
        obj.ScaleMode = "Absolute"

        # Random
        obj.addProperty("App::PropertyBool", "RandomizePlacement", "Random",
                        "Apply random transforms?")
        obj.RandomizePlacement = False
        obj.addProperty("App::PropertyFloat", "RandomOffsetRange", "Random", 
                        "Max random offset ± mm.")
        obj.RandomOffsetRange = 0.0
        obj.addProperty("App::PropertyFloat", "RandomRotationRange", "Random", 
                        "Max random rotation ± deg.")
        obj.RandomRotationRange = 0.0
        obj.addProperty("App::PropertyFloat", "RandomScaleMin", "Random",
                        "Min random scale factor.")
        obj.RandomScaleMin = 1.0
        obj.addProperty("App::PropertyFloat", "RandomScaleMax", "Random",
                        "Max random scale factor.")
        obj.RandomScaleMax = 1.0

        # Lock / Clamping
        obj.addProperty("App::PropertyBool", "LockToBase", "Placement",
                        "If True, user transform is re-clipped.")
        obj.LockToBase = False

        # Additional distribution parameters
        obj.addProperty("App::PropertyInteger", "RadialCount", "Radial",
                        "Number of copies around the circle.")
        obj.RadialCount = 8
        obj.addProperty("App::PropertyFloat", "RadialRadius", "Radial",
                        "Radius for radial distribution.")
        obj.RadialRadius = 50.0
        obj.addProperty("App::PropertyInteger", "ConcentricCount", "Concentric",
                        "Number of rings.")
        obj.ConcentricCount = 5
        obj.addProperty("App::PropertyFloat", "ConcentricSpacing", "Concentric",
                        "Spacing between rings.")
        obj.ConcentricSpacing = 10.0
        obj.addProperty("App::PropertyInteger", "RandomCount", "Random",
                        "Number of random placements.")
        obj.RandomCount = 30

        # Pattern Placement Mode
        obj.addProperty("App::PropertyEnumeration", "PatternPlacementMode", "Hatch",
                        "Positioning of the pattern within each tile.")
        obj.PatternPlacementMode = [
            "Origin", "Center", 
            "TopLeft", "TopRight", "BottomLeft", "BottomRight",
            "TopCenter", "BottomCenter", "LeftCenter", "RightCenter",
            "Custom"
        ]
        obj.PatternPlacementMode = "Origin"

        obj.addProperty("App::PropertyBool", "ShowFaces", "Rendering",
                        "If True, tries to convert lines to faces.")
        obj.ShowFaces = False

        obj.addProperty("App::PropertyBool", "ApplyTo3DSurface", "Rendering",
                        "If True, map the pattern onto the selected 3D face.")
        obj.ApplyTo3DSurface = False

        obj.addProperty("App::PropertyEnumeration", "ClipMode", "Rendering",
                        "How to handle open wires vs. faces when clipping with the base.")
        obj.ClipMode = ["BooleanOnly", "PreserveLinesNoClip"]
        obj.ClipMode = "BooleanOnly"

        # ===== NEW: Surface projection properties =====
        obj.addProperty("App::PropertyBool", "UseSurfaceProjection", "Surface",
                        "If True, project pattern onto the surface of the base shape (works on walls, sloped roofs, etc.).")
        obj.UseSurfaceProjection = True
        obj.addProperty("App::PropertyBool", "ForceXYPlane", "Surface",
                        "If True, ignore surface projection and use XY plane only (old behavior).")
        obj.ForceXYPlane = False

        # Performance limit
        obj.addProperty("App::PropertyInteger", "MaxTilesAllowed", "Performance",
                        "Maximum number of tiles to generate.")
        obj.MaxTilesAllowed = 1000

        # Variation properties
        obj.addProperty("App::PropertyFloat", "DensityFactor", "Variation",
                        "Density factor for random distribution (0-1).")
        obj.DensityFactor = 1.0
        obj.addProperty("App::PropertyFloat", "RandomRotationMin", "Variation",
                        "Minimum random rotation (deg).")
        obj.RandomRotationMin = 0.0
        obj.addProperty("App::PropertyFloat", "RandomRotationMax", "Variation",
                        "Maximum random rotation (deg).")
        obj.RandomRotationMax = 0.0
        obj.addProperty("App::PropertyBool", "EnableColorVariation", "Variation",
                        "If true, random colors are assigned to each tile.")
        obj.EnableColorVariation = False
        obj.addProperty("App::PropertyFloat", "ColorVariationIntensity", "Variation",
                        "Intensity of color variation (0-1).")
        obj.ColorVariationIntensity = 0.5
        obj.addProperty("App::PropertyFloat", "SpacingVariation", "Variation",
                        "Random factor for spacing variation (0-1).")
        obj.SpacingVariation = 0.0
        obj.addProperty("App::PropertyBool", "EnableShapeDistortion", "Variation",
                        "If true, shapes might be distorted in random ways.")
        obj.EnableShapeDistortion = False

        # Statistics
        obj.addProperty("App::PropertyFloat", "GenerationTime", "Statistics", "Time taken to generate the hatch")
        obj.GenerationTime = 0.0
        obj.addProperty("App::PropertyInteger", "TileCount", "Statistics", "Number of tiles generated")
        obj.TileCount = 0

        if hasattr(obj, "ViewObject") and obj.ViewObject:
            safeSetDisplayMode(obj.ViewObject, "Flat Lines")

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        self.Type = state

    def onChanged(self, fp, prop):
        if prop == "BaseTileObject" and fp.UpdateTileOnChange:
            if prop == "BaseTileObject":
                if fp.BaseTileObject:
                    FreeCADGui.Selection.clearSelection()
                    FreeCADGui.Selection.addSelection(fp.BaseTileObject)
            self.safe_delayed_execute(fp, 100)
        if prop == "PatternPlacementMode":
            self.safe_delayed_execute(fp, 100) 
        if prop == "DistributionMode":
            props = fp.PropertiesList
            def setModeIfExists(pn, mv):
                if pn in props:
                    fp.setEditorMode(pn, mv)
            setModeIfExists("BaseSpacing", 0)
            setModeIfExists("RepetitionsX", 0)
            setModeIfExists("RepetitionsY", 0)

        if prop == "LockToBase":
            if fp.LockToBase:
                fp.setEditorMode("Placement", 2)  # read-only
            else:
                fp.setEditorMode("Placement", 0)

        if prop == "BaseTileObject" and fp.UpdateTileOnChange:
            self.execute(fp)

    def safe_delayed_execute(self, fp, delay):
        """Safely schedule execute() with object existence check"""
        if not fp or not fp.Document:
            return
        
        # Capture object IDENTIFIERS instead of object references
        doc_name = fp.Document.Name
        obj_name = fp.Name
        
        def callback():
            # Check if document/object still exist
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return
            obj = doc.getObject(obj_name)
            if obj and obj.Proxy == self:
                self.execute(obj)
        
        QtCore.QTimer.singleShot(delay, callback)

    def clampPlacementInsideBaseBounding(self, fp):
        baseObj = fp.BaseObject
        if not baseObj:
            return
        bBB = baseObj.Shape.BoundBox
        shape = fp.Shape.copy()
        if shape.isNull():
            return
        newBB = shape.BoundBox
        if newBB.XMin < bBB.XMin:
            shift = bBB.XMin - newBB.XMin
            oldPlacement = fp.Placement
            p = oldPlacement.Base
            p.x += shift
            fp.Placement = FreeCAD.Placement(p, oldPlacement.Rotation)
        if newBB.XMax > bBB.XMax:
            shift = bBB.XMax - newBB.XMax
            oldPlacement = fp.Placement
            p = oldPlacement.Base
            p.x += shift
            fp.Placement = FreeCAD.Placement(p, oldPlacement.Rotation)

    def execute(self, fp):
        """Recompute the hatch shape.
        
        Guarantees fp.Shape is ALWAYS set to something after execute()
        completes, so FreeCAD always calls purgeTouched() and the
        'still touched after recompute' message never fires.
        """
        import datetime
        start_time = datetime.datetime.now()
        doc = fp.Document
        
        # Safety: Always ensure we set fp.Shape even on failure
        try:
            if not doc:
                fp.Shape = Part.makeCompound([])
                return

            baseShapes = []

            # Gather BaseObject
            if fp.BaseObject:
                temp_shape = getClosedWiresAsFaces(fp.BaseObject)
                if temp_shape and not temp_shape.isNull():
                    shape_conv = temp_shape
                else:
                    shape_conv = fp.BaseObject.Shape.copy()

                if shape_conv and not shape_conv.isNull():
                    baseShapes.append(shape_conv)

            # Gather BaseObjects (the list)
            if fp.BaseObjects:
                for bobj in (fp.BaseObjects or []):
                    if bobj:
                        temp_face = getClosedWiresAsFaces(bobj)
                        if temp_face and not temp_face.isNull():
                            face_shape = temp_face
                        else:
                            face_shape = bobj.Shape.copy()
                        if face_shape and not face_shape.isNull():
                            baseShapes.append(face_shape)

            if not baseShapes:
                fp.Shape = Part.makeCompound([])
                return
                
            validBaseShapes = [sh for sh in baseShapes if sh and not sh.isNull()]
            if not validBaseShapes:
                fp.Shape = Part.makeCompound([])
                return

            # Compute unified bounding box for consistent tiling across multiple base shapes
            unified_bb = _compute_unified_bb(validBaseShapes)

            distMode = fp.DistributionMode
            autoScale = fp.AutoScaleToFitBase
            scaleVal = fp.PatternScale
            rotVal = fp.RotationDeg
            spacingVal = fp.BaseSpacing
            if fp.UseUnits:
                spacingVal = convertBaseSpacingValue(spacingVal, fp.UseUnits, fp.SelectedUnitSystem)

            rx = fp.RepetitionsX
            ry = fp.RepetitionsY
            randPlace = fp.RandomizePlacement
            randOff = fp.RandomOffsetRange
            rotationMin = fp.RandomRotationMin
            rotationMax = fp.RandomRotationMax
            randSmin = fp.RandomScaleMin
            randSmax = fp.RandomScaleMax
            radialCount = fp.RadialCount
            radialRadius = fp.RadialRadius
            concentricCount = fp.ConcentricCount
            concentricSpace = fp.ConcentricSpacing
            randomCount = fp.RandomCount
            pOffX = fp.PatternOffsetX
            pOffY = fp.PatternOffsetY
            scaleMode = fp.ScaleMode
            tileObj = fp.BaseTileObject if hasattr(fp, "BaseTileObject") else None
            tileVisibility = fp.TileVisibility
            showFaces = fp.ShowFaces
            apply3D = fp.ApplyTo3DSurface
            maxTiles = fp.MaxTilesAllowed
            densityFactor = fp.DensityFactor
            enableColorVar = fp.EnableColorVariation
            colorVarInt = fp.ColorVariationIntensity
            spacingVariation = fp.SpacingVariation
            shapeDistortion = fp.EnableShapeDistortion
            
            # Surface projection properties
            useSurfaceProjection = getattr(fp, "UseSurfaceProjection", True)
            forceXYPlane = getattr(fp, "ForceXYPlane", False)

            # Subtractions
            subObjs = fp.Subtractions

            # Decide pattern shape
            if fp.PatternType == "SolidFill":
                combined = Part.makeCompound(validBaseShapes)
                fp.GenerationTime = (datetime.datetime.now() - start_time).total_seconds()
                fp.TileCount = len(validBaseShapes)
                tileObjName = fp.Name + "_TileRep"
                existingTileObj = doc.getObject(tileObjName)
                if existingTileObj:
                    doc.removeObject(existingTileObj.Name)
            else:
                # If PatternType == "CustomObject", gather from PatternObject + PatternObjects
                if fp.PatternType == "CustomObject":
                    allPatternShapes = []
                    if fp.PatternObject and hasattr(fp.PatternObject, "Shape"):
                        pat_shp = getClosedWiresAsFaces(fp.PatternObject) if showFaces else fp.PatternObject.Shape.copy()
                        if pat_shp and not pat_shp.isNull():
                            allPatternShapes.append(pat_shp)

                    if fp.PatternObjects:
                        for pobj in fp.PatternObjects:
                            if pobj and hasattr(pobj, "Shape"):
                                pat2 = getClosedWiresAsFaces(pobj) if showFaces else pobj.Shape.copy()
                                if pat2 and not pat2.isNull():
                                    allPatternShapes.append(pat2)

                    if not allPatternShapes:
                        fp.Shape = Part.makeCompound([])
                        return
                    if len(allPatternShapes) == 1:
                        patternShape = allPatternShapes[0]
                    else:
                        patternShape = allPatternShapes[0]
                        for extraShape in allPatternShapes[1:]:
                            patternShape = patternShape.fuse(extraShape)
                else:
                    patternShape = generateBuiltInPatternShape(fp.PatternType)

                if not patternShape or patternShape.isNull():
                    fp.Shape = Part.makeCompound([])
                    return

                total_tiles = 0
                finalParts = []
                
                for bShape in validBaseShapes:
                    try:
                        # Check if we should use surface projection
                        use_projection = (useSurfaceProjection and not forceXYPlane and 
                                          bShape.Faces and len(bShape.Faces) > 0)
                        
                        if use_projection:
                            # Get the first face for transformation
                            target_face = bShape.Faces[0]
                            
                            # Build surface transforms
                            to_local, to_world = buildSurfaceTransforms(target_face)
                            
                            # Project base shape into local 2D coordinates
                            localBase = projectShapeToLocal(bShape, to_local)
                            
                            # Generate hatch in local 2D space with unified BB
                            localHatch, tiles = buildHatchShape(
                                baseShape=localBase,
                                overrideBB=_compute_unified_bb([localBase]),
                                patternShape=patternShape,
                                distributionMode=distMode,
                                autoScaleToFitBase=autoScale,
                                patternScale=scaleVal,
                                rotationDeg=rotVal,
                                baseSpacing=spacingVal,
                                repX=rx,
                                repY=ry,
                                randRotMin=rotationMin,
                                randRotMax=rotationMax,
                                randomizePlacement=randPlace,
                                randomOffsetRange=randOff,
                                randomScaleMin=randSmin,
                                randomScaleMax=randSmax,
                                radialCount=radialCount,
                                radialRadius=radialRadius,
                                concentricCount=concentricCount,
                                concentricSpacing=concentricSpace,
                                randomCount=randomCount,
                                offsetX=pOffX,
                                offsetY=pOffY,
                                scaleMode=scaleMode,
                                tileShape=(tileObj.Shape if tileObj else None),
                                tileVisibility=tileVisibility,
                                showFaces=showFaces,
                                maxTiles=maxTiles,
                                densityFactor=densityFactor,
                                enableColorVar=enableColorVar,
                                colorVarInt=colorVarInt,
                                spacingVariation=spacingVariation,
                                shapeDistortion=shapeDistortion,
                                apply3D=apply3D,
                                placement_mode=fp.PatternPlacementMode,
                                clipMode=fp.ClipMode
                            )
                            
                            # Unproject back to world coordinates
                            shaped = unprojectShapeToWorld(localHatch, to_world)
                            total_tiles += tiles
                            finalParts.append(shaped)
                        else:
                            # Original XY-plane path with unified bounding box
                            shaped, tiles = buildHatchShape(
                                baseShape=bShape,
                                overrideBB=unified_bb,  # KEY FIX: unified BB for consistent tiling
                                patternShape=patternShape,
                                distributionMode=distMode,
                                autoScaleToFitBase=autoScale,
                                patternScale=scaleVal,
                                rotationDeg=rotVal,
                                baseSpacing=spacingVal,
                                repX=rx,
                                repY=ry,
                                randRotMin=rotationMin,
                                randRotMax=rotationMax,
                                randomizePlacement=randPlace,
                                randomOffsetRange=randOff,
                                randomScaleMin=randSmin,
                                randomScaleMax=randSmax,
                                radialCount=radialCount,
                                radialRadius=radialRadius,
                                concentricCount=concentricCount,
                                concentricSpacing=concentricSpace,
                                randomCount=randomCount,
                                offsetX=pOffX,
                                offsetY=pOffY,
                                scaleMode=scaleMode,
                                tileShape=(tileObj.Shape if tileObj else None),
                                tileVisibility=tileVisibility,
                                showFaces=showFaces,
                                maxTiles=maxTiles,
                                densityFactor=densityFactor,
                                enableColorVar=enableColorVar,
                                colorVarInt=colorVarInt,
                                spacingVariation=spacingVariation,
                                shapeDistortion=shapeDistortion,
                                apply3D=apply3D,
                                placement_mode=fp.PatternPlacementMode,
                                clipMode=fp.ClipMode
                            )
                            total_tiles += tiles
                            finalParts.append(shaped)
                    except Exception as e:
                        FreeCAD.Console.PrintError(f"Error building hatch on base shape: {str(e)}\n")
                        continue

                if not finalParts:
                    combined = Part.makeCompound([])
                else:
                    combined = finalParts[0]
                    for c in finalParts[1:]:
                        combined = combined.fuse(c)

                fp.TileCount = total_tiles

                # Cleanup tile objects if any
                tileObjName = fp.Name + "_TileRep"
                existingTileObj = doc.getObject(tileObjName)

                if tileVisibility and tileObj and not tileObj.Shape.isNull():
                    if not existingTileObj:
                        existingTileObj = doc.addObject("Part::Feature", tileObjName)
                    doc.openTransaction("Update Tiles")
                    try:
                        repeatedTile, _ = buildHatchShape(
                            baseShape=validBaseShapes[0],
                            overrideBB=unified_bb,
                            patternShape=tileObj.Shape,
                            distributionMode=distMode,
                            autoScaleToFitBase=autoScale,
                            patternScale=scaleVal,
                            rotationDeg=rotVal,
                            baseSpacing=spacingVal,
                            repX=rx,
                            repY=ry,
                            randRotMin=rotationMin,
                            randRotMax=rotationMax,
                            randomizePlacement=randPlace,
                            randomOffsetRange=randOff,
                            randomScaleMin=randSmin,
                            randomScaleMax=randSmax,
                            offsetX=pOffX,
                            offsetY=pOffY,
                            scaleMode=scaleMode,
                            tileShape=None,
                            tileVisibility=False,
                            showFaces=False,
                            maxTiles=maxTiles,
                            densityFactor=densityFactor,
                            enableColorVar=False,
                            colorVarInt=0.0,
                            spacingVariation=spacingVariation,
                            shapeDistortion=False,
                            apply3D=apply3D,
                            placement_mode=fp.PatternPlacementMode
                        )
                        existingTileObj.Shape = repeatedTile
                        applyTileViewOverrides(existingTileObj, existingTileObj.ViewObject)
                    finally:
                        doc.commitTransaction()
                else:
                    if existingTileObj:
                        doc.removeObject(existingTileObj.Name)

            # === Apply subtractions (for either SolidFill or normal)
            if subObjs:
                for so in subObjs:
                    if so and hasattr(so, "Shape") and not so.Shape.isNull():
                        try:
                            combined = combined.cut(so.Shape)
                        except Exception as e:
                            FreeCAD.Console.PrintError(f"Error subtracting {so.Name}: {str(e)}\n")

            fp.Shape = combined
            fp.GenerationTime = (datetime.datetime.now() - start_time).total_seconds()
            if fp.PatternType == "SolidFill":
                fp.TileCount = len(validBaseShapes)
                
        except Exception as e:
            # CRITICAL FIX: Always set Shape even on failure
            FreeCAD.Console.PrintWarning(
                f"CustomHatch '{fp.Name}' execute failed: {e}\n"
            )
            # ALWAYS set shape — this is what clears the touched state
            try:
                fp.Shape = Part.makeCompound([])
            except Exception:
                pass

    def onDocumentRestored(self, obj):
        """Handle document restoration"""
        obj.Proxy = self
        self._is_recomputing = False

# ================================
# ViewProvider
# ================================
class CustomHatchViewProvider:
    def __init__(self, vobj):
        vobj.Proxy = self
        self.Object = vobj.Object

    def setupContextMenu(self, obj, menu):
        action_copy = menu.addAction("Copy Hatch")
        action_copy.triggered.connect(lambda: self.copyHatch(obj.Object))
        action_duplicate = menu.addAction("Duplicate Hatch")
        action_duplicate.triggered.connect(lambda: self.duplicateHatch(obj.Object))
        action_remove = menu.addAction("Remove Hatch")
        action_remove.triggered.connect(lambda: self.removeHatch(obj.Object))

    def copyHatch(self, doc_obj):
        try:
            new_name = "CopyOf_" + doc_obj.Name
            new_obj = makeCustomHatch(name=new_name)
            excluded_props = ["Name", "Label", "ExpressionEngine"]
            for prop in doc_obj.PropertiesList:
                if prop not in excluded_props and not prop.startswith("Proxy"):
                    setattr(new_obj, prop, getattr(doc_obj, prop))
            FreeCAD.ActiveDocument.recompute()
        except Exception as e:
            FreeCAD.Console.PrintError(f"Copy failed: {str(e)}\n")

    def duplicateHatch(self, doc_obj):
        try:
            new_name = FreeCAD.ActiveDocument.getUniqueObjectName("Hatch")
            new_obj = makeCustomHatch(name=new_name)
            excluded_props = ["Name", "Label", "ExpressionEngine"]
            for prop in doc_obj.PropertiesList:
                if prop not in excluded_props and not prop.startswith("Proxy"):
                    setattr(new_obj, prop, getattr(doc_obj, prop))
            FreeCAD.ActiveDocument.recompute()
        except Exception as e:
            FreeCAD.Console.PrintError(f"Duplicate failed: {str(e)}\n")

    def removeHatch(self, doc_obj):
        try:
            FreeCAD.ActiveDocument.removeObject(doc_obj.Name)
            FreeCAD.ActiveDocument.recompute()
        except Exception as e:
            FreeCAD.Console.PrintError(f"Remove failed: {str(e)}\n")

    # PATCH 1: FIXED doubleClicked - opens edit panel for the clicked hatch
    def doubleClicked(self, vobj):
        panel = HatchTaskPanel(hatch_obj=vobj.Object)
        FreeCADGui.Control.showDialog(panel)
        return True

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def getDisplayModes(self, obj):
        return ["Flat Lines", "Wireframe", "Shaded"]

    def getDefaultDisplayMode(self):
        return "Flat Lines"

    def setDisplayMode(self, mode):
        return mode

    def onChanged(self, vobj, prop):
        pass

    def getIcon(self):
        return ":/icons/Draft_Hatch.svg"

    def __getstate__(self):
        return {"Object": self.Object.Name}

    def __setstate__(self, state):
        if "Object" in state:
            self.Object = FreeCAD.ActiveDocument.getObject(state["Object"])
        return None

def makeCustomHatch(name="CustomHatchFP"):
    doc = FreeCAD.ActiveDocument
    obj = doc.addObject("Part::FeaturePython", name)
    CustomHatchFeature(obj)
    CustomHatchViewProvider(obj.ViewObject)
    return obj


# ============================================================================
# ======================== FIXED UI: TASK PANEL VERSION ======================
# ============================================================================

class HatchTaskPanel:
    """
    Task panel for the FreeCAD Hatch Generator.
    This class creates the UI widget and handles the creation logic.
    It replaces the old QDialog.
    """
    # PATCH 2a: ADDED edit mode support with hatch_obj parameter
    def __init__(self, hatch_obj=None):
        self.editing_obj = hatch_obj   # None = create mode, object = edit mode
        FreeCAD.Console.PrintMessage(
            f"Hatch Task Panel: {'Edit' if hatch_obj else 'Create'} mode\n"
        )
        self.doc = FreeCAD.ActiveDocument
        if not self.doc:
            QtWidgets.QMessageBox.warning(None, "Error", "No active document found.")
            return

        self.current_temp_preview_name = None
        self.masterObjectList = self.getAllObjectsClassified()
        self.last_manual_scale = None
        
        # Build the UI Form (the widget displayed in the Task panel)
        self.form = self.createUI()
        
        # Initial setup
        self.initialPatternSetup()
        
        # If editing, load the existing object's values into the UI
        if self.editing_obj is not None:
            self._load_from_object(self.editing_obj)
        
    # ------------------------------------------------------------------------
    # UI Creation
    # ------------------------------------------------------------------------
    def createUI(self):
        # Main container widget
        main_widget = QtWidgets.QWidget()
        main_layout = QtWidgets.QVBoxLayout(main_widget)
        
        # Create a scroll area to contain the tabs
        scroll_area = QtWidgets.QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        scroll_area.setFrameShape(QtWidgets.QFrame.NoFrame)
        
        # Container for scrollable content
        scroll_content = QtWidgets.QWidget()
        scroll_layout = QtWidgets.QVBoxLayout(scroll_content)
        
        # Tabs
        self.tabs = QtWidgets.QTabWidget()
        self.tabs.setMinimumHeight(450)
        self.tabs.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        self.mainTab = QtWidgets.QWidget()
        self.mainTabLayout = QtWidgets.QFormLayout(self.mainTab)
        self.advancedTab = QtWidgets.QWidget()
        self.advancedTabLayout = QtWidgets.QVBoxLayout(self.advancedTab)
        self.tabs.addTab(self.mainTab, "Main")
        self.tabs.addTab(self.advancedTab, "Advanced")
        scroll_layout.addWidget(self.tabs)

        # --- Instantiate UI Elements ---
        self.baseLabel = QtWidgets.QLabel("Base Shape:")
        self.baseCombo = QtWidgets.QComboBox()
        self.pickBaseBtn = QtWidgets.QPushButton("Pick Base Shape from Selection")
        self.pickBaseBtn.clicked.connect(self.pickBaseShape)
        
        # Face Extractor group (NEW)
        self.faceExtractorGroup = QtWidgets.QGroupBox("Face Extractor (pick any 3D face)")
        faceExtLayout = QtWidgets.QVBoxLayout(self.faceExtractorGroup)

        self.faceExtractorInfo = QtWidgets.QLabel(
            "Select a face on any 3D object in the viewport,\n"
            "then click 'Pick Selected Face' to create a\n"
            "FaceExtractor object you can use as Base Shape."
        )
        self.faceExtractorInfo.setWordWrap(True)
        faceExtLayout.addWidget(self.faceExtractorInfo)

        self.pickFaceBtn = QtWidgets.QPushButton("Pick Selected Face → Create FaceExtractor")
        self.pickFaceBtn.setToolTip(
            "Click a face on a wall/slab/solid in the 3D view, "
            "then press this button. A parametric FaceExtractor object "
            "will be created and automatically set as the Base Shape."
        )
        self.pickFaceBtn.clicked.connect(self.pickFaceAndCreateExtractor)
        faceExtLayout.addWidget(self.pickFaceBtn)

        self.faceExtractorStatus = QtWidgets.QLabel("")
        self.faceExtractorStatus.setStyleSheet("color: green; font-style: italic;")
        faceExtLayout.addWidget(self.faceExtractorStatus)
        
        self.baseSearchLabel = QtWidgets.QLabel("Search:")
        self.baseSearchField = QtWidgets.QLineEdit()
        self.baseTypeFilterLabel = QtWidgets.QLabel("Type Filter:")
        self.baseTypeFilterCombo = QtWidgets.QComboBox()
        for cat in ["All", "Sketch", "PartFeature", "Compound", "Other"]:
            self.baseTypeFilterCombo.addItem(cat)
            
        self.baseObjectsLabel = QtWidgets.QLabel("Additional Base Objects:")
        self.baseObjectsList = QtWidgets.QListWidget()
        self.baseObjectsList.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)

        self.patternSourceLabel = QtWidgets.QLabel("Pattern Source:")
        self.patternSourceCombo = QtWidgets.QComboBox()
        self.patternSourceCombo.addItems(["Built-in", "Custom"])
        self.patternSourceCombo.currentIndexChanged.connect(self.onPatternSourceChanged)

        self.builtinPatternLabel = QtWidgets.QLabel("Built-in Pattern:")
        self.builtinPatternCombo = QtWidgets.QComboBox()
        self.builtinPatternCombo.addItems([
            "SolidFill", "HorizontalLines", "VerticalLines", "Crosshatch", "Herringbone",
            "BrickPattern", "RandomDots", "OverlappingSquares", "Checkerboard",
            "CheckerboardCircles", "RotatingHexagons", "NestedTriangles", "InterlockingCircles",
            "RecursiveSquares", "FlowerOfLife", "VoronoiMesh", "OffsetChecker", "ZigZag",
            "HexagonalHoriz", "HexagonalVerti", "HexagonalPattern", "TrianglesGrid",
            "MidEastMosaic", "StarGridPattern", "BasketWeave", "Honeycomb", "SineWave",
            "SpaceFrame", "HoneycombDual", "ArtDeco", "StainedGlass", "PenroseTriangle",
            "GreekKey", "ChainLinks", "TriangleForest", "CeramicTile", "CirclesGrid",
            "PlusSigns", "WavesPattern", "GalaxyStarsPattern", "GridDots", "HexDots",
            "FractalTree", "Voronoi", "FractalBranches", "OrganicMaze", "BiomorphicCells",
            "RadialSunburst", "Sunburst", "Ziggurat", "SpiralPattern", "PentaflakeFractal",
            "HilbertCurve", "SierpinskiTriangle", "PenroseTiling", "EinsteinMonotile",
            "LeafVeins", "WoodPlanks", "ParquetHerringbone", "WoodGrain", "DrywallOrangePeel",
            "DrywallKnockdown", "StuccoSandFloat", "StuccoDash", "DrywallSkipTrowel",
            "Concrete", "ConcreteStampedPattern", "ConcreteSaltFinish", "ConcreteFormTiePattern",
            "ConcreteSandblastPattern", "ConcreteControlJoint", "ConcreteGridPattern",
            "WoodKnotPattern", "ConcreteAggregatePattern", "BrushedConcrete", "PebbleConcrete",
            "CrackedConcrete", "AggregateConcrete", "StampedConcrete", "Insulation", "Rebar", "RoofTiles"
        ])

        self.customPatternLabel = QtWidgets.QLabel("Custom Pattern:")
        self.customPatternCombo = QtWidgets.QComboBox()
        self.pickCustomBtn = QtWidgets.QPushButton("Pick Custom Pattern from Selection")
        self.pickCustomBtn.clicked.connect(self.pickCustomPattern)
        
        self.patternSearchLabel = QtWidgets.QLabel("Search:")
        self.patternSearchField = QtWidgets.QLineEdit()
        self.patternTypeFilterLabel = QtWidgets.QLabel("Type Filter:")
        self.patternTypeFilterCombo = QtWidgets.QComboBox()
        for cat in ["All", "Sketch", "PartFeature", "Compound", "Other"]:
            self.patternTypeFilterCombo.addItem(cat)

        self.patternObjectsLabel = QtWidgets.QLabel("Additional Pattern Objects:")
        self.patternObjectsList = QtWidgets.QListWidget()
        self.patternObjectsList.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)

        self.distLabel = QtWidgets.QLabel("Distribution Mode:")
        self.distCombo = QtWidgets.QComboBox()
        self.distCombo.addItems([
            "CenteredTiling", "RelativeSpacing", "SeamlessTiling",
            "LinearGrid", "RadialDistribution", "ConcentricDistribution",
            "RandomDistribution", "AdaptiveDistribution"
        ])
        self.distCombo.setCurrentText("SeamlessTiling")
        
        self.autoScaleCheck = QtWidgets.QCheckBox("Auto Scale to Fit Base?")
        
        self.scaleLabel = QtWidgets.QLabel("Pattern Scale:")
        self.scaleSpin = QtWidgets.QDoubleSpinBox()
        self.scaleSpin.setRange(0.0001, 1e5)
        self.scaleSpin.setValue(1.0)
        self.scaleSpin.valueChanged.connect(self.onScaleChanged)
        
        self.scaleResetBtn = QtWidgets.QPushButton("↺")
        self.scaleResetBtn.setToolTip("Reset to pattern default scale")
        self.scaleResetBtn.clicked.connect(lambda: self.setSmartDefaultScale(
            self.patternSourceCombo.currentIndex() == 0
        ))
        
        self.rotLabel = QtWidgets.QLabel("Rotation (deg):")
        self.rotSpin = QtWidgets.QDoubleSpinBox()
        self.rotSpin.setRange(-360, 360)
        
        self.spacingLabel = QtWidgets.QLabel("Base Spacing:")
        self.spacingInput = FreeCADGui.UiLoader().createWidget("Gui::InputField")
        self.spacingInput.setProperty("unitCategory", "Length")
        self.spacingInput.setText("10 mm")
        
        self.repXLabel = QtWidgets.QLabel("Repetitions X:")
        self.repXSpin = QtWidgets.QSpinBox()
        self.repXSpin.setRange(1, 999)
        self.repXSpin.setValue(5)
        self.repYLabel = QtWidgets.QLabel("Repetitions Y:")
        self.repYSpin = QtWidgets.QSpinBox()
        self.repYSpin.setRange(1, 999)
        self.repYSpin.setValue(5)

        self.showFacesCheck = QtWidgets.QCheckBox("Show Faces?")
        self.showFacesCheck.setChecked(False)
        
        self.subtractionsLabel = QtWidgets.QLabel("Subtractions (cut these out):")
        self.subtractionsList = QtWidgets.QListWidget()
        self.subtractionsList.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)
        self.pickSubBtn = QtWidgets.QPushButton("Pick Subtractions from Selection")
        self.pickSubBtn.clicked.connect(self.pickSubtractions)

        # --- PREVIEW BUTTON ON MAIN TAB (NEW) ---
        self.previewBtnMain = QtWidgets.QPushButton("Preview (quick test)")
        self.previewBtnMain.setToolTip(
            "Generate a quick preview of the hatch with current settings. "
            "Use MaxTilesAllowed to limit compute time."
        )
        self.previewBtnMain.clicked.connect(self.onPreview)

        # Advanced Tab Elements
        self.tileLabel = QtWidgets.QLabel("Base Tile (Optional):")
        self.tileCombo = QtWidgets.QComboBox()
        self.tileCombo.addItem("")
        self.pickTileBtn = QtWidgets.QPushButton("Pick Base Tile from Selection")
        self.pickTileBtn.clicked.connect(self.pickBaseTile)
        
        self.tileVisibilityCheck = QtWidgets.QCheckBox("Tile Visibility?")
        self.tileVisibilityCheck.setChecked(True)

        self.placementModeLabel = QtWidgets.QLabel("Pattern Placement Mode:")
        self.placementModeCombo = QtWidgets.QComboBox()
        self.placementModeCombo.addItems([
            "Origin", "Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight",
            "TopCenter", "BottomCenter", "LeftCenter", "RightCenter", "Custom"
        ])
        self.placementModeCombo.setCurrentText("Origin")

        self.lockCheck = QtWidgets.QCheckBox("Lock to Base?")
        
        self.offsetXLabel = QtWidgets.QLabel("Pattern Offset X:")
        self.offsetXSpin = QtWidgets.QDoubleSpinBox()
        self.offsetXSpin.setRange(-1e5, 1e5)
        self.offsetYLabel = QtWidgets.QLabel("Pattern Offset Y:")
        self.offsetYSpin = QtWidgets.QDoubleSpinBox()
        self.offsetYSpin.setRange(-1e5, 1e5)
        
        self.scaleModeLabel = QtWidgets.QLabel("Scale Mode:")
        self.scaleModeCombo = QtWidgets.QComboBox()
        self.scaleModeCombo.addItems(["Absolute", "FitWidth", "FitHeight", "FitMinDim", "FitMaxDim"])
        
        self.randomCheck = QtWidgets.QCheckBox("Randomize Placement?")
        self.offRangeLabel = QtWidgets.QLabel("Random Offset ±:")
        self.offRangeSpin = QtWidgets.QDoubleSpinBox()
        self.offRangeSpin.setRange(0.0, 1e4)
        self.rotRangeLabel = QtWidgets.QLabel("Random Rot ± (deg):")
        self.rotRangeSpin = QtWidgets.QDoubleSpinBox()
        self.rotRangeSpin.setRange(0.0, 360.0)
        self.scaleMinLabel = QtWidgets.QLabel("Random Scale Min:")
        self.scaleMinSpin = QtWidgets.QDoubleSpinBox()
        self.scaleMinSpin.setRange(0.01, 1e4)
        self.scaleMinSpin.setValue(1.0)
        self.scaleMaxLabel = QtWidgets.QLabel("Random Scale Max:")
        self.scaleMaxSpin = QtWidgets.QDoubleSpinBox()
        self.scaleMaxSpin.setRange(0.01, 1e4)
        self.scaleMaxSpin.setValue(1.0)
        
        self.apply3DCheck = QtWidgets.QCheckBox("Apply to 3D Surface?")
        self.maxTilesLabel = QtWidgets.QLabel("Max Tiles Allowed:")
        self.maxTilesSpin = QtWidgets.QSpinBox()
        self.maxTilesSpin.setRange(1, 999999)
        self.maxTilesSpin.setValue(1000)
        
        self.densityLabel = QtWidgets.QLabel("Density Factor (0-1):")
        self.densitySpin = QtWidgets.QDoubleSpinBox()
        self.densitySpin.setRange(0.0, 1.0)
        self.densitySpin.setValue(1.0)
        
        self.enableColorVarCheck = QtWidgets.QCheckBox("Enable Color Variation?")
        self.colorVarLabel = QtWidgets.QLabel("Color Variation Intensity (0-1):")
        self.colorVarSpin = QtWidgets.QDoubleSpinBox()
        self.colorVarSpin.setRange(0.0, 1.0)
        self.colorVarSpin.setValue(0.5)

        self.clipModeLabel = QtWidgets.QLabel("Clip Mode:")
        self.clipModeCombo = QtWidgets.QComboBox()
        self.clipModeCombo.addItems(["BooleanOnly", "PreserveLinesNoClip"])

        # Surface Projection Group
        self.surfaceProjectionGroup = QtWidgets.QGroupBox("Surface Projection")
        self.surfaceProjectionLayout = QtWidgets.QFormLayout(self.surfaceProjectionGroup)
        self.useSurfaceProjectionCheck = QtWidgets.QCheckBox("Use Surface Projection (works on walls, sloped roofs)")
        self.useSurfaceProjectionCheck.setChecked(True)
        self.forceXYPlaneCheck = QtWidgets.QCheckBox("Force XY Plane (old behavior)")
        self.forceXYPlaneCheck.setChecked(False)
        self.surfaceProjectionLayout.addRow(self.useSurfaceProjectionCheck)
        self.surfaceProjectionLayout.addRow(self.forceXYPlaneCheck)

        # Preview Button on Advanced Tab
        self.previewBtn = QtWidgets.QPushButton("Preview")
        self.previewBtn.clicked.connect(self.onPreview)
        
        # --- KEEP PREVIEW CHECKBOX (NEW) ---
        self.keepPreviewCheck = QtWidgets.QCheckBox(
            "Keep preview shape (non-parametric)"
        )
        self.keepPreviewCheck.setToolTip(
            "When checked, the preview shape is kept as a static Part::Feature "
            "after preview generation. Useful when you want to work with a "
            "simplified non-parametric shape without committing to a full hatch."
        )
        self.keepPreviewCheck.setChecked(False)

        # --- Populate Main Tab Layout ---
        baseRowLayout = QtWidgets.QHBoxLayout()
        baseRowLayout.addWidget(self.baseSearchLabel)
        baseRowLayout.addWidget(self.baseSearchField)
        baseRowLayout.addWidget(self.baseTypeFilterLabel)
        baseRowLayout.addWidget(self.baseTypeFilterCombo)

        self.mainTabLayout.addRow(self.baseLabel, self.baseCombo)
        self.mainTabLayout.addRow("", self.pickBaseBtn)
        self.mainTabLayout.addRow(self.faceExtractorGroup)  # NEW: Face Extractor group
        self.mainTabLayout.addRow(baseRowLayout)
        self.mainTabLayout.addRow(self.baseObjectsLabel, self.baseObjectsList)
        self.mainTabLayout.addRow(self.patternSourceLabel, self.patternSourceCombo)
        self.mainTabLayout.addRow(self.builtinPatternLabel, self.builtinPatternCombo)
        
        patternRowLayout = QtWidgets.QHBoxLayout()
        patternRowLayout.addWidget(self.patternSearchLabel)
        patternRowLayout.addWidget(self.patternSearchField)
        patternRowLayout.addWidget(self.patternTypeFilterLabel)
        patternRowLayout.addWidget(self.patternTypeFilterCombo)
        self.mainTabLayout.addRow(self.customPatternLabel, self.customPatternCombo)
        self.mainTabLayout.addRow("", self.pickCustomBtn)
        self.mainTabLayout.addRow(patternRowLayout)
        self.mainTabLayout.addRow(self.patternObjectsLabel, self.patternObjectsList)
        
        self.mainTabLayout.addRow(self.distLabel, self.distCombo)
        self.mainTabLayout.addRow(self.autoScaleCheck)
        
        scaleRow = QtWidgets.QHBoxLayout()
        scaleRow.addWidget(self.scaleSpin)
        scaleRow.addWidget(self.scaleResetBtn)
        self.mainTabLayout.addRow(self.scaleLabel, scaleRow)
        
        self.mainTabLayout.addRow(self.rotLabel, self.rotSpin)
        self.mainTabLayout.addRow(self.spacingLabel, self.spacingInput)
        self.mainTabLayout.addRow(self.repXLabel, self.repXSpin)
        self.mainTabLayout.addRow(self.repYLabel, self.repYSpin)
        self.mainTabLayout.addRow(self.showFacesCheck)
        self.mainTabLayout.addRow(self.subtractionsLabel, self.subtractionsList)
        self.mainTabLayout.addRow("", self.pickSubBtn)
        
        # --- ADD PREVIEW BUTTON TO MAIN TAB (NEW) ---
        self.mainTabLayout.addRow(self.previewBtnMain)

        # PATCH 3: FIXED Advanced Tab Layout - use scroll area with VBoxLayout instead of QFormLayout
        # --- Advanced Tab: use a scroll area containing a plain VBoxLayout ---
        # This prevents groups from being squashed into label/widget columns.

        advScrollArea = QtWidgets.QScrollArea()
        advScrollArea.setWidgetResizable(True)
        advScrollArea.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        advScrollArea.setFrameShape(QtWidgets.QFrame.NoFrame)

        advScrollContent = QtWidgets.QWidget()
        advVBox = QtWidgets.QVBoxLayout(advScrollContent)
        advVBox.setSpacing(8)
        advVBox.setContentsMargins(4, 4, 4, 4)

        # --- Tile Settings ---
        tileGroup = QtWidgets.QGroupBox("Tile Settings")
        tileForm = QtWidgets.QFormLayout(tileGroup)
        tileForm.setFieldGrowthPolicy(QtWidgets.QFormLayout.ExpandingFieldsGrow)
        tileForm.addRow(self.tileLabel, self.tileCombo)
        tileForm.addRow("", self.pickTileBtn)
        tileForm.addRow(self.tileVisibilityCheck)
        advVBox.addWidget(tileGroup)

        # --- Placement ---
        placementGroup = QtWidgets.QGroupBox("Placement")
        placementForm = QtWidgets.QFormLayout(placementGroup)
        placementForm.setFieldGrowthPolicy(QtWidgets.QFormLayout.ExpandingFieldsGrow)
        placementForm.addRow(self.placementModeLabel, self.placementModeCombo)
        placementForm.addRow(self.lockCheck)
        placementForm.addRow(self.offsetXLabel, self.offsetXSpin)
        placementForm.addRow(self.offsetYLabel, self.offsetYSpin)
        placementForm.addRow(self.scaleModeLabel, self.scaleModeCombo)
        advVBox.addWidget(placementGroup)

        # --- Randomization ---
        randGroup = QtWidgets.QGroupBox("Randomization")
        randForm = QtWidgets.QFormLayout(randGroup)
        randForm.setFieldGrowthPolicy(QtWidgets.QFormLayout.ExpandingFieldsGrow)
        randForm.addRow(self.randomCheck)
        randForm.addRow(self.offRangeLabel, self.offRangeSpin)
        randForm.addRow(self.rotRangeLabel, self.rotRangeSpin)
        randForm.addRow(self.scaleMinLabel, self.scaleMinSpin)
        randForm.addRow(self.scaleMaxLabel, self.scaleMaxSpin)
        advVBox.addWidget(randGroup)

        # --- Rendering & Performance ---
        renderGroup = QtWidgets.QGroupBox("Rendering & Performance")
        renderForm = QtWidgets.QFormLayout(renderGroup)
        renderForm.setFieldGrowthPolicy(QtWidgets.QFormLayout.ExpandingFieldsGrow)
        renderForm.addRow(self.clipModeLabel, self.clipModeCombo)
        renderForm.addRow(self.apply3DCheck)
        renderForm.addRow(self.maxTilesLabel, self.maxTilesSpin)
        advVBox.addWidget(renderGroup)

        # --- Variation ---
        variationGroup = QtWidgets.QGroupBox("Variation")
        variationForm = QtWidgets.QFormLayout(variationGroup)
        variationForm.setFieldGrowthPolicy(QtWidgets.QFormLayout.ExpandingFieldsGrow)
        variationForm.addRow(self.densityLabel, self.densitySpin)
        variationForm.addRow(self.enableColorVarCheck)
        variationForm.addRow(self.colorVarLabel, self.colorVarSpin)
        advVBox.addWidget(variationGroup)

        # --- Surface Projection ---
        advVBox.addWidget(self.surfaceProjectionGroup)

        # --- Preview and Keep Checkbox ---
        advVBox.addWidget(self.previewBtn)
        advVBox.addWidget(self.keepPreviewCheck)   # ← ADD THIS LINE
        
        advVBox.addStretch(1)  # pushes groups to top, leaves space at bottom

        advScrollArea.setWidget(advScrollContent)
        # Replace the advancedTab's default layout content with the scroll area
        self.advancedTabLayout.addWidget(advScrollArea)

        # Set scroll area widget and add to main layout
        scroll_area.setWidget(scroll_content)
        main_layout.addWidget(scroll_area)

        # Connect signals
        self.baseCombo.currentIndexChanged.connect(self.onBaseComboIndexChanged)
        self.baseTypeFilterCombo.currentIndexChanged.connect(self.refreshBaseCombo)
        self.baseSearchField.textChanged.connect(lambda: QtCore.QTimer.singleShot(300, self.refreshBaseCombo))
        
        self.patternTypeFilterCombo.currentIndexChanged.connect(self.refreshCustomPatternCombo)
        self.patternSearchField.textChanged.connect(lambda: QtCore.QTimer.singleShot(300, self.refreshCustomPatternCombo))
        self.customPatternCombo.currentIndexChanged.connect(self.onCustomPatternSelected)
        self.tileCombo.currentIndexChanged.connect(self.onTileComboIndexChanged)
        
        # Initial population
        self.refreshBaseCombo()
        self.refreshCustomPatternCombo()
        self.populateMultiList(self.baseObjectsList)
        self.populateMultiList(self.patternObjectsList)
        self.populateMultiList(self.subtractionsList)
        self.populateTileCombo()
        
        return main_widget

    # ------------------------------------------------------------------------
    # Face Extractor Method (NEW)
    # ------------------------------------------------------------------------
    def pickFaceAndCreateExtractor(self):
        """
        Task panel method — reads the current viewport selection, 
        creates FaceExtractor objects for each selected face, 
        and sets the first one as the BaseCombo selection.
        """
        sel = FreeCADGui.Selection.getSelectionEx()
        if not sel:
            self.faceExtractorStatus.setText("No selection. Click a face in 3D view first.")
            self.faceExtractorStatus.setStyleSheet("color: red; font-style: italic;")
            return

        # Check we have at least one Face sub-element
        has_face = any(
            sub.startswith("Face")
            for s in sel
            for sub in s.SubElementNames
        )
        if not has_face:
            self.faceExtractorStatus.setText(
                "No face selected. Click a face surface, not an edge or vertex."
            )
            self.faceExtractorStatus.setStyleSheet("color: red; font-style: italic;")
            return

        created = makeFaceExtractorFromSelection()

        if not created:
            self.faceExtractorStatus.setText("Face extraction failed. See Report View.")
            self.faceExtractorStatus.setStyleSheet("color: red; font-style: italic;")
            return

        # Refresh the master object list so the new FaceExtractor appears
        self.masterObjectList = self.getAllObjectsClassified()
        self.refreshBaseCombo()

        # Auto-select the first created extractor as the base shape
        first = created[0]
        idx = self.baseCombo.findText(first.Name)
        if idx == -1:
            # Try by label
            idx = self.baseCombo.findText(first.Label)
        if idx >= 0:
            self.baseCombo.setCurrentIndex(idx)

        names = ", ".join(fe.Name for fe in created)
        self.faceExtractorStatus.setText(f"Created: {names}")
        self.faceExtractorStatus.setStyleSheet("color: green; font-style: italic;")
        FreeCAD.Console.PrintMessage(
            f"FaceExtractor(s) created and set as Base Shape: {names}\n"
        )

    # ------------------------------------------------------------------------
    # Logic Methods
    # ------------------------------------------------------------------------
    def getAllObjectsClassified(self):
        data = []
        if self.doc:
            for obj in self.doc.Objects:
                if not hasattr(obj, "Shape"):
                    continue
                cat = self.classifyObject(obj)
                data.append((obj.Name, cat, obj))
        return data

    def classifyObject(self, obj):
        if obj.isDerivedFrom("Sketcher::SketchObject"):
            return "Sketch"
        elif obj.isDerivedFrom("Part::Compound"):
            return "Compound"
        elif obj.isDerivedFrom("Part::Feature"):
            return "PartFeature"
        else:
            return "Other"

    def filterObjects(self, searchText, category):
        results = []
        lowSearch = searchText.lower().strip()
        for name, cat, obj in self.masterObjectList:
            if category != "All" and cat != category:
                continue
            if lowSearch and lowSearch not in name.lower():
                continue
            results.append((name, cat, obj))
        return results

    def refreshBaseCombo(self):
        sText = self.baseSearchField.text()
        cText = self.baseTypeFilterCombo.currentText()
        flist = self.filterObjects(sText, cText)
        self.baseCombo.clear()
        for (nm, cat, o) in flist:
            self.baseCombo.addItem(nm)

    def refreshCustomPatternCombo(self):
        sText = self.patternSearchField.text()
        cText = self.patternTypeFilterCombo.currentText()
        filtered = self.filterObjects(sText, cText)
        self.customPatternCombo.clear()
        self.customPatternCombo.addItem("")
        for (name, category, obj) in filtered:
            self.customPatternCombo.addItem(name)

    def populateMultiList(self, listWidget):
        listWidget.clear()
        for (nm, cat, obj) in self.masterObjectList:
            listWidget.addItem(nm)

    def populateTileCombo(self):
        self.tileCombo.clear()
        self.tileCombo.addItem("")
        for (nm, cat, obj) in self.masterObjectList:
            self.tileCombo.addItem(nm)

    def getSelectedObjectsFromList(self, listWidget):
        selectedItems = listWidget.selectedItems()
        selectedObjs = []
        for it in selectedItems:
            objName = it.text()
            fcObj = self.doc.getObject(objName)
            if fcObj:
                selectedObjs.append(fcObj)
        return selectedObjs

    def initialPatternSetup(self):
        is_builtin = self.patternSourceCombo.currentIndex() == 0
        self.setSmartDefaultScale(is_builtin)
        self.last_manual_scale = None
        self._update_pattern_controls_visibility(show_builtin=is_builtin)

    def onPatternSourceChanged(self, index):
        is_builtin = (index == 0)
        if self.last_manual_scale is None:
            self.setSmartDefaultScale(is_builtin)
        else:
            self.scaleSpin.setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; }")
        self._update_pattern_controls_visibility(show_builtin=is_builtin)

    def _update_pattern_controls_visibility(self, show_builtin=True):
        self.builtinPatternLabel.setVisible(show_builtin)
        self.builtinPatternCombo.setVisible(show_builtin)
        show_custom = not show_builtin
        self.customPatternLabel.setVisible(show_custom)
        self.customPatternCombo.setVisible(show_custom)
        self.patternSearchLabel.setVisible(show_custom)
        self.patternSearchField.setVisible(show_custom)
        self.patternTypeFilterLabel.setVisible(show_custom)
        self.patternTypeFilterCombo.setVisible(show_custom)
        self.pickCustomBtn.setVisible(show_custom)
        self.patternObjectsLabel.setVisible(show_custom)
        self.patternObjectsList.setVisible(show_custom)

    def setSmartDefaultScale(self, is_builtin):
        default_scale = 20.0 if is_builtin else 1.0
        self.scaleSpin.setValue(default_scale)
        self.last_manual_scale = None
        self.scaleSpin.setStyleSheet("")

    def onScaleChanged(self, value):
        self.last_manual_scale = value
        self.scaleSpin.setStyleSheet("")

    def onBaseComboIndexChanged(self, index):
        name = self.baseCombo.itemText(index)
        self.highlightObject(name)

    def onTileComboIndexChanged(self, index):
        name = self.tileCombo.itemText(index)
        if name:
            self.highlightObject(name)

    def onCustomPatternSelected(self):
        name = self.customPatternCombo.currentText()
        if name:
            self.highlightObject(name)

    def highlightObject(self, objName):
        if not objName:
            return
        FreeCADGui.Selection.clearSelection()
        obj = self.doc.getObject(objName)
        if obj:
            FreeCADGui.Selection.addSelection(obj)

    def pickBaseShape(self):
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            QtWidgets.QMessageBox.warning(self.form, "No selection", "Select an object in 3D view or tree.")
            return
        obj = sel[0]
        label = obj.Label
        idx = self.baseCombo.findText(label)
        if idx == -1:
            self.baseCombo.addItem(label)
            idx = self.baseCombo.findText(label)
        self.baseCombo.setCurrentIndex(idx)

    def pickCustomPattern(self):
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            QtWidgets.QMessageBox.warning(self.form, "No selection", "Select an object in 3D view or tree.")
            return
        obj = sel[0]
        label = obj.Label
        self.patternSourceCombo.setCurrentIndex(1)
        idx = self.customPatternCombo.findText(label)
        if idx == -1:
            self.customPatternCombo.addItem(label)
            idx = self.customPatternCombo.findText(label)
        self.customPatternCombo.setCurrentIndex(idx)

    def pickSubtractions(self):
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            QtWidgets.QMessageBox.warning(self.form, "No selection", "Select one or more objects.")
            return
        for obj in sel:
            label = obj.Label
            items = self.subtractionsList.findItems(label, QtCore.Qt.MatchExactly)
            if not items:
                self.subtractionsList.addItem(label)

    def pickBaseTile(self):
        sel = FreeCADGui.Selection.getSelection()
        if not sel:
            QtWidgets.QMessageBox.warning(self.form, "No selection", "Select an object in 3D view or tree.")
            return
        obj = sel[0]
        label = obj.Label
        idx = self.tileCombo.findText(label)
        if idx == -1:
            self.tileCombo.addItem(label)
            idx = self.tileCombo.findText(label)
        self.tileCombo.setCurrentIndex(idx)

    def getBaseSpacingInMM(self):
        q = self.spacingInput.property("quantity")
        return q.Value if q else 0.0

    # ============================================================================
    # PATCH 4: REAL PREVIEW IMPLEMENTATION
    # ============================================================================
    def onPreview(self):
        """
        Generate a fast preview hatch using current settings.

        Strategy for speed:
        - Creates a temporary CustomHatch param object
        - Calls recompute() (MaxTilesAllowed limits tile count)
        - Copies the resulting shape to a HatchPreview Part::Feature
        - Removes the temp param object immediately
        - Colors the preview green + 50% transparent so it's visually distinct
        - If keepPreviewCheck is checked, leaves the feature in the document
          so the user can use it as a static non-parametric shape
        """
        doc = self.doc
        if not doc:
            return

        baseName = self.baseCombo.currentText()
        base_obj = doc.getObject(baseName)
        if not base_obj or not hasattr(base_obj, "Shape"):
            QtWidgets.QMessageBox.warning(
                self.form, "Preview Error",
                "Select a valid base shape before previewing."
            )
            return

        # Remove any existing preview from a previous run
        if self.current_temp_preview_name:
            old = doc.getObject(self.current_temp_preview_name)
            if old:
                doc.removeObject(self.current_temp_preview_name)
            self.current_temp_preview_name = None

        temp_name = None
        preview_name = None
        
        try:
            # 1. Create a temporary param object (not shown in tree)
            temp_name = doc.getUniqueObjectName("_HatchPreviewTemp")
            tempHatch = makeCustomHatch(name=temp_name)

            # 2. Wire all current UI values onto it
            if not self._apply_properties_to(tempHatch):
                doc.removeObject(temp_name)
                return

            # Clamp tiles for speed — use at most 100 for preview
            tempHatch.MaxTilesAllowed = min(
                int(self.maxTilesSpin.value()), 100
            )

            # 3. Recompute so execute() runs and Shape is populated
            doc.recompute()

            # 4. Copy the shape
            if tempHatch.Shape and not tempHatch.Shape.isNull():
                preview_shape = tempHatch.Shape.copy()
            else:
                doc.removeObject(temp_name)
                QtWidgets.QMessageBox.warning(
                    self.form, "Preview", "Preview generated an empty shape. "
                    "Check base object and pattern settings."
                )
                return

            show_faces = tempHatch.ShowFaces

            # 5. Remove the temp param object immediately
            doc.removeObject(temp_name)
            temp_name = None

            # 6. Create the preview feature
            preview_name = doc.getUniqueObjectName("HatchPreview")
            preview_feat = doc.addObject("Part::Feature", preview_name)
            preview_feat.Shape = preview_shape

            if FreeCAD.GuiUp:
                if show_faces:
                    preview_feat.ViewObject.DisplayMode = "Flat Lines"
                    preview_feat.ViewObject.ShapeColor = (0.0, 0.8, 0.2)
                    preview_feat.ViewObject.Transparency = 50
                else:
                    preview_feat.ViewObject.DisplayMode = "Wireframe"
                    preview_feat.ViewObject.ShapeColor = (0.0, 0.8, 0.2)
                    preview_feat.ViewObject.LineWidth = 2.0

            doc.recompute()

            keep = self.keepPreviewCheck.isChecked()
            if keep:
                # Rename to something clean without the temp suffix
                preview_feat.Label = "HatchPreview_Kept"
                FreeCAD.Console.PrintMessage(
                    f"Preview shape kept as '{preview_name}'. "
                    "It is a static non-parametric shape — "
                    "it will NOT update if you change settings.\n"
                )
                self.current_temp_preview_name = None  # don't auto-delete next time
            else:
                self.current_temp_preview_name = preview_name

            # Report stats from the completed shape
            num_edges = len(preview_shape.Edges)
            num_faces = len(preview_shape.Faces)
            QtWidgets.QMessageBox.information(
                self.form, "Preview Generated",
                f"Preview complete (≤100 tiles for speed).\n"
                f"Edges: {num_edges}  Faces: {num_faces}\n\n"
                f"{'Shape kept as non-parametric feature.' if keep else 'Shape will be removed on next preview or on Create.'}\n\n"
                "Tip: Adjust MaxTilesAllowed in Advanced tab\n"
                "and Base Spacing / Scale to tune density."
            )

        except Exception as e:
            FreeCAD.Console.PrintError(f"Preview failed: {e}\n")
            QtWidgets.QMessageBox.critical(
                self.form, "Preview Error", f"Preview failed:\n{str(e)}"
            )
            # Clean up any temp objects if something went wrong
            for name in [temp_name, preview_name]:
                if name:
                    obj = doc.getObject(name)
                    if obj:
                        doc.removeObject(name)

    def _cleanup_preview(self):
        """Remove any temporary preview shape."""
        if self.current_temp_preview_name:
            obj = self.doc.getObject(self.current_temp_preview_name)
            if obj:
                self.doc.removeObject(self.current_temp_preview_name)
            self.current_temp_preview_name = None

    # PATCH 2b: _load_from_object method for populating UI from existing hatch
    def _load_from_object(self, obj):
        """Populate all UI widgets from an existing hatch object's properties."""
        try:
            # Base object
            if obj.BaseObject:
                name = obj.BaseObject.Name
                idx = self.baseCombo.findText(name)
                if idx == -1:
                    self.baseCombo.addItem(name)
                    idx = self.baseCombo.findText(name)
                self.baseCombo.setCurrentIndex(max(idx, 0))

            # Pattern type
            pt = getattr(obj, "PatternType", "CustomObject")
            if pt == "CustomObject":
                self.patternSourceCombo.setCurrentIndex(1)
                if obj.PatternObject:
                    pname = obj.PatternObject.Name
                    pidx = self.customPatternCombo.findText(pname)
                    if pidx == -1:
                        self.customPatternCombo.addItem(pname)
                        pidx = self.customPatternCombo.findText(pname)
                    self.customPatternCombo.setCurrentIndex(max(pidx, 0))
            else:
                self.patternSourceCombo.setCurrentIndex(0)
                bidx = self.builtinPatternCombo.findText(pt)
                if bidx >= 0:
                    self.builtinPatternCombo.setCurrentIndex(bidx)

            # Distribution / Scale / Rotation / Spacing
            didx = self.distCombo.findText(getattr(obj, "DistributionMode", "SeamlessTiling"))
            if didx >= 0:
                self.distCombo.setCurrentIndex(didx)
            self.autoScaleCheck.setChecked(bool(getattr(obj, "AutoScaleToFitBase", False)))
            self.scaleSpin.setValue(float(getattr(obj, "PatternScale", 1.0)))
            self.rotSpin.setValue(float(getattr(obj, "RotationDeg", 0.0)))
            spacing = getattr(obj, "BaseSpacing", 10.0)
            self.spacingInput.setProperty("quantity",
                FreeCAD.Units.Quantity(spacing, FreeCAD.Units.Length))
            self.repXSpin.setValue(int(getattr(obj, "RepetitionsX", 5)))
            self.repYSpin.setValue(int(getattr(obj, "RepetitionsY", 5)))

            # Placement / offsets
            smidx = self.scaleModeCombo.findText(getattr(obj, "ScaleMode", "Absolute"))
            if smidx >= 0:
                self.scaleModeCombo.setCurrentIndex(smidx)
            self.lockCheck.setChecked(bool(getattr(obj, "LockToBase", False)))
            self.offsetXSpin.setValue(float(getattr(obj, "PatternOffsetX", 0.0)))
            self.offsetYSpin.setValue(float(getattr(obj, "PatternOffsetY", 0.0)))
            pmidx = self.placementModeCombo.findText(
                getattr(obj, "PatternPlacementMode", "Origin"))
            if pmidx >= 0:
                self.placementModeCombo.setCurrentIndex(pmidx)

            # Random
            self.randomCheck.setChecked(bool(getattr(obj, "RandomizePlacement", False)))
            self.offRangeSpin.setValue(float(getattr(obj, "RandomOffsetRange", 0.0)))
            self.rotRangeSpin.setValue(float(getattr(obj, "RandomRotationRange", 0.0)))
            self.scaleMinSpin.setValue(float(getattr(obj, "RandomScaleMin", 1.0)))
            self.scaleMaxSpin.setValue(float(getattr(obj, "RandomScaleMax", 1.0)))

            # Rendering
            self.showFacesCheck.setChecked(bool(getattr(obj, "ShowFaces", False)))
            self.apply3DCheck.setChecked(bool(getattr(obj, "ApplyTo3DSurface", False)))
            self.maxTilesSpin.setValue(int(getattr(obj, "MaxTilesAllowed", 1000)))
            cmidx = self.clipModeCombo.findText(getattr(obj, "ClipMode", "BooleanOnly"))
            if cmidx >= 0:
                self.clipModeCombo.setCurrentIndex(cmidx)

            # Variation
            self.densitySpin.setValue(float(getattr(obj, "DensityFactor", 1.0)))
            self.enableColorVarCheck.setChecked(bool(getattr(obj, "EnableColorVariation", False)))
            self.colorVarSpin.setValue(float(getattr(obj, "ColorVariationIntensity", 0.5)))

            # Surface projection
            self.useSurfaceProjectionCheck.setChecked(
                bool(getattr(obj, "UseSurfaceProjection", True)))
            self.forceXYPlaneCheck.setChecked(bool(getattr(obj, "ForceXYPlane", False)))

            # Tile
            if getattr(obj, "BaseTileObject", None):
                tname = obj.BaseTileObject.Name
                tidx = self.tileCombo.findText(tname)
                if tidx == -1:
                    self.tileCombo.addItem(tname)
                    tidx = self.tileCombo.findText(tname)
                self.tileCombo.setCurrentIndex(tidx)
            self.tileVisibilityCheck.setChecked(bool(getattr(obj, "TileVisibility", True)))

        except Exception as e:
            FreeCAD.Console.PrintWarning(f"_load_from_object partial failure: {e}\n")

    # PATCH 2c: Replaced accept with create/edit branching logic
    def accept(self):
        """OK pressed — create a new hatch OR update the one being edited."""
        # Clean up preview before closing
        self._cleanup_preview()
        
        if self.editing_obj is not None:
            self._accept_edit()
        else:
            self._accept_create()
        return True

    def _apply_properties_to(self, hatchObj):
        """Write all UI values onto hatchObj (shared by create and edit paths)."""
        baseName = self.baseCombo.currentText()
        base_obj = self.doc.getObject(baseName)
        if not base_obj or not hasattr(base_obj, "Shape"):
            QtWidgets.QMessageBox.warning(
                self.form, "Error", "Invalid base object selection")
            return False

        if self.patternSourceCombo.currentIndex() == 0:
            hatchObj.PatternType = self.builtinPatternCombo.currentText()
        else:
            hatchObj.PatternType = "CustomObject"
            pname = self.customPatternCombo.currentText()
            if pname:
                po = self.doc.getObject(pname)
                if po:
                    hatchObj.PatternObject = po

        hatchObj.BaseObject = base_obj
        hatchObj.ClipMode = self.clipModeCombo.currentText()
        hatchObj.UseSurfaceProjection = self.useSurfaceProjectionCheck.isChecked()
        hatchObj.ForceXYPlane = self.forceXYPlaneCheck.isChecked()
        hatchObj.BaseObjects = self.getSelectedObjectsFromList(self.baseObjectsList)
        hatchObj.PatternObjects = self.getSelectedObjectsFromList(self.patternObjectsList)
        hatchObj.Subtractions = self.getSelectedObjectsFromList(self.subtractionsList)
        hatchObj.DistributionMode = self.distCombo.currentText()
        hatchObj.AutoScaleToFitBase = self.autoScaleCheck.isChecked()
        hatchObj.PatternScale = float(self.scaleSpin.value())
        hatchObj.RotationDeg = float(self.rotSpin.value())
        hatchObj.BaseSpacing = float(self.getBaseSpacingInMM())
        hatchObj.RepetitionsX = int(self.repXSpin.value())
        hatchObj.RepetitionsY = int(self.repYSpin.value())
        hatchObj.RandomizePlacement = self.randomCheck.isChecked()
        hatchObj.RandomOffsetRange = float(self.offRangeSpin.value())
        hatchObj.RandomRotationRange = float(self.rotRangeSpin.value())
        hatchObj.RandomScaleMin = float(self.scaleMinSpin.value())
        hatchObj.RandomScaleMax = float(self.scaleMaxSpin.value())
        hatchObj.LockToBase = self.lockCheck.isChecked()
        hatchObj.PatternOffsetX = float(self.offsetXSpin.value())
        hatchObj.PatternOffsetY = float(self.offsetYSpin.value())
        hatchObj.ScaleMode = self.scaleModeCombo.currentText()
        hatchObj.ShowFaces = self.showFacesCheck.isChecked()
        hatchObj.ApplyTo3DSurface = self.apply3DCheck.isChecked()
        hatchObj.MaxTilesAllowed = int(self.maxTilesSpin.value())
        hatchObj.DensityFactor = float(self.densitySpin.value())
        hatchObj.EnableColorVariation = self.enableColorVarCheck.isChecked()
        hatchObj.ColorVariationIntensity = float(self.colorVarSpin.value())
        hatchObj.PatternPlacementMode = self.placementModeCombo.currentText()
        tname = self.tileCombo.currentText().strip()
        if tname:
            to = self.doc.getObject(tname)
            if to:
                hatchObj.BaseTileObject = to
        hatchObj.TileVisibility = self.tileVisibilityCheck.isChecked()
        return True

    def _accept_create(self):
        """Create a new hatch object."""
        try:
            FreeCAD.ActiveDocument.openTransaction("Create Hatch")
            hatch_objects = [o for o in self.doc.Objects if o.Name.startswith("CustomHatch")]
            max_num = 0
            for o in hatch_objects:
                suffix = o.Name[len("CustomHatch"):]
                if suffix.isdigit():
                    max_num = max(max_num, int(suffix))
            hatch_name = "CustomHatch" if max_num == 0 else f"CustomHatch{max_num + 1:03d}"
            hatchObj = makeCustomHatch(name=hatch_name)
            if not self._apply_properties_to(hatchObj):
                self.doc.removeObject(hatchObj.Name)
                FreeCAD.ActiveDocument.abortTransaction()
                FreeCADGui.Control.closeDialog()
                return
            self.doc.recompute()
            FreeCAD.ActiveDocument.commitTransaction()
            QtWidgets.QMessageBox.information(
                self.form, "Success",
                f"Hatch created!\nTime: {hatchObj.GenerationTime:.2f}s\n"
                f"Tiles: {hatchObj.TileCount}")
        except Exception as e:
            FreeCAD.ActiveDocument.abortTransaction()
            QtWidgets.QMessageBox.critical(self.form, "Error",
                f"Hatch creation failed:\n{str(e)}")
        FreeCADGui.Control.closeDialog()

    def _accept_edit(self):
        """Update the existing hatch object in place."""
        try:
            FreeCAD.ActiveDocument.openTransaction("Edit Hatch")
            if not self._apply_properties_to(self.editing_obj):
                FreeCAD.ActiveDocument.abortTransaction()
                FreeCADGui.Control.closeDialog()
                return
            self.doc.recompute()
            FreeCAD.ActiveDocument.commitTransaction()
            QtWidgets.QMessageBox.information(
                self.form, "Updated",
                f"Hatch updated!\nTime: {self.editing_obj.GenerationTime:.2f}s\n"
                f"Tiles: {self.editing_obj.TileCount}")
        except Exception as e:
            FreeCAD.ActiveDocument.abortTransaction()
            QtWidgets.QMessageBox.critical(self.form, "Error",
                f"Hatch update failed:\n{str(e)}")
        FreeCADGui.Control.closeDialog()

    def reject(self):
        """Called when user presses Cancel or closes the Task panel."""
        # Clean up any preview shape left from onPreview()
        self._cleanup_preview()
        
        FreeCAD.Console.PrintMessage("Hatch creation cancelled.\n")
        FreeCADGui.Control.closeDialog()
        return True


# ============================================================================
# FreeCAD Command Registration - FaceExtractor Command (NEW)
# ============================================================================
class _CommandFaceExtractor:
    """
    FreeCAD command: select a face in the 3D view, run this command,
    and a parametric FaceExtractor object is created.
    This can be used as the Base Shape for a hatch without opening the dialog.
    """

    def GetResources(self):
        return {
            "Pixmap": "Part_Face",
            "MenuText": "Extract Face",
            "ToolTip": (
                "Select a face on any 3D object in the viewport, then run "
                "this command to create a parametric FaceExtractor object. "
                "The extractor updates automatically when the parent changes."
            ),
        }

    def IsActive(self):
        if not FreeCAD.ActiveDocument:
            return False
        if not FreeCAD.GuiUp:
            return False
        sel = FreeCADGui.Selection.getSelectionEx()
        return any(
            sub.startswith("Face")
            for s in sel
            for sub in s.SubElementNames
        )

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Extract Face")
        try:
            created = makeFaceExtractorFromSelection()
            if created:
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.Console.PrintMessage(
                    f"Created {len(created)} FaceExtractor object(s).\n"
                )
            else:
                FreeCAD.ActiveDocument.abortTransaction()
        except Exception as e:
            FreeCAD.ActiveDocument.abortTransaction()
            FreeCAD.Console.PrintError(f"FaceExtractor failed: {e}\n")


class _CommandHatch:
    """FreeCAD Command for the Hatch Generator."""
    def GetResources(self):
        return {
            'Pixmap': 'Draft_Hatch',
            'MenuText': 'Create Hatch',
            'ToolTip': 'Generate parametric hatch patterns on surfaces'
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        panel = HatchTaskPanel()
        FreeCADGui.Control.showDialog(panel)


# ============================================================================
# Command Registration
# ============================================================================
if FreeCAD.GuiUp:
    if 'BIM_Hatch_Dialog' not in FreeCADGui.listCommands():
        FreeCADGui.addCommand('BIM_Hatch_Dialog', _CommandHatch())
        FreeCAD.Console.PrintMessage("Hatch Generator command registered as 'BIM_Hatch_Dialog'\n")
    
    if 'BIM_FaceExtractor' not in FreeCADGui.listCommands():
        FreeCADGui.addCommand('BIM_FaceExtractor', _CommandFaceExtractor())
        FreeCAD.Console.PrintMessage("FaceExtractor command registered as 'BIM_FaceExtractor'\n")


# ============================================================================
# Standalone Macro Support
# ============================================================================
def runAsMacro():
    """Fallback to dialog mode if run as a macro. Includes scroll area fix."""
    from PySide import QtWidgets
    
    mw = FreeCADGui.getMainWindow()
    dlg = QtWidgets.QDialog(mw)
    dlg.setWindowTitle("Custom Hatch Generator (Macro Mode)")
    dlg.resize(450, 700)
    
    layout = QtWidgets.QVBoxLayout(dlg)
    
    scroll = QtWidgets.QScrollArea()
    scroll.setWidgetResizable(True)
    scroll.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
    scroll.setFrameShape(QtWidgets.QFrame.NoFrame)
    
    task_panel = HatchTaskPanel()
    widget = task_panel.form
    
    scroll.setWidget(widget)
    layout.addWidget(scroll)
    
    button_box = QtWidgets.QDialogButtonBox(QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel)
    button_box.accepted.connect(lambda: (task_panel.accept(), dlg.accept()))
    button_box.rejected.connect(lambda: (task_panel.reject(), dlg.reject()))
    layout.addWidget(button_box)
    
    dlg.exec_()

def runHatchGeneratorDialog():
    """Legacy function to maintain compatibility with double-click handlers."""
    if FreeCAD.GuiUp:
        FreeCADGui.runCommand('BIM_Hatch_Dialog')

if __name__ == "__main__":
    if FreeCAD.GuiUp:
        runAsMacro()