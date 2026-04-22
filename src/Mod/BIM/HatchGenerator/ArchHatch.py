# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                          *
# *   Copyright (c) 2026 Regis Benoit Brice Nde Tene <regisndetene@gmail.com>*
# *                                                                          *
# *   This file is part of FreeCAD.                                         *
# *                                                                          *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                          *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                          *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                          *
# ***************************************************************************

import os

import FreeCAD
import FreeCADGui
import Part
import math
from PySide import QtCore, QtGui, QtWidgets
import datetime

from .ArchHatchPatterns import generate_built_in_pattern_shape as generateBuiltInPatternShape
from .ArchHatchCore import buildHatchShape, normalizePatternShape
from .ArchFaceExtractor import make_face_extractor_from_selection


def _icon_path(filename):
    return os.path.join(os.path.dirname(__file__), "Resources", "icons", filename)


def convertBaseSpacingValue(spacing_value, use_units, selected_unit_system):
    if not use_units:
        return spacing_value

    if selected_unit_system == "FreeCAD Default":
        return spacing_value
    elif selected_unit_system == "Metric (m)":
        return spacing_value * 1000.0
    elif selected_unit_system == "Imperial (ft)":
        return spacing_value * 304.8
    elif selected_unit_system == "BIM Workbench Unit":
        return spacing_value * 25.4
    else:
        return spacing_value


def _copy_normalized_vector(vec):
    if vec is None:
        return None
    try:
        out = FreeCAD.Vector(vec.x, vec.y, vec.z)
    except Exception:
        return None
    if out.Length <= 1e-12:
        return None
    try:
        out.normalize()
    except Exception:
        return None
    return out


def get_preferred_face_normal_from_source(source_obj):
    try:
        stored = getattr(source_obj, "StoredNormal", None)
    except Exception:
        stored = None
    return _copy_normalized_vector(stored)


def get_face_local_frame(face, preferred_normal=None):
    normal = None

    try:
        umin, umax, vmin, vmax = face.ParameterRange
        u_mid = (umin + umax) * 0.5
        v_mid = (vmin + vmax) * 0.5
        normal = _copy_normalized_vector(face.normalAt(u_mid, v_mid))
    except Exception:
        pass

    if normal is None:
        try:
            normal = _copy_normalized_vector(face.normalAt(0, 0))
        except Exception:
            pass

    if normal is None:
        FreeCAD.Console.PrintWarning(
            "get_face_local_frame: could not read face normal, defaulting to Z-up.\n"
        )
        normal = FreeCAD.Vector(0, 0, 1)

    preferred = _copy_normalized_vector(preferred_normal)
    if preferred is not None and normal.dot(preferred) < 0:
        normal = FreeCAD.Vector(-normal.x, -normal.y, -normal.z)

    ref = FreeCAD.Vector(1, 0, 0)
    if abs(normal.dot(ref)) > 0.95:
        ref = FreeCAD.Vector(0, 1, 0)

    proj_len = ref.dot(normal)
    x_axis = FreeCAD.Vector(
        ref.x - normal.x * proj_len,
        ref.y - normal.y * proj_len,
        ref.z - normal.z * proj_len,
    )
    x_axis = _copy_normalized_vector(x_axis)

    if x_axis is None:
        ref = FreeCAD.Vector(0, 0, 1)
        proj_len = ref.dot(normal)
        x_axis = FreeCAD.Vector(
            ref.x - normal.x * proj_len,
            ref.y - normal.y * proj_len,
            ref.z - normal.z * proj_len,
        )
        x_axis = _copy_normalized_vector(x_axis)

    if x_axis is None:
        x_axis = FreeCAD.Vector(1, 0, 0)

    y_axis = FreeCAD.Vector(
        normal.y * x_axis.z - normal.z * x_axis.y,
        normal.z * x_axis.x - normal.x * x_axis.z,
        normal.x * x_axis.y - normal.y * x_axis.x,
    )
    y_axis = _copy_normalized_vector(y_axis)
    if y_axis is None:
        y_axis = FreeCAD.Vector(0, 1, 0)

    if abs(x_axis.x) >= abs(x_axis.y):
        if x_axis.x < 0:
            x_axis = FreeCAD.Vector(-x_axis.x, -x_axis.y, -x_axis.z)
            y_axis = FreeCAD.Vector(-y_axis.x, -y_axis.y, -y_axis.z)
    else:
        if x_axis.y < 0:
            x_axis = FreeCAD.Vector(-x_axis.x, -x_axis.y, -x_axis.z)
            y_axis = FreeCAD.Vector(-y_axis.x, -y_axis.y, -y_axis.z)

    origin = face.CenterOfMass
    return origin, x_axis, y_axis, normal


def build_surface_transforms(face, preferred_normal=None):
    origin, x_axis, y_axis, normal = get_face_local_frame(face, preferred_normal)

    to_local = FreeCAD.Matrix()
    to_local.A11 = x_axis.x
    to_local.A12 = x_axis.y
    to_local.A13 = x_axis.z
    to_local.A21 = y_axis.x
    to_local.A22 = y_axis.y
    to_local.A23 = y_axis.z
    to_local.A31 = normal.x
    to_local.A32 = normal.y
    to_local.A33 = normal.z

    translation = FreeCAD.Matrix()
    translation.move(FreeCAD.Vector(-origin.x, -origin.y, -origin.z))

    to_local = to_local.multiply(translation)
    to_world = to_local.inverse()

    return to_local, to_world


def compute_stable_world_anchor(face, preferred_normal=None, offset_x=0.0, offset_y=0.0):
    try:
        origin, x_axis, y_axis, _normal = get_face_local_frame(face, preferred_normal)
        world_anchor_x = (
            -(x_axis.x * origin.x + x_axis.y * origin.y + x_axis.z * origin.z) + offset_x
        )
        world_anchor_y = (
            -(y_axis.x * origin.x + y_axis.y * origin.y + y_axis.z * origin.z) + offset_y
        )
        return world_anchor_x, world_anchor_y
    except Exception:
        return offset_x, offset_y


def project_shape_to_local(shape, to_local_matrix):
    projected = shape.copy()
    projected.transformShape(to_local_matrix)
    return projected


def unproject_shape_to_world(shape, to_world_matrix):
    world_shape = shape.copy()
    world_shape.transformShape(to_world_matrix)
    return world_shape


def _placements_are_close(pl1, pl2, tol_mm=1.0, tol_rad=0.001):
    """Return True if two placements represent nearly the same transform."""
    if (pl1.Base - pl2.Base).Length > tol_mm:
        return False
    rel_angle = pl1.inverse().multiply(pl2).Rotation.Angle
    return abs(rel_angle) < tol_rad


def _bake_shape_to_geometry(shape, placement=None):
    """
    Return a copy whose transform is baked into the underlying geometry.

    Important: use transformGeometry() first, not transformShape(). For pure
    translation/rotation transforms, transformShape() may preserve an OCCT
    Location instead of fully flattening it. That is exactly what causes the
    hatch object to inherit a non-identity Placement after boolean cuts.
    """
    if shape is None or shape.isNull():
        return shape

    baked = shape.copy()
    pl = placement if placement is not None else baked.Placement

    baked.Placement = FreeCAD.Placement()
    if pl.isIdentity():
        return baked

    mat = pl.toMatrix()
    try:
        baked = baked.transformGeometry(mat)
    except RuntimeError:
        # Fallback only if transformGeometry is unavailable for the shape
        # subtype. transformShape is less reliable for flattening Locations
        # but still better than leaving the transform unapplied.
        baked.transformShape(mat)

    baked.Placement = FreeCAD.Placement()
    return baked


def _copy_object_shape_baked_world(obj):
    """
    Copy an object's shape and bake its effective world transform into geometry.

    Mirrors the dual-convention logic already used by FaceExtractor:
      A) Part/Draft objects: shape internal Placement ~= object Placement
      B) Arch-style objects: shape internal Placement is independent of the
         document object's Placement, so both must be composed.
    """
    shape_copy = obj.Shape.copy()
    obj_pl = obj.Placement
    shape_pl = shape_copy.Placement

    if _placements_are_close(shape_pl, obj_pl):
        total_pl = obj_pl
    else:
        total_pl = obj_pl.multiply(shape_pl)

    return _bake_shape_to_geometry(shape_copy, total_pl)


def apply_tile_view_overrides(tile_obj, view_provider, override_color=None):
    if not tile_obj or not hasattr(tile_obj, "ViewObject"):
        return

    if override_color is not None:
        color = override_color
    else:
        hatch_obj = tile_obj.Document.getObject(tile_obj.Name.replace("_TileRep", ""))
        if hatch_obj and hasattr(hatch_obj, "TileRepColor"):
            color = hatch_obj.TileRepColor
        else:
            color = (1.0, 0.0, 0.0)

    view_provider.ShapeColor = color
    view_provider.DisplayMode = "Wireframe"
    view_provider.LineWidth = 2.0


def safe_set_display_mode(view_object, desired_mode="Flat Lines"):
    try:
        modes = view_object.getDisplayModes()
    except Exception:
        modes = []
    if desired_mode in modes:
        view_object.DisplayMode = desired_mode
    elif modes:
        view_object.DisplayMode = modes[0]


def get_closed_wires_as_faces(obj):
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
                face = Part.Face(wire)
                faces.append(face)
            except Exception as e:
                FreeCAD.Console.PrintWarning(f"Cannot make face from wire: {e}\n")

    if faces:
        return Part.makeCompound(faces)
    else:
        return shape


def _compute_unified_bb(valid_base_shapes):
    if len(valid_base_shapes) == 1:
        return valid_base_shapes[0].BoundBox
    compound = Part.makeCompound(valid_base_shapes)
    return compound.BoundBox


