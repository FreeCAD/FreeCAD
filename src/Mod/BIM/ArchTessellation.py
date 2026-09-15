# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD BIM workbench.
# You can find the full license text in the LICENSE file in the root directory.

import FreeCAD
import Part
from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum, auto

# Minimum tile or scale dimension in mm.
MIN_DIMENSION = 0.1
TILE_MIN = 1.0
# Minimum joint width in mm. Below this, boolean operations become numerically unstable.
JOINT_THRESHOLD = 0.1
# Minimum thickness in mm to treat geometry as a solid rather than a flat face.
MIN_EXTRUSION_THICKNESS = 0.001
TOO_MANY_TILES = 10000
GRID_LIMIT = 100000
# Fraction of the theoretical tile area/volume at or above which a tile is counted as full.
FULL_TILE_THRESHOLD = 0.995
# Z offset in mm applied to flat geometry to prevent Z-fighting in the viewport.
VIEWPORT_Z_OFFSET = 0.05
# Extra tile rows/columns added beyond the face diagonal to ensure rotated grids fully cover the face.
ROTATION_COVERAGE_BUFFER = 4


def resolve_stagger(stagger_type, stagger_custom, length, joint):
    """Return ``(cycle, offset_u)`` for the given stagger settings.

    Parameters
    ----------
    stagger_type : str
        One of the ``StaggerType`` enumeration strings.
    stagger_custom : float
        Custom offset in mm, used only when ``stagger_type == "Custom"``.
    length : float
        Tile length in mm (dimension along the U axis).
    joint : float
        Joint width in mm.

    Returns
    -------
    tuple[int, float]
        ``cycle`` — number of rows before the pattern repeats.
        ``offset_u`` — row shift in mm. For preset bonds this equals an exact fraction of
        ``length``; for Custom bonds it equals ``stagger_custom``.
    """
    _presets = {
        "Half Bond (1/2)": (2, length / 2.0),
        "Third Bond (1/3)": (3, length / 3.0),
        "Quarter Bond (1/4)": (4, length / 4.0),
    }
    if stagger_type in _presets:
        return _presets[stagger_type]

    if stagger_type == "Custom":
        period_u = length + joint
        if period_u > 0:
            shift_frac = (stagger_custom / period_u) % 1.0
            if 1e-4 < shift_frac < 1 - 1e-4:
                for n in range(2, 9):
                    if abs((n * shift_frac) - round(n * shift_frac)) < 1e-3:
                        return (n, stagger_custom)
                return (8, stagger_custom)

    return (1, 0.0)


class TessellationStatus(Enum):
    OK = auto()
    JOINT_TOO_SMALL = auto()
    COUNT_TOO_HIGH = auto()
    EXTREME_COUNT = auto()
    INVALID_DIMENSIONS = auto()


class TessellationQuantities:
    """Container for calculated BIM quantities."""

    def __init__(
        self,
        count_full=0,
        count_partial=0,
        area_net=0.0,
        area_gross=0.0,
        length_joints=0.0,
        waste_area=0.0,
        length_perimeter=0.0,
        area_outer=0.0,
        areas_holes=None,
    ):
        self.count_full = count_full
        self.count_partial = count_partial
        self.area_net = area_net
        self.area_gross = area_gross
        self.length_joints = length_joints
        self.waste_area = waste_area
        self.length_perimeter = length_perimeter
        self.area_outer = area_outer
        self.areas_holes = areas_holes if areas_holes is not None else []


class TessellationResult:
    """Container for tessellator output and metadata."""

    def __init__(
        self,
        geometry=None,
        placement=None,
        centerlines=None,
        quantities=None,
        unit_area=0.0,
        unit_volume=0.0,
        status=TessellationStatus.OK,
    ):
        self.geometry = geometry
        self.placement = placement or FreeCAD.Placement()
        self.centerlines = centerlines
        self.quantities = quantities if quantities else TessellationQuantities()
        self.unit_area = unit_area
        self.unit_volume = unit_volume
        self.status = status


