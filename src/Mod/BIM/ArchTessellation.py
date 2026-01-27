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
        # Hard stop for invalid dimensions
        if self.length < TILE_MIN or self.width < TILE_MIN:
            return TessellationResult(status=TessellationStatus.INVALID_DIMENSIONS)

        step_u = self.length + self.joint
        step_v = self.width + self.joint
        bbox = substrate.BoundBox
        diag = bbox.DiagonalLength
        count_u = int(diag / step_u) + 4
        count_v = int(diag / step_v) + 4
        total_count = count_u * count_v

        # Reference metadata
        unit_area = float(self.length * self.width)
        unit_volume = float(unit_area * self.thickness)

        # Determine output mode
        status = TessellationStatus.OK
        if self.joint < JOINT_THRESHOLD:
            status = TessellationStatus.JOINT_TOO_SMALL
        elif total_count > TOO_MANY_TILES:
            status = TessellationStatus.COUNT_TOO_HIGH

        # Generate visual layout lines (centerlines)
        tr = FreeCAD.Placement(
            origin,
            FreeCAD.Rotation(u_vec, v_vec, normal).multiply(
                FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), self.rotation)
            ),
        )

        final_cl = None
        h_edges, v_edges = [], []
        if total_count <= GRID_LIMIT:
            full_len_u, full_len_v = 2 * count_u * step_u, 2 * count_v * step_v
            start_u, start_v = -count_u * step_u, -count_v * step_v

            # Use continuous lines for quantity calculation, separated by orientation
            for j in range(-count_v, count_v):
                lv = j * step_v + self.width + (self.joint / 2)
                h_edges.append(
                    Part.makeLine(
                        FreeCAD.Vector(start_u, lv, 0), FreeCAD.Vector(start_u + full_len_u, lv, 0)
                    )
                )
            for i in range(-count_u, count_u):
                lu = i * step_u + self.length + (self.joint / 2)
                v_edges.append(
                    Part.makeLine(
                        FreeCAD.Vector(lu, start_v, 0), FreeCAD.Vector(lu, start_v + full_len_v, 0)
                    )
                )
            final_cl = Part.Compound(h_edges + v_edges)
            final_cl.Placement = tr
        else:
            status = TessellationStatus.EXTREME_COUNT

        # Prepare quantities
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
                    h_compound.Placement = tr
                    clipped_h = substrate.common(h_compound)
                    total_length += clipped_h.Length
                if v_edges:
                    v_compound = Part.Compound(v_edges)
                    v_compound.Placement = tr
                    clipped_v = substrate.common(v_compound)
                    total_length += clipped_v.Length
                quantities.length_joints = total_length
            except Exception:
                pass
        elif status == TessellationStatus.EXTREME_COUNT:
            # Analytical fallback for mosaic-like counts. The statistical estimation of grid length
            # is based on density
            # Total length approx (Area / Step_U) + (Area / Step_V)
            # This statistical total includes the geometry of the "field". To approximate internal
            # joints consistent with the geometric mode (which mostly excludes boundaries), we
            # subtract the perimeter.
            stat_total = (substrate.Area / step_u) + (substrate.Area / step_v)
            # Clamp to 0 to avoid negative values on edge cases
            quantities.length_joints = max(0.0, stat_total - quantities.length_perimeter)

        # Geometry generation

        # Analytical mode (monolithic tile)
        if status != TessellationStatus.OK:
            # Mathematical quantity fallback
            # Use rounding to 6 decimal places to handle geometric epsilon errors deterministically
            count_rounded = round(substrate.Area / unit_area, 6)
            quantities.count_full = int(count_rounded)
            quantities.count_partial = 1 if (count_rounded - quantities.count_full) > 0 else 0
            quantities.area_gross = (quantities.count_full + quantities.count_partial) * unit_area
            quantities.waste_area = max(0.0, quantities.area_gross - quantities.area_net)

            if self.thickness > 0:
                monolithic = substrate.extrude(normal * self.thickness)
            else:
                mat = FreeCAD.Matrix()
                mat.move(normal.normalize() * 0.05)
                monolithic = substrate.transformGeometry(mat)

            final_geo = monolithic
            # Visual overlay if available
            if final_cl:
                final_geo = Part.Compound([monolithic, final_cl])

            return TessellationResult(
                final_geo, final_cl, quantities, unit_area, unit_volume, status
            )

        # Physical mode (Boolean cut)
        cutters_h, cutters_v = [], []
        start_u, start_v = -count_u * step_u, -count_v * step_v
        full_len_u, full_len_v = 2 * count_u * step_u, 2 * count_v * step_v
        z_off = -0.5 if self.thickness == 0 else -0.05
        cut_thick = 1.0 if self.thickness == 0 else self.thickness * 1.1

        # Horizontal cutters
        for j in range(-count_v, count_v):
            rv_off = self.offset_v if (j % 2 != 0) else 0
            cutters_h.append(
                Part.makeBox(
                    full_len_u,
                    self.joint,
                    cut_thick,
                    FreeCAD.Vector(start_u, j * step_v + self.width + rv_off, z_off),
                )
            )

        # Vertical cutters
        is_stack = abs(self.offset_u) < 1e-6 and abs(self.offset_v) < 1e-6
        for i in range(-count_u, count_u):
            lu = i * step_u + self.length
            if is_stack:
                cutters_v.append(
                    Part.makeBox(
                        self.joint, full_len_v, cut_thick, FreeCAD.Vector(lu, start_v, z_off)
                    )
                )
            else:
                for j in range(-count_v, count_v):
                    rv = j * step_v + (self.offset_v if (j % 2 != 0) else 0)
                    cutters_v.append(
                        Part.makeBox(
                            self.joint,
                            step_v,
                            cut_thick,
                            FreeCAD.Vector(lu + (self.offset_u if (j % 2 != 0) else 0), rv, z_off),
                        )
                    )

        # Perform cuts
        comp_h = Part.Compound(cutters_h) if cutters_h else None
        comp_v = Part.Compound(cutters_v) if cutters_v else None
        if comp_h:
            comp_h.Placement = tr
        if comp_v:
            comp_v.Placement = tr

        if self.thickness > 0:
            layer = substrate.extrude(normal * self.thickness)
            if comp_h:
                layer = layer.cut(comp_h)
            if comp_v:
                layer = layer.cut(comp_v)
            final_geo = layer

            # Geometric counting
            full_cnt, part_cnt = 0, 0
            if final_geo.Solids:
                for sol in final_geo.Solids:
                    if sol.Volume >= (unit_volume * 0.995):
                        full_cnt += 1
                    else:
                        part_cnt += 1
        else:
            layer = substrate
            if comp_h:
                layer = layer.cut(comp_h)
            if comp_v:
                layer = layer.cut(comp_v)

            # Geometric counting
            full_cnt, part_cnt = 0, 0
            if layer.Faces:
                for f in layer.Faces:
                    if f.Area >= (unit_area * 0.995):
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
        quantities.area_gross = (full_cnt + part_cnt) * unit_area
        quantities.waste_area = max(0.0, quantities.area_gross - quantities.area_net)

        return TessellationResult(final_geo, final_cl, quantities, unit_area, unit_volume, status)


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
