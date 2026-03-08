# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
# You can find the full license text in the LICENSE file in the root directory.

import math
import FreeCAD
import Part
from enum import Enum, auto
from abc import ABC, abstractmethod


# Canonical translation shims for headless/static analysis
def translate(context, sourceText, disambiguation=None, n=-1):
    return sourceText


def QT_TRANSLATE_NOOP(context, sourceText):
    return sourceText


if FreeCAD.GuiUp:
    translate = FreeCAD.Qt.translate
    QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP

# Constants for geometry and performance thresholds
TILE_MIN = 1.0
MIN_DIMENSION = 0.1
JOINT_THRESHOLD = MIN_DIMENSION
TOO_MANY_TILES = 10000
GRID_LIMIT = 100000


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
    ):
        self.count_full = count_full
        self.count_partial = count_partial
        self.area_net = area_net
        self.area_gross = area_gross
        self.length_joints = length_joints
        self.waste_area = waste_area
        self.length_perimeter = length_perimeter


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
    Typed configuration for rectangular tile tessellation modes (Solid Tiles, Parametric
    Pattern, Monolithic).

    Centralises all parameter validation and the stagger-offset resolution that was
    previously scattered across ``create_tessellator``.  Using a dataclass instead of a
    plain dict means a typo in a key is a ``AttributeError`` at construction time, not a
    silent ``0`` default discovered during a recompute.

    Parameters
    ----------
    finish_mode : str
        One of ``"Solid Tiles"``, ``"Parametric Pattern"``, or ``"Monolithic"``.
    length, width, thickness, joint : float
        Tile dimensions in mm.
    rotation : float
        Grid rotation in degrees.
    stagger_type : str
        Running-bond preset name.  Resolved to ``offset_u`` automatically.
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
        rotation=0.0,
        stagger_type="Stacked (None)",
        stagger_custom=0.0,
    ):
        self.finish_mode = finish_mode
        self.length = length
        self.width = width
        self.thickness = thickness
        self.joint = joint
        self.rotation = rotation
        self.stagger_type = stagger_type
        self.stagger_custom = stagger_custom

        # Derived flags
        self.extrude = finish_mode in ("Solid Tiles", "Monolithic")
        self.monolithic = finish_mode == "Monolithic"

        # Resolve the stagger enum to a concrete mm offset.
        # Keeping this logic here means create_tessellator and the caller both stay
        # ignorant of the enum strings; they just see a numeric offset.
        _stagger_map = {
            "Half Bond (1/2)": length / 2.0,
            "Third Bond (1/3)": length / 3.0,
            "Quarter Bond (1/4)": length / 4.0,
        }
        if stagger_type in _stagger_map:
            self.offset_u = _stagger_map[stagger_type]
        elif stagger_type == "Custom":
            self.offset_u = stagger_custom
        else:
            self.offset_u = 0.0


class HatchConfig:
    """
    Typed configuration for hatch-pattern tessellation.

    Parameters
    ----------
    filename : str
        Path to the ``.pat`` file.
    pattern_name : str
        Name of the pattern within the file.
    scale : float
        Pattern scale factor.
    rotation : float
        Pattern rotation in degrees.
    thickness : float
        Substrate thickness in mm (for the backing solid).
    """

    def __init__(self, filename, pattern_name, scale=1.0, rotation=0.0, thickness=0.0):
        self.filename = filename
        self.pattern_name = pattern_name
        self.scale = max(MIN_DIMENSION, scale)
        self.rotation = rotation
        self.thickness = thickness