class TileConfig:
    """
    Configuration for rectangular tile tessellation modes (Solid Tiles, Parametric Pattern,
    Monolithic).

    Holds all parameters for a rectangular tile layout. Validation and stagger-offset resolution are
    applied at construction time.

    Parameters
    ----------
    finish_mode : str
        One of ``"Solid Tiles"``, ``"Parametric Pattern"``, or ``"Monolithic"``.
    length, width, thickness, joint : float
        Tile dimensions in mm.
    stagger_type : str
        Running-bond preset name. Resolved to ``offset_u`` automatically.
    stagger_custom : float
        Custom stagger offset in mm, used only when ``stagger_type == "Custom"``.
    """

    def __init__(
        self,
        finish_mode,
        length,
        width,
        thickness,
        joint,
        stagger_type="Stacked (None)",
        stagger_custom=0.0,
    ):
        self.finish_mode = finish_mode
        self.length = length
        self.width = width
        self.thickness = thickness
        self.joint = joint
        self.stagger_type = stagger_type
        self.stagger_custom = stagger_custom

        # Derived flags
        self.extrude = finish_mode in ("Solid Tiles", "Monolithic")
        self.monolithic = finish_mode == "Monolithic"

        self.cycle, self.offset_u = resolve_stagger(stagger_type, stagger_custom, length, joint)


class HatchConfig:
    """
    Configuration for hatch-pattern tessellation.

    Parameters
    ----------
    filename : str
        Path to the ``.pat`` file.
    pattern_name : str
        Name of the pattern within the file.
    scale : float
        Pattern scale factor.
    thickness : float
        Substrate thickness in mm (for the backing solid).
    """

    def __init__(self, filename, pattern_name, scale=1.0, thickness=0.0):
        self.filename = filename
        self.pattern_name = pattern_name
        self.scale = max(MIN_DIMENSION, scale)
        self.thickness = thickness


@dataclass(frozen=True)
class _GridParams:
    """Internal parameter block computed by ``_calculate_grid_params``."""

    step_u: float
    step_v: float
    count_u: int
    count_v: int
    total_count: int
    unit_area: float
    unit_volume: float
    status: TessellationStatus
    full_len_u: float
    full_len_v: float
    start_u: float
    start_v: float


def _to_local_space(substrate, origin, u_vec, normal):
    """
    Builds a local coordinate frame from the substrate's origin and orientation, then returns the
    substrate transformed into that frame along with the placement that maps it back to world space.

    Parameters
    ----------
    substrate : Part.Face
        The substrate in world space.
    origin : FreeCAD.Vector
        World-space grid anchor (becomes the local origin).
    u_vec : FreeCAD.Vector
        World-space U tangent of the substrate (need not be normalised).
    normal : FreeCAD.Vector
        World-space face normal (need not be normalised).

    Returns
    -------
    local_substrate : Part.Shape
        A copy of ``substrate`` transformed into local space (origin at ``origin``, local Z aligned
        with ``normal``).
    placement : FreeCAD.Placement
        The world placement that maps local space back to world space. Assign to ``obj.Placement``
        to position the finished geometry correctly in the scene.
    """
    n_vec = FreeCAD.Vector(normal).normalize()
    u_vec = FreeCAD.Vector(u_vec).normalize()
    # Re-derive v and u from n to guarantee a strict right-handed frame (U × V = N).
    # getFaceFrame already orthonormalises its output, but floating-point drift or a
    # caller passing a non-frame u_vec could leave the basis slightly non-orthogonal.
    v_vec = n_vec.cross(u_vec).normalize()

    placement = FreeCAD.Placement(origin, FreeCAD.Rotation(u_vec, v_vec, n_vec))
    to_local = placement.inverse().toMatrix()

    local_substrate = substrate.copy()
    local_substrate.transformShape(to_local)
    return local_substrate, placement


