# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
# You can find the full license text in the LICENSE file in the root directory.

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
        centerlines=None,
        quantities=None,
        unit_area=0.0,
        unit_volume=0.0,
        status=TessellationStatus.OK,
    ):
        self.geometry = geometry
        self.centerlines = centerlines
        self.quantities = quantities if quantities else TessellationQuantities()
        self.unit_area = unit_area
        self.unit_volume = unit_volume
        self.status = status


class Tessellator(ABC):
    """Abstract base class for pattern tessellators."""

    @abstractmethod
    def compute(self, substrate, origin, u_vec, v_vec, normal):
        """
        Generates the tessellated geometry.
        substrate: Part.Face to be tiled.
        origin: Vector starting point of the grid.
        u_vec, v_vec, normal: Vectors defining the orientation.
        """
        pass


class RectangularTessellator(Tessellator):
    """Generates standard grid or running bond patterns."""

    def __init__(self, length, width, thickness, joint, offset_u=0.0, offset_v=0.0, rotation=0.0):
        self.length = length
        self.width = width
        self.thickness = thickness
        self.joint = joint
        self.offset_u = offset_u
        self.offset_v = offset_v
        self.rotation = rotation

    def compute(self, substrate, origin, u_vec, v_vec, normal):
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
        tr, h_edges, v_edges, final_cl = self._generate_visual_grid(
            origin, u_vec, v_vec, normal, params, substrate.BoundBox
        )

        # Quantity take-off (linear measurements)
        # Calculate Net Area, Perimeter, and Total Joint Length.
        # Uses geometric intersection for standard modes, or statistical estimation for extreme
        # modes.
        quantities = self._calculate_linear_quantities(substrate, params, h_edges, v_edges, tr)

        # Geometry generation strategy
        # Branch based on the calculated performance status.
        if params["status"] != TessellationStatus.OK:
            # Analytical path:
            # Used for tiny joints (where booleans fail) or massive counts (where booleans freeze).
            # Returns a monolithic solid with mathematically derived counts.
            final_geo = self._generate_analytical_geo(
                substrate, normal, params, final_cl, quantities
            )
        else:
            # Physical path:
            # Standard mode. Generates "cutter" volumes and performs boolean subtraction.
            # Counts are derived by measuring the resulting solids/faces.
            final_geo = self._generate_pyhsical_geo(substrate, normal, tr, params, quantities)

        # Result assembly
        return TessellationResult(
            final_geo,
            final_cl,
            quantities,
            params["unit_area"],
            params["unit_volume"],
            params["status"],
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
        self, count, step, ortho_dim, line_start, line_len, is_horizontal, placement, bbox
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
        placement : FreeCAD.Placement
            The global placement of the grid.
        bbox : FreeCAD.BoundBox
            The global bounding box of the substrate.

        Returns
        -------
        list
            A list of Part.Edge objects.
        """
        edges = []
        for i in range(-count, count):
            # Calculate position on the orthogonal axis
            # The line is placed after the tile body + half the joint
            pos = i * step + ortho_dim + (self.joint / 2)

            if is_horizontal:
                p1 = FreeCAD.Vector(line_start, pos, 0)
                p2 = FreeCAD.Vector(line_start + line_len, pos, 0)
            else:
                p1 = FreeCAD.Vector(pos, line_start, 0)
                p2 = FreeCAD.Vector(pos, line_start + line_len, 0)

            # Performance: Bounding Box pre-filter to avoid processing lines in "air"
            p1_g, p2_g = placement.multVec(p1), placement.multVec(p2)
            edge_bb = FreeCAD.BoundBox()
            edge_bb.add(p1_g)
            edge_bb.add(p2_g)
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
            The global bounding box of the substrate.

        Returns
        -------
        tuple
            (placement, h_edges_list, v_edges_list, compound_shape)
        """
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
                tr,
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
                tr,
                bbox,
            )

            final_cl = Part.Compound(h_edges + v_edges)
            final_cl.Placement = tr

        return tr, h_edges, v_edges, final_cl

    def _calculate_linear_quantities(self, substrate, params, h_edges, v_edges, placement):
        """
        Calculates areas, perimeter, and joint lengths.

        Parameters
        ----------
        substrate : Part.Face
            The face being tiled.
        params : dict
            Grid parameters.
        h_edges, v_edges : list
            Lists of grid edges.
        placement : FreeCAD.Placement
            The local coordinate placement.

        Returns
        -------
        TessellationQuantities
            The populated quantities object.
        """
        quantities = TessellationQuantities()
        quantities.area_net = substrate.Area
        quantities.length_perimeter = substrate.Length

        # Calculate joint length via clipping centerlines
        if h_edges or v_edges:
            try:
                # Use a hybrid approach to calculate joint length. Intersect horizontal and vertical
                # joint compounds separately to avoid geometric kernel instability with sparse,
                # non-parallel compounds.
                total_length = 0
                if h_edges:
                    h_compound = Part.Compound(h_edges)
                    h_compound.Placement = placement
                    clipped_h = substrate.common(h_compound)
                    total_length += clipped_h.Length
                if v_edges:
                    v_compound = Part.Compound(v_edges)
                    v_compound.Placement = placement
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

    def _create_cutters(self, params, z_off, cut_thick):
        """
        Generates the cutter boxes for the boolean operations.

        Parameters
        ----------
        params : dict
            Grid parameters.
        z_off : float
            Z-offset for the cutter boxes.
        cut_thick : float
            Thickness of the cutter boxes.

        Returns
        -------
        tuple
            (list_h_cutters, list_v_cutters)
        """
        cutters_h, cutters_v = [], []

        # Horizontal cutters
        for j in range(-params["count_v"], params["count_v"]):
            rv_off = self.offset_v if (j % 2 != 0) else 0
            cutters_h.append(
                Part.makeBox(
                    params["full_len_u"],
                    self.joint,
                    cut_thick,
                    FreeCAD.Vector(
                        params["start_u"],
                        j * params["step_v"] + self.width + rv_off,
                        z_off,
                    ),
                )
            )

        # Vertical cutters
        # Check if vertical alignment is stacked (grid) or staggered (running bond)
        is_stack = abs(self.offset_u) < 1e-6 and abs(self.offset_v) < 1e-6

        for i in range(-params["count_u"], params["count_u"]):
            lu = i * params["step_u"] + self.length

            if is_stack:
                # Optimized: One long cutter per column
                cutters_v.append(
                    Part.makeBox(
                        self.joint,
                        params["full_len_v"],
                        cut_thick,
                        FreeCAD.Vector(lu, params["start_v"], z_off),
                    )
                )
            else:
                # Staggered: Individual cutters per tile
                for j in range(-params["count_v"], params["count_v"]):
                    rv = j * params["step_v"] + (self.offset_v if (j % 2 != 0) else 0)
                    cutters_v.append(
                        Part.makeBox(
                            self.joint,
                            params["step_v"],
                            cut_thick,
                            FreeCAD.Vector(lu + (self.offset_u if (j % 2 != 0) else 0), rv, z_off),
                        )
                    )
        return cutters_h, cutters_v

    def _generate_analytical_geo(self, substrate, normal, params, final_cl, quantities):
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
        normal : FreeCAD.Vector
            The extrusion direction.
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

        if self.thickness > 0:
            monolithic = substrate.extrude(normal * self.thickness)
        else:
            mat = FreeCAD.Matrix()
            mat.move(normal.normalize() * 0.05)
            monolithic = substrate.transformGeometry(mat)

        final_geo = monolithic
        if final_cl:
            final_geo = Part.Compound([monolithic, final_cl])

        return final_geo

    def _generate_pyhsical_geo(self, substrate, normal, placement, params, quantities):
        """
        Generates the discrete tile geometry by physically cutting the substrate.

        In practice, "Physical" means the algorithm constructs a 3D grid of "cutter" volumes (boxes
        representing the grout/joints) and performs a Boolean Cut operation on the base substrate.

        This results in a compound of independent solids (or faces) for each individual tile. This
        mode allows for:
        1. Visualization of the gap between tiles.
        2. Geometric counting, where "Full" vs "Partial" tiles are determined by measuring the
           volume/area of the resulting fragments against the theoretical unit size.

        Parameters
        ----------
        substrate : Part.Face
            The base face.
        normal : FreeCAD.Vector
            The extrusion direction.
        placement : FreeCAD.Placement
            The grid placement.
        params : dict
            Grid parameters.
        quantities : TessellationQuantities
            The quantities object to populate with geometric counts.

        Returns
        -------
        Part.Shape
            The resulting geometry.
        """
        z_off = -0.5 if self.thickness == 0 else -0.05
        cut_thick = 1.0 if self.thickness == 0 else self.thickness * 1.1

        cutters_h, cutters_v = self._create_cutters(params, z_off, cut_thick)

        comp_h = Part.Compound(cutters_h) if cutters_h else None
        comp_v = Part.Compound(cutters_v) if cutters_v else None
        if comp_h:
            comp_h.Placement = placement
        if comp_v:
            comp_v.Placement = placement

        if self.thickness > 0:
            layer = substrate.extrude(normal * self.thickness)
            if comp_h:
                layer = layer.cut(comp_h)
            if comp_v:
                layer = layer.cut(comp_v)
            final_geo = layer

            # Geometric counting (Solids)
            full_cnt, part_cnt = 0, 0
            if final_geo.Solids:
                for sol in final_geo.Solids:
                    if sol.Volume >= (params["unit_volume"] * 0.995):
                        full_cnt += 1
                    else:
                        part_cnt += 1
        else:
            layer = substrate
            if comp_h:
                layer = layer.cut(comp_h)
            if comp_v:
                layer = layer.cut(comp_v)

            # Geometric counting (Faces)
            full_cnt, part_cnt = 0, 0
            if layer.Faces:
                for f in layer.Faces:
                    if f.Area >= (params["unit_area"] * 0.995):
                        full_cnt += 1
                    else:
                        part_cnt += 1

            # Convert to wires for 2D representation
            wires = [wire for f in layer.Faces for wire in f.Wires]
            if wires:
                final_geo = Part.Compound(wires)
                mat = FreeCAD.Matrix()
                mat.move(normal.normalize() * 0.05)
                final_geo = final_geo.transformGeometry(mat)
            else:
                final_geo = Part.Shape()

        quantities.count_full = full_cnt
        quantities.count_partial = part_cnt
        quantities.area_gross = (full_cnt + part_cnt) * params["unit_area"]
        quantities.waste_area = max(0.0, quantities.area_gross - quantities.area_net)

        return final_geo