class CustomHatchFeature:
    def __init__(self, obj):
        obj.Proxy = self
        self.Type = "CustomHatchFP"
        self._is_recomputing = False

        obj.addProperty("App::PropertyLink", "BaseObject", "Hatch", "Object with the base shape.")
        obj.addProperty(
            "App::PropertyLinkList",
            "BaseObjects",
            "Hatch",
            "Optional list of multiple base objects.",
        )
        obj.addProperty(
            "App::PropertyLink", "PatternObject", "Hatch", "Object with the pattern shape."
        )
        obj.addProperty(
            "App::PropertyLinkList",
            "PatternObjects",
            "Hatch",
            "Optional list of multiple pattern objects (fused).",
        )
        obj.addProperty(
            "App::PropertyLink",
            "BaseTileObject",
            "Hatch",
            "Optional tile shape object (if set, used to define bounding box).",
        )
        obj.addProperty(
            "App::PropertyBool",
            "TileVisibility",
            "Hatch",
            "Toggle tile shape visibility as a separate object.",
        )
        obj.TileVisibility = True
        obj.addProperty(
            "App::PropertyBool",
            "UpdateTileOnChange",
            "Hatch",
            "Update tile shapes automatically when Base Tile Object changes.",
        )
        obj.UpdateTileOnChange = True
        obj.addProperty(
            "App::PropertyLinkList",
            "Subtractions",
            "Hatch",
            "List of objects to subtract from the hatch pattern.",
        )

        obj.addProperty(
            "App::PropertyEnumeration",
            "DistributionMode",
            "Hatch",
            "How to distribute the pattern.",
        )
        obj.DistributionMode = [
            "CenteredTiling",
            "RelativeSpacing",
            "SeamlessTiling",
            "LinearGrid",
            "RadialDistribution",
            "ConcentricDistribution",
            "RandomDistribution",
            "AdaptiveDistribution",
        ]
        obj.DistributionMode = "SeamlessTiling"

        obj.addProperty(
            "App::PropertyEnumeration",
            "PatternType",
            "Hatch",
            "Choose built-in patterns or custom PatternObject.",
        )
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

        obj.addProperty(
            "App::PropertyBool",
            "AutoScaleToFitBase",
            "Scaling",
            "If true, automatically compute scale.",
        )
        obj.AutoScaleToFitBase = False
        obj.addProperty("App::PropertyFloat", "PatternScale", "Scaling", "Manual scale factor.")
        obj.PatternScale = 1.0
        obj.addProperty(
            "App::PropertyFloat", "RotationDeg", "Hatch", "Rotation around Z (degrees)."
        )
        obj.RotationDeg = 0.0
        obj.addProperty("App::PropertyFloat", "BaseSpacing", "Hatch", "Base spacing (mm or %).")
        obj.BaseSpacing = 0.0
        obj.addProperty(
            "App::PropertyBool",
            "UseUnits",
            "Hatch",
            "If true, interpret BaseSpacing with a chosen unit system.",
        )
        obj.UseUnits = False
        obj.addProperty(
            "App::PropertyEnumeration",
            "SelectedUnitSystem",
            "Hatch",
            "User-chosen unit system for BaseSpacing.",
        )
        obj.SelectedUnitSystem = [
            "FreeCAD Default",
            "Metric (m)",
            "Imperial (ft)",
            "BIM Workbench Unit",
        ]
        obj.SelectedUnitSystem = "FreeCAD Default"
        obj.addProperty("App::PropertyInteger", "RepetitionsX", "Hatch", "Number of repeats in X.")
        obj.RepetitionsX = 5
        obj.addProperty("App::PropertyInteger", "RepetitionsY", "Hatch", "Number of repeats in Y.")
        obj.RepetitionsY = 5
        obj.addProperty(
            "App::PropertyFloat", "PatternOffsetX", "Hatch", "Offset the pattern in X direction."
        )
        obj.PatternOffsetX = 0.0
        obj.addProperty(
            "App::PropertyFloat", "PatternOffsetY", "Hatch", "Offset the pattern in Y direction."
        )
        obj.PatternOffsetY = 0.0

        obj.addProperty(
            "App::PropertyEnumeration", "ScaleMode", "Scaling", "How PatternScale is interpreted."
        )
        obj.ScaleMode = ["Absolute", "FitWidth", "FitHeight", "FitMinDim", "FitMaxDim"]
        obj.ScaleMode = "Absolute"

        obj.addProperty(
            "App::PropertyBool", "RandomizePlacement", "Random", "Apply random transforms?"
        )
        obj.RandomizePlacement = False
        obj.addProperty(
            "App::PropertyFloat", "RandomOffsetRange", "Random", "Max random offset ± mm."
        )
        obj.RandomOffsetRange = 0.0
        obj.addProperty(
            "App::PropertyFloat", "RandomRotationRange", "Random", "Max random rotation ± deg."
        )
        obj.RandomRotationRange = 0.0
        obj.addProperty(
            "App::PropertyFloat", "RandomScaleMin", "Random", "Min random scale factor."
        )
        obj.RandomScaleMin = 1.0
        obj.addProperty(
            "App::PropertyFloat", "RandomScaleMax", "Random", "Max random scale factor."
        )
        obj.RandomScaleMax = 1.0

        obj.addProperty(
            "App::PropertyBool", "LockToBase", "Placement", "If True, user transform is re-clipped."
        )
        obj.LockToBase = False

        obj.addProperty(
            "App::PropertyInteger", "RadialCount", "Radial", "Number of copies around the circle."
        )
        obj.RadialCount = 8
        obj.addProperty(
            "App::PropertyFloat", "RadialRadius", "Radial", "Radius for radial distribution."
        )
        obj.RadialRadius = 50.0
        obj.addProperty("App::PropertyInteger", "ConcentricCount", "Concentric", "Number of rings.")
        obj.ConcentricCount = 5
        obj.addProperty(
            "App::PropertyFloat", "ConcentricSpacing", "Concentric", "Spacing between rings."
        )
        obj.ConcentricSpacing = 10.0
        obj.addProperty(
            "App::PropertyInteger", "RandomCount", "Random", "Number of random placements."
        )
        obj.RandomCount = 30

        obj.addProperty(
            "App::PropertyEnumeration",
            "PatternPlacementMode",
            "Hatch",
            "Positioning of the pattern within each tile.",
        )
        obj.PatternPlacementMode = [
            "Origin",
            "Center",
            "TopLeft",
            "TopRight",
            "BottomLeft",
            "BottomRight",
            "TopCenter",
            "BottomCenter",
            "LeftCenter",
            "RightCenter",
            "Custom",
        ]
        obj.PatternPlacementMode = "Origin"

        obj.addProperty(
            "App::PropertyBool",
            "ShowFaces",
            "Rendering",
            "If True, tries to convert lines to faces.",
        )
        obj.ShowFaces = False

        obj.addProperty(
            "App::PropertyBool",
            "ApplyTo3DSurface",
            "Rendering",
            "If True, map the pattern onto the selected 3D face.",
        )
        obj.ApplyTo3DSurface = False

        obj.addProperty(
            "App::PropertyEnumeration",
            "ClipMode",
            "Rendering",
            "How to handle open wires vs. faces when clipping with the base.",
        )
        obj.ClipMode = ["BooleanOnly", "PreserveLinesNoClip"]
        obj.ClipMode = "BooleanOnly"

        obj.addProperty(
            "App::PropertyBool",
            "UseSurfaceProjection",
            "Surface",
            "If True, project pattern onto the surface of the base shape.",
        )
        obj.UseSurfaceProjection = True
        obj.addProperty(
            "App::PropertyBool",
            "ForceXYPlane",
            "Surface",
            "If True, ignore surface projection and use XY plane only.",
        )
        obj.ForceXYPlane = False

        obj.addProperty(
            "App::PropertyInteger",
            "MaxTilesAllowed",
            "Performance",
            "Maximum number of tiles allowed.",
        )
        obj.MaxTilesAllowed = 5000

        obj.addProperty(
            "App::PropertyFloat",
            "DensityFactor",
            "Variation",
            "Density factor for random distribution (0-1).",
        )
        obj.DensityFactor = 1.0
        obj.addProperty(
            "App::PropertyFloat", "RandomRotationMin", "Variation", "Minimum random rotation (deg)."
        )
        obj.RandomRotationMin = 0.0
        obj.addProperty(
            "App::PropertyFloat", "RandomRotationMax", "Variation", "Maximum random rotation (deg)."
        )
        obj.RandomRotationMax = 0.0
        obj.addProperty(
            "App::PropertyBool",
            "EnableColorVariation",
            "Variation",
            "If true, random colors are assigned to each tile.",
        )
        obj.EnableColorVariation = False
        obj.addProperty(
            "App::PropertyFloat",
            "ColorVariationIntensity",
            "Variation",
            "Intensity of color variation (0-1).",
        )
        obj.ColorVariationIntensity = 0.5
        obj.addProperty(
            "App::PropertyFloat",
            "SpacingVariation",
            "Variation",
            "Random factor for spacing variation (0-1).",
        )
        obj.SpacingVariation = 0.0
        obj.addProperty(
            "App::PropertyBool",
            "EnableShapeDistortion",
            "Variation",
            "If true, shapes might be distorted in random ways.",
        )
        obj.EnableShapeDistortion = False

        obj.addProperty(
            "App::PropertyFloat", "GenerationTime", "Statistics", "Time taken to generate the hatch"
        )
        obj.GenerationTime = 0.0
        obj.addProperty(
            "App::PropertyInteger", "TileCount", "Statistics", "Number of tiles generated"
        )
        obj.TileCount = 0

        if "HatchRole" not in obj.PropertiesList:
            obj.addProperty(
                "App::PropertyEnumeration",
                "HatchRole",
                "Hatch",
                "Definition: reusable hatch style asset (no base required). Applied: generates hatch on target faces.",
            )
            obj.HatchRole = ["Definition", "Applied"]
            obj.HatchRole = "Applied"

        if hasattr(obj, "ViewObject") and obj.ViewObject:
            safe_set_display_mode(obj.ViewObject, "Flat Lines")

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        self.Type = state

    def onChanged(self, fp, prop):
        if prop == "BaseTileObject" and fp.UpdateTileOnChange:
            if fp.BaseTileObject:
                FreeCADGui.Selection.clearSelection()
                FreeCADGui.Selection.addSelection(fp.BaseTileObject)
            self.safe_delayed_execute(fp, 100)
        if prop == "PatternPlacementMode":
            self.safe_delayed_execute(fp, 100)
        # Delay execute when BaseObject / BaseObjects changes so that FreeCAD's
        # dependency graph has time to fully resolve the linked shapes before we
        # compute the unified bounding box. Without this delay the first
        # recompute fires while the newly added object's shape is still in a
        # stale/unresolved state, producing a wrong unified BB and an apparent
        # position shift that only corrects itself on the next recompute.
        if prop in ("BaseObject", "BaseObjects"):
            self.safe_delayed_execute(fp, 200)
        if prop == "DistributionMode":
            props = fp.PropertiesList

            def set_mode_if_exists(prop_name, mode_value):
                if prop_name in props:
                    fp.setEditorMode(prop_name, mode_value)

            set_mode_if_exists("BaseSpacing", 0)
            set_mode_if_exists("RepetitionsX", 0)
            set_mode_if_exists("RepetitionsY", 0)

        if prop == "LockToBase":
            if fp.LockToBase:
                fp.setEditorMode("Placement", 2)
            else:
                fp.setEditorMode("Placement", 0)

        # Propagate recipe changes from a Definition hatch to all Applied
        # hatches that reference it. When switching PatternType (especially
        # in/out of SolidFill), the generated material-applied hatch would
        # remain in a stale shape-mode until the user manually toggled
        # HatchSurfaces / HatchCaps. This propagation fixes that.
        _RECIPE_PROPS = {
            "PatternType",
            "PatternObject",
            "PatternObjects",
            "BaseSpacing",
            "PatternScale",
            "RotationDeg",
            "PatternOffsetX",
            "PatternOffsetY",
            "DistributionMode",
            "ScaleMode",
            "PatternPlacementMode",
            "ClipMode",
        }
        if prop in _RECIPE_PROPS and getattr(fp, "HatchRole", "Applied") == "Definition":
            self._propagate_to_applied_hatches(fp, prop)

        if prop == "BaseTileObject" and fp.UpdateTileOnChange:
            self.execute(fp)

    def safe_delayed_execute(self, fp, delay):
        if not fp or not fp.Document:
            return

        doc_name = fp.Document.Name
        obj_name = fp.Name

        def callback():
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return
            obj = doc.getObject(obj_name)
            if obj and obj.Proxy == self:
                self.execute(obj)

        QtCore.QTimer.singleShot(delay, callback)

    def _propagate_to_applied_hatches(self, definition_fp, changed_prop):
        """
        When a Definition hatch changes a recipe property, force-refresh
        every Applied hatch in the document whose PatternObject points at it.

        For PatternType changes that cross the SolidFill branch boundary we
        also immediately zero out the stale shape so the viewport never shows
        the 'exploded' intermediate state while the recompute is pending.
        """
        doc = getattr(definition_fp, "Document", None)
        if not doc:
            return

        is_type_change = changed_prop == "PatternType"

        for obj in list(doc.Objects):
            try:
                if getattr(obj, "HatchRole", "") != "Applied":
                    continue
                pattern_obj = getattr(obj, "PatternObject", None)
                if pattern_obj is None or pattern_obj.Name != definition_fp.Name:
                    continue

                if is_type_change:
                    try:
                        obj.Shape = Part.makeCompound([])
                    except Exception as e:
                        FreeCAD.Console.PrintWarning(f"Failed to clear shape for {obj.Name}: {e}\n")

                applied_proxy = getattr(obj, "Proxy", None)
                if applied_proxy is not None and hasattr(applied_proxy, "safe_delayed_execute"):
                    try:
                        applied_proxy.safe_delayed_execute(obj, 150)
                        continue
                    except Exception as e:
                        FreeCAD.Console.PrintWarning(
                            f"Failed to schedule delayed execute for {obj.Name}: {e}\n"
                        )

                _doc_name = doc.Name

                def _fallback_recompute(dn=_doc_name):
                    d = FreeCAD.getDocument(dn)
                    if d:
                        try:
                            d.recompute()
                        except Exception as e:
                            FreeCAD.Console.PrintWarning(f"Fallback recompute failed: {e}\n")

                QtCore.QTimer.singleShot(200, _fallback_recompute)

            except Exception as e:
                FreeCAD.Console.PrintWarning(
                    f"Failed to propagate to {getattr(obj, 'Name', 'unknown')}: {e}\n"
                )

    def _is_definition_hatch(self, obj):
        try:
            return bool(obj) and getattr(obj, "HatchRole", "Applied") == "Definition"
        except Exception:
            return False

    def _get_effective_recipe_source(self, fp):
        try:
            if getattr(fp, "PatternType", "CustomObject") == "CustomObject":
                pattern_obj = getattr(fp, "PatternObject", None)
                if self._is_definition_hatch(pattern_obj):
                    return pattern_obj
        except Exception:
            pass
        return fp

    def _collect_pattern_shapes_from_source(self, source_obj, show_faces):
        all_pattern_shapes = []

        src_pattern_type = getattr(source_obj, "PatternType", "CustomObject")
        if src_pattern_type == "CustomObject":
            src_pattern_obj = getattr(source_obj, "PatternObject", None)
            if src_pattern_obj and hasattr(src_pattern_obj, "Shape"):
                pattern_shape = (
                    get_closed_wires_as_faces(src_pattern_obj)
                    if show_faces
                    else src_pattern_obj.Shape.copy()
                )
                if pattern_shape and not pattern_shape.isNull():
                    all_pattern_shapes.append(pattern_shape)

            for pattern_obj in getattr(source_obj, "PatternObjects", None) or []:
                if pattern_obj and hasattr(pattern_obj, "Shape"):
                    extra_shape = (
                        get_closed_wires_as_faces(pattern_obj)
                        if show_faces
                        else pattern_obj.Shape.copy()
                    )
                    if extra_shape and not extra_shape.isNull():
                        all_pattern_shapes.append(extra_shape)

            if not all_pattern_shapes:
                return None
            if len(all_pattern_shapes) == 1:
                return all_pattern_shapes[0]

            # Use makeCompound instead of fuse() so that mixed shape types
            # (Sketch wires + Draft Wires, edges + faces, etc.) can coexist
            # without triggering OCCT's Boolean engine. fuse() requires
            # manifold/compatible topology and raises ValueError: Null shape
            # whenever the two inputs are both wire/edge geometry (as is always
            # the case when combining Sketch and Draft objects).
            return Part.makeCompound(all_pattern_shapes)

        return generateBuiltInPatternShape(src_pattern_type)

    def execute(self, fp):
        start_time = datetime.datetime.now()
        doc = fp.Document

        try:
            if not doc:
                fp.Shape = Part.makeCompound([])
                return

            hatch_role = getattr(fp, "HatchRole", "Applied")
            if hatch_role == "Definition":
                self._execute_definition_preview(fp, start_time)
                return

            base_shape_pairs = []

            if fp.BaseObject:
                temp_shape = get_closed_wires_as_faces(fp.BaseObject)
                if temp_shape and not temp_shape.isNull():
                    shape_conv = temp_shape
                else:
                    shape_conv = fp.BaseObject.Shape.copy()

                if shape_conv and not shape_conv.isNull():
                    base_shape_pairs.append((shape_conv, fp.BaseObject))

            if fp.BaseObjects:
                for base_obj in fp.BaseObjects or []:
                    if base_obj:
                        temp_face = get_closed_wires_as_faces(base_obj)
                        if temp_face and not temp_face.isNull():
                            face_shape = temp_face
                        else:
                            face_shape = base_obj.Shape.copy()

                        if face_shape and not face_shape.isNull():
                            base_shape_pairs.append((face_shape, base_obj))

            if not base_shape_pairs:
                fp.Shape = Part.makeCompound([])
                return

            valid_base_pairs = [
                (shape, source)
                for shape, source in base_shape_pairs
                if shape and not shape.isNull()
            ]
            if not valid_base_pairs:
                fp.Shape = Part.makeCompound([])
                return

            valid_base_shapes = [shape for shape, _source in valid_base_pairs]
            unified_bb = _compute_unified_bb(valid_base_shapes)

            recipe = self._get_effective_recipe_source(fp)

            dist_mode = getattr(recipe, "DistributionMode", fp.DistributionMode)
            auto_scale = getattr(recipe, "AutoScaleToFitBase", fp.AutoScaleToFitBase)
            scale_val = getattr(recipe, "PatternScale", fp.PatternScale)
            rot_val = getattr(recipe, "RotationDeg", fp.RotationDeg)
            spacing_val = getattr(recipe, "BaseSpacing", fp.BaseSpacing)
            use_units = getattr(recipe, "UseUnits", getattr(fp, "UseUnits", False))
            unit_system = getattr(
                recipe, "SelectedUnitSystem", getattr(fp, "SelectedUnitSystem", "FreeCAD Default")
            )
            if use_units:
                spacing_val = convertBaseSpacingValue(spacing_val, use_units, unit_system)

            rep_x = getattr(recipe, "RepetitionsX", fp.RepetitionsX)
            rep_y = getattr(recipe, "RepetitionsY", fp.RepetitionsY)
            rand_placement = getattr(recipe, "RandomizePlacement", fp.RandomizePlacement)
            rand_offset = getattr(recipe, "RandomOffsetRange", fp.RandomOffsetRange)
            rotation_min = getattr(recipe, "RandomRotationMin", fp.RandomRotationMin)
            rotation_max = getattr(recipe, "RandomRotationMax", fp.RandomRotationMax)
            rand_scale_min = getattr(recipe, "RandomScaleMin", fp.RandomScaleMin)
            rand_scale_max = getattr(recipe, "RandomScaleMax", fp.RandomScaleMax)
            radial_count = getattr(recipe, "RadialCount", fp.RadialCount)
            radial_radius = getattr(recipe, "RadialRadius", fp.RadialRadius)
            concentric_count = getattr(recipe, "ConcentricCount", fp.ConcentricCount)
            concentric_spacing = getattr(recipe, "ConcentricSpacing", fp.ConcentricSpacing)
            random_count = getattr(recipe, "RandomCount", fp.RandomCount)
            pattern_offset_x = getattr(recipe, "PatternOffsetX", fp.PatternOffsetX)
            pattern_offset_y = getattr(recipe, "PatternOffsetY", fp.PatternOffsetY)
            scale_mode = getattr(recipe, "ScaleMode", fp.ScaleMode)
            tile_obj = (
                getattr(recipe, "BaseTileObject", None)
                if hasattr(recipe, "BaseTileObject")
                else None
            )
            tile_visibility = getattr(recipe, "TileVisibility", fp.TileVisibility)
            show_faces = getattr(recipe, "ShowFaces", fp.ShowFaces)
            apply_3d = getattr(recipe, "ApplyTo3DSurface", fp.ApplyTo3DSurface)
            max_tiles = getattr(recipe, "MaxTilesAllowed", fp.MaxTilesAllowed)
            density_factor = getattr(recipe, "DensityFactor", fp.DensityFactor)
            enable_color_var = getattr(recipe, "EnableColorVariation", fp.EnableColorVariation)
            color_var_intensity = getattr(
                recipe, "ColorVariationIntensity", fp.ColorVariationIntensity
            )
            spacing_variation = getattr(recipe, "SpacingVariation", fp.SpacingVariation)
            shape_distortion = getattr(recipe, "EnableShapeDistortion", fp.EnableShapeDistortion)

            use_surface_projection = getattr(
                recipe, "UseSurfaceProjection", getattr(fp, "UseSurfaceProjection", True)
            )
            force_xy_plane = getattr(recipe, "ForceXYPlane", getattr(fp, "ForceXYPlane", False))
            effective_placement_mode = getattr(
                recipe, "PatternPlacementMode", fp.PatternPlacementMode
            )
            effective_clip_mode = getattr(recipe, "ClipMode", fp.ClipMode)
            effective_pattern_type = getattr(recipe, "PatternType", fp.PatternType)

            subtraction_objects = fp.Subtractions

            if effective_pattern_type == "SolidFill":
                final_parts = []

                for base_shape, base_source_obj in valid_base_pairs:
                    try:
                        use_projection = (
                            use_surface_projection
                            and not force_xy_plane
                            and base_shape.Faces
                            and len(base_shape.Faces) > 0
                        )

                        if use_projection:
                            faces_to_process = (
                                base_shape.Faces
                                if len(base_shape.Faces) > 1
                                else [base_shape.Faces[0]]
                            )

                            for target_face in faces_to_process:
                                try:
                                    try:
                                        fill_face = target_face.copy()
                                    except Exception:
                                        try:
                                            fill_face = Part.Face(target_face.OuterWire)
                                        except Exception:
                                            fill_face = None

                                    if fill_face and not fill_face.isNull():
                                        final_parts.append(fill_face)
                                except Exception as face_err:
                                    FreeCAD.Console.PrintWarning(
                                        f"SolidFill face handling failed, skipping: {face_err}\n"
                                    )

                        else:
                            if base_shape.Faces:
                                for face in base_shape.Faces:
                                    try:
                                        try:
                                            fill_face = face.copy()
                                        except Exception:
                                            try:
                                                fill_face = Part.Face(face.OuterWire)
                                            except Exception:
                                                fill_face = None
                                        if fill_face and not fill_face.isNull():
                                            final_parts.append(fill_face)
                                    except Exception:
                                        pass
                            else:
                                final_parts.append(base_shape.copy())

                    except Exception as e:
                        FreeCAD.Console.PrintWarning(
                            f"SolidFill base shape handling failed, skipping: {e}\n"
                        )

                if not final_parts:
                    combined = Part.makeCompound([])
                elif len(final_parts) == 1:
                    combined = final_parts[0]
                else:
                    combined = Part.makeCompound(final_parts)

                fp.GenerationTime = (datetime.datetime.now() - start_time).total_seconds()
                fp.TileCount = len(final_parts)

                tile_obj_name = fp.Name + "_TileRep"
                existing_tile_obj = doc.getObject(tile_obj_name)
                if existing_tile_obj:
                    doc.removeObject(existing_tile_obj.Name)

            else:
                pattern_shape = self._collect_pattern_shapes_from_source(recipe, show_faces)
                if pattern_shape is None or pattern_shape.isNull():
                    fp.Shape = Part.makeCompound([])
                    return

                total_tiles = 0
                final_parts = []

                for base_shape, base_source_obj in valid_base_pairs:
                    try:
                        use_projection = (
                            use_surface_projection
                            and not force_xy_plane
                            and base_shape.Faces
                            and len(base_shape.Faces) > 0
                        )

                        if use_projection:
                            preferred_normal = get_preferred_face_normal_from_source(
                                base_source_obj
                            )
                            faces_to_process = (
                                base_shape.Faces
                                if len(base_shape.Faces) > 1
                                else [base_shape.Faces[0]]
                            )

                            for target_face in faces_to_process:
                                try:
                                    try:
                                        face_shell = Part.Shell([target_face])
                                    except Exception:
                                        face_shell = target_face

                                    to_local, to_world = build_surface_transforms(
                                        target_face,
                                        preferred_normal=preferred_normal,
                                    )

                                    local_base = project_shape_to_local(face_shell, to_local)

                                    if local_base.Faces:
                                        try:
                                            local_base = Part.Face(local_base.Faces[0].OuterWire)
                                        except Exception:
                                            try:
                                                local_base = local_base.Faces[0]
                                            except Exception:
                                                pass

                                    # Do NOT use compute_stable_world_anchor here.
                                    # That function produces -(x_axis · CenterOfMass),
                                    # which interacts with buildHatchShape's internal
                                    # BB.XMin-relative tiling to create a net shift of
                                    # -halfWidth in world space. When the face resizes,
                                    # halfWidth changes and the tiles move in the OPPOSITE
                                    # direction to the face (the "reverse mirror" bug).
                                    # Passing the user offsets directly makes the tiling
                                    # face-relative (consistent with the non-projection
                                    # path), which is both correct and stable under resize.
                                    local_hatch, tiles = buildHatchShape(
                                        baseShape=local_base,
                                        overrideBB=_compute_unified_bb([local_base]),
                                        patternShape=pattern_shape,
                                        distributionMode=dist_mode,
                                        autoScaleToFitBase=auto_scale,
                                        patternScale=scale_val,
                                        rotationDeg=rot_val,
                                        baseSpacing=spacing_val,
                                        repX=rep_x,
                                        repY=rep_y,
                                        randRotMin=rotation_min,
                                        randRotMax=rotation_max,
                                        randomizePlacement=rand_placement,
                                        randomOffsetRange=rand_offset,
                                        randomScaleMin=rand_scale_min,
                                        randomScaleMax=rand_scale_max,
                                        radialCount=radial_count,
                                        radialRadius=radial_radius,
                                        concentricCount=concentric_count,
                                        concentricSpacing=concentric_spacing,
                                        randomCount=random_count,
                                        offsetX=pattern_offset_x,
                                        offsetY=pattern_offset_y,
                                        scaleMode=scale_mode,
                                        tileShape=(tile_obj.Shape if tile_obj else None),
                                        tileVisibility=tile_visibility,
                                        showFaces=show_faces,
                                        maxTiles=max_tiles,
                                        densityFactor=density_factor,
                                        enableColorVar=enable_color_var,
                                        colorVarInt=color_var_intensity,
                                        spacingVariation=spacing_variation,
                                        shapeDistortion=shape_distortion,
                                        apply3D=apply_3d,
                                        placement_mode=effective_placement_mode,
                                        clipMode=effective_clip_mode,
                                    )

                                    hatch_world = unproject_shape_to_world(local_hatch, to_world)
                                    total_tiles += tiles
                                    if hatch_world and not hatch_world.isNull():
                                        final_parts.append(hatch_world)
                                except Exception as face_err:
                                    FreeCAD.Console.PrintWarning(
                                        f"Surface projection failed for one face, skipping: {face_err}\n"
                                    )
                        else:
                            shaped, tiles = buildHatchShape(
                                baseShape=base_shape,
                                overrideBB=unified_bb,
                                patternShape=pattern_shape,
                                distributionMode=dist_mode,
                                autoScaleToFitBase=auto_scale,
                                patternScale=scale_val,
                                rotationDeg=rot_val,
                                baseSpacing=spacing_val,
                                repX=rep_x,
                                repY=rep_y,
                                randRotMin=rotation_min,
                                randRotMax=rotation_max,
                                randomizePlacement=rand_placement,
                                randomOffsetRange=rand_offset,
                                randomScaleMin=rand_scale_min,
                                randomScaleMax=rand_scale_max,
                                radialCount=radial_count,
                                radialRadius=radial_radius,
                                concentricCount=concentric_count,
                                concentricSpacing=concentric_spacing,
                                randomCount=random_count,
                                offsetX=pattern_offset_x,
                                offsetY=pattern_offset_y,
                                scaleMode=scale_mode,
                                tileShape=(tile_obj.Shape if tile_obj else None),
                                tileVisibility=tile_visibility,
                                showFaces=show_faces,
                                maxTiles=max_tiles,
                                densityFactor=density_factor,
                                enableColorVar=enable_color_var,
                                colorVarInt=color_var_intensity,
                                spacingVariation=spacing_variation,
                                shapeDistortion=shape_distortion,
                                apply3D=apply_3d,
                                placement_mode=effective_placement_mode,
                                clipMode=effective_clip_mode,
                            )
                            total_tiles += tiles
                            final_parts.append(shaped)
                    except Exception as e:
                        FreeCAD.Console.PrintError(
                            f"Error building hatch on base shape: {str(e)}\n"
                        )
                        continue

                if not final_parts:
                    combined = Part.makeCompound([])
                else:
                    combined = final_parts[0]
                    for part in final_parts[1:]:
                        combined = combined.fuse(part)

                fp.TileCount = total_tiles

                tile_obj_name = fp.Name + "_TileRep"
                existing_tile_obj = doc.getObject(tile_obj_name)

                if tile_visibility and tile_obj and not tile_obj.Shape.isNull():
                    if not existing_tile_obj:
                        existing_tile_obj = doc.addObject("Part::Feature", tile_obj_name)
                    doc.openTransaction("Update Tiles")
                    try:
                        repeated_tile, _ = buildHatchShape(
                            baseShape=valid_base_shapes[0],
                            overrideBB=unified_bb,
                            patternShape=tile_obj.Shape,
                            distributionMode=dist_mode,
                            autoScaleToFitBase=auto_scale,
                            patternScale=scale_val,
                            rotationDeg=rot_val,
                            baseSpacing=spacing_val,
                            repX=rep_x,
                            repY=rep_y,
                            randRotMin=rotation_min,
                            randRotMax=rotation_max,
                            randomizePlacement=rand_placement,
                            randomOffsetRange=rand_offset,
                            randomScaleMin=rand_scale_min,
                            randomScaleMax=rand_scale_max,
                            offsetX=pattern_offset_x,
                            offsetY=pattern_offset_y,
                            scaleMode=scale_mode,
                            tileShape=None,
                            tileVisibility=False,
                            showFaces=False,
                            maxTiles=max_tiles,
                            densityFactor=density_factor,
                            enableColorVar=False,
                            colorVarInt=0.0,
                            spacingVariation=spacing_variation,
                            shapeDistortion=False,
                            apply3D=apply_3d,
                            placement_mode=effective_placement_mode,
                        )
                        existing_tile_obj.Shape = repeated_tile
                        apply_tile_view_overrides(existing_tile_obj, existing_tile_obj.ViewObject)
                    finally:
                        doc.commitTransaction()
                else:
                    if existing_tile_obj:
                        doc.removeObject(existing_tile_obj.Name)

            if subtraction_objects:
                combined = _bake_shape_to_geometry(combined)
                for sub_obj in subtraction_objects:
                    if sub_obj and hasattr(sub_obj, "Shape") and not sub_obj.Shape.isNull():
                        try:
                            # Do the boolean in fully baked world geometry.
                            #
                            # The previous fix still used transformShape() while
                            # trying to "bake" the subtraction object. For rigid
                            # transforms, that can preserve an OCCT Location
                            # instead of flattening it. The cut result then keeps
                            # that Location, and assigning it to fp.Shape makes
                            # FreeCAD absorb it into fp.Placement — visually
                            # shifting a hatch whose geometry was already in world
                            # coordinates.
                            sub_shape = _copy_object_shape_baked_world(sub_obj)
                            combined = combined.cut(sub_shape)
                            combined = _bake_shape_to_geometry(combined)
                        except Exception as e:
                            FreeCAD.Console.PrintError(
                                f"Error subtracting {sub_obj.Name}: {str(e)}\n"
                            )

            # Always assign an identity-placed, world-baked result. This makes
            # the applied hatch deterministic and prevents stale/absorbed OCCT
            # Locations from leaking into fp.Placement across recomputes.
            combined = _bake_shape_to_geometry(combined)
            fp.Placement = FreeCAD.Placement()
            fp.Shape = combined
            fp.GenerationTime = (datetime.datetime.now() - start_time).total_seconds()

        except Exception as e:
            FreeCAD.Console.PrintWarning(f"CustomHatch '{fp.Name}' execute failed: {e}\n")
            try:
                fp.Shape = Part.makeCompound([])
            except Exception:
                pass

    def _execute_definition_preview(self, fp, start_time):
        auto_label = None
        try:
            pattern_type = getattr(fp, "PatternType", "CustomObject")
            if pattern_type == "CustomObject":
                pattern_obj = getattr(fp, "PatternObject", None)
                if pattern_obj:
                    auto_label = f"HatchDef-{pattern_obj.Label}"
                else:
                    auto_label = "HatchDef"
            else:
                auto_label = f"HatchDef-{pattern_type}"
        except Exception:
            pass

        if auto_label:
            current_label = getattr(fp, "Label", "")
            if current_label.startswith("HatchDef") or current_label.startswith("CustomHatch"):
                try:
                    fp.Label = auto_label
                except Exception:
                    pass

        pattern_shape = None
        try:
            pattern_type = getattr(fp, "PatternType", "CustomObject")
            if pattern_type == "CustomObject":
                pattern_obj = getattr(fp, "PatternObject", None)
                if pattern_obj and hasattr(pattern_obj, "Shape") and not pattern_obj.Shape.isNull():
                    pattern_shape = pattern_obj.Shape.copy()
            else:
                pattern_shape = generateBuiltInPatternShape(pattern_type)
        except Exception:
            pass

        tiles_in_view = 3
        min_swatch = 200.0
        max_swatch = 10000.0

        swatch_size_x = min_swatch
        swatch_size_y = min_swatch

        if pattern_shape and not pattern_shape.isNull():
            try:
                spacing_val = fp.BaseSpacing
                if fp.UseUnits:
                    spacing_val = convertBaseSpacingValue(
                        spacing_val, fp.UseUnits, fp.SelectedUnitSystem
                    )
                _, pattern_bb = normalizePatternShape(pattern_shape)
                tile_obj = getattr(fp, "BaseTileObject", None)
                if tile_obj and hasattr(tile_obj, "Shape") and not tile_obj.Shape.isNull():
                    _, tile_bb = normalizePatternShape(tile_obj.Shape)
                    tile_width = max(tile_bb.XLength, 1e-6)
                    tile_height = max(tile_bb.YLength, 1e-6)
                else:
                    tile_width = max(pattern_bb.XLength, 1e-6)
                    tile_height = max(pattern_bb.YLength, 1e-6)

                swatch_size_x = min(
                    max_swatch, max(min_swatch, tile_width * fp.PatternScale * tiles_in_view)
                )
                swatch_size_y = min(
                    max_swatch, max(min_swatch, tile_height * fp.PatternScale * tiles_in_view)
                )
            except Exception:
                pass

        half_x = swatch_size_x / 2.0
        half_y = swatch_size_y / 2.0

        try:
            swatch = Part.makePlane(
                swatch_size_x,
                swatch_size_y,
                FreeCAD.Vector(-half_x, -half_y, 0),
                FreeCAD.Vector(0, 0, 1),
            )
        except Exception:
            swatch = Part.Face(
                Part.makePolygon(
                    [
                        FreeCAD.Vector(-half_x, -half_y, 0),
                        FreeCAD.Vector(half_x, -half_y, 0),
                        FreeCAD.Vector(half_x, half_y, 0),
                        FreeCAD.Vector(-half_x, half_y, 0),
                        FreeCAD.Vector(-half_x, -half_y, 0),
                    ]
                )
            )

        if pattern_shape is None or pattern_shape.isNull():
            try:
                fp.Shape = Part.makeCompound([swatch.OuterWire])
            except Exception:
                fp.Shape = Part.makeCompound([])
            return

        # SolidFill definition preview should be a plain filled swatch,
        # not a tiled hatch run.
        if pattern_type == "SolidFill":
            fp.Shape = swatch
            fp.GenerationTime = (datetime.datetime.now() - start_time).total_seconds()
            fp.TileCount = 1
            return

        try:
            spacing_val = fp.BaseSpacing
            if fp.UseUnits:
                spacing_val = convertBaseSpacingValue(
                    spacing_val, fp.UseUnits, fp.SelectedUnitSystem
                )
            tile_obj = getattr(fp, "BaseTileObject", None)
            hatch, _ = buildHatchShape(
                baseShape=swatch,
                patternShape=pattern_shape,
                distributionMode=fp.DistributionMode,
                patternScale=fp.PatternScale,
                rotationDeg=fp.RotationDeg,
                baseSpacing=spacing_val,
                repX=fp.RepetitionsX,
                repY=fp.RepetitionsY,
                scaleMode=fp.ScaleMode,
                tileShape=(tile_obj.Shape if tile_obj else None),
                showFaces=fp.ShowFaces,
                maxTiles=tiles_in_view * tiles_in_view * 4,
                placement_mode=fp.PatternPlacementMode,
                clipMode=fp.ClipMode,
            )

            if hatch and not hatch.isNull():
                fp.Shape = Part.makeCompound([swatch.OuterWire, hatch])
            else:
                fp.Shape = Part.makeCompound([swatch.OuterWire])
        except Exception as e:
            FreeCAD.Console.PrintWarning(f"HatchDefinition preview failed '{fp.Name}': {e}\n")
            try:
                fp.Shape = Part.makeCompound([swatch.OuterWire])
            except Exception:
                fp.Shape = Part.makeCompound([])

        try:
            fp.GenerationTime = (datetime.datetime.now() - start_time).total_seconds()
            fp.TileCount = 0
        except Exception:
            pass

    def onDocumentRestored(self, obj):
        obj.Proxy = self
        self._is_recomputing = False