def _localise_substrate(substrate_3d, origin_3d, u_vec, normal):
    """
    Establish an orthonormal local frame, derive a world placement from it, and
    return the substrate transformed into that local frame.

    Both ``RectangularTessellator`` and ``HatchTessellator`` need exactly this
    three-step preamble.  Keeping one copy here ensures they can never drift apart.

    Parameters
    ----------
    substrate_3d : Part.Face
        The source face in world space.
    origin_3d : FreeCAD.Vector
        World-space grid anchor (becomes the local origin).
    u_vec : FreeCAD.Vector
        World-space U tangent of the face (need not be normalised).
    normal : FreeCAD.Vector
        World-space face normal (need not be normalised).

    Returns
    -------
    substrate : Part.Shape
        A copy of ``substrate_3d`` transformed into local space (origin at
        ``origin_3d``, local Z aligned with ``normal``).
    placement : FreeCAD.Placement
        The world placement that maps local→world.  Assign to ``obj.Placement``
        to position the finished geometry correctly in the scene.
    """
    n_vec = FreeCAD.Vector(normal).normalize()
    u_vec = FreeCAD.Vector(u_vec).normalize()
    v_vec = n_vec.cross(u_vec).normalize()
    u_vec = v_vec.cross(n_vec).normalize()

    placement = FreeCAD.Placement(origin_3d, FreeCAD.Rotation(u_vec, v_vec, n_vec))
    to_local = placement.inverse().toMatrix()

    substrate = substrate_3d.copy()
    substrate.transformShape(to_local)
    return substrate, placement