class HatchTessellator(Tessellator):
    """Generates hatch patterns using the Draft Hatch engine."""

    def __init__(self, filename, name, scale, rotation):
        self.filename = filename
        self.name = name
        self.scale = max(MIN_DIMENSION, scale)
        self.rotation = rotation

    def compute(self, substrate, origin, u_vec, v_vec, normal):
        from draftobjects.hatch import Hatch

        final_geo = substrate
        if self.filename:
            pat = Hatch.hatch(
                substrate, self.filename, self.name, scale=self.scale, rotation=self.rotation
            )
            if pat:
                final_geo = Part.Compound([substrate, pat])

        # Hatching does not calculate tile quantities
        return TessellationResult(geometry=final_geo)


def create_tessellator(mode, config):
    match mode:
        case "Hatch Pattern":
            return HatchTessellator(
                config.get("PatternFile"),
                config.get("PatternName"),
                config.get("PatternScale", 1.0),
                config.get("Rotation", 0.0),
            )
        case "Solid Tiles" | "Parametric Pattern":
            tile_offset = config.get("TileOffset", FreeCAD.Vector())
            return RectangularTessellator(
                config.get("TileLength", 0),
                config.get("TileWidth", 0),
                config.get("TileThickness", 0),
                config.get("JointWidth", 0),
                tile_offset.x,
                tile_offset.y,
                config.get("Rotation", 0),
            )
        case _:
            return None