class CustomHatchViewProvider:
    def __init__(self, view_object):
        view_object.Proxy = self
        self.Object = view_object.Object

    def setupContextMenu(self, obj, menu):
        action_copy = menu.addAction("Copy Hatch")
        action_copy.triggered.connect(lambda: self.copy_hatch(obj.Object))
        action_duplicate = menu.addAction("Duplicate Hatch")
        action_duplicate.triggered.connect(lambda: self.duplicate_hatch(obj.Object))
        action_remove = menu.addAction("Remove Hatch")
        action_remove.triggered.connect(lambda: self.remove_hatch(obj.Object))

    def copy_hatch(self, doc_obj):
        try:
            new_name = "CopyOf_" + doc_obj.Name
            new_obj = make_custom_hatch(name=new_name)
            excluded_props = ["Name", "Label", "ExpressionEngine"]
            for prop in doc_obj.PropertiesList:
                if prop not in excluded_props and not prop.startswith("Proxy"):
                    setattr(new_obj, prop, getattr(doc_obj, prop))
            FreeCAD.ActiveDocument.recompute()
        except Exception as e:
            FreeCAD.Console.PrintError(f"Copy failed: {str(e)}\n")

    def duplicate_hatch(self, doc_obj):
        try:
            new_name = FreeCAD.ActiveDocument.getUniqueObjectName("Hatch")
            new_obj = make_custom_hatch(name=new_name)
            excluded_props = ["Name", "Label", "ExpressionEngine"]
            for prop in doc_obj.PropertiesList:
                if prop not in excluded_props and not prop.startswith("Proxy"):
                    setattr(new_obj, prop, getattr(doc_obj, prop))
            FreeCAD.ActiveDocument.recompute()
        except Exception as e:
            FreeCAD.Console.PrintError(f"Duplicate failed: {str(e)}\n")

    def remove_hatch(self, doc_obj):
        try:
            FreeCAD.ActiveDocument.removeObject(doc_obj.Name)
            FreeCAD.ActiveDocument.recompute()
        except Exception as e:
            FreeCAD.Console.PrintError(f"Remove failed: {str(e)}\n")

    def doubleClicked(self, view_object):
        panel = HatchTaskPanel(hatch_obj=view_object.Object)
        FreeCADGui.Control.showDialog(panel)
        return True

    def attach(self, view_object):
        self.ViewObject = view_object
        self.Object = view_object.Object

    def getDisplayModes(self, obj):
        return ["Flat Lines", "Wireframe", "Shaded"]

    def getDefaultDisplayMode(self):
        return "Flat Lines"

    def setDisplayMode(self, mode):
        return mode

    def onChanged(self, view_object, prop):
        pass

    def getIcon(self):
        try:
            obj = self.Object
            if obj and getattr(obj, "HatchRole", "Applied") == "Definition":
                return _icon_path("BIM_HatchDef.svg")
        except Exception:
            pass
        return _icon_path("BIM_HatchApplied.svg")

    def __getstate__(self):
        return {"Object": self.Object.Name}

    def __setstate__(self, state):
        if "Object" in state:
            self.Object = FreeCAD.ActiveDocument.getObject(state["Object"])
        return None


def make_custom_hatch(name="CustomHatchFP", role="Applied"):
    doc = FreeCAD.ActiveDocument
    if not doc:
        raise RuntimeError("No active document")

    obj = doc.addObject("Part::FeaturePython", name)
    CustomHatchFeature(obj)

    if FreeCAD.GuiUp:
        CustomHatchViewProvider(obj.ViewObject)

    if hasattr(obj, "HatchRole"):
        obj.HatchRole = role

    try:
        obj.TileVisibility = False
    except Exception:
        pass

    return obj


# Backward-compatibility API aliases

makeCustomHatch = make_custom_hatch
getFaceLocalFrame = get_face_local_frame
buildSurfaceTransforms = build_surface_transforms
projectShapeToLocal = project_shape_to_local
unprojectShapeToWorld = unproject_shape_to_world
getPreferredFaceNormalFromSource = get_preferred_face_normal_from_source
computeStableWorldAnchor = compute_stable_world_anchor
applyTileViewOverrides = apply_tile_view_overrides
safeSetDisplayMode = safe_set_display_mode


# Material hatch bridge

_MATERIAL_HATCH_PREFIX = "_MaterialHatch_"


def _material_hatch_name(target_obj):
    return f"{_MATERIAL_HATCH_PREFIX}{target_obj.Name}"


def _remove_material_hatch(target_obj):
    if not target_obj:
        return False

    doc = getattr(target_obj, "Document", None) or FreeCAD.ActiveDocument
    if not doc:
        return False

    existing = doc.getObject(_material_hatch_name(target_obj))
    if existing:
        try:
            doc.removeObject(existing.Name)
            return True
        except Exception:
            return False
    return False


def _copy_definition_recipe_to_applied(applied_obj, hatch_def):
    """
    Point an Applied hatch at a Definition hatch so execute() uses the
    definition as the recipe source via _get_effective_recipe_source().
    """
    applied_obj.HatchRole = "Applied"
    applied_obj.PatternType = "CustomObject"
    applied_obj.PatternObject = hatch_def
    applied_obj.PatternObjects = []

    applied_obj.UseSurfaceProjection = True
    applied_obj.ForceXYPlane = False
    applied_obj.ApplyTo3DSurface = True

    try:
        applied_obj.TileVisibility = False
    except Exception:
        pass


# Geometry-change observer


class _HatchGeometryObserver:
    """
    Lightweight FreeCAD DocumentObserver.
    Watches for Shape changes on any object. When the changed object is the
    BaseObject of a _MaterialHatch_* applied hatch, it touches that hatch and
    schedules a deferred doc.recompute() so execute() sees the fresh geometry.

    The recompute is deferred via QTimer to avoid calling doc.recompute()
    synchronously inside a change notification (which can cause recursion).
    """

    # Class-level registry: doc_name -> observer instance.
    # Prevents installing duplicate observers on the same document.
    _registry = {}

    @classmethod
    def install(cls, doc):
        doc_name = getattr(doc, "Name", None)
        if not doc_name or doc_name in cls._registry:
            return
        obs = cls(doc_name)
        try:
            FreeCAD.addDocumentObserver(obs)
            cls._registry[doc_name] = obs
        except Exception:
            pass

    @classmethod
    def uninstall(cls, doc_name):
        obs = cls._registry.pop(doc_name, None)
        if obs:
            try:
                FreeCAD.removeDocumentObserver(obs)
            except Exception:
                pass

    def __init__(self, doc_name):
        self._doc_name = doc_name
        self._pending = False

    # FreeCAD calls this after ANY property on ANY object changes.
    def slotChangedObject(self, obj, prop):
        if prop != "Shape":
            return

        doc = FreeCAD.getDocument(self._doc_name)
        if not doc:
            return

        # Check whether a _MaterialHatch_ object depends on this object.
        mat_name = _material_hatch_name(obj)
        generated = doc.getObject(mat_name)
        if generated is None:
            return

        # Touch the generated hatch so it's in the next recompute cycle.
        try:
            generated.touch()
        except Exception:
            pass

        # Defer doc.recompute() to avoid calling it from within a change
        # notification callback (risk of recursion / crash).
        if not self._pending:
            self._pending = True
            doc_name = self._doc_name

            def _do_recompute():
                obs = _HatchGeometryObserver._registry.get(doc_name)
                if obs:
                    obs._pending = False
                d = FreeCAD.getDocument(doc_name)
                if d:
                    try:
                        d.recompute()
                    except Exception:
                        pass

            QtCore.QTimer.singleShot(80, _do_recompute)