class Tessellator(ABC):
    """Abstract base class for pattern tessellators."""

    def compute(self, substrate, origin, u_vec, normal):
        """
        Generates the tessellated geometry for the given substrate.

        Transforms the substrate into local space, delegates to ``_compute_local``, then
        stamps the world placement onto the result before returning it.

        Parameters
        ----------
        substrate : Part.Face
            The face to be tiled.
        origin : FreeCAD.Vector
            The 3D starting point of the tiling grid.
        u_vec : FreeCAD.Vector
            Vector defining the local U (Length) direction.
        normal : FreeCAD.Vector
            Vector defining the extrusion direction (Thickness).

        Returns
        -------
        TessellationResult
            The resulting geometry and associated metadata.
        """
        area_outer, areas_holes = self._compute_face_areas(substrate)
        substrate, placement = _to_local_space(substrate, origin, u_vec, normal)
        result = self._compute_local(substrate)
        result.placement = placement
        result.quantities.area_outer = area_outer
        result.quantities.areas_holes = areas_holes
        return result

    @staticmethod
    def _compute_face_areas(face):
        """Return the outer-wire area and the sorted list of hole areas for ``face``.

        The outer area is the area enclosed by the outer wire alone, ignoring any holes.
        Hole areas are sorted in descending order so the largest hole is reported first.
        """
        outer_area = Part.Face(face.OuterWire).Area
        outer_hash = face.OuterWire.hashCode()
        hole_areas = sorted(
            (Part.Face(w).Area for w in face.Wires if w.hashCode() != outer_hash),
            reverse=True,
        )
        return outer_area, hole_areas

    def _overlay_lines(self, substrate, lines, thickness):
        """
        Combines a substrate with an overlay of line geometry (joint centerlines or hatch lines).

        If ``thickness`` is positive, the substrate is extruded into a solid and the lines are
        placed just above its top face. Otherwise the lines float just above the flat substrate.
        In both cases a small Z offset is added to prevent Z-fighting in the viewport.

        Parameters
        ----------
        substrate : Part.Face
            The base face in local space.
        lines : Part.Shape
            The line geometry to place on top.
        thickness : float
            Extrusion depth in mm. Pass 0 for a flat (2D) result.

        Returns
        -------
        Part.Compound
        """
        local_normal = FreeCAD.Vector(0, 0, 1)
        if thickness > 0:
            body = substrate.extrude(local_normal * thickness)
            lines.translate(local_normal * (thickness + VIEWPORT_Z_OFFSET))
            return Part.makeCompound([body, lines])
        lines.translate(local_normal * VIEWPORT_Z_OFFSET)
        return Part.makeCompound([substrate, lines])

    @abstractmethod
    def _compute_local(self, substrate):
        """
        Generates geometry for a substrate that is already in local space.

        Called by ``compute`` after the coordinate-frame transformation. Subclasses must
        return a ``TessellationResult`` with ``placement`` left at its default; the base
        class overwrites it with the correct world placement.
        """
        pass