class Tessellator(ABC):
    """Abstract base class for pattern tessellators."""

    @abstractmethod
    def compute(self, substrate_3d, origin_3d, u_vec, normal):
        """
        Generates the tessellated geometry.
        substrate: Part.Face to be tiled.
        origin: Vector starting point of the grid.
        u_vec, v_vec, normal: Vectors defining the orientation.
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
        self.rotation = config.rotation
        self.extrude = config.extrude
        self.monolithic = config.monolithic

    def compute(self, substrate_3d, origin_3d, u_vec, normal):
        """
        Orchestrates the generation of the tessellated geometry, selecting between physical
        discretization and analytical approximation based on performance constraints.

        This method acts as the main pipeline controller. It determines the tessellation strategy
        based on tile count and dimensions:

        1. **Physical mode (standard):**
           If the configuration is within safe limits (Status.OK), it generates individual solids
           for tiles by performing boolean subtractions of the joints from the base volume.

        2. **Analytical mode (performance/fallback):**
           If the joint width is too small for stable boolean operations (< 0.1 mm), or if the tile
           count exceeds performance thresholds (> 10,000), it switches to a monolithic
           representation (i.e. one big tile).
           - Returns a single solid (the substrate extruded).
           - Calculates tile counts and waste analytically (mathematically) rather than
             geometrically to ensure speed and stability.

        Parameters
        ----------
        substrate : Part.Face
            The source face to be tiled.
        origin : FreeCAD.Vector
            The 3D starting point of the tiling grid.
        u_vec, v_vec : FreeCAD.Vector
            Unit vectors defining the local U (Length) and V (Width) directions.
        normal : FreeCAD.Vector
            Unit vector defining the extrusion direction (Thickness).

        Returns
        -------
        TessellationResult
            A container object holding:
            - geometry: The resulting Part.Shape (Compound or Solid).
            - quantities: Calculated BIM data (counts, areas, joint lengths).
            - status: The performance flag indicating how the result was computed.
        """
        # Localise the substrate into the grid's local coordinate frame.
        # The shared helper normalises the basis, builds the placement, and transforms
        # the substrate — all in one place so RectangularTessellator and HatchTessellator
        # cannot drift apart.
        substrate, placement = _localise_substrate(substrate_3d, origin_3d, u_vec, normal)

        # Monolithic Fast-Path
        if self.monolithic:
            quantities = TessellationQuantities()
            quantities.area_net = substrate.Area
            quantities.length_perimeter = substrate.Length
            quantities.count_full = 1
            quantities.area_gross = quantities.area_net

            if self.extrude and self.thickness > 0.001:
                final_geo = substrate.extrude(FreeCAD.Vector(0, 0, self.thickness))
            else:
                # 2D representation with micro-offset for visibility
                mat = FreeCAD.Matrix()
                mat.move(normal.normalize() * 0.05)
                final_geo = substrate.transformGeometry(mat)

            return TessellationResult(
                geometry=final_geo,
                placement=placement,
                quantities=quantities,
                unit_area=quantities.area_net,
                unit_volume=quantities.area_net * self.thickness,
                status=TessellationStatus.OK,
            )

        # Check for non-physical dimensions that would cause math errors.
        if self.length < TILE_MIN or self.width < TILE_MIN:
            return TessellationResult(status=TessellationStatus.INVALID_DIMENSIONS)

        # Grid calculation
        # Compute step sizes, matrix counts, and determine the performance status
        # (e.g., OK, EXTREME_COUNT, JOINT_TOO_SMALL).
        params = self._calculate_grid_params(substrate)

        # Visual grid generation
        # Generate the centerline wires for visualization.
        # If status is EXTREME_COUNT, this returns empty lists to save memory.
        h_edges, v_edges, final_cl = self._generate_visual_grid(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(1, 0, 0),
            FreeCAD.Vector(0, 1, 0),
            FreeCAD.Vector(0, 0, 1),
            params,
            substrate.BoundBox,
        )

        # Quantity take-off (linear measurements)
        # Calculate Net Area, Perimeter, and Total Joint Length.
        # Uses geometric intersection for standard modes, or statistical estimation for extreme
        # modes.
        quantities = self._calculate_linear_quantities(substrate, params, h_edges, v_edges)

        # Geometry generation strategy
        # Branch based on the calculated performance status.
        if params["status"] != TessellationStatus.OK:
            # Analytical path:
            # Used for tiny joints (where booleans fail) or massive counts (where booleans freeze).
            # Returns a monolithic solid with mathematically derived counts.
            final_geo = self._generate_analytical_geo(substrate, params, final_cl, quantities)
        else:
            # Physical path:
            # Standard mode. Generates "cutter" volumes and performs boolean subtraction.
            # Counts are derived by measuring the resulting solids/faces.
            final_geo = self._generate_physical_geo(substrate, params, quantities)

        # Result assembly
        return TessellationResult(
            geometry=final_geo,
            placement=placement,
            centerlines=final_cl,
            quantities=quantities,
            unit_area=params["unit_area"],
            unit_volume=params["unit_volume"],
            status=params["status"],
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
        dict
            A dictionary containing calculated grid parameters, steps, counts, unit dimensions, and
            the tessellation status.
        """
        step_u = self.length + self.joint
        step_v = self.width + self.joint
        bbox = substrate.BoundBox
        diag = bbox.DiagonalLength

        # Add buffer to counts to ensure coverage of rotated faces
        count_u = int(diag / step_u) + 4
        count_v = int(diag / step_v) + 4
        total_count = count_u * count_v

        # Reference metadata
        unit_area = float(self.length * self.width)
        unit_volume = float(unit_area * self.thickness)

        # Determine output mode
        status = TessellationStatus.OK
        if total_count > GRID_LIMIT:
            status = TessellationStatus.EXTREME_COUNT
        elif self.joint < JOINT_THRESHOLD:
            status = TessellationStatus.JOINT_TOO_SMALL
        elif total_count > TOO_MANY_TILES:
            status = TessellationStatus.COUNT_TOO_HIGH

        return {
            "step_u": step_u,
            "step_v": step_v,
            "count_u": count_u,
            "count_v": count_v,
            "total_count": total_count,
            "unit_area": unit_area,
            "unit_volume": unit_volume,
            "status": status,
            "full_len_u": 2 * count_u * step_u,
            "full_len_v": 2 * count_v * step_v,
            "start_u": -count_u * step_u,
            "start_v": -count_v * step_v,
        }

    def _generate_lines_along_axis(
        self, count, step, ortho_dim, line_start, line_len, is_horizontal, bbox
    ):
        """
        Generates a list of parallel edges for the grid visualization.

        Parameters
        ----------
        count : int
            Half the number of lines to generate (range will be -count to count).
        step : float
            The distance between lines.
        ortho_dim : float
            The dimension of the tile orthogonal to the line direction (width or length).
        line_start : float
            The starting coordinate of the line along its axis.
        line_len : float
            The total length of the line.
        is_horizontal : bool
            If True, generates horizontal lines (Y-varying). If False, vertical (X-varying).
        bbox : FreeCAD.BoundBox
            The local bounding box of the substrate (after localization).

        Returns
        -------
        list
            A list of Part.Edge objects.
        """
        # NOTE: Joint length is measured along centerlines, not face widths.
        # For a joint of width W, the centerline sits W/2 inside each tile edge.
        # The resulting length_joints quantity therefore accurately represents the
        # total run of jointing material for joints that are narrow relative to
        # tile size (the common case), but will be slightly underestimated if a
        # very wide joint (e.g. 50 mm) straddles the substrate boundary — the
        # clipped centerline is shorter than the clipped joint face would be.
        edges = []
        for i in range(-count, count):
            # The centerline sits at: tile_body_end + half_joint
            pos = i * step + ortho_dim + (self.joint / 2)

            if is_horizontal:
                p1 = FreeCAD.Vector(line_start, pos, 0)
                p2 = FreeCAD.Vector(line_start + line_len, pos, 0)
            else:
                p1 = FreeCAD.Vector(pos, line_start, 0)
                p2 = FreeCAD.Vector(pos, line_start + line_len, 0)

            # Performance: Bounding Box pre-filter to avoid processing lines in "air".
            # Both p1/p2 and bbox are in the same local coordinate frame (substrate already
            # localized), so compare directly without applying the global placement.
            edge_bb = FreeCAD.BoundBox()
            edge_bb.add(p1)
            edge_bb.add(p2)
            if bbox.intersect(edge_bb):
                edges.append(Part.makeLine(p1, p2))

        return edges

    def _generate_visual_grid(self, origin, u_vec, v_vec, normal, params, bbox):
        """
        Generates the visual centerlines compound and the local coordinate placement.

        Parameters
        ----------
        origin : FreeCAD.Vector
            The grid origin point.
        u_vec, v_vec, normal : FreeCAD.Vector
            The orientation vectors.
        params : dict
            The grid parameters calculated by _calculate_grid_params.
        bbox : FreeCAD.BoundBox
            The local bounding box of the substrate (after localization).

        Returns
        -------
        tuple
            (h_edges_list, v_edges_list, compound_shape)
        """
        # tr is local: it encodes the grid rotation so the visual centerlines
        # render in the right orientation, but the caller never needs it
        # (the world placement is returned by compute() via res.placement).
        tr = FreeCAD.Placement(
            origin,
            FreeCAD.Rotation(u_vec, v_vec, normal).multiply(
                FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), self.rotation)
            ),
        )

        h_edges, v_edges = [], []
        final_cl = None

        if params["status"] != TessellationStatus.EXTREME_COUNT:
            # Horizontal lines (vary along V axis)
            h_edges = self._generate_lines_along_axis(
                params["count_v"],
                params["step_v"],
                self.width,
                params["start_u"],
                params["full_len_u"],
                True,
                bbox,
            )
            # Vertical lines (vary along U axis)
            v_edges = self._generate_lines_along_axis(
                params["count_u"],
                params["step_u"],
                self.length,
                params["start_v"],
                params["full_len_v"],
                False,
                bbox,
            )

            final_cl = Part.Compound(h_edges + v_edges)
            final_cl.Placement = tr

        return h_edges, v_edges, final_cl

    def _calculate_linear_quantities(self, substrate, params, h_edges, v_edges):
        """
        Calculates areas, perimeter, and joint lengths.

        Parameters
        ----------
        substrate : Part.Face
            The face being tiled.
        params : dict
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

        # Calculate joint length via clipping centerlines.
        # Both substrate and the edge compounds are in the same local coordinate frame
        # (origin at grid anchor, Z=0), so no Placement assignment is needed before common().
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
                pass
        elif params["status"] == TessellationStatus.EXTREME_COUNT:
            # Analytical fallback for mosaic-like counts
            # Statistical estimation: Sum of grid lines in U and V directions minus perimeter
            stat_total = (substrate.Area / params["step_u"]) + (substrate.Area / params["step_v"])
            quantities.length_joints = max(0.0, stat_total - quantities.length_perimeter)

        return quantities

    def _generate_analytical_geo(self, substrate, params, final_cl, quantities):
        """
        Generates a monolithic representation of the tiled surface and calculates quantities
        mathematically.

        In practice, "Analytical" means the geometry remains a single continuous solid (or face)
        without physical gaps. This mode is used as a fallback when specific tile geometry is
        unnecessary (e.g., joints < 0.1 mm) or computationally too expensive (e.g., > 10,000 tiles).

        Instead of counting geometric objects, this method derives tile counts and waste estimation
        purely from the surface area and unit tile dimensions (statistical quantity take-off).

        Parameters
        ----------
        substrate : Part.Face
            The base face.
        params : dict
            Grid parameters.
        final_cl : Part.Compound
            The visual grid lines.
        quantities : TessellationQuantities
            The quantities object to populate with calculated counts.

        Returns
        -------
        Part.Shape
            The resulting geometry.
        """
        # Mathematical quantity fallback
        count_rounded = round(substrate.Area / params["unit_area"], 6)
        quantities.count_full = int(count_rounded)
        quantities.count_partial = 1 if (count_rounded - quantities.count_full) > 0 else 0
        quantities.area_gross = (quantities.count_full + quantities.count_partial) * params[
            "unit_area"
        ]
        quantities.waste_area = max(0.0, quantities.area_gross - quantities.area_net)

        local_normal = FreeCAD.Vector(0, 0, 1)

        # Ensure we do not pass a null vector to the extrusion engine.
        if self.extrude and abs(self.thickness) > 1e-6:
            monolithic = substrate.extrude(local_normal * self.thickness)
        else:
            # 2D fallback: apply micro-offset to prevent Z-fighting in the viewport.
            mat = FreeCAD.Matrix()
            mat.move(local_normal * 0.05)
            monolithic = substrate.transformGeometry(mat)

        final_geo = monolithic
        if final_cl:
            final_geo = Part.Compound([monolithic, final_cl])

        return final_geo

    def _generate_physical_geo(self, substrate, params, quantities):
        """
        Generates the discrete tile geometry by intersecting a grid of tiles with the substrate.

        In practice, "Physical" means the algorithm constructs a grid of individual tile
        primitives (solids for 3D or faces for 2D) and performs a Boolean Intersection (common)
        operation with the substrate.

        This results in a compound of independent solids (or wires) for each individual tile.
        This mode allows for:
        1. Visualization of the physical gaps (joints) between tiles.
        2. Geometric counting, where "Full" vs "Partial" tiles are determined by measuring the
           volume/area of the resulting fragments against the theoretical unit size.

        Both the substrate and the tile grid are kept in the same local coordinate frame
        (grid anchor at origin, Z=0 plane) throughout. No Placement is assigned to the tile
        grid before the boolean intersection.

        Parameters
        ----------
        substrate : Part.Face
            The base face defining the boundary of the covering, already localized to origin.
        params : dict
            Grid parameters including tile dimensions and counts.
        quantities : TessellationQuantities
            The quantities object to populate with geometric counts and areas.

        Returns
        -------
        Part.Shape
            A compound containing the discrete tile geometry.
        """
        import Part
        import FreeCAD

        tiles = []
        unit_area = params["unit_area"]
        unit_volume = params["unit_volume"]

        # Build the grid of tiles at local origin.
        # Tile positions are expressed in the same local frame as the substrate:
        # U along X, V along Y, extrusion along Z.
        for j in range(-params["count_v"], params["count_v"]):
            # Calculate row-based stagger offsets
            stagger_x = self.offset_u if (j % 2 != 0) else 0.0
            stagger_y = self.offset_v if (j % 2 != 0) else 0.0

            v_pos = j * params["step_v"] + stagger_y

            for i in range(-params["count_u"], params["count_u"]):
                u_pos = i * params["step_u"] + stagger_x

                # Create tile at calculated local position
                if self.extrude and self.thickness > 0:
                    tile = Part.makeBox(
                        self.length, self.width, self.thickness, FreeCAD.Vector(u_pos, v_pos, 0)
                    )
                else:
                    tile = Part.makePlane(self.length, self.width, FreeCAD.Vector(u_pos, v_pos, 0))
                tiles.append(tile)

        # Combine in local frame.
        tile_grid = Part.makeCompound(tiles)

        # Apply rotation around the local Z axis (the face normal in local space).
        # self.rotation is the user-specified grid rotation in degrees. We apply it
        # here via transformShape so the tile grid and the substrate remain in the
        # same coordinate frame for the boolean intersection that follows.
        # This mirrors the rotation already encoded in final_cl.Placement = tr.
        if self.rotation:
            rot_mat = FreeCAD.Matrix()
            rot_mat.rotateZ(math.radians(self.rotation))
            tile_grid.transformShape(rot_mat)

        local_normal = FreeCAD.Vector(0, 0, 1)

        # Boundary clipping
        if self.extrude and self.thickness > 0.001:
            # For solid mode, intersect the tiles with the substrate volume
            substrate_vol = substrate.extrude(local_normal * self.thickness)
            # Use common() to keep only the parts of tiles inside the substrate
            final_geo = substrate_vol.common(tile_grid)

            # Geometric counting (solids)
            full_cnt, part_cnt = 0, 0
            if final_geo.Solids:
                for sol in final_geo.Solids:
                    # Tolerance check for full vs clipped tiles
                    if sol.Volume >= (unit_volume * 0.995):
                        full_cnt += 1
                    else:
                        part_cnt += 1
        else:
            # For pattern mode, intersect the 2D tiles with the 2D substrate
            final_geo = substrate.common(tile_grid)

            # Geometric counting (faces)
            full_cnt, part_cnt = 0, 0
            if final_geo.Faces:
                for f in final_geo.Faces:
                    if f.Area >= (unit_area * 0.995):
                        full_cnt += 1
                    else:
                        part_cnt += 1

            # Convert the resulting faces to wires for 2D visual representation
            wires = [wire for f in final_geo.Faces for wire in f.Wires]
            if wires:
                pattern_lines = Part.makeCompound(wires)

                if self.thickness > 0:
                    # Create a monolithic solid body for realism at no performance cost
                    body = substrate.extrude(local_normal * self.thickness)
                    # Move lines to the top of the solid
                    # Apply micro-offset to prevent Z-fighting with the body/base face
                    pattern_lines.translate(local_normal * (self.thickness + 0.05))
                    final_geo = Part.makeCompound([body, pattern_lines])
                else:
                    pattern_lines.translate(local_normal * 0.05)
                    final_geo = Part.makeCompound([substrate, pattern_lines])
            else:
                final_geo = Part.Shape()

        # Update Quantity Take-Off metadata
        quantities.count_full = full_cnt
        quantities.count_partial = part_cnt
        quantities.area_gross = (full_cnt + part_cnt) * unit_area
        quantities.waste_area = max(0.0, quantities.area_gross - quantities.area_net)

        return final_geo


class HatchTessellator(Tessellator):
    """Generates hatch patterns using the Draft Hatch engine from a :class:`HatchConfig`."""

    def __init__(self, config):
        self.filename = config.filename
        self.pattern_name = config.pattern_name
        self.scale = config.scale  # already clamped in HatchConfig
        self.rotation = config.rotation
        self.thickness = config.thickness

    def _resolve_pattern_name(self):
        """
        Return the pattern name to use, auto-detecting from the file if the
        configured name is empty.

        Issue 7 fix: the resolved name is returned as a local value and never
        written back to ``self``.  This keeps ``compute()`` side-effect-free with
        respect to instance state, so tessellators are safe to cache or reuse.
        """
        import os

        if self.pattern_name:
            return self.pattern_name

        if not self.filename or not os.path.exists(self.filename):
            return ""

        try:
            with open(self.filename, "r", encoding="utf-8") as f:
                for line in f:
                    if line.startswith("*"):
                        parts = line.split(",")
                        if parts:
                            return parts[0][1:].strip()
        except Exception as e:
            FreeCAD.Console.PrintWarning(f"ArchTessellation: Could not read pattern file: {e}\n")
        return ""

    def compute(self, substrate_3d, origin_3d, u_vec, normal):
        import TechDraw

        # Localise the substrate into the grid's local coordinate frame.
        substrate, placement = _localise_substrate(substrate_3d, origin_3d, u_vec, normal)

        # TechDraw requires a Part.Face instance. We extract it once here
        # so it is available for both the hatching logic and the fallback paths.
        if hasattr(substrate, "Faces") and len(substrate.Faces) > 0:
            local_face = substrate.Faces[0]
        else:
            local_face = substrate

        # Initialize the result geometry to the localized substrate.
        final_local_geo = substrate

        # Resolve the pattern name without mutating self (Issue 7).
        pattern_name = self._resolve_pattern_name()

        # Only proceed if we have both a file and a valid name
        if self.filename and pattern_name:
            pat_shape = None
            try:
                # Apply pattern rotation
                if self.rotation:
                    local_face.rotate(
                        FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1), -self.rotation
                    )

                # Generate hatch
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
                        local_face, float(self.scale), str(pattern_name), str(self.filename)
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

                if pat_shape and len(pat_shape.Edges) > 0:
                    # Restore global position
                    if self.rotation:
                        pat_shape.rotate(
                            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1), self.rotation
                        )

                # Ensure the covering has thickness even if the hatch lines fail.
                if self.thickness > 0:
                    body = substrate.extrude(FreeCAD.Vector(0, 0, self.thickness))
                    if pat_shape and len(pat_shape.Edges) > 0:
                        # Offset lines slightly to prevent Z-fighting with the top face
                        pat_shape.translate(FreeCAD.Vector(0, 0, self.thickness + 0.05))
                        final_local_geo = Part.Compound([body, pat_shape])
                    else:
                        final_local_geo = body
                elif pat_shape and len(pat_shape.Edges) > 0:
                    # Applied micro-offset to prevent Z-fighting with the base face
                    pat_shape.translate(FreeCAD.Vector(0, 0, 0.05))
                    final_local_geo = Part.Compound([substrate, pat_shape])
                else:
                    final_local_geo = local_face

            except Exception as e:
                FreeCAD.Console.PrintError(f"ArchTessellation error: {e}\n")
        else:
            final_local_geo = local_face

        return TessellationResult(geometry=final_local_geo, placement=placement)


def create_tessellator(config):
    """
    Instantiate the correct :class:`Tessellator` subclass for the given typed config.

    Issue 1 fix: accepts :class:`TileConfig` or :class:`HatchConfig` instead of a
    plain dict.  The config type determines the tessellator class unambiguously, so
    callers cannot accidentally pass the wrong keys and get a silently wrong result.

    Parameters
    ----------
    config : TileConfig | HatchConfig
        Fully-specified tessellation configuration.  Construct it in the caller so
        that Python raises ``AttributeError`` immediately if a required field is missing.

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