def ensure_material_hatch(target_obj, material_obj=None, hatch_obj=None):
    """
    Create or update the generated Applied hatch driven by target.Material.Hatch.
    Returns the generated hatch object, or None if nothing should be shown.
    """
    if not target_obj:
        return None

    doc = getattr(target_obj, "Document", None) or FreeCAD.ActiveDocument
    if not doc:
        return None

    if material_obj is None:
        material_obj = getattr(target_obj, "Material", None)

    if hatch_obj is None and material_obj is not None:
        hatch_obj = getattr(material_obj, "Hatch", None)

    enabled = bool(
        getattr(target_obj, "HatchSurfaces", False)
        or getattr(target_obj, "HatchCaps", False)
        or getattr(target_obj, "HatchGhost", False)
    )

    if not enabled or hatch_obj is None:
        _remove_material_hatch(target_obj)
        return None

    if not hasattr(target_obj, "Shape") or target_obj.Shape.isNull():
        _remove_material_hatch(target_obj)
        return None

    # Install the geometry-change observer for this document once.
    # It is a no-op if already installed.
    _HatchGeometryObserver.install(doc)

    name = _material_hatch_name(target_obj)
    generated = doc.getObject(name)
    if generated is None:
        generated = make_custom_hatch(name=name, role="Applied")

    generated.Label = f"{target_obj.Label}_MaterialHatch"
    generated.BaseObject = target_obj
    generated.BaseObjects = []
    generated.Subtractions = []

    _copy_definition_recipe_to_applied(generated, hatch_obj)

    # Mark the generated object as needing recompute. This ensures it is
    # always in FreeCAD's dirty queue regardless of when it was created
    # relative to the current recompute cycle.
    try:
        generated.touch()
    except Exception:
        pass

    try:
        if FreeCAD.GuiUp and hasattr(generated, "ViewObject"):
            generated.ViewObject.Visibility = not bool(getattr(target_obj, "HatchGhost", False))
    except Exception:
        pass

    return generated


def apply_material_hatches(target_obj=None, material_obj=None, hatch_obj=None, recompute=True):
    """
    Public bridge expected by external BIM/material callbacks.

    Accepted call shapes:
      apply_material_hatches(obj)
      apply_material_hatches(obj, material)
      apply_material_hatches(target_obj=obj, material_obj=mat)
    """
    try:
        generated = ensure_material_hatch(
            target_obj=target_obj,
            material_obj=material_obj,
            hatch_obj=hatch_obj,
        )

        doc = None
        if target_obj is not None:
            doc = getattr(target_obj, "Document", None)
        if doc is None:
            doc = FreeCAD.ActiveDocument

        if recompute and doc:
            doc.recompute()

        return generated
    except Exception as e:
        FreeCAD.Console.PrintWarning(f"apply_material_hatches failed: {e}\n")
        return None


# Hatch Task Panel