class RectangularTessellator(Tessellator):
    """Generates standard grid or running bond patterns from a :class:`TileConfig`."""

    def __init__(self, config):
        self.length = config.length
        self.width = config.width
        self.thickness = config.thickness
        self.joint = config.joint
        self.offset_u = config.offset_u
        self.offset_v = 0.0  # V-axis stagger is not exposed in the UI
        self.cycle = config.cycle
        self.extrude = config.extrude
        self.monolithic = config.monolithic

    def _extrude_or_offset(self, substrate):
        """
        Returns a solid if the thickness is large enough to extrude, otherwise returns a copy of
        the substrate shifted slightly along Z to prevent Z-fighting in the viewport.
        """
        if self.extrude and self.thickness > MIN_EXTRUSION_THICKNESS:
            return substrate.extrude(FreeCAD.Vector(0, 0, self.thickness))
        offset_matrix = FreeCAD.Matrix()
        offset_matrix.move(FreeCAD.Vector(0, 0, VIEWPORT_Z_OFFSET))
        return substrate.transformGeometry(offset_matrix)

    def _compute_local(self, substrate):
        """
        Generates the tile geometry for a substrate already in local space.

        Chooses between two strategies based on tile count and dimensions:

        1. **Physical mode (standard):** Generates individual tile solids and clips them to the
           substrate boundary. Used when the tile count and joint width are within safe limits.

        2. **Fallback mode:** Returns the substrate as a single solid with tile counts derived
           from area. Used when joints are too narrow (< 0.1 mm) for stable boolean operations,
           or when the tile count exceeds the performance threshold (> 10,000 tiles).

        Parameters
        ----------
        substrate : Part.Face
            The source face to be tiled, already transformed into local space.

        Returns
        -------
        TessellationResult
            A container holding the geometry, BIM quantities, and tessellation status.
            ``placement`` is left at its default and is set by the base ``compute`` method.
        """
        # Monolithic mode: return the substrate as a single solid without individual tiles.
        if self.monolithic:
            quantities = TessellationQuantities()
            quantities.area_net = substrate.Area
            quantities.length_perimeter = substrate.Length
            quantities.count_full = 1
            quantities.area_gross = quantities.area_net

            final_geo = self._extrude_or_offset(substrate)

            return TessellationResult(
                geometry=final_geo,
                quantities=quantities,
                unit_area=quantities.area_net,
                unit_volume=quantities.area_net * self.thickness,
                status=TessellationStatus.OK,
            )

        # Check for non-physical dimensions that would cause math errors.
        if self.length < TILE_MIN or self.width < TILE_MIN:
            return TessellationResult(status=TessellationStatus.INVALID_DIMENSIONS)

        # Compute step sizes, tile counts, and the performance status
        # (e.g., OK, EXTREME_COUNT, JOINT_TOO_SMALL).
        params = self._calculate_grid_params(substrate)

        # Generate the centerline wires for visualization.
        # If status is EXTREME_COUNT, this returns empty lists to save memory.
        h_edges, v_edges, centerlines = self._generate_visual_grid(params, substrate.BoundBox)

        # Calculate net area, perimeter, and joint length.
        quantities = self._calculate_linear_quantities(substrate, params, h_edges, v_edges)

        if params.status != TessellationStatus.OK:
            final_geo, full_count, partial_count = self._generate_analytical_geo(
                substrate, params, centerlines
            )
        else:
            final_geo, full_count, partial_count = self._generate_physical_geo(substrate, params)

        quantities.count_full = full_count
        quantities.count_partial = partial_count
        quantities.area_gross = (full_count + partial_count) * params.unit_area
        quantities.waste_area = max(0.0, quantities.area_gross - quantities.area_net)

        return TessellationResult(
            geometry=final_geo,
            centerlines=centerlines,
            quantities=quantities,
            unit_area=params.unit_area,
            unit_volume=params.unit_volume,
            status=params.status,
        )

    def _calculate_grid_params(self, substrate):
        """
        Calculates grid dimensions, step sizes, and performance status.

        Parameters
        ----------
        substrate : Part.Face
            The face to be tiled.

        Returns
        -------
        _GridParams
            Calculated grid parameters, steps, counts, unit dimensions, and tessellation status.
        """
        step_u = self.length + self.joint
        step_v = self.width + self.joint
        bbox = substrate.BoundBox
        diag = bbox.DiagonalLength

        # Add buffer to counts to ensure coverage of rotated faces
        count_u = int(diag / step_u) + ROTATION_COVERAGE_BUFFER
        count_v = int(diag / step_v) + ROTATION_COVERAGE_BUFFER
        total_count = count_u * count_v

        unit_area = float(self.length * self.width)
        unit_volume = float(unit_area * self.thickness)

        status = TessellationStatus.OK
        if total_count > GRID_LIMIT:
            status = TessellationStatus.EXTREME_COUNT
        elif self.joint < JOINT_THRESHOLD:
            status = TessellationStatus.JOINT_TOO_SMALL
        elif total_count > TOO_MANY_TILES:
            status = TessellationStatus.COUNT_TOO_HIGH

        return _GridParams(
            step_u=step_u,
            step_v=step_v,
            count_u=count_u,
            count_v=count_v,
            total_count=total_count,
            unit_area=unit_area,
            unit_volume=unit_volume,
            status=status,
            full_len_u=2 * count_u * step_u,
            full_len_v=2 * count_v * step_v,
            start_u=-count_u * step_u,
            start_v=-count_v * step_v,
        )

    def _make_h_edges(self, count, step, tile_width, span_start, span_len, bbox):
        """
        Generates horizontal joint centerlines (constant Y, spanning along X).

        Parameters
        ----------
        count : int
            Half the number of lines (range is ``-count`` to ``count``).
        step : float
            Distance between lines (tile width + joint width).
        tile_width : float
            Tile dimension in the V direction, used to position the centerline.
        span_start : float
            X coordinate where each line begins.
        span_len : float
            Total length of each line along X.
        bbox : FreeCAD.BoundBox
            Local bounding box of the substrate, used to skip lines outside the face.

        Returns
        -------
        list of Part.Edge
        """
        edges = []
        for i in range(-count, count):
            pos = i * step + tile_width + (self.joint / 2)
            line_start = FreeCAD.Vector(span_start, pos, 0)
            line_end = FreeCAD.Vector(span_start + span_len, pos, 0)
            edge_bb = FreeCAD.BoundBox()
            edge_bb.add(line_start)
            edge_bb.add(line_end)
            if bbox.intersect(edge_bb):
                edges.append(Part.makeLine(line_start, line_end))
        return edges

    def _make_v_edges(self, count, step, tile_length, span_start, span_len, bbox):
        """
        Generates vertical joint centerlines (constant X, spanning along Y).

        Parameters
        ----------
        count : int
            Half the number of lines (range is ``-count`` to ``count``).
        step : float
            Distance between lines (tile length + joint width).
        tile_length : float
            Tile dimension in the U direction, used to position the centerline.
        span_start : float
            Y coordinate where each line begins.
        span_len : float
            Total length of each line along Y.
        bbox : FreeCAD.BoundBox
            Local bounding box of the substrate, used to skip lines outside the face.

        Returns
        -------
        list of Part.Edge
        """
        edges = []
        for i in range(-count, count):
            pos = i * step + tile_length + (self.joint / 2)
            line_start = FreeCAD.Vector(pos, span_start, 0)
            line_end = FreeCAD.Vector(pos, span_start + span_len, 0)
            edge_bb = FreeCAD.BoundBox()
            edge_bb.add(line_start)
            edge_bb.add(line_end)
            if bbox.intersect(edge_bb):
                edges.append(Part.makeLine(line_start, line_end))
        return edges

    def _generate_visual_grid(self, params, bbox):
        """
        Generates the centerline compound for grid visualization.

        The substrate is already in local space (origin at (0,0,0), axes aligned), so the
        orientation vectors are constants. The grid rotation is encoded into the compound's
        placement so the lines render in the correct direction.

        Parameters
        ----------
        params : _GridParams
            Grid parameters calculated by ``_calculate_grid_params``.
        bbox : FreeCAD.BoundBox
            The local bounding box of the substrate.

        Returns
        -------
        tuple
            (h_edges_list, v_edges_list, compound_shape)
        """
        h_edges, v_edges = [], []
        centerlines = None

        if params.status != TessellationStatus.EXTREME_COUNT:
            h_edges = self._make_h_edges(
                params.count_v, params.step_v, self.width, params.start_u, params.full_len_u, bbox
            )
            v_edges = self._make_v_edges(
                params.count_u, params.step_u, self.length, params.start_v, params.full_len_v, bbox
            )

            centerlines = Part.Compound(h_edges + v_edges)

        return h_edges, v_edges, centerlines

    def _calculate_linear_quantities(self, substrate, params, h_edges, v_edges):
        """
        Calculates areas, perimeter, and joint lengths.

        Parameters
        ----------
        substrate : Part.Face
            The face being tiled.
        params : _GridParams
            Grid parameters.
        h_edges, v_edges : list
            Lists of grid edges in local coordinates.

        Returns
        -------
        TessellationQuantities
            The populated quantities object.
        """
        quantities = TessellationQuantities()
        quantities.area_net = substrate.Area
        quantities.length_perimeter = substrate.Length

        # Joint length is measured along centerlines, not face widths. For a joint of width W,
        # the centerline sits W/2 inside each tile edge. This accurately represents the total run
        # of jointing material when joints are narrow relative to tile size (the common case),
        # but slightly underestimates it when a very wide joint straddles the substrate boundary —
        # the clipped centerline is shorter than the clipped joint face would be.
        #
        # Both substrate and the edge compounds are in the same local coordinate frame
        # (origin at grid anchor, Z=0), so no Placement adjustment is needed before common().
        if h_edges or v_edges:
            try:
                # Use a hybrid approach to calculate joint length. Intersect horizontal and vertical
                # joint compounds separately to avoid geometric kernel instability with sparse,
                # non-parallel compounds.
                total_length = 0
                if h_edges:
                    h_compound = Part.Compound(h_edges)
                    clipped_h = substrate.common(h_compound)
                    total_length += clipped_h.Length
                if v_edges:
                    v_compound = Part.Compound(v_edges)
                    clipped_v = substrate.common(v_compound)
                    total_length += clipped_v.Length
                quantities.length_joints = total_length
            except Exception:
                # Joint length measurement is non-critical: a boolean failure here leaves
                # length_joints at its default (0.0) without aborting the tessellation.
                pass
        elif params.status == TessellationStatus.EXTREME_COUNT:
            # For mosaic-like tile counts, estimate joint length statistically rather than
            # geometrically: sum the number of grid lines in each direction times face area,
            # then subtract the perimeter to avoid double-counting border segments.
            estimated_joint_length = (substrate.Area / params.step_u) + (
                substrate.Area / params.step_v
            )
            quantities.length_joints = max(
                0.0, estimated_joint_length - quantities.length_perimeter
            )

        return quantities

    def _generate_analytical_geo(self, substrate, params, centerlines):
        """
        Returns the substrate as a single solid (or face) and derives tile counts from area.

        Used as a fallback when joints are too narrow for stable boolean operations (< 0.1 mm)
        or when the tile count exceeds the performance threshold (> 10,000 tiles).

        Parameters
        ----------
        substrate : Part.Face
            The base face.
        params : _GridParams
            Grid parameters.
        centerlines : Part.Compound
            The visual grid lines.

        Returns
        -------
        tuple of (Part.Shape, int, int)
            The geometry, full tile count, and partial tile count.
        """
        count_rounded = round(substrate.Area / params.unit_area, 6)
        count_full = int(count_rounded)
        count_partial = 1 if (count_rounded - count_full) > 0 else 0

        solid_body = self._extrude_or_offset(substrate)
        final_geo = solid_body
        if centerlines:
            final_geo = Part.Compound([solid_body, centerlines])

        return final_geo, count_full, count_partial

    def _generate_physical_geo(self, substrate, params):
        """
        Generates the discrete tile geometry by intersecting a grid of tiles with the substrate.

        Builds a grid of individual tile primitives and intersects them with the substrate to
        produce a compound of separate solids (3D) or faces (2D), with visible joint gaps between
        tiles. Full and partial tiles are identified by comparing each fragment's volume or area
        against the theoretical unit size.

        Both the substrate and the tile grid remain in the same local coordinate frame throughout,
        so no placement adjustment is needed before the boolean intersection.

        Parameters
        ----------
        substrate : Part.Face
            The base face defining the boundary of the covering, already in local space.
        params : _GridParams
            Grid parameters including tile dimensions and counts.

        Returns
        -------
        tuple of (Part.Shape, int, int)
            The geometry, full tile count, and partial tile count.
        """
        tiles = []

        # Build the grid of tiles at local origin.
        # Tile positions are expressed in the same local frame as the substrate:
        # U along X, V along Y, extrusion along Z.
        for j in range(-params.count_v, params.count_v):
            row_idx = j % self.cycle
            stagger_u = row_idx * self.offset_u
            stagger_v = row_idx * self.offset_v

            v_pos = j * params.step_v + stagger_v

            for i in range(-params.count_u, params.count_u):
                u_pos = i * params.step_u + stagger_u

                if self.extrude and self.thickness > MIN_EXTRUSION_THICKNESS:
                    tile = Part.makeBox(
                        self.length, self.width, self.thickness, FreeCAD.Vector(u_pos, v_pos, 0)
                    )
                else:
                    tile = Part.makePlane(self.length, self.width, FreeCAD.Vector(u_pos, v_pos, 0))
                tiles.append(tile)

        tile_grid = Part.makeCompound(tiles)

        local_normal = FreeCAD.Vector(0, 0, 1)
        full_count, partial_count = 0, 0

        if self.extrude and self.thickness > MIN_EXTRUSION_THICKNESS:
            substrate_vol = substrate.extrude(local_normal * self.thickness)
            final_geo = substrate_vol.common(tile_grid)

            if final_geo.Solids:
                for solid in final_geo.Solids:
                    if solid.Volume >= (params.unit_volume * FULL_TILE_THRESHOLD):
                        full_count += 1
                    else:
                        partial_count += 1
        else:
            final_geo = substrate.common(tile_grid)

            if final_geo.Faces:
                for face in final_geo.Faces:
                    if face.Area >= (params.unit_area * FULL_TILE_THRESHOLD):
                        full_count += 1
                    else:
                        partial_count += 1

            # Convert the resulting faces to wires for 2D visual representation.
            wires = [wire for f in final_geo.Faces for wire in f.Wires]
            if wires:
                final_geo = self._overlay_lines(substrate, Part.makeCompound(wires), self.thickness)
            else:
                final_geo = Part.Shape()

        return final_geo, full_count, partial_count