class CollapsibleSectionBox(QtWidgets.QWidget):
    """Compact accordion widget with click-to-toggle section headers."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._items = []
        self._current_index = -1
        self._layout = QtWidgets.QVBoxLayout(self)
        self._layout.setContentsMargins(0, 0, 0, 0)
        self._layout.setSpacing(6)
        self.setStyleSheet(
            """
            QToolButton {
                background: #e4e4e4;
                border: 1px solid #c0c0c0;
                border-radius: 3px;
                padding: 6px 8px;
                font-weight: normal;
                color: #333333;
                text-align: left;
            }
            QToolButton[expanded="true"] {
                background: #2255aa;
                color: white;
                font-weight: bold;
                border: 1px solid #1a4488;
            }
            QToolButton:hover:!checked {
                background: #d0ddf0;
                color: #1a3a7a;
            }
            QFrame[sectionContent="true"] {
                border: 1px solid #d7d7d7;
                border-top: 0;
                border-radius: 0 0 4px 4px;
                background: #ffffff;
            }
            """
        )

    def addItem(self, widget, title):
        index = len(self._items)
        button = QtWidgets.QToolButton(self)
        button.setText(title)
        button.setCheckable(True)
        button.setChecked(False)
        button.setToolButtonStyle(QtCore.Qt.ToolButtonTextOnly)
        button.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Fixed)
        button.setProperty("expanded", False)
        button.clicked.connect(lambda _checked=False, idx=index: self.toggleIndex(idx))

        frame = QtWidgets.QFrame(self)
        frame.setProperty("sectionContent", True)
        frame_layout = QtWidgets.QVBoxLayout(frame)
        frame_layout.setContentsMargins(6, 6, 6, 6)
        frame_layout.setSpacing(0)
        frame_layout.addWidget(widget)
        frame.setVisible(False)

        self._layout.addWidget(button)
        self._layout.addWidget(frame)
        self._items.append((button, frame, widget))
        return index

    def toggleIndex(self, index):
        if self._current_index == index:
            self.setCurrentIndex(-1)
        else:
            self.setCurrentIndex(index)

    def setCurrentIndex(self, index):
        self._current_index = index if 0 <= index < len(self._items) else -1
        for i, (button, frame, _widget) in enumerate(self._items):
            expanded = i == self._current_index
            frame.setVisible(expanded)
            button.setChecked(expanded)
            button.setProperty("expanded", expanded)
            button.style().unpolish(button)
            button.style().polish(button)

    def currentIndex(self):
        return self._current_index


class HatchTaskPanel:
    def __init__(self, hatch_obj=None):
        self.editing_obj = hatch_obj
        self.doc = FreeCAD.ActiveDocument
        if not self.doc:
            QtWidgets.QMessageBox.warning(None, "Error", "No active document found.")
            return

        self.current_temp_preview_name = None
        self.master_object_list = self.get_all_objects_classified()
        self.last_manual_scale = None
        self._session_warned_for = set()
        self._recommended_scale = None
        self.selected_base_object_names = set()
        self.selected_pattern_object_names = set()

        self.form = self.create_ui()
        self.initial_pattern_setup()

        if self.editing_obj is not None:
            self._load_from_object(self.editing_obj)

        self._connect_refresh_signals()
        self._refresh_distribution_ui()
        self._refresh_units_ui()
        self._refresh_recipe_summary()
        self._refresh_subtraction_status()
        QtCore.QTimer.singleShot(100, self._update_tile_estimate)

    def _connect_refresh_signals(self):
        self.baseCombo.currentIndexChanged.connect(self._update_tile_estimate)
        self.baseCombo.currentIndexChanged.connect(self._refresh_recipe_summary)
        self.builtinPatternCombo.currentIndexChanged.connect(self._update_tile_estimate)
        self.builtinPatternCombo.currentIndexChanged.connect(self._refresh_recipe_summary)
        self.customPatternCombo.currentIndexChanged.connect(self._update_tile_estimate)
        self.customPatternCombo.currentIndexChanged.connect(self._refresh_recipe_summary)
        self.distCombo.currentIndexChanged.connect(self._update_tile_estimate)
        self.distCombo.currentIndexChanged.connect(self._refresh_distribution_ui)
        self.distCombo.currentIndexChanged.connect(self._refresh_recipe_summary)
        self.repXSpin.valueChanged.connect(self._update_tile_estimate)
        self.repYSpin.valueChanged.connect(self._update_tile_estimate)
        self.scaleSpin.valueChanged.connect(self._update_tile_estimate)
        self.radialCountSpin.valueChanged.connect(self._update_tile_estimate)
        self.concentricCountSpin.valueChanged.connect(self._update_tile_estimate)
        self.randomCountSpin.valueChanged.connect(self._update_tile_estimate)
        if self.spacingInput is not None:
            try:
                self.spacingInput.valueChanged.connect(self._refresh_recipe_summary)
            except Exception:
                pass
        elif self.spacingSpin is not None:
            self.spacingSpin.valueChanged.connect(self._refresh_recipe_summary)
        self.rotSpin.valueChanged.connect(self._refresh_recipe_summary)
        self.offsetXSpin.valueChanged.connect(self._refresh_recipe_summary)
        self.offsetYSpin.valueChanged.connect(self._refresh_recipe_summary)
        self.useUnitsCheck.toggled.connect(self._refresh_units_ui)
        self.useUnitsCheck.toggled.connect(self._refresh_recipe_summary)
        self.unitSystemCombo.currentIndexChanged.connect(self._refresh_units_ui)
        self.unitSystemCombo.currentIndexChanged.connect(self._refresh_recipe_summary)
        self.useSurfaceProjectionCheck.toggled.connect(self._refresh_recipe_summary)
        self.forceXYPlaneCheck.toggled.connect(self._refresh_recipe_summary)
        self.hatchRoleCombo.currentIndexChanged.connect(self._refresh_recipe_summary)
        self.patternObjectsList.itemSelectionChanged.connect(self._refresh_recipe_summary)
        self.subtractionsTable.itemSelectionChanged.connect(self._on_subtraction_selection_changed)

    def _resolve_obj_name(self, text):
        if not text or not self.doc:
            return None
        obj = self.doc.getObject(text)
        if obj:
            return obj
        for obj in self.doc.Objects:
            if obj.Label == text:
                return obj
        return None

    def _display_name_for_object(self, obj):
        if not obj:
            return ""
        if getattr(obj, "Label", "") and obj.Label != obj.Name:
            return f"{obj.Label} ({obj.Name})"
        return obj.Name

    def _combo_set_object(self, combo, obj):
        if not obj:
            combo.setCurrentIndex(-1)
            return
        idx = combo.findText(obj.Name)
        if idx == -1:
            combo.addItem(obj.Name)
            idx = combo.findText(obj.Name)
        combo.setCurrentIndex(max(idx, 0))

    def _create_labeled_spin(self, minimum, maximum, value=0.0, decimals=2, step=1.0):
        spin = QtWidgets.QDoubleSpinBox()
        spin.setRange(minimum, maximum)
        spin.setDecimals(decimals)
        spin.setSingleStep(step)
        spin.setValue(value)
        return spin

    def _shape_kind_for_object(self, obj):
        if not obj or not hasattr(obj, "Shape"):
            return "No Shape"
        shape = obj.Shape
        if shape.isNull():
            return "Null"
        if shape.Solids:
            return "Solid"
        if shape.Shells:
            return "Shell"
        if shape.Faces:
            return "Face"
        if shape.Wires:
            return "Wire"
        if shape.Edges:
            return "Edge"
        if shape.Vertexes:
            return "Vertex"
        return "Empty"

    def _subtraction_status_for_object(self, obj):
        if not obj or not hasattr(obj, "Shape"):
            return "[!] Missing Shape", QtGui.QColor("#aa5500")
        shape = obj.Shape
        if shape.isNull():
            return "[!] Null Shape", QtGui.QColor("#aa5500")
        if shape.Faces or shape.Solids or shape.Shells:
            return "[ok] Ready", QtGui.QColor("#0a7d32")
        if shape.Wires or shape.Edges:
            return "[!] Open-wire only", QtGui.QColor("#aa5500")
        return "[!] Empty", QtGui.QColor("#aa5500")

    def _get_list_item_object_name(self, item):
        if item is None:
            return ""
        name = item.data(QtCore.Qt.UserRole)
        if name:
            return str(name)
        return item.text()

    def _make_object_list_item(self, obj):
        item = QtWidgets.QListWidgetItem(self._display_name_for_object(obj))
        item.setData(QtCore.Qt.UserRole, obj.Name)
        tooltip = f"{obj.Name}\nType: {self.classify_object(obj)}\nShape: {self._shape_kind_for_object(obj)}"
        item.setToolTip(tooltip)
        return item

    def _estimate_tile_count(self):
        is_definition = self.hatchRoleCombo.currentIndex() == 0
        if is_definition:
            width = 200.0
            height = 200.0
        else:
            base_obj = self._resolve_obj_name(self.baseCombo.currentText())
            if not base_obj or not hasattr(base_obj, "Shape") or base_obj.Shape.isNull():
                return 0, None, "-"
            bounding_box = base_obj.Shape.BoundBox
            width = bounding_box.XLength
            height = bounding_box.YLength
            if width < 1e-6 or height < 1e-6:
                return 0, None, "-"

        current_scale = self.scaleSpin.value()
        dist_mode = self.distCombo.currentText()

        pattern_width, pattern_height = 10.0, 10.0
        try:
            if self.patternSourceCombo.currentIndex() == 0:
                from .ArchHatchPatterns import (
                    generate_built_in_pattern_shape as generateBuiltInPatternShape,
                )

                pattern_shape = generateBuiltInPatternShape(self.builtinPatternCombo.currentText())
                if pattern_shape and not pattern_shape.isNull():
                    pattern_width = max(pattern_shape.BoundBox.XLength, 1e-6)
                    pattern_height = max(pattern_shape.BoundBox.YLength, 1e-6)
            else:
                pattern_obj = self._resolve_obj_name(self.customPatternCombo.currentText())
                if pattern_obj and hasattr(pattern_obj, "Shape") and not pattern_obj.Shape.isNull():
                    pattern_width = max(pattern_obj.Shape.BoundBox.XLength, 1e-6)
                    pattern_height = max(pattern_obj.Shape.BoundBox.YLength, 1e-6)
        except Exception:
            pass

        if dist_mode == "SeamlessTiling":
            effective_width = pattern_width * current_scale
            effective_height = pattern_height * current_scale
            if effective_width < 1e-6 or effective_height < 1e-6:
                tiles = 0
            else:
                tiles = math.ceil(width / effective_width) * math.ceil(height / effective_height)
        elif dist_mode in ("CenteredTiling", "RelativeSpacing", "LinearGrid"):
            tiles = int(self.repXSpin.value()) * int(self.repYSpin.value())
        elif dist_mode == "RadialDistribution":
            tiles = int(self.radialCountSpin.value())
        elif dist_mode == "ConcentricDistribution":
            tiles = max(1, int(self.concentricCountSpin.value())) * 8
        elif dist_mode == "RandomDistribution":
            tiles = int(self.randomCountSpin.value())
        else:
            tiles = 50

        recommended_scale = None
        if dist_mode == "SeamlessTiling" and tiles > 300 and pattern_width > 0 and pattern_height > 0:
            target = 200
            recommended_scale = current_scale * math.sqrt(tiles / target)
            if recommended_scale > 0:
                magnitude = 10 ** math.floor(math.log10(recommended_scale))
                recommended_scale = math.ceil(recommended_scale / magnitude) * magnitude

        if tiles <= 100:
            severity = "Light"
        elif tiles <= 500:
            severity = "Moderate"
        elif tiles <= 2000:
            severity = "Heavy!"
        else:
            severity = "Very Heavy!!"

        return int(tiles), recommended_scale, severity

    def _update_tile_estimate(self):
        tiles, recommended_scale, severity = self._estimate_tile_count()

        if tiles <= 0:
            self.tileEstLabel.setText("Est. tiles: -")
            self.tileRecommendBtn.setVisible(False)
            self.statsFooterLabel.setText("Ready")
            return

        self.tileEstLabel.setText("Est. tiles: ~%d  [%s]" % (tiles, severity))
        self._recommended_scale = recommended_scale

        if "Very Heavy" in severity:
            self.tileEstLabel.setStyleSheet("color: #cc0000; font-size: 10px; font-weight: bold;")
        elif "Heavy" in severity:
            self.tileEstLabel.setStyleSheet("color: #cc6600; font-size: 10px;")
        elif "Moderate" in severity:
            self.tileEstLabel.setStyleSheet("color: #888800; font-size: 10px;")
        else:
            self.tileEstLabel.setStyleSheet("color: #006600; font-size: 10px;")
        # NOTE: "Light", "Moderate", "Heavy!", "Very Heavy!!" -- all ASCII for Windows safety

        show_recommend = (
            recommended_scale is not None and recommended_scale > self.scaleSpin.value() * 1.2
        )
        self.tileRecommendBtn.setVisible(show_recommend)
        if show_recommend:
            self.tileRecommendBtn.setText(f"Use {recommended_scale:g}")

        stats = [f"{tiles:,} est. tiles", severity]
        if self.editing_obj is not None and hasattr(self.editing_obj, "GenerationTime"):
            stats.append(f"last gen {float(getattr(self.editing_obj, 'GenerationTime', 0.0)):.2f}s")
            stats.append(f"last tiles {int(getattr(self.editing_obj, 'TileCount', 0))}")
        self.statsFooterLabel.setText(" | ".join(stats))

    def on_use_recommended_scale(self):
        if self._recommended_scale:
            self.scaleSpin.setValue(self._recommended_scale)

    def _check_dangerous_tile_count(self):
        tiles, recommended_scale, severity = self._estimate_tile_count()
        if tiles <= 2000:
            return True

        warn_key = (
            f"{self.baseCombo.currentText()}_{self.builtinPatternCombo.currentText()}_"
            f"{self.customPatternCombo.currentText()}_{self.scaleSpin.value()}_{self.distCombo.currentText()}"
        )
        if warn_key in self._session_warned_for:
            return True

        self._session_warned_for.add(warn_key)

        message = (
            f"Estimated tile count: ~{tiles:,} -- this may take a long time or freeze FreeCAD.\n\n"
        )
        if recommended_scale:
            message += f"Recommended scale: {recommended_scale:g} (~200 tiles).\n\n"
        message += "Continue with current settings?"

        reply = QtWidgets.QMessageBox.warning(
            self.form,
            "Performance Warning",
            message,
            QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
        )
        return reply == QtWidgets.QMessageBox.Yes

    def create_ui(self):
        main_widget = QtWidgets.QWidget()
        main_layout = QtWidgets.QVBoxLayout(main_widget)
        main_layout.setContentsMargins(8, 8, 8, 8)
        main_layout.setSpacing(8)

        summary_group = QtWidgets.QGroupBox("Hatch Summary")
        summary_layout = QtWidgets.QVBoxLayout(summary_group)
        summary_layout.setContentsMargins(6, 4, 6, 4)
        summary_layout.setSpacing(2)
        self.recipeSummaryLabel = QtWidgets.QLabel("Configure the target, recipe, and preview.")
        self.recipeSummaryLabel.setWordWrap(True)
        self.recipeSummaryLabel.setStyleSheet("font-weight: bold; font-size: 10px;")
        self.summaryHintLabel = QtWidgets.QLabel(
            "Target -> Pattern -> Placement -> Advanced -> Preview"
        )
        self.summaryHintLabel.setStyleSheet("color: #666666; font-size: 10px;")
        self.summaryHintLabel.setWordWrap(True)
        summary_layout.addWidget(self.recipeSummaryLabel)
        summary_layout.addWidget(self.summaryHintLabel)
        main_layout.addWidget(summary_group)

        scroll_area = QtWidgets.QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        scroll_area.setFrameShape(QtWidgets.QFrame.NoFrame)
        scroll_content = QtWidgets.QWidget()
        scroll_layout = QtWidgets.QVBoxLayout(scroll_content)
        scroll_layout.setContentsMargins(0, 0, 0, 0)
        scroll_layout.setSpacing(8)

        self.sectionsBox = CollapsibleSectionBox()
        self.sectionsBox.setSizePolicy(
            QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding
        )
        scroll_layout.addWidget(self.sectionsBox)

        # Shared core widgets
        self.hatchRoleLabel = QtWidgets.QLabel("Mode:")
        self.hatchRoleCombo = QtWidgets.QComboBox()
        self.hatchRoleCombo.addItems(["Definition (reusable style)", "Applied (on face)"])
        self.hatchRoleCombo.setCurrentIndex(0)
        self.hatchRoleCombo.setToolTip(
            "Definition: create a reusable hatch style.\n"
            "Applied: generate hatch directly on a selected base shape or extracted face."
        )
        self.hatchRoleCombo.currentIndexChanged.connect(self._on_hatch_role_changed)

        self.definitionNote = QtWidgets.QLabel(
            "Definition mode has no base target. Use it to author reusable hatch recipes."
        )
        self.definitionNote.setWordWrap(True)
        self.definitionNote.setStyleSheet("color: #006600; font-style: italic;")

        self.baseLabel = QtWidgets.QLabel("Base Shape:")
        self.baseCombo = QtWidgets.QComboBox()
        self.pickBaseBtn = QtWidgets.QPushButton("Pick Base from Selection")
        self.pickBaseBtn.clicked.connect(self.pick_base_shape)

        self.face_extractor_group = QtWidgets.QGroupBox("Face Extractor")
        face_ext_hl = QtWidgets.QHBoxLayout(self.face_extractor_group)
        face_ext_hl.setContentsMargins(6, 4, 6, 4)
        face_ext_hl.setSpacing(6)
        # Keep info compact — no paragraph so the group doesn't balloon when narrow
        self.face_extractor_info = QtWidgets.QLabel("Click a face, then:")
        self.pickFaceBtn = QtWidgets.QPushButton("Pick Face -> FaceExtractor")
        self.pickFaceBtn.clicked.connect(self.pick_face_and_create_extractor)
        self.face_extractor_status = QtWidgets.QLabel("")
        self.face_extractor_status.setStyleSheet(
            "color: green; font-style: italic; font-size: 10px;"
        )
        self.face_extractor_status.setWordWrap(True)
        face_ext_hl.addWidget(self.face_extractor_info)
        face_ext_hl.addWidget(self.pickFaceBtn)
        face_ext_hl.addWidget(self.face_extractor_status, 1)

        self.baseSearchField = QtWidgets.QLineEdit()
        self.baseSearchField.setPlaceholderText("Filter base objects...")
        self.baseTypeFilterCombo = QtWidgets.QComboBox()
        for category in ["All", "Sketch", "PartFeature", "Compound", "Other"]:
            self.baseTypeFilterCombo.addItem(category)

        self.baseObjectsList = QtWidgets.QListWidget()
        self.baseObjectsList.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)
        self.baseObjectsList.setMaximumHeight(80)

        self.subtractionsTable = QtWidgets.QTreeWidget()
        self.subtractionsTable.setColumnCount(4)
        self.subtractionsTable.setHeaderLabels(["Object", "Kind", "Shape", "Status"])
        self.subtractionsTable.setRootIsDecorated(False)
        self.subtractionsTable.setAlternatingRowColors(True)
        self.subtractionsTable.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.subtractionsTable.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.subtractionsTable.setMaximumHeight(120)
        self.subtractionsTable.header().setStretchLastSection(False)
        self.subtractionsTable.header().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.subtractionsTable.header().setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        self.subtractionsTable.header().setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeToContents)
        self.subtractionsTable.header().setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeToContents)
        self.subtractionsInfoLabel = QtWidgets.QLabel(
            "Add cut-out objects from selection. Rows show the geometry class and readiness of each subtraction operand."
        )
        self.subtractionsInfoLabel.setWordWrap(True)
        self.subtractionsInfoLabel.setStyleSheet("color: #666666; font-size: 11px;")
        self.pickSubBtn = QtWidgets.QPushButton("Add from Selection")
        self.pickSubBtn.clicked.connect(self.add_subtractions_from_selection)
        self.removeSubBtn = QtWidgets.QPushButton("Remove Selected")
        self.removeSubBtn.clicked.connect(self.remove_selected_subtractions)
        self.clearSubBtn = QtWidgets.QPushButton("Clear")
        self.clearSubBtn.clicked.connect(self.clear_subtractions)
        self.selectSubBtn = QtWidgets.QPushButton("Select in Tree")
        self.selectSubBtn.clicked.connect(self.select_subtraction_in_tree)
        self.zoomSubBtn = QtWidgets.QPushButton("Zoom to Selected")
        self.zoomSubBtn.clicked.connect(self.zoom_to_subtraction)

        self.patternSourceLabel = QtWidgets.QLabel("Pattern Source:")
        self.patternSourceCombo = QtWidgets.QComboBox()
        self.patternSourceCombo.addItems(["Built-in", "Custom"])
        self.patternSourceCombo.currentIndexChanged.connect(self.on_pattern_source_changed)

        self.builtinPatternLabel = QtWidgets.QLabel("Built-in Pattern:")
        self.builtinPatternCombo = QtWidgets.QComboBox()
        self.builtinPatternCombo.addItems([
            "SolidFill", "HorizontalLines", "VerticalLines", "Crosshatch", "Herringbone",
            "BrickPattern", "RandomDots", "OverlappingSquares", "Checkerboard",
            "CheckerboardCircles", "RotatingHexagons", "NestedTriangles",
            "InterlockingCircles", "RecursiveSquares", "FlowerOfLife", "VoronoiMesh",
            "OffsetChecker", "ZigZag", "HexagonalHoriz", "HexagonalVerti", "HexagonalPattern",
            "TrianglesGrid", "MidEastMosaic", "StarGridPattern", "BasketWeave", "Honeycomb",
            "SineWave", "SpaceFrame", "HoneycombDual", "ArtDeco", "StainedGlass",
            "PenroseTriangle", "GreekKey", "ChainLinks", "TriangleForest", "CeramicTile",
            "CirclesGrid", "PlusSigns", "WavesPattern", "GalaxyStarsPattern", "GridDots",
            "HexDots", "FractalTree", "Voronoi", "FractalBranches", "OrganicMaze",
            "BiomorphicCells", "RadialSunburst", "Sunburst", "Ziggurat", "SpiralPattern",
            "PentaflakeFractal", "HilbertCurve", "SierpinskiTriangle", "PenroseTiling",
            "EinsteinMonotile", "LeafVeins", "WoodPlanks", "ParquetHerringbone", "WoodGrain",
            "DrywallOrangePeel", "DrywallKnockdown", "StuccoSandFloat", "StuccoDash",
            "DrywallSkipTrowel", "Concrete", "ConcreteStampedPattern", "ConcreteSaltFinish",
            "ConcreteFormTiePattern", "ConcreteSandblastPattern", "ConcreteControlJoint",
            "ConcreteGridPattern", "WoodKnotPattern", "ConcreteAggregatePattern",
            "BrushedConcrete", "PebbleConcrete", "CrackedConcrete", "AggregateConcrete",
            "StampedConcrete", "Insulation", "Rebar", "RoofTiles",
        ])

        self.customPatternLabel = QtWidgets.QLabel("Custom Pattern:")
        self.customPatternCombo = QtWidgets.QComboBox()
        self.pickCustomBtn = QtWidgets.QPushButton("Pick Custom from Selection")
        self.pickCustomBtn.clicked.connect(self.pick_custom_pattern)
        self.patternSearchField = QtWidgets.QLineEdit()
        self.patternSearchField.setPlaceholderText("Filter custom pattern objects...")
        self.patternTypeFilterCombo = QtWidgets.QComboBox()
        for category in ["All", "Sketch", "PartFeature", "Compound", "Other"]:
            self.patternTypeFilterCombo.addItem(category)

        self.patternAvailableList = QtWidgets.QListWidget()
        self.patternAvailableList.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.patternAvailableList.setMaximumHeight(90)
        self.patternAvailableList.setToolTip(
            "Filtered pattern objects available to use as the main pattern or as extra fused pattern sources."
        )

        self.patternObjectsList = QtWidgets.QListWidget()
        self.patternObjectsList.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.patternObjectsList.setMaximumHeight(90)
        self.patternObjectsList.setToolTip(
            "Selected extra pattern objects that will be fused into the recipe."
        )
        self.addPatternBtn = QtWidgets.QPushButton("Add Highlighted")
        self.addPatternBtn.clicked.connect(self.add_selected_patterns_from_available)
        self.addCurrentPatternBtn = QtWidgets.QPushButton("Add Current")
        self.addCurrentPatternBtn.clicked.connect(self.add_current_pattern_to_extras)
        self.addPatternSelectionBtn = QtWidgets.QPushButton("Add Selection")
        self.addPatternSelectionBtn.clicked.connect(self.add_patterns_from_selection)
        self.removePatternBtn = QtWidgets.QPushButton("Remove Selected")
        self.removePatternBtn.clicked.connect(self.remove_selected_pattern_objects)
        self.clearPatternBtn = QtWidgets.QPushButton("Clear")
        self.clearPatternBtn.clicked.connect(self.clear_pattern_objects)
        self.extraPatternInfoLabel = QtWidgets.QLabel(
            "Use the filtered list to build a reusable multi-object pattern recipe."
        )
        self.extraPatternInfoLabel.setWordWrap(True)
        self.extraPatternInfoLabel.setStyleSheet("color: #666666; font-size: 11px;")

        self.distLabel = QtWidgets.QLabel("Distribution Mode:")
        self.distCombo = QtWidgets.QComboBox()
        self.distCombo.addItems([
            "CenteredTiling", "RelativeSpacing", "SeamlessTiling", "LinearGrid",
            "RadialDistribution", "ConcentricDistribution", "RandomDistribution",
            "AdaptiveDistribution",
        ])
        self.distCombo.setCurrentText("SeamlessTiling")

        self.autoScaleCheck = QtWidgets.QCheckBox("Auto Scale to Fit Base")

        self.scaleLabel = QtWidgets.QLabel("Pattern Scale:")
        self.scaleSpin = QtWidgets.QDoubleSpinBox()
        self.scaleSpin.setRange(0.0001, 1e5)
        self.scaleSpin.setDecimals(4)
        self.scaleSpin.setValue(1.0)
        self.scaleSpin.valueChanged.connect(self.on_scale_changed)

        self.scaleResetBtn = QtWidgets.QPushButton("Reset")
        self.scaleResetBtn.setToolTip("Reset to smart default scale")
        self.scaleResetBtn.clicked.connect(
            lambda: self.set_smart_default_scale(self.patternSourceCombo.currentIndex() == 0)
        )
        self.scaleRecommendBtn = QtWidgets.QPushButton("Rec")
        self.scaleRecommendBtn.setToolTip("Set the recommended scale for preview performance")
        self.scaleRecommendBtn.clicked.connect(self.on_use_recommended_scale)
        self.scaleRecommendBtn.setFixedWidth(35)
        self.tileEstLabel = QtWidgets.QLabel("Est. tiles: -")
        self.tileEstLabel.setStyleSheet("color: gray; font-size: 10px;")
        self.tileRecommendBtn = QtWidgets.QPushButton("Use recommended")
        self.tileRecommendBtn.setVisible(False)
        self.tileRecommendBtn.clicked.connect(self.on_use_recommended_scale)

        self.rotLabel = QtWidgets.QLabel("Rotation (deg):")
        self.rotSpin = QtWidgets.QDoubleSpinBox()
        self.rotSpin.setRange(-360.0, 360.0)
        self.rotSpin.setDecimals(2)

        self.spacingLabel = QtWidgets.QLabel("Base Spacing:")
        # Prefer the unit-aware FreeCAD InputField widget; fall back to a plain
        # spinbox if the GUI loader is unavailable (e.g. during testing).
        try:
            self.spacingInput = FreeCADGui.UiLoader().createWidget("Gui::InputField")
            self.spacingInput.setProperty("unitCategory", "Length")
            self.spacingInput.setText("0 mm")
            self.spacingSpin = None  # not used when InputField is available
        except Exception:
            self.spacingInput = None
            self.spacingSpin = QtWidgets.QDoubleSpinBox()
            self.spacingSpin.setRange(0.0, 1e6)
            self.spacingSpin.setDecimals(4)
            self.spacingSpin.setValue(0.0)
            self.spacingSpin.setSuffix(" mm")
        self.useUnitsCheck = QtWidgets.QCheckBox("Interpret spacing using explicit unit system")
        self.unitSystemCombo = QtWidgets.QComboBox()
        self.unitSystemCombo.addItems([
            "FreeCAD Default", "Metric (m)", "Imperial (ft)", "BIM Workbench Unit"
        ])
        self.spacingUnitLabel = QtWidgets.QLabel("")
        self.spacingUnitLabel.setStyleSheet("color: #666666; font-size: 11px;")
        self.spacingUnitLabel.setWordWrap(True)

        self.repXSpin = QtWidgets.QSpinBox()
        self.repXSpin.setRange(1, 999)
        self.repXSpin.setValue(5)
        self.repYSpin = QtWidgets.QSpinBox()
        self.repYSpin.setRange(1, 999)
        self.repYSpin.setValue(5)
        self.radialCountSpin = QtWidgets.QSpinBox()
        self.radialCountSpin.setRange(1, 9999)
        self.radialCountSpin.setValue(8)
        self.radialRadiusSpin = self._create_labeled_spin(0.0, 1e6, 50.0, decimals=2, step=1.0)
        self.concentricCountSpin = QtWidgets.QSpinBox()
        self.concentricCountSpin.setRange(1, 9999)
        self.concentricCountSpin.setValue(5)
        self.concentricSpacingSpin = self._create_labeled_spin(0.0, 1e6, 10.0, decimals=2, step=1.0)
        self.randomCountSpin = QtWidgets.QSpinBox()
        self.randomCountSpin.setRange(1, 999999)
        self.randomCountSpin.setValue(30)

        self.tileLabel = QtWidgets.QLabel("Base Tile:")
        self.tileCombo = QtWidgets.QComboBox()
        self.pickTileBtn = QtWidgets.QPushButton("Pick Tile from Selection")
        self.pickTileBtn.clicked.connect(self.pick_base_tile)
        self.tileVisibilityCheck = QtWidgets.QCheckBox("Show helper tile")
        self.tileVisibilityCheck.setChecked(True)

        self.placementModeCombo = QtWidgets.QComboBox()
        self.placementModeCombo.addItems([
            "Origin", "Center", "TopLeft", "TopRight", "BottomLeft", "BottomRight",
            "TopCenter", "BottomCenter", "LeftCenter", "RightCenter", "Custom",
        ])
        self.placementModeCombo.setCurrentText("Origin")
        self.lockCheck = QtWidgets.QCheckBox("Lock to Base")
        self.offsetXSpin = self._create_labeled_spin(-1e5, 1e5, 0.0, decimals=2, step=1.0)
        self.offsetYSpin = self._create_labeled_spin(-1e5, 1e5, 0.0, decimals=2, step=1.0)
        self.scaleModeCombo = QtWidgets.QComboBox()
        self.scaleModeCombo.addItems(["Absolute", "FitWidth", "FitHeight", "FitMinDim", "FitMaxDim"])
        self.useSurfaceProjectionCheck = QtWidgets.QCheckBox(
            "Use Surface Projection (walls, roofs, sloped faces)"
        )
        self.useSurfaceProjectionCheck.setChecked(True)
        self.forceXYPlaneCheck = QtWidgets.QCheckBox("Force XY Plane (legacy behavior)")
        self.forceXYPlaneCheck.setChecked(False)

        self.randomCheck = QtWidgets.QCheckBox("Randomize Placement")
        self.offRangeSpin = self._create_labeled_spin(0.0, 1e4, 0.0, decimals=2, step=1.0)
        self.rotRangeSpin = self._create_labeled_spin(0.0, 360.0, 0.0, decimals=2, step=1.0)
        self.scaleMinSpin = self._create_labeled_spin(0.01, 1e4, 1.0, decimals=3, step=0.1)
        self.scaleMaxSpin = self._create_labeled_spin(0.01, 1e4, 1.0, decimals=3, step=0.1)
        self.apply3DCheck = QtWidgets.QCheckBox("Apply to 3D Surface")
        self.maxTilesSpin = QtWidgets.QSpinBox()
        self.maxTilesSpin.setRange(1, 999999)
        self.maxTilesSpin.setValue(5000)
        self.clipModeCombo = QtWidgets.QComboBox()
        self.clipModeCombo.addItems(["BooleanOnly", "PreserveLinesNoClip"])
        self.showFacesCheck = QtWidgets.QCheckBox("Show Faces")
        self.showFacesCheck.setChecked(False)
        self.densitySpin = self._create_labeled_spin(0.0, 1.0, 1.0, decimals=3, step=0.05)
        self.enableColorVarCheck = QtWidgets.QCheckBox("Enable Color Variation")
        self.colorVarSpin = self._create_labeled_spin(0.0, 1.0, 0.5, decimals=3, step=0.05)
        self.spacingVariationSpin = self._create_labeled_spin(0.0, 1.0, 0.0, decimals=3, step=0.05)
        self.shapeDistortionCheck = QtWidgets.QCheckBox("Enable Shape Distortion")

        self.previewBtn = QtWidgets.QPushButton("Preview")
        self.previewBtn.clicked.connect(self.on_preview)
        self.previewBtnMain = self.previewBtn
        self.keepPreviewCheck = QtWidgets.QCheckBox("Keep preview shape (non-parametric)")
        self.keepPreviewCheck.setChecked(False)
        self.previewAtLocationCheck = QtWidgets.QCheckBox("Preview at surface location")
        self.previewAtLocationCheck.setChecked(True)
        self.previewAtLocationCheck.setToolTip(
            "Checked: preview appears at the base object's world position.\n"
            "Unchecked: preview is moved to the origin for easier close-up inspection."
        )
        self.previewStatusLabel = QtWidgets.QLabel(
            "Preview uses the same execute() path as the final hatch, limited to <=100 tiles for speed."
        )
        self.previewStatusLabel.setWordWrap(True)
        self.previewStatusLabel.setStyleSheet("color: #666666; font-size: 11px;")

        # Target page
        target_page = QtWidgets.QWidget()
        target_layout = QtWidgets.QVBoxLayout(target_page)
        target_layout.setContentsMargins(8, 8, 8, 8)
        target_layout.setSpacing(8)
        mode_group = QtWidgets.QGroupBox("Target")
        mode_form = QtWidgets.QFormLayout(mode_group)
        mode_form.addRow(self.hatchRoleLabel, self.hatchRoleCombo)
        mode_form.addRow("", self.definitionNote)
        mode_form.addRow(self.baseLabel, self.baseCombo)
        mode_form.addRow("", self.pickBaseBtn)

        base_filter_row = QtWidgets.QHBoxLayout()
        base_filter_row.addWidget(QtWidgets.QLabel("Search:"))
        base_filter_row.addWidget(self.baseSearchField)
        base_filter_row.addWidget(QtWidgets.QLabel("Type:"))
        base_filter_row.addWidget(self.baseTypeFilterCombo)
        mode_form.addRow("", base_filter_row)
        mode_form.addRow("", self.face_extractor_group)
        target_layout.addWidget(mode_group)

        multi_base_group = QtWidgets.QGroupBox("Additional Base Objects")
        multi_base_layout = QtWidgets.QVBoxLayout(multi_base_group)
        multi_base_hint = QtWidgets.QLabel(
            "Select extra base objects when you want one hatch recipe to span multiple shapes."
        )
        multi_base_hint.setWordWrap(True)
        multi_base_hint.setStyleSheet("color: #666666; font-size: 11px;")
        multi_base_layout.addWidget(multi_base_hint)
        multi_base_layout.addWidget(self.baseObjectsList)
        target_layout.addWidget(multi_base_group)

        self.subtractionsGroup = QtWidgets.QGroupBox("Subtractions")
        sub_layout = QtWidgets.QVBoxLayout(self.subtractionsGroup)
        sub_layout.addWidget(self.subtractionsInfoLabel)
        sub_layout.addWidget(self.subtractionsTable)
        sub_btn_row = QtWidgets.QHBoxLayout()
        sub_btn_row.addWidget(self.pickSubBtn)
        sub_btn_row.addWidget(self.removeSubBtn)
        sub_btn_row.addWidget(self.clearSubBtn)
        sub_btn_row.addStretch(1)
        sub_btn_row.addWidget(self.selectSubBtn)
        sub_btn_row.addWidget(self.zoomSubBtn)
        sub_layout.addLayout(sub_btn_row)
        target_layout.addWidget(self.subtractionsGroup)
        target_layout.addStretch(1)
        self.sectionsBox.addItem(target_page, "1. Target")

        # Pattern page
        pattern_page = QtWidgets.QWidget()
        pattern_layout = QtWidgets.QVBoxLayout(pattern_page)
        pattern_layout.setContentsMargins(8, 8, 8, 8)
        pattern_layout.setSpacing(8)

        source_group = QtWidgets.QGroupBox("Pattern Source")
        source_layout = QtWidgets.QVBoxLayout(source_group)
        source_layout.setContentsMargins(6, 6, 6, 6)
        source_layout.setSpacing(8)

        top_form = QtWidgets.QFormLayout()
        top_form.addRow(self.patternSourceLabel, self.patternSourceCombo)
        top_form.addRow(self.builtinPatternLabel, self.builtinPatternCombo)
        top_form.addRow(self.customPatternLabel, self.customPatternCombo)
        top_form.addRow("", self.pickCustomBtn)
        source_layout.addLayout(top_form)

        pattern_filter_row = QtWidgets.QHBoxLayout()
        pattern_filter_row.addWidget(self.patternSearchField, 1)
        pattern_filter_row.addWidget(self.patternTypeFilterCombo)
        source_layout.addLayout(pattern_filter_row)

        available_label = QtWidgets.QLabel("Available Pattern Objects")
        available_label.setStyleSheet("font-weight: bold;")
        source_layout.addWidget(available_label)
        source_layout.addWidget(self.patternAvailableList)

        pattern_btn_row = QtWidgets.QHBoxLayout()
        pattern_btn_row.addWidget(self.addPatternBtn)
        pattern_btn_row.addWidget(self.addCurrentPatternBtn)
        pattern_btn_row.addWidget(self.addPatternSelectionBtn)
        pattern_btn_row.addStretch(1)
        source_layout.addLayout(pattern_btn_row)

        selected_label = QtWidgets.QLabel("Selected Extra Pattern Objects")
        selected_label.setStyleSheet("font-weight: bold;")
        source_layout.addWidget(selected_label)
        source_layout.addWidget(self.patternObjectsList)

        pattern_selected_btn_row = QtWidgets.QHBoxLayout()
        pattern_selected_btn_row.addWidget(self.removePatternBtn)
        pattern_selected_btn_row.addWidget(self.clearPatternBtn)
        pattern_selected_btn_row.addStretch(1)
        source_layout.addLayout(pattern_selected_btn_row)
        source_layout.addWidget(self.extraPatternInfoLabel)
        pattern_layout.addWidget(source_group)

        recipe_group = QtWidgets.QGroupBox("Recipe")
        recipe_form = QtWidgets.QFormLayout(recipe_group)
        recipe_form.addRow(self.distLabel, self.distCombo)
        recipe_form.addRow("", self.autoScaleCheck)
        scale_row = QtWidgets.QHBoxLayout()
        scale_row.addWidget(self.scaleSpin)
        scale_row.addWidget(self.scaleResetBtn)
        scale_row.addWidget(self.scaleRecommendBtn)
        recipe_form.addRow(self.scaleLabel, scale_row)
        est_row = QtWidgets.QHBoxLayout()
        est_row.addWidget(self.tileEstLabel)
        est_row.addStretch(1)
        est_row.addWidget(self.tileRecommendBtn)
        recipe_form.addRow("", est_row)
        recipe_form.addRow("", self.showFacesCheck)
        recipe_form.addRow(self.rotLabel, self.rotSpin)
        spacing_row = QtWidgets.QHBoxLayout()
        if self.spacingInput is not None:
            spacing_row.addWidget(self.spacingInput)
        else:
            spacing_row.addWidget(self.spacingSpin)
        spacing_row.addWidget(self.unitSystemCombo)
        recipe_form.addRow(self.spacingLabel, spacing_row)
        recipe_form.addRow("", self.useUnitsCheck)
        recipe_form.addRow("", self.spacingUnitLabel)
        pattern_layout.addWidget(recipe_group)

        self.gridModeGroup = QtWidgets.QGroupBox("Grid / Tiling Controls")
        grid_form = QtWidgets.QFormLayout(self.gridModeGroup)
        grid_form.addRow("Repetitions X:", self.repXSpin)
        grid_form.addRow("Repetitions Y:", self.repYSpin)
        pattern_layout.addWidget(self.gridModeGroup)

        self.radialModeGroup = QtWidgets.QGroupBox("Radial Controls")
        radial_form = QtWidgets.QFormLayout(self.radialModeGroup)
        radial_form.addRow("Radial Count:", self.radialCountSpin)
        radial_form.addRow("Radial Radius:", self.radialRadiusSpin)
        pattern_layout.addWidget(self.radialModeGroup)

        self.concentricModeGroup = QtWidgets.QGroupBox("Concentric Controls")
        concentric_form = QtWidgets.QFormLayout(self.concentricModeGroup)
        concentric_form.addRow("Concentric Count:", self.concentricCountSpin)
        concentric_form.addRow("Concentric Spacing:", self.concentricSpacingSpin)
        pattern_layout.addWidget(self.concentricModeGroup)

        self.randomModeGroup = QtWidgets.QGroupBox("Random Distribution Controls")
        random_dist_form = QtWidgets.QFormLayout(self.randomModeGroup)
        random_dist_form.addRow("Random Count:", self.randomCountSpin)
        pattern_layout.addWidget(self.randomModeGroup)
        pattern_layout.addStretch(1)
        self.sectionsBox.addItem(pattern_page, "2. Pattern")

        # Placement page
        placement_page = QtWidgets.QWidget()
        placement_layout = QtWidgets.QVBoxLayout(placement_page)
        placement_layout.setContentsMargins(8, 8, 8, 8)
        placement_layout.setSpacing(8)
        placement_group = QtWidgets.QGroupBox("Placement & Projection")
        placement_form = QtWidgets.QFormLayout(placement_group)
        placement_form.addRow("Pattern Placement Mode:", self.placementModeCombo)
        placement_form.addRow("", self.lockCheck)
        placement_form.addRow("Pattern Offset X:", self.offsetXSpin)
        placement_form.addRow("Pattern Offset Y:", self.offsetYSpin)
        placement_form.addRow("Scale Mode:", self.scaleModeCombo)
        placement_form.addRow("", self.useSurfaceProjectionCheck)
        placement_form.addRow("", self.forceXYPlaneCheck)
        placement_layout.addWidget(placement_group)

        tile_group = QtWidgets.QGroupBox("Tile Helper")
        tile_form = QtWidgets.QFormLayout(tile_group)
        tile_form.addRow(self.tileLabel, self.tileCombo)
        tile_form.addRow("", self.pickTileBtn)
        tile_form.addRow("", self.tileVisibilityCheck)
        placement_layout.addWidget(tile_group)
        placement_layout.addStretch(1)
        self.sectionsBox.addItem(placement_page, "3. Placement")

        # Advanced page
        advanced_page = QtWidgets.QWidget()
        advanced_layout = QtWidgets.QVBoxLayout(advanced_page)
        advanced_layout.setContentsMargins(8, 8, 8, 8)
        advanced_layout.setSpacing(8)

        rand_group = QtWidgets.QGroupBox("Randomization")
        rand_form = QtWidgets.QFormLayout(rand_group)
        rand_form.addRow("", self.randomCheck)
        rand_form.addRow("Random Offset +/-:", self.offRangeSpin)
        rand_form.addRow("Random Rot +/- (deg):", self.rotRangeSpin)
        rand_form.addRow("Random Scale Min:", self.scaleMinSpin)
        rand_form.addRow("Random Scale Max:", self.scaleMaxSpin)
        advanced_layout.addWidget(rand_group)

        render_group = QtWidgets.QGroupBox("Rendering & Performance")
        render_form = QtWidgets.QFormLayout(render_group)
        render_form.addRow("Clip Mode:", self.clipModeCombo)
        render_form.addRow("", self.apply3DCheck)
        render_form.addRow("Max Tiles Allowed:", self.maxTilesSpin)
        advanced_layout.addWidget(render_group)

        variation_group = QtWidgets.QGroupBox("Variation")
        variation_form = QtWidgets.QFormLayout(variation_group)
        variation_form.addRow("Density Factor:", self.densitySpin)
        variation_form.addRow("", self.enableColorVarCheck)
        variation_form.addRow("Color Variation Intensity:", self.colorVarSpin)
        variation_form.addRow("Spacing Variation:", self.spacingVariationSpin)
        variation_form.addRow("", self.shapeDistortionCheck)
        advanced_layout.addWidget(variation_group)
        advanced_layout.addStretch(1)
        self.sectionsBox.addItem(advanced_page, "4. Advanced")

        # Preview page
        preview_page = QtWidgets.QWidget()
        preview_layout = QtWidgets.QVBoxLayout(preview_page)
        preview_layout.setContentsMargins(8, 8, 8, 8)
        preview_layout.setSpacing(8)
        preview_group = QtWidgets.QGroupBox("Preview")
        preview_form = QtWidgets.QFormLayout(preview_group)
        preview_form.addRow("", self.previewAtLocationCheck)
        preview_form.addRow("", self.keepPreviewCheck)
        preview_form.addRow("", self.previewStatusLabel)
        preview_form.addRow("", self.previewBtn)
        preview_layout.addWidget(preview_group)
        preview_layout.addStretch(1)
        self.sectionsBox.addItem(preview_page, "5. Preview")

        scroll_layout.addStretch(1)
        scroll_area.setWidget(scroll_content)
        main_layout.addWidget(scroll_area)

        footer_group = QtWidgets.QGroupBox("Status")
        footer_hl = QtWidgets.QHBoxLayout(footer_group)
        footer_hl.setContentsMargins(6, 4, 6, 4)
        footer_hl.setSpacing(6)
        self.statsFooterLabel = QtWidgets.QLabel("Ready")
        self.statsFooterLabel.setWordWrap(True)
        self.statsFooterLabel.setStyleSheet("color: #444444; font-size: 10px;")
        # Persistent preview button - always visible regardless of active section
        self.footerPreviewBtn = QtWidgets.QPushButton("Preview")
        self.footerPreviewBtn.setFixedWidth(72)
        self.footerPreviewBtn.setFixedHeight(24)
        self.footerPreviewBtn.setToolTip(
            "Run a quick preview (<=100 tiles). "
            "Visible in all sections so you can tweak and preview without navigating away."
        )
        self.footerPreviewBtn.clicked.connect(self.on_preview)
        footer_hl.addWidget(self.statsFooterLabel, 1)
        footer_hl.addWidget(self.footerPreviewBtn)
        main_layout.addWidget(footer_group)

        # General hookups
        self.baseCombo.currentIndexChanged.connect(self.on_base_combo_index_changed)
        self.baseTypeFilterCombo.currentIndexChanged.connect(self.refresh_base_views)
        self.baseSearchField.textChanged.connect(
            lambda: QtCore.QTimer.singleShot(250, self.refresh_base_views)
        )
        self.baseObjectsList.itemSelectionChanged.connect(self._on_base_objects_selection_changed)
        self.patternTypeFilterCombo.currentIndexChanged.connect(self.refresh_pattern_views)
        self.patternSearchField.textChanged.connect(
            lambda: QtCore.QTimer.singleShot(250, self.refresh_pattern_views)
        )
        self.patternAvailableList.itemDoubleClicked.connect(
            lambda _item: self.add_selected_patterns_from_available()
        )
        self.customPatternCombo.currentIndexChanged.connect(self.on_custom_pattern_selected)
        self.tileCombo.currentIndexChanged.connect(self.on_tile_combo_index_changed)

        self.refresh_base_views()
        self.refresh_pattern_views()
        self.populate_tile_combo()
        self._on_hatch_role_changed(self.hatchRoleCombo.currentIndex())
        self.sectionsBox.setCurrentIndex(0)
        return main_widget

    def _on_hatch_role_changed(self, index):
        is_definition = index == 0
        target_widgets = [
            self.baseLabel,
            self.baseCombo,
            self.pickBaseBtn,
            self.baseSearchField,
            self.baseTypeFilterCombo,
            self.face_extractor_group,
            self.baseObjectsList,
            self.subtractionsGroup,
        ]
        for widget in target_widgets:
            widget.setVisible(not is_definition)
        self.definitionNote.setVisible(is_definition)
        self.subtractionsInfoLabel.setVisible(not is_definition)
        self._refresh_recipe_summary()
        self._update_tile_estimate()

    def pick_face_and_create_extractor(self):
        selection = FreeCADGui.Selection.getSelectionEx()
        if not selection:
            self.face_extractor_status.setText("No selection. Click a face in 3D view first.")
            self.face_extractor_status.setStyleSheet("color: red; font-style: italic;")
            return

        has_face = any(sub.startswith("Face") for sel in selection for sub in sel.SubElementNames)
        if not has_face:
            self.face_extractor_status.setText(
                "No face selected. Click a face surface, not an edge or vertex."
            )
            self.face_extractor_status.setStyleSheet("color: red; font-style: italic;")
            return

        FreeCAD.ActiveDocument.openTransaction("Extract Face")
        try:
            created = make_face_extractor_from_selection()
            if not created:
                FreeCAD.ActiveDocument.abortTransaction()
                self.face_extractor_status.setText("Face extraction failed. See Report View.")
                self.face_extractor_status.setStyleSheet("color: red; font-style: italic;")
                return

            FreeCAD.ActiveDocument.commitTransaction()
            self.master_object_list = self.get_all_objects_classified()
            self.refresh_base_views()
            self.refresh_pattern_views()
            self.populate_tile_combo()
            first = created[0]
            self._combo_set_object(self.baseCombo, first)
            names = ", ".join(fe.Name for fe in created)
            self.face_extractor_status.setText(f"Created: {names}")
            self.face_extractor_status.setStyleSheet("color: green; font-style: italic;")
        except Exception as e:
            FreeCAD.ActiveDocument.abortTransaction()
            self.face_extractor_status.setText(f"Face extraction failed: {e}")
            self.face_extractor_status.setStyleSheet("color: red; font-style: italic;")
            FreeCAD.Console.PrintError(f"FaceExtractor failed from panel: {e}\n")

    def pick_base_shape(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            QtWidgets.QMessageBox.warning(
                self.form, "No selection", "Select an object in 3D view or tree."
            )
            return
        self._combo_set_object(self.baseCombo, selection[0])
        self._refresh_recipe_summary()

    def pick_custom_pattern(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            QtWidgets.QMessageBox.warning(
                self.form, "No selection", "Select an object in 3D view or tree."
            )
            return
        obj = selection[0]
        self.patternSourceCombo.setCurrentIndex(1)
        self._combo_set_object(self.customPatternCombo, obj)
        self.refresh_pattern_views()
        self._refresh_recipe_summary()

    def add_selected_patterns_from_available(self):
        added = False
        for item in self.patternAvailableList.selectedItems():
            name = self._get_list_item_object_name(item)
            if name:
                self.selected_pattern_object_names.add(name)
                added = True
        if added:
            self.refresh_selected_pattern_objects_list()
            self._refresh_recipe_summary()

    def add_current_pattern_to_extras(self):
        name = self.customPatternCombo.currentText().strip()
        if not name:
            return
        self.selected_pattern_object_names.add(name)
        self.refresh_selected_pattern_objects_list()
        self._refresh_recipe_summary()

    def add_patterns_from_selection(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            QtWidgets.QMessageBox.warning(
                self.form,
                "No selection",
                "Select one or more objects in the 3D view or tree to add as extra pattern sources.",
            )
            return
        for obj in selection:
            if hasattr(obj, "Shape"):
                self.selected_pattern_object_names.add(obj.Name)
        self.refresh_selected_pattern_objects_list()
        self._refresh_recipe_summary()

    def remove_selected_pattern_objects(self):
        removed = False
        for item in self.patternObjectsList.selectedItems():
            name = self._get_list_item_object_name(item)
            if name in self.selected_pattern_object_names:
                self.selected_pattern_object_names.remove(name)
                removed = True
        if removed:
            self.refresh_selected_pattern_objects_list()
            self._refresh_recipe_summary()

    def clear_pattern_objects(self):
        if not self.selected_pattern_object_names:
            return
        self.selected_pattern_object_names.clear()
        self.refresh_selected_pattern_objects_list()
        self._refresh_recipe_summary()

    def pick_subtractions(self):
        self.add_subtractions_from_selection()

    def add_subtractions_from_selection(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            QtWidgets.QMessageBox.warning(
                self.form, "No selection", "Select one or more objects to use as subtraction masks."
            )
            return
        for obj in selection:
            self._add_subtraction_object(obj)
        self._refresh_subtraction_status()
        self._refresh_recipe_summary()

    def _add_subtraction_object(self, obj):
        if not obj:
            return
        for row in range(self.subtractionsTable.topLevelItemCount()):
            item = self.subtractionsTable.topLevelItem(row)
            if item.data(0, QtCore.Qt.UserRole) == obj.Name:
                return

        kind = self.classify_object(obj)
        shape_kind = self._shape_kind_for_object(obj)
        status_text, status_color = self._subtraction_status_for_object(obj)
        item = QtWidgets.QTreeWidgetItem([
            self._display_name_for_object(obj), kind, shape_kind, status_text
        ])
        item.setData(0, QtCore.Qt.UserRole, obj.Name)
        item.setToolTip(0, f"{obj.Name}\nLabel: {obj.Label}")
        item.setForeground(3, QtGui.QBrush(status_color))
        self.subtractionsTable.addTopLevelItem(item)

    def _set_subtraction_objects(self, objects):
        self.subtractionsTable.clear()
        for obj in objects or []:
            self._add_subtraction_object(obj)
        self._refresh_subtraction_status()

    def get_subtraction_objects(self):
        out = []
        for row in range(self.subtractionsTable.topLevelItemCount()):
            item = self.subtractionsTable.topLevelItem(row)
            obj = self.doc.getObject(item.data(0, QtCore.Qt.UserRole))
            if obj:
                out.append(obj)
        return out

    def remove_selected_subtractions(self):
        selected = self.subtractionsTable.selectedItems()
        if not selected:
            return
        for item in selected:
            idx = self.subtractionsTable.indexOfTopLevelItem(item)
            if idx >= 0:
                self.subtractionsTable.takeTopLevelItem(idx)
        self._refresh_subtraction_status()
        self._refresh_recipe_summary()

    def clear_subtractions(self):
        self.subtractionsTable.clear()
        self._refresh_subtraction_status()
        self._refresh_recipe_summary()

    def select_subtraction_in_tree(self):
        selected = self.subtractionsTable.selectedItems()
        if not selected:
            return
        FreeCADGui.Selection.clearSelection()
        for item in selected:
            obj = self.doc.getObject(item.data(0, QtCore.Qt.UserRole))
            if obj:
                FreeCADGui.Selection.addSelection(obj)

    def zoom_to_subtraction(self):
        self.select_subtraction_in_tree()
        if FreeCAD.GuiUp:
            try:
                FreeCADGui.SendMsgToActiveView("ViewSelection")
            except Exception:
                pass

    def _on_subtraction_selection_changed(self):
        self.select_subtraction_in_tree()
        self._refresh_subtraction_status()

    def _refresh_subtraction_status(self):
        count = self.subtractionsTable.topLevelItemCount()
        if count == 0:
            self.subtractionsInfoLabel.setText(
                "No subtraction objects yet. Add cut-out shapes from selection to make holes or keep-out regions."
            )
            return
        ready = 0
        warnings = 0
        for row in range(count):
            item = self.subtractionsTable.topLevelItem(row)
            obj = self.doc.getObject(item.data(0, QtCore.Qt.UserRole))
            if not obj:
                item.setText(3, "[!] Missing")
                item.setForeground(3, QtGui.QBrush(QtGui.QColor("#aa5500")))
                warnings += 1
                continue
            shape_kind = self._shape_kind_for_object(obj)
            item.setText(2, shape_kind)
            status_text, status_color = self._subtraction_status_for_object(obj)
            item.setText(3, status_text)
            item.setForeground(3, QtGui.QBrush(status_color))
            if status_text.startswith("[ok]"):
                ready += 1
            else:
                warnings += 1
        self.subtractionsInfoLabel.setText(
            f"{count} subtraction object(s) loaded -- {ready} ready, {warnings} warning(s)."
        )

    def pick_base_tile(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            QtWidgets.QMessageBox.warning(
                self.form, "No selection", "Select an object in 3D view or tree."
            )
            return
        obj = selection[0]
        self._combo_set_object(self.tileCombo, obj)
        self._refresh_recipe_summary()

    def get_all_objects_classified(self):
        data = []
        if self.doc:
            for obj in self.doc.Objects:
                if not hasattr(obj, "Shape"):
                    continue
                category = self.classify_object(obj)
                data.append((obj.Name, category, obj))
        return data

    def classify_object(self, obj):
        if obj.isDerivedFrom("Sketcher::SketchObject"):
            return "Sketch"
        if obj.isDerivedFrom("Part::Compound"):
            return "Compound"
        if obj.isDerivedFrom("Part::Feature"):
            return "PartFeature"
        return "Other"

    def filter_objects(self, search_text, category):
        results = []
        low_search = search_text.lower().strip()
        for name, cat, obj in self.master_object_list:
            haystack = f"{name} {obj.Label}".lower()
            if category != "All" and cat != category:
                continue
            if low_search and low_search not in haystack:
                continue
            results.append((name, cat, obj))
        return results

    def _sync_selection_set_from_filtered_list(self, list_widget, backing_set):
        visible_names = set()
        visible_selected = set()
        for i in range(list_widget.count()):
            item = list_widget.item(i)
            name = self._get_list_item_object_name(item)
            visible_names.add(name)
            if item.isSelected():
                visible_selected.add(name)
        backing_set.difference_update(visible_names)
        backing_set.update(visible_selected)

    def _populate_filtered_multiselect_list(self, list_widget, filtered_list, selected_names):
        list_widget.clear()
        for _name, _cat, obj in filtered_list:
            item = self._make_object_list_item(obj)
            list_widget.addItem(item)
            if obj.Name in selected_names:
                item.setSelected(True)

    def _populate_named_list(self, list_widget, object_names):
        list_widget.clear()
        for name in object_names:
            obj = self.doc.getObject(name)
            if obj:
                list_widget.addItem(self._make_object_list_item(obj))

    def refresh_base_views(self):
        current = self.baseCombo.currentText()
        self._sync_selection_set_from_filtered_list(self.baseObjectsList, self.selected_base_object_names)
        filtered_list = self.filter_objects(
            self.baseSearchField.text(), self.baseTypeFilterCombo.currentText()
        )
        self.baseCombo.blockSignals(True)
        self.baseCombo.clear()
        for name, _cat, _obj in filtered_list:
            self.baseCombo.addItem(name)
        if current:
            idx = self.baseCombo.findText(current)
            if idx >= 0:
                self.baseCombo.setCurrentIndex(idx)
        self.baseCombo.blockSignals(False)
        self._populate_filtered_multiselect_list(
            self.baseObjectsList, filtered_list, self.selected_base_object_names
        )

    def refresh_selected_pattern_objects_list(self):
        ordered_names = [
            obj.Name for _name, _cat, obj in self.master_object_list if obj.Name in self.selected_pattern_object_names
        ]
        extras = [name for name in sorted(self.selected_pattern_object_names) if name not in ordered_names]
        ordered_names.extend(extras)
        self._populate_named_list(self.patternObjectsList, ordered_names)
        self.extraPatternInfoLabel.setText(
            f"{len(ordered_names)} extra pattern object(s) selected for fusion into the recipe."
            if ordered_names
            else "Use the filtered list to build a reusable multi-object pattern recipe."
        )

    def refresh_pattern_views(self):
        current = self.customPatternCombo.currentText()
        filtered_list = self.filter_objects(
            self.patternSearchField.text(), self.patternTypeFilterCombo.currentText()
        )
        self.customPatternCombo.blockSignals(True)
        self.customPatternCombo.clear()
        self.customPatternCombo.addItem("")
        for name, _category, _obj in filtered_list:
            self.customPatternCombo.addItem(name)
        if current:
            idx = self.customPatternCombo.findText(current)
            if idx >= 0:
                self.customPatternCombo.setCurrentIndex(idx)
        self.customPatternCombo.blockSignals(False)
        self.patternAvailableList.clear()
        for _name, _cat, obj in filtered_list:
            self.patternAvailableList.addItem(self._make_object_list_item(obj))
        self.refresh_selected_pattern_objects_list()

    def populate_tile_combo(self):
        current = self.tileCombo.currentText()
        self.tileCombo.clear()
        self.tileCombo.addItem("")
        for name, cat, obj in self.master_object_list:
            self.tileCombo.addItem(name)
        if current:
            idx = self.tileCombo.findText(current)
            if idx >= 0:
                self.tileCombo.setCurrentIndex(idx)

    def _set_list_selection_from_objects(self, list_widget, objects):
        wanted = {obj.Name for obj in (objects or []) if obj is not None}
        for i in range(list_widget.count()):
            item = list_widget.item(i)
            item.setSelected(self._get_list_item_object_name(item) in wanted)

    def get_selected_objects_from_list(self, list_widget):
        selected_items = list_widget.selectedItems()
        selected_objects = []
        for item in selected_items:
            obj_name = self._get_list_item_object_name(item)
            fc_obj = self.doc.getObject(obj_name)
            if fc_obj:
                selected_objects.append(fc_obj)
        return selected_objects

    def get_all_objects_from_list(self, list_widget):
        objects = []
        for i in range(list_widget.count()):
            item = list_widget.item(i)
            obj_name = self._get_list_item_object_name(item)
            fc_obj = self.doc.getObject(obj_name)
            if fc_obj:
                objects.append(fc_obj)
        return objects

    def _on_base_objects_selection_changed(self):
        self._sync_selection_set_from_filtered_list(self.baseObjectsList, self.selected_base_object_names)
        self._refresh_recipe_summary()

    def initial_pattern_setup(self):
        is_builtin = self.patternSourceCombo.currentIndex() == 0
        self.set_smart_default_scale(is_builtin)
        self.last_manual_scale = None
        self._update_pattern_controls_visibility(show_builtin=is_builtin)

    def on_pattern_source_changed(self, index):
        is_builtin = index == 0
        if self.last_manual_scale is None:
            self.set_smart_default_scale(is_builtin)
        else:
            self.scaleSpin.setStyleSheet("QDoubleSpinBox { background-color: #f0f0f0; }")
        self._update_pattern_controls_visibility(show_builtin=is_builtin)
        self._refresh_recipe_summary()
        self._update_tile_estimate()

    def _update_pattern_controls_visibility(self, show_builtin=True):
        self.builtinPatternLabel.setVisible(show_builtin)
        self.builtinPatternCombo.setVisible(show_builtin)
        show_custom = not show_builtin
        self.customPatternLabel.setVisible(show_custom)
        self.customPatternCombo.setVisible(show_custom)
        self.pickCustomBtn.setVisible(show_custom)
        self.patternSearchField.setVisible(show_custom)
        self.patternTypeFilterCombo.setVisible(show_custom)
        self.patternAvailableList.setVisible(show_custom)
        self.patternObjectsList.setVisible(show_custom)
        self.addPatternBtn.setVisible(show_custom)
        self.addCurrentPatternBtn.setVisible(show_custom)
        self.addPatternSelectionBtn.setVisible(show_custom)
        self.removePatternBtn.setVisible(show_custom)
        self.clearPatternBtn.setVisible(show_custom)
        self.extraPatternInfoLabel.setVisible(show_custom)

    def _refresh_distribution_ui(self):
        mode = self.distCombo.currentText()
        show_grid = mode in ("CenteredTiling", "RelativeSpacing", "LinearGrid")
        show_radial = mode == "RadialDistribution"
        show_concentric = mode == "ConcentricDistribution"
        show_random = mode == "RandomDistribution"

        self.gridModeGroup.setVisible(show_grid)
        self.radialModeGroup.setVisible(show_radial)
        self.concentricModeGroup.setVisible(show_concentric)
        self.randomModeGroup.setVisible(show_random)

        show_spacing = mode in (
            "CenteredTiling", "RelativeSpacing", "SeamlessTiling", "ConcentricDistribution"
        )
        self.spacingLabel.setVisible(show_spacing)

        spacing_widget = self.spacingInput if self.spacingInput is not None else self.spacingSpin
        if spacing_widget is not None:
            spacing_widget.setVisible(show_spacing)

        self.useUnitsCheck.setVisible(show_spacing)
        self.unitSystemCombo.setVisible(show_spacing)
        self.spacingUnitLabel.setVisible(show_spacing)

    def _refresh_units_ui(self):
        use_units = self.useUnitsCheck.isChecked()
        self.unitSystemCombo.setEnabled(use_units)
        selected = self.unitSystemCombo.currentText()
        if not use_units:
            suffix = "mm"
            hint = "Base spacing is interpreted directly in FreeCAD document length units (typically mm)."
        elif selected == "Metric (m)":
            suffix = "m"
            hint = "Base spacing value is stored in meters and converted to mm at generation time."
        elif selected == "Imperial (ft)":
            suffix = "ft"
            hint = "Base spacing value is stored in feet and converted to mm at generation time."
        elif selected == "BIM Workbench Unit":
            suffix = "BIM unit"
            hint = "Base spacing follows the BIM workbench unit conversion path."
        else:
            suffix = "doc unit"
            hint = "Base spacing follows FreeCAD's default unit interpretation."
        self.spacingUnitLabel.setText(f"Current spacing context: {suffix}. {hint}")

    def _refresh_recipe_summary(self):
        role = "Definition" if self.hatchRoleCombo.currentIndex() == 0 else "Applied"
        if self.patternSourceCombo.currentIndex() == 0:
            recipe_source = f"Built-in: {self.builtinPatternCombo.currentText()}"
        else:
            recipe_source = self.customPatternCombo.currentText().strip() or "Custom pattern not selected"
            recipe_source = f"Custom: {recipe_source}"

        if role == "Definition":
            target = "200 mm swatch preview"
        else:
            base_text = self.baseCombo.currentText().strip() or "No base selected"
            target = base_text

        projection = "Surface" if self.useSurfaceProjectionCheck.isChecked() else "XY"
        if self.forceXYPlaneCheck.isChecked():
            projection = "Forced XY"

        subtraction_count = self.subtractionsTable.topLevelItemCount()
        self._sync_selection_set_from_filtered_list(self.baseObjectsList, self.selected_base_object_names)
        extra_bases = len(self.selected_base_object_names)
        extra_patterns = self.patternObjectsList.count()
        summary = (
            f"{role} | Target: {target} | Recipe: {recipe_source} | Mode: {self.distCombo.currentText()} "
            f"| Projection: {projection} | Subtractions: {subtraction_count}"
        )
        if extra_bases:
            summary += f" | +{extra_bases} extra base"
        if extra_patterns:
            summary += f" | +{extra_patterns} extra pattern"
        self.recipeSummaryLabel.setText(summary)

    def set_smart_default_scale(self, is_builtin):
        default_scale = 100.0 if is_builtin else 10.0
        self.scaleSpin.setValue(default_scale)
        self.last_manual_scale = None
        self.scaleSpin.setStyleSheet("")
        QtCore.QTimer.singleShot(100, self._update_tile_estimate)

    def on_scale_changed(self, value):
        self.last_manual_scale = value
        self.scaleSpin.setStyleSheet("")
        self._refresh_recipe_summary()

    def on_base_combo_index_changed(self, index):
        name = self.baseCombo.itemText(index)
        self.highlight_object(name)

    def on_tile_combo_index_changed(self, index):
        name = self.tileCombo.itemText(index)
        if name:
            self.highlight_object(name)

    def on_custom_pattern_selected(self):
        name = self.customPatternCombo.currentText()
        if name:
            self.highlight_object(name)

    def highlight_object(self, obj_name):
        if not obj_name:
            return
        FreeCADGui.Selection.clearSelection()
        obj = self.doc.getObject(obj_name)
        if obj:
            FreeCADGui.Selection.addSelection(obj)

    def get_base_spacing_in_mm(self):
        if self.spacingInput is not None:
            try:
                qty = self.spacingInput.property("quantity")
                return qty.Value if qty else 0.0
            except Exception:
                return 0.0
        return float(self.spacingSpin.value()) if self.spacingSpin is not None else 0.0

    def on_preview(self):
        if not self._check_dangerous_tile_count():
            return

        doc = self.doc
        if not doc:
            return

        is_definition = self.hatchRoleCombo.currentIndex() == 0
        if not is_definition:
            base_name = self.baseCombo.currentText()
            base_obj = self._resolve_obj_name(base_name)
            if not base_obj or not hasattr(base_obj, "Shape"):
                QtWidgets.QMessageBox.warning(
                    self.form,
                    "Preview Error",
                    "Select a valid base shape before previewing.",
                )
                return

        if self.current_temp_preview_name:
            old = doc.getObject(self.current_temp_preview_name)
            if old:
                doc.removeObject(self.current_temp_preview_name)
            self.current_temp_preview_name = None

        temp_name = None
        preview_name = None

        try:
            temp_name = doc.getUniqueObjectName("_HatchPreviewTemp")
            temp_hatch = make_custom_hatch(
                name=temp_name, role="Definition" if is_definition else "Applied"
            )

            if not self._apply_properties_to(temp_hatch):
                doc.removeObject(temp_name)
                temp_name = None
                return

            temp_hatch.MaxTilesAllowed = min(int(self.maxTilesSpin.value()), 100)
            doc.recompute()

            if not temp_hatch.Shape or temp_hatch.Shape.isNull():
                doc.removeObject(temp_name)
                temp_name = None
                QtWidgets.QMessageBox.warning(
                    self.form,
                    "Preview",
                    "Preview generated an empty shape. Check base object and pattern settings.",
                )
                return

            show_faces = temp_hatch.ShowFaces
            preview_at_surface = self.previewAtLocationCheck.isChecked() or is_definition

            if preview_at_surface:
                preview_name = temp_name
                temp_name = None
                preview_obj = doc.getObject(preview_name)
            else:
                world_shape = _bake_shape_to_geometry(temp_hatch.Shape, temp_hatch.Placement)
                try:
                    c = world_shape.BoundBox.Center
                    mat = FreeCAD.Matrix()
                    mat.move(FreeCAD.Vector(-c.x, -c.y, -c.z))
                    world_shape.transformShape(mat)
                except Exception:
                    pass

                doc.removeObject(temp_name)
                temp_name = None
                preview_name = doc.getUniqueObjectName("HatchPreview")
                preview_obj = doc.addObject("Part::Feature", preview_name)
                preview_obj.Shape = world_shape
                preview_obj.Placement = FreeCAD.Placement()
                doc.recompute()

            if FreeCAD.GuiUp:
                vobj = preview_obj.ViewObject
                line_color = (1 / 255, 255 / 255, 1 / 255)
                shape_color = (0 / 255, 204 / 255, 51 / 255)
                safe_set_display_mode(vobj, "Flat Lines" if show_faces else "Wireframe")
                vobj.ShapeColor = shape_color
                try:
                    vobj.LineColor = line_color
                except Exception:
                    pass
                vobj.Transparency = 60
                vobj.LineWidth = 1.0

            keep = self.keepPreviewCheck.isChecked()
            if keep:
                preview_obj.Label = "HatchPreview_Kept"
                self.current_temp_preview_name = None
            else:
                self.current_temp_preview_name = preview_name

            num_edges = len(preview_obj.Shape.Edges)
            num_faces = len(preview_obj.Shape.Faces)
            self.previewStatusLabel.setText(
                "Preview ready | Edges: %d | Faces: %d" % (num_edges, num_faces)
            )
            QtWidgets.QMessageBox.information(
                self.form,
                "Preview Generated",
                "Preview complete (<=100 tiles for speed).\n"
                f"Edges: {num_edges}  Faces: {num_faces}\n\n"
                f"{'Shape kept as non-parametric feature.' if keep else 'Shape will be removed on next preview or on Create.'}\n\n"
                "Tip: use the Advanced section to tune Max Tiles, clipping, and variation.",
            )
        except Exception as e:
            FreeCAD.Console.PrintError(f"Preview failed: {e}\n")
            self.previewStatusLabel.setText(f"Preview failed: {e}")
            QtWidgets.QMessageBox.critical(self.form, "Preview Error", f"Preview failed:\n{str(e)}")
            for name in [temp_name, preview_name]:
                if name:
                    obj = doc.getObject(name)
                    if obj:
                        doc.removeObject(name)

    def _cleanup_preview(self):
        if self.current_temp_preview_name:
            obj = self.doc.getObject(self.current_temp_preview_name)
            if obj:
                self.doc.removeObject(self.current_temp_preview_name)
            self.current_temp_preview_name = None

    def _load_from_object(self, obj):
        try:
            role = getattr(obj, "HatchRole", "Applied")
            self.hatchRoleCombo.setCurrentIndex(0 if role == "Definition" else 1)

            if getattr(obj, "BaseObject", None):
                self._combo_set_object(self.baseCombo, obj.BaseObject)

            pattern_type = getattr(obj, "PatternType", "CustomObject")
            if pattern_type == "CustomObject":
                self.patternSourceCombo.setCurrentIndex(1)
                if getattr(obj, "PatternObject", None):
                    self._combo_set_object(self.customPatternCombo, obj.PatternObject)
            else:
                self.patternSourceCombo.setCurrentIndex(0)
                builtin_idx = self.builtinPatternCombo.findText(pattern_type)
                if builtin_idx >= 0:
                    self.builtinPatternCombo.setCurrentIndex(builtin_idx)

            dist_idx = self.distCombo.findText(getattr(obj, "DistributionMode", "SeamlessTiling"))
            if dist_idx >= 0:
                self.distCombo.setCurrentIndex(dist_idx)

            self.autoScaleCheck.setChecked(bool(getattr(obj, "AutoScaleToFitBase", False)))
            self.scaleSpin.setValue(float(getattr(obj, "PatternScale", 1.0)))
            self.rotSpin.setValue(float(getattr(obj, "RotationDeg", 0.0)))
            _spacing = float(getattr(obj, "BaseSpacing", 0.0))
            if self.spacingInput is not None:
                try:
                    self.spacingInput.setProperty(
                        "quantity", FreeCAD.Units.Quantity(_spacing, FreeCAD.Units.Length)
                    )
                except Exception:
                    pass
            elif self.spacingSpin is not None:
                self.spacingSpin.setValue(_spacing)
            self.useUnitsCheck.setChecked(bool(getattr(obj, "UseUnits", False)))
            unit_idx = self.unitSystemCombo.findText(
                getattr(obj, "SelectedUnitSystem", "FreeCAD Default")
            )
            if unit_idx >= 0:
                self.unitSystemCombo.setCurrentIndex(unit_idx)

            self.repXSpin.setValue(int(getattr(obj, "RepetitionsX", 5)))
            self.repYSpin.setValue(int(getattr(obj, "RepetitionsY", 5)))
            self.radialCountSpin.setValue(int(getattr(obj, "RadialCount", 8)))
            self.radialRadiusSpin.setValue(float(getattr(obj, "RadialRadius", 50.0)))
            self.concentricCountSpin.setValue(int(getattr(obj, "ConcentricCount", 5)))
            self.concentricSpacingSpin.setValue(float(getattr(obj, "ConcentricSpacing", 10.0)))
            self.randomCountSpin.setValue(int(getattr(obj, "RandomCount", 30)))

            scale_mode_idx = self.scaleModeCombo.findText(getattr(obj, "ScaleMode", "Absolute"))
            if scale_mode_idx >= 0:
                self.scaleModeCombo.setCurrentIndex(scale_mode_idx)
            self.lockCheck.setChecked(bool(getattr(obj, "LockToBase", False)))
            self.offsetXSpin.setValue(float(getattr(obj, "PatternOffsetX", 0.0)))
            self.offsetYSpin.setValue(float(getattr(obj, "PatternOffsetY", 0.0)))
            placement_mode_idx = self.placementModeCombo.findText(
                getattr(obj, "PatternPlacementMode", "Origin")
            )
            if placement_mode_idx >= 0:
                self.placementModeCombo.setCurrentIndex(placement_mode_idx)

            self.randomCheck.setChecked(bool(getattr(obj, "RandomizePlacement", False)))
            self.offRangeSpin.setValue(float(getattr(obj, "RandomOffsetRange", 0.0)))
            rot_range = float(getattr(obj, "RandomRotationRange", 0.0))
            if not rot_range:
                rot_min = float(getattr(obj, "RandomRotationMin", 0.0))
                rot_max = float(getattr(obj, "RandomRotationMax", 0.0))
                rot_range = max(abs(rot_min), abs(rot_max))
            self.rotRangeSpin.setValue(rot_range)
            self.scaleMinSpin.setValue(float(getattr(obj, "RandomScaleMin", 1.0)))
            self.scaleMaxSpin.setValue(float(getattr(obj, "RandomScaleMax", 1.0)))

            self.showFacesCheck.setChecked(bool(getattr(obj, "ShowFaces", False)))
            self.apply3DCheck.setChecked(bool(getattr(obj, "ApplyTo3DSurface", False)))
            self.maxTilesSpin.setValue(int(getattr(obj, "MaxTilesAllowed", 5000)))
            clip_mode_idx = self.clipModeCombo.findText(getattr(obj, "ClipMode", "BooleanOnly"))
            if clip_mode_idx >= 0:
                self.clipModeCombo.setCurrentIndex(clip_mode_idx)

            self.densitySpin.setValue(float(getattr(obj, "DensityFactor", 1.0)))
            self.enableColorVarCheck.setChecked(bool(getattr(obj, "EnableColorVariation", False)))
            self.colorVarSpin.setValue(float(getattr(obj, "ColorVariationIntensity", 0.5)))
            self.spacingVariationSpin.setValue(float(getattr(obj, "SpacingVariation", 0.0)))
            self.shapeDistortionCheck.setChecked(bool(getattr(obj, "EnableShapeDistortion", False)))

            self.useSurfaceProjectionCheck.setChecked(
                bool(getattr(obj, "UseSurfaceProjection", True))
            )
            self.forceXYPlaneCheck.setChecked(bool(getattr(obj, "ForceXYPlane", False)))

            if getattr(obj, "BaseTileObject", None):
                self._combo_set_object(self.tileCombo, obj.BaseTileObject)
            self.tileVisibilityCheck.setChecked(bool(getattr(obj, "TileVisibility", True)))

            self.selected_base_object_names = {
                extra.Name for extra in getattr(obj, "BaseObjects", []) if extra is not None
            }
            self.selected_pattern_object_names = {
                extra.Name for extra in getattr(obj, "PatternObjects", []) if extra is not None
            }
            self.refresh_base_views()
            self.refresh_pattern_views()
            self._set_subtraction_objects(getattr(obj, "Subtractions", []))

        except Exception as e:
            FreeCAD.Console.PrintWarning(f"_load_from_object partial failure: {e}\n")

    def accept(self):
        self._cleanup_preview()
        if not self._check_dangerous_tile_count():
            return True
        if self.editing_obj is not None:
            self._accept_edit()
        else:
            self._accept_create()
        return True

    def _apply_properties_to(self, hatch_obj):
        hatch_obj.HatchRole = "Definition" if self.hatchRoleCombo.currentIndex() == 0 else "Applied"

        if hatch_obj.HatchRole == "Applied":
            base_obj = self._resolve_obj_name(self.baseCombo.currentText())
            if not base_obj or not hasattr(base_obj, "Shape"):
                QtWidgets.QMessageBox.warning(self.form, "Error", "Invalid base object selection")
                return False
            hatch_obj.BaseObject = base_obj
        else:
            hatch_obj.BaseObject = None

        if self.patternSourceCombo.currentIndex() == 0:
            hatch_obj.PatternType = self.builtinPatternCombo.currentText()
            hatch_obj.PatternObject = None
        else:
            hatch_obj.PatternType = "CustomObject"
            pattern_name = self.customPatternCombo.currentText().strip()
            hatch_obj.PatternObject = self._resolve_obj_name(pattern_name) if pattern_name else None

        hatch_obj.ClipMode = self.clipModeCombo.currentText()
        hatch_obj.UseSurfaceProjection = self.useSurfaceProjectionCheck.isChecked()
        hatch_obj.ForceXYPlane = self.forceXYPlaneCheck.isChecked()
        self._sync_selection_set_from_filtered_list(self.baseObjectsList, self.selected_base_object_names)
        hatch_obj.BaseObjects = [
            self.doc.getObject(name) for name in sorted(self.selected_base_object_names) if self.doc.getObject(name)
        ]
        hatch_obj.PatternObjects = self.get_all_objects_from_list(self.patternObjectsList)
        hatch_obj.Subtractions = self.get_subtraction_objects()
        hatch_obj.DistributionMode = self.distCombo.currentText()
        hatch_obj.AutoScaleToFitBase = self.autoScaleCheck.isChecked()
        hatch_obj.PatternScale = float(self.scaleSpin.value())
        hatch_obj.RotationDeg = float(self.rotSpin.value())
        hatch_obj.BaseSpacing = float(self.get_base_spacing_in_mm())
        hatch_obj.UseUnits = self.useUnitsCheck.isChecked()
        hatch_obj.SelectedUnitSystem = self.unitSystemCombo.currentText()
        hatch_obj.RepetitionsX = int(self.repXSpin.value())
        hatch_obj.RepetitionsY = int(self.repYSpin.value())
        hatch_obj.RadialCount = int(self.radialCountSpin.value())
        hatch_obj.RadialRadius = float(self.radialRadiusSpin.value())
        hatch_obj.ConcentricCount = int(self.concentricCountSpin.value())
        hatch_obj.ConcentricSpacing = float(self.concentricSpacingSpin.value())
        hatch_obj.RandomCount = int(self.randomCountSpin.value())
        hatch_obj.RandomizePlacement = self.randomCheck.isChecked()
        hatch_obj.RandomOffsetRange = float(self.offRangeSpin.value())
        hatch_obj.RandomRotationRange = float(self.rotRangeSpin.value())
        hatch_obj.RandomRotationMin = -float(self.rotRangeSpin.value())
        hatch_obj.RandomRotationMax = float(self.rotRangeSpin.value())
        hatch_obj.RandomScaleMin = float(self.scaleMinSpin.value())
        hatch_obj.RandomScaleMax = float(self.scaleMaxSpin.value())
        hatch_obj.LockToBase = self.lockCheck.isChecked()
        hatch_obj.PatternOffsetX = float(self.offsetXSpin.value())
        hatch_obj.PatternOffsetY = float(self.offsetYSpin.value())
        hatch_obj.ScaleMode = self.scaleModeCombo.currentText()
        hatch_obj.ShowFaces = self.showFacesCheck.isChecked()
        hatch_obj.ApplyTo3DSurface = self.apply3DCheck.isChecked()
        hatch_obj.MaxTilesAllowed = int(self.maxTilesSpin.value())
        hatch_obj.DensityFactor = float(self.densitySpin.value())
        hatch_obj.EnableColorVariation = self.enableColorVarCheck.isChecked()
        hatch_obj.ColorVariationIntensity = float(self.colorVarSpin.value())
        hatch_obj.SpacingVariation = float(self.spacingVariationSpin.value())
        hatch_obj.EnableShapeDistortion = self.shapeDistortionCheck.isChecked()
        hatch_obj.PatternPlacementMode = self.placementModeCombo.currentText()
        tile_name = self.tileCombo.currentText().strip()
        hatch_obj.BaseTileObject = self._resolve_obj_name(tile_name) if tile_name else None
        hatch_obj.TileVisibility = self.tileVisibilityCheck.isChecked()
        return True

    def _accept_create(self):
        try:
            FreeCAD.ActiveDocument.openTransaction("Create Hatch")
            hatch_objects = [o for o in self.doc.Objects if o.Name.startswith("CustomHatch")]
            max_num = 0
            for obj in hatch_objects:
                suffix = obj.Name[len("CustomHatch"):]
                if suffix.isdigit():
                    max_num = max(max_num, int(suffix))
            hatch_name = "CustomHatch" if max_num == 0 else f"CustomHatch{max_num + 1:03d}"
            role = "Definition" if self.hatchRoleCombo.currentIndex() == 0 else "Applied"
            hatch_obj = make_custom_hatch(name=hatch_name, role=role)
            if not self._apply_properties_to(hatch_obj):
                self.doc.removeObject(hatch_obj.Name)
                FreeCAD.ActiveDocument.abortTransaction()
                FreeCADGui.Control.closeDialog()
                return
            self.doc.recompute()
            FreeCAD.ActiveDocument.commitTransaction()
            QtWidgets.QMessageBox.information(
                self.form,
                "Success",
                f"Hatch created!\nTime: {hatch_obj.GenerationTime:.2f}s\nTiles: {hatch_obj.TileCount}",
            )
        except Exception as e:
            FreeCAD.ActiveDocument.abortTransaction()
            QtWidgets.QMessageBox.critical(self.form, "Error", f"Hatch creation failed:\n{str(e)}")
        FreeCADGui.Control.closeDialog()

    def _accept_edit(self):
        try:
            FreeCAD.ActiveDocument.openTransaction("Edit Hatch")
            if not self._apply_properties_to(self.editing_obj):
                FreeCAD.ActiveDocument.abortTransaction()
                FreeCADGui.Control.closeDialog()
                return
            self.doc.recompute()
            FreeCAD.ActiveDocument.commitTransaction()
            QtWidgets.QMessageBox.information(
                self.form,
                "Updated",
                f"Hatch updated!\nTime: {self.editing_obj.GenerationTime:.2f}s\nTiles: {self.editing_obj.TileCount}",
            )
        except Exception as e:
            FreeCAD.ActiveDocument.abortTransaction()
            QtWidgets.QMessageBox.critical(self.form, "Error", f"Hatch update failed:\n{str(e)}")
        FreeCADGui.Control.closeDialog()

    def reject(self):
        self._cleanup_preview()
        FreeCADGui.Control.closeDialog()
        return True


class _CommandFaceExtractor:
    def GetResources(self):
        return {
            "Pixmap": "Draft_Facebinder",
            "MenuText": "Extract Face",
            "ToolTip": "Select a face on any 3D object, then run this command to create a FaceExtractor object.",
        }

    def IsActive(self):
        if not FreeCAD.ActiveDocument:
            return False
        if not FreeCAD.GuiUp:
            return False
        selection = FreeCADGui.Selection.getSelectionEx()
        return any(sub.startswith("Face") for sel in selection for sub in sel.SubElementNames)

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Extract Face")
        try:
            created = make_face_extractor_from_selection()
            if created:
                FreeCAD.ActiveDocument.commitTransaction()
            else:
                FreeCAD.ActiveDocument.abortTransaction()
        except Exception as e:
            FreeCAD.ActiveDocument.abortTransaction()
            FreeCAD.Console.PrintError(f"FaceExtractor failed: {e}\n")


class _CommandHatch:
    def GetResources(self):
        return {
            "Pixmap": _icon_path("BIM_Hatch.svg"),
            "MenuText": "Create Hatch",
            "ToolTip": "Generate parametric hatch patterns on surfaces",
        }

    def IsActive(self):
        return FreeCAD.ActiveDocument is not None

    def Activated(self):
        panel = HatchTaskPanel()
        FreeCADGui.Control.showDialog(panel)


if FreeCAD.GuiUp:
    if "BIM_Hatch_Dialog" not in FreeCADGui.listCommands():
        FreeCADGui.addCommand("BIM_Hatch_Dialog", _CommandHatch())
    if "BIM_FaceExtractor" not in FreeCADGui.listCommands():
        FreeCADGui.addCommand("BIM_FaceExtractor", _CommandFaceExtractor())


def run_as_macro():
    mw = FreeCADGui.getMainWindow()
    dialog = QtWidgets.QDialog(mw)
    dialog.setWindowTitle("Custom Hatch Generator (Macro Mode)")
    dialog.resize(450, 700)

    layout = QtWidgets.QVBoxLayout(dialog)

    scroll = QtWidgets.QScrollArea()
    scroll.setWidgetResizable(True)
    scroll.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
    scroll.setFrameShape(QtWidgets.QFrame.NoFrame)

    task_panel = HatchTaskPanel()
    widget = task_panel.form

    scroll.setWidget(widget)
    layout.addWidget(scroll)

    button_box = QtWidgets.QDialogButtonBox(
        QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel
    )
    button_box.accepted.connect(lambda: (task_panel.accept(), dialog.accept()))
    button_box.rejected.connect(lambda: (task_panel.reject(), dialog.reject()))
    layout.addWidget(button_box)

    dialog.exec_()


def run_hatch_generator_dialog():
    if FreeCAD.GuiUp:
        FreeCADGui.runCommand("BIM_Hatch_Dialog")


if __name__ == "__main__":
    if FreeCAD.GuiUp:
        run_as_macro()