class HatchTessellator(Tessellator):
    """Generates hatch patterns using the Draft Hatch engine from a :class:`HatchConfig`."""

    def __init__(self, config):
        self.filename = config.filename
        self.pattern_name = config.pattern_name
        self.scale = config.scale  # already clamped in HatchConfig
        self.thickness = config.thickness

    def _resolve_pattern_name(self):
        """
        Return the pattern name to use, auto-detecting from the file if the configured name is
        empty.

        The resolved name is returned as a local value and never written back to ``self``, so
        tessellators are safe to cache or reuse.
        """
        import Arch

        return self.pattern_name or Arch.first_pat_pattern_name(self.filename)

    def _compute_local(self, substrate):
        """
        Generates the hatch pattern geometry for a substrate already in local space.

        Parameters
        ----------
        substrate : Part.Face
            The face to be hatched, already transformed into local space.

        Returns
        -------
        TessellationResult
            The resulting geometry. ``placement`` is left at its default and is set by the base
            ``compute`` method.
        """
        import TechDraw

        # Default result: the untouched substrate. Overwritten below if a hatch pattern is
        # successfully generated or if thickness requires an extrusion.
        final_local_geo = substrate

        # Resolve the pattern name without mutating self.
        pattern_name = self._resolve_pattern_name()

        # Only proceed if we have both a file and a valid name
        if self.filename and pattern_name:
            pat_shape = None
            try:
                param_grp = FreeCAD.ParamGet(
                    "User parameter:BaseApp/Preferences/Mod/TechDraw/debug"
                )
                old_allow = (
                    param_grp.GetBool("allowCrazyEdge")
                    if "allowCrazyEdge" in param_grp.GetBools()
                    else None
                )
                param_grp.SetBool("allowCrazyEdge", True)

                try:
                    pat_shape = TechDraw.makeGeomHatch(
                        substrate, float(self.scale), str(pattern_name), str(self.filename)
                    )
                except Exception as e:
                    FreeCAD.Console.PrintWarning(
                        f"ArchTessellation: Hatch generation failed: {e}\n"
                    )

                # Restore preferences
                if old_allow is None:
                    param_grp.RemBool("allowCrazyEdge")
                else:
                    param_grp.SetBool("allowCrazyEdge", old_allow)

                hatch_succeeded = bool(pat_shape and pat_shape.Edges)

                if hatch_succeeded:
                    final_local_geo = self._overlay_lines(substrate, pat_shape, self.thickness)
                elif self.thickness > 0:
                    final_local_geo = substrate.extrude(FreeCAD.Vector(0, 0, self.thickness))

            except Exception as e:
                FreeCAD.Console.PrintError(f"ArchTessellation error: {e}\n")

        quantities = TessellationQuantities(
            count_full=1,
            area_net=substrate.Area,
            area_gross=substrate.Area,
            length_perimeter=substrate.Length,
        )
        return TessellationResult(geometry=final_local_geo, quantities=quantities)


def create_tessellator(config):
    """
    Instantiate the correct :class:`Tessellator` subclass for the given typed config.

    Parameters
    ----------
    config : TileConfig | HatchConfig
        Fully-specified tessellation configuration.

    Returns
    -------
    Tessellator
        A ready-to-call tessellator instance.
    """
    if isinstance(config, HatchConfig):
        return HatchTessellator(config)
    if isinstance(config, TileConfig):
        return RectangularTessellator(config)
    raise TypeError(f"create_tessellator: unknown config type {type(config).__name__!r}")
