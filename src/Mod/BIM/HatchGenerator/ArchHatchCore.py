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

import FreeCAD
import Part
import math
import random


def shape_to_edges(shape):
    """Convert a shape to a compound of edges."""
    if shape.isNull():
        return shape
    edges = shape.Edges
    if not edges:
        return shape
    compound = Part.makeCompound(edges)
    return compound


def normalizePatternShape(shape):
    """
    Normalize pattern shape to origin (0,0) based on its minimum corner.

    This restores the expected semantics for PatternPlacementMode="Origin":
    the pattern's local origin stays at the tile anchor instead of being
    re-centered around its bbox center.
    """
    if shape.isNull():
        return shape, None

    bounding_box = shape.BoundBox
    if not bounding_box.isValid():
        return shape, bounding_box

    dx = -bounding_box.XMin
    dy = -bounding_box.YMin

    translation = FreeCAD.Matrix()
    translation.move(FreeCAD.Vector(dx, dy, 0))

    normalized = shape.copy()
    normalized.transformShape(translation)
    return normalized, normalized.BoundBox


def clipShapeToBase(baseShape, tileShape, clipMode):
    """Clip tile shape against base shape."""
    if tileShape.isNull():
        return None

    if clipMode == "PreserveLinesNoClip":
        return tileShape

    try:
        common = baseShape.common(tileShape)
        if common and not common.isNull():
            if common.Faces or common.Edges:
                return common
        return None
    except Exception as e:
        FreeCAD.Console.PrintError(f"Clip failed: {str(e)}\n")
        return None


def _face_fills_bbox_rectangle(face, tol=1e-6):
    """
    Return True only when the face completely fills its own axis-aligned bbox.

    This is the conservative condition required by the SeamlessTiling Zone B
    optimization. A single face with one outer wire is NOT sufficient: concave
    polygons and sloped polygons can still leave large regions inside the bbox
    that are outside the actual face.
    """
    if face is None:
        return False

    try:
        if len(face.Wires) != 1:
            return False
    except Exception:
        return False

    try:
        bb = face.BoundBox
        if not bb.isValid():
            return False
    except Exception:
        return False

    bbox_area = bb.XLength * bb.YLength
    if bbox_area <= tol:
        return False

    try:
        face_area = face.Area
    except Exception:
        return False

    return abs(face_area - bbox_area) <= max(tol, bbox_area * 1e-6)


def _get_bbox_anchor(xmin, xmax, ymin, ymax, placement_mode):
    """
    Return the anchor point inside a bounding box for a given placement mode.

    For Origin, keep the literal local origin (0,0) semantics instead of
    snapping to bbox min corner.
    """
    center_x = 0.5 * (xmin + xmax)
    center_y = 0.5 * (ymin + ymax)

    if placement_mode == "Center":
        return center_x, center_y
    elif placement_mode == "TopLeft":
        return xmin, ymax
    elif placement_mode == "TopRight":
        return xmax, ymax
    elif placement_mode == "BottomLeft":
        return xmin, ymin
    elif placement_mode == "BottomRight":
        return xmax, ymin
    elif placement_mode == "TopCenter":
        return center_x, ymax
    elif placement_mode == "BottomCenter":
        return center_x, ymin
    elif placement_mode == "LeftCenter":
        return xmin, center_y
    elif placement_mode == "RightCenter":
        return xmax, center_y
    else:
        return 0.0, 0.0


def _compute_transformed_tile_bounds(
    tile_x, tile_y, tile_width, tile_height, base_scale, base_rot_rad, placement_mode
):
    """
    Conservative broad-phase bounds for one tile after:
      1) scaling
      2) rotation around local origin
      3) placement alignment
      4) translation to (tile_x, tile_y)
    """
    scaled_width = max(tile_width * base_scale, 0.0)
    scaled_height = max(tile_height * base_scale, 0.0)

    if scaled_width <= 1e-12 and scaled_height <= 1e-12:
        return tile_x, tile_x, tile_y, tile_y

    points = [
        (0.0, 0.0),
        (scaled_width, 0.0),
        (scaled_width, scaled_height),
        (0.0, scaled_height),
    ]

    cos_angle = math.cos(base_rot_rad)
    sin_angle = math.sin(base_rot_rad)

    rotated_points = []
    for px, py in points:
        rotated_x = (px * cos_angle) - (py * sin_angle)
        rotated_y = (px * sin_angle) + (py * cos_angle)
        rotated_points.append((rotated_x, rotated_y))

    xmin = min(p[0] for p in rotated_points)
    xmax = max(p[0] for p in rotated_points)
    ymin = min(p[1] for p in rotated_points)
    ymax = max(p[1] for p in rotated_points)

    anchor_x, anchor_y = _get_bbox_anchor(xmin, xmax, ymin, ymax, placement_mode)

    xmin = xmin - anchor_x + tile_x
    xmax = xmax - anchor_x + tile_x
    ymin = ymin - anchor_y + tile_y
    ymax = ymax - anchor_y + tile_y

    return xmin, xmax, ymin, ymax


def make_tile_and_clip(
    base_shape,
    normed_pattern,
    tile_x,
    tile_y,
    base_scale,
    base_rot_rad,
    randomize,
    random_offset_range,
    random_rot_min,
    random_rot_max,
    random_scale_min,
    random_scale_max,
    show_faces,
    shape_distortion,
    enable_color_var,
    color_var_intensity,
    placement_mode="Center",
    clip_mode="BooleanOnly",
    skip_clip=False,
):
    """
    Create a single tile instance, transform it, and clip it against the base shape.

    When skip_clip=True the OCC common() call is skipped entirely.  Only pass
    True when the caller has already proven the tile cannot overlap the face
    boundary (Zone B interior tiles in buildHatchShape).
    """
    tile_copy = normed_pattern.copy()

    if shape_distortion:
        try:
            scale_x = random.uniform(0.8, 1.2)
            scale_y = random.uniform(0.8, 1.2)
            shear = random.uniform(-0.3, 0.3)
            distortion = FreeCAD.Matrix()
            distortion.A11 = scale_x
            distortion.A12 = shear
            distortion.A21 = 0.0
            distortion.A22 = scale_y
            tile_copy.transformShape(distortion)
        except Exception:
            pass

    final_scale = base_scale
    if randomize and random_scale_max >= random_scale_min and random_scale_min > 0:
        scale_factor = random.uniform(max(0.0001, random_scale_min), random_scale_max)
        final_scale *= scale_factor

    scale_matrix = FreeCAD.Matrix()
    scale_matrix.scale(final_scale, final_scale, final_scale)
    tile_copy.transformShape(scale_matrix)

    final_rotation = base_rot_rad
    if randomize and random_rot_max > random_rot_min:
        final_rotation += math.radians(random.uniform(random_rot_min, random_rot_max))

    rotation_matrix = FreeCAD.Matrix()
    rotation_matrix.rotateZ(final_rotation)
    tile_copy.transformShape(rotation_matrix)

    bounding_box = tile_copy.BoundBox
    anchor_x, anchor_y = _get_bbox_anchor(
        bounding_box.XMin, bounding_box.XMax, bounding_box.YMin, bounding_box.YMax, placement_mode
    )

    if abs(anchor_x) > 1e-12 or abs(anchor_y) > 1e-12:
        placement_matrix = FreeCAD.Matrix()
        placement_matrix.move(FreeCAD.Vector(-anchor_x, -anchor_y, 0))
        tile_copy.transformShape(placement_matrix)

    if randomize and random_offset_range > 0:
        jitter_x = random.uniform(-random_offset_range, random_offset_range)
        jitter_y = random.uniform(-random_offset_range, random_offset_range)
        jitter_matrix = FreeCAD.Matrix()
        jitter_matrix.move(FreeCAD.Vector(jitter_x, jitter_y, 0))
        tile_copy.transformShape(jitter_matrix)

    translation_matrix = FreeCAD.Matrix()
    translation_matrix.move(FreeCAD.Vector(tile_x, tile_y, 0))
    tile_copy.transformShape(translation_matrix)

    if show_faces:
        try:
            faces = []
            for sub in tile_copy.SubShapes:
                if isinstance(sub, Part.Wire):
                    faces.append(Part.Face(sub))
                elif isinstance(sub, Part.Face):
                    faces.append(sub)
            if faces:
                tile_copy = Part.makeCompound(faces)
        except Exception as ex:
            FreeCAD.Console.PrintWarning(f"makeFace() failed: {str(ex)}\n")
    else:
        tile_copy = shape_to_edges(tile_copy)

    if skip_clip:
        # Zone B: tile proven fully interior — skip OCC boolean entirely.
        result = tile_copy
    else:
        try:
            result = clipShapeToBase(base_shape, tile_copy, clip_mode)
        except Exception as e:
            FreeCAD.Console.PrintError(f"Boolean operation error: {str(e)}\n")
            return None, None

    random_color = None
    if enable_color_var:
        import colorsys

        hue = random.random()
        saturation = 0.5 + 0.5 * color_var_intensity
        value = 1.0
        random_color = colorsys.hsv_to_rgb(hue, saturation, value)

    return result, random_color


def separate_faces_and_edges(shape):
    """Return (faces, edges) from a shape."""
    if shape.isNull():
        return [], []
    return shape.Faces, shape.Edges


def buildHatchShape(
    baseShape,
    overrideBB=None,
    patternShape=None,
    distributionMode="SeamlessTiling",
    autoScaleToFitBase=False,
    patternScale=1.0,
    rotationDeg=0.0,
    gridRotationDeg=None,
    tileRotationDeg=None,
    baseSpacing=10.0,
    repX=5,
    repY=5,
    randRotMin=0.0,
    randRotMax=0.0,
    randomizePlacement=False,
    randomOffsetRange=0.0,
    randomScaleMin=1.0,
    randomScaleMax=1.0,
    radialCount=8,
    radialRadius=50.0,
    concentricCount=5,
    concentricSpacing=10.0,
    randomCount=30,
    offsetX=0.0,
    offsetY=0.0,
    scaleMode="Absolute",
    tileShape=None,
    tileVisibility=False,
    showFaces=False,
    maxTiles=5000,
    densityFactor=1.0,
    enableColorVar=False,
    colorVarInt=0.5,
    spacingVariation=0.0,
    shapeDistortion=False,
    apply3D=False,
    placement_mode="Origin",
    clipMode="BooleanOnly",
):
    """
    Generate hatch pattern on base shape.

    Returns (hatch_shape, tile_count).
    """
    if baseShape.isNull():
        return Part.makeCompound([]), 0

    if patternShape is None or patternShape.isNull():
        return Part.makeCompound([]), 0

    bounding_box = overrideBB if overrideBB is not None else baseShape.BoundBox
    if bounding_box is None:
        return Part.makeCompound([]), 0
    try:
        if not bounding_box.isValid():
            return Part.makeCompound([]), 0
    except Exception:
        return Part.makeCompound([]), 0

    normed_pattern, pattern_bbox = normalizePatternShape(patternShape)
    if normed_pattern.isNull():
        return Part.makeCompound([]), 0

    tile_width = max(pattern_bbox.XLength, 1e-6)
    tile_height = max(pattern_bbox.YLength, 1e-6)

    if tileShape is not None and not tileShape.isNull():
        tile_normed, tile_bbox = normalizePatternShape(tileShape)
        if not tile_normed.isNull():
            tile_width = max(tile_bbox.XLength, 1e-6)
            tile_height = max(tile_bbox.YLength, 1e-6)

    if autoScaleToFitBase:
        base_width = bounding_box.XLength
        base_height = bounding_box.YLength
        if tile_width > 1e-6 and tile_height > 1e-6:
            scale_x = base_width / tile_width
            scale_y = base_height / tile_height
            if scaleMode == "FitWidth":
                patternScale = scale_x
            elif scaleMode == "FitHeight":
                patternScale = scale_y
            elif scaleMode == "FitMinDim":
                patternScale = min(scale_x, scale_y)
            elif scaleMode == "FitMaxDim":
                patternScale = max(scale_x, scale_y)
            else:
                patternScale = (scale_x + scale_y) * 0.5

    if gridRotationDeg is None:
        gridRotationDeg = 0.0
    if tileRotationDeg is None:
        tileRotationDeg = rotationDeg

    grid_rotation_rad = math.radians(gridRotationDeg)
    tile_rotation_rad = math.radians(tileRotationDeg)
    effective_spacing = max(baseSpacing, 1e-6)

    step_x = tile_width * patternScale + effective_spacing
    step_y = tile_height * patternScale + effective_spacing

    if distributionMode == "SeamlessTiling":
        if step_x <= 0 or step_y <= 0:
            return Part.makeCompound([]), 0

        grid_ref_x = offsetX
        grid_ref_y = offsetY
        cos_grid = math.cos(grid_rotation_rad)
        sin_grid = math.sin(grid_rotation_rad)

        corners = [
            (bounding_box.XMin, bounding_box.YMin),
            (bounding_box.XMin, bounding_box.YMax),
            (bounding_box.XMax, bounding_box.YMin),
            (bounding_box.XMax, bounding_box.YMax),
        ]
        local_xs = []
        local_ys = []
        for cx, cy in corners:
            rel_x = cx - grid_ref_x
            rel_y = cy - grid_ref_y
            local_xs.append(rel_x * cos_grid + rel_y * sin_grid)
            local_ys.append(-rel_x * sin_grid + rel_y * cos_grid)

        coverage_margin = max(step_x, step_y) + math.sqrt(
            (tile_width * patternScale) ** 2 + (tile_height * patternScale) ** 2
        )
        i_start = int(math.floor((min(local_xs) - coverage_margin) / step_x))
        i_end = int(math.ceil((max(local_xs) + coverage_margin) / step_x))
        j_start = int(math.floor((min(local_ys) - coverage_margin) / step_y))
        j_end = int(math.ceil((max(local_ys) + coverage_margin) / step_y))

        tile_count = 0
        tile_parts = []
        tile_colors = []

        # --- Zone B safe-interior setup ---
        # A tile whose axis-aligned bounds fit entirely within the base bbox
        # shrunk by the tile diagonal on all sides cannot possibly straddle the
        # face boundary.  For those tiles we skip OCC common() (Zone C path)
        # and build them directly (Zone B path).
        #
        # Guards — all must hold or we fall back to always-clip:
        #   • rotationDeg == 0  (non-zero rotation makes the AA bbox too loose
        #     to guarantee interior; can be tightened later by computing the
        #     rotated tile AABB, but 0° covers most section-hatch usage)
        #   • not randomizePlacement / not shapeDistortion  (same as broadphase
        #     cull guard — random jitter makes the bounds unreliable)
        #   • baseShape is a single face that COMPLETELY FILLS its axis-aligned
        #     bbox in the current frame. This keeps Zone B only for true
        #     rectangle-like bases. Concave polygons, triangles, H-shapes, and
        #     any sloped polygon do not satisfy this and must always clip.
        _tile_diag = math.sqrt((tile_width * patternScale) ** 2 + (tile_height * patternScale) ** 2)
        try:
            _base_is_safe_bbox_rect = (
                gridRotationDeg == 0.0
                and tileRotationDeg == 0.0
                and not randomizePlacement
                and not shapeDistortion
                and len(baseShape.Faces) == 1
                and _face_fills_bbox_rectangle(baseShape.Faces[0])
            )
        except Exception:
            _base_is_safe_bbox_rect = False

        if _base_is_safe_bbox_rect:
            _safe_xmin = bounding_box.XMin + _tile_diag
            _safe_xmax = bounding_box.XMax - _tile_diag
            _safe_ymin = bounding_box.YMin + _tile_diag
            _safe_ymax = bounding_box.YMax - _tile_diag
            _has_safe_interior = _safe_xmax > _safe_xmin and _safe_ymax > _safe_ymin
        else:
            _has_safe_interior = False
            _safe_xmin = _safe_xmax = _safe_ymin = _safe_ymax = 0.0

        def tile_and_clip(tile_x, tile_y, base_scale, base_rot_rad, placement_mode):
            nonlocal tile_count

            can_broadphase_cull = (not randomizePlacement) and (not shapeDistortion)

            if can_broadphase_cull:
                minx, maxx, miny, maxy = _compute_transformed_tile_bounds(
                    tile_x=tile_x,
                    tile_y=tile_y,
                    tile_width=tile_width,
                    tile_height=tile_height,
                    base_scale=base_scale,
                    base_rot_rad=base_rot_rad,
                    placement_mode=placement_mode,
                )

                if (
                    maxx < bounding_box.XMin
                    or minx > bounding_box.XMax
                    or maxy < bounding_box.YMin
                    or miny > bounding_box.YMax
                ):
                    return None, None  # Zone A — completely outside bbox

                # Zone B — tile entirely inside the safe interior shrunk by
                # tile_diag.  It cannot straddle the face boundary, so we can
                # build it without an OCC boolean.
                # NOTE: _has_safe_interior already implies can_broadphase_cull,
                # so minx/maxx/miny/maxy are always available here.
                if _has_safe_interior and (
                    minx >= _safe_xmin
                    and maxx <= _safe_xmax
                    and miny >= _safe_ymin
                    and maxy <= _safe_ymax
                ):
                    tile_shape, tile_color = make_tile_and_clip(
                        baseShape,
                        normed_pattern,
                        tile_x,
                        tile_y,
                        base_scale,
                        base_rot_rad,
                        randomizePlacement,
                        randomOffsetRange,
                        randRotMin,
                        randRotMax,
                        randomScaleMin,
                        randomScaleMax,
                        showFaces,
                        shapeDistortion,
                        enableColorVar,
                        colorVarInt,
                        placement_mode=placement_mode,
                        clip_mode=clipMode,
                        skip_clip=True,  # Zone B: provably interior, no clip needed
                    )
                    if tile_shape and not tile_shape.isNull():
                        tile_count += 1
                    return tile_shape, tile_color

            # Zone C — tile intersects the face boundary; must clip.
            tile_shape, tile_color = make_tile_and_clip(
                baseShape,
                normed_pattern,
                tile_x,
                tile_y,
                base_scale,
                base_rot_rad,
                randomizePlacement,
                randomOffsetRange,
                randRotMin,
                randRotMax,
                randomScaleMin,
                randomScaleMax,
                showFaces,
                shapeDistortion,
                enableColorVar,
                colorVarInt,
                placement_mode=placement_mode,
                clip_mode=clipMode,
            )

            if tile_shape and not tile_shape.isNull():
                tile_count += 1

            return tile_shape, tile_color

        for i in range(i_start, i_end + 1):
            if tile_count >= maxTiles:
                break
            for j in range(j_start, j_end + 1):
                if tile_count >= maxTiles:
                    break
                tile_x = grid_ref_x + (i * step_x * cos_grid) - (j * step_y * sin_grid)
                tile_y = grid_ref_y + (i * step_x * sin_grid) + (j * step_y * cos_grid)
                tile, color = tile_and_clip(
                    tile_x, tile_y, patternScale, tile_rotation_rad, placement_mode
                )
                if tile and not tile.isNull():
                    tile_parts.append(tile)
                    if color:
                        tile_colors.append(color)

        if not tile_parts:
            return Part.makeCompound([]), 0

        result = tile_parts[0]
        for part in tile_parts[1:]:
            result = result.fuse(part)

        if enableColorVar and tile_colors and hasattr(result, "ViewObject"):
            try:
                result.ViewObject.ShapeColor = tile_colors[0]
            except Exception:
                pass

        return result, tile_count

    elif distributionMode == "LinearGrid":
        cos_grid = math.cos(grid_rotation_rad)
        sin_grid = math.sin(grid_rotation_rad)

        tile_count = 0
        tile_parts = []
        tile_colors = []

        if placement_mode == "Center":
            center_x = bounding_box.Center.x + offsetX
            center_y = bounding_box.Center.y + offsetY
            for i in range(repX):
                if tile_count >= maxTiles:
                    break
                for j in range(repY):
                    if tile_count >= maxTiles:
                        break
                    local_i = i - (repX - 1) * 0.5
                    local_j = j - (repY - 1) * 0.5
                    tile_x = center_x + local_i * step_x * cos_grid - local_j * step_y * sin_grid
                    tile_y = center_y + local_i * step_x * sin_grid + local_j * step_y * cos_grid
                    tile, color = make_tile_and_clip(
                        baseShape,
                        normed_pattern,
                        tile_x,
                        tile_y,
                        patternScale,
                        tile_rotation_rad,
                        randomizePlacement,
                        randomOffsetRange,
                        randRotMin,
                        randRotMax,
                        randomScaleMin,
                        randomScaleMax,
                        showFaces,
                        shapeDistortion,
                        enableColorVar,
                        colorVarInt,
                        placement_mode=placement_mode,
                        clip_mode=clipMode,
                    )
                    if tile and not tile.isNull():
                        tile_parts.append(tile)
                        tile_count += 1
                        if color:
                            tile_colors.append(color)
        else:
            start_x = bounding_box.XMin + offsetX
            start_y = bounding_box.YMin + offsetY
            for i in range(repX):
                if tile_count >= maxTiles:
                    break
                for j in range(repY):
                    if tile_count >= maxTiles:
                        break
                    tile_x = start_x + i * step_x * cos_grid - j * step_y * sin_grid
                    tile_y = start_y + i * step_x * sin_grid + j * step_y * cos_grid
                    tile, color = make_tile_and_clip(
                        baseShape,
                        normed_pattern,
                        tile_x,
                        tile_y,
                        patternScale,
                        tile_rotation_rad,
                        randomizePlacement,
                        randomOffsetRange,
                        randRotMin,
                        randRotMax,
                        randomScaleMin,
                        randomScaleMax,
                        showFaces,
                        shapeDistortion,
                        enableColorVar,
                        colorVarInt,
                        placement_mode=placement_mode,
                        clip_mode=clipMode,
                    )
                    if tile and not tile.isNull():
                        tile_parts.append(tile)
                        tile_count += 1
                        if color:
                            tile_colors.append(color)

        if not tile_parts:
            return Part.makeCompound([]), 0

        result = tile_parts[0]
        for part in tile_parts[1:]:
            result = result.fuse(part)

        if enableColorVar and tile_colors and hasattr(result, "ViewObject"):
            try:
                result.ViewObject.ShapeColor = tile_colors[0]
            except Exception:
                pass

        return result, tile_count

    elif distributionMode == "RadialDistribution":
        tile_parts = []
        tile_count = 0
        center_x = bounding_box.Center.x + offsetX
        center_y = bounding_box.Center.y + offsetY

        for i in range(radialCount):
            if tile_count >= maxTiles:
                break
            angle = 2 * math.pi * i / radialCount
            tile_x = center_x + radialRadius * math.cos(angle)
            tile_y = center_y + radialRadius * math.sin(angle)
            tile, color = make_tile_and_clip(
                baseShape,
                normed_pattern,
                tile_x,
                tile_y,
                patternScale,
                tile_rotation_rad,
                randomizePlacement,
                randomOffsetRange,
                randRotMin,
                randRotMax,
                randomScaleMin,
                randomScaleMax,
                showFaces,
                shapeDistortion,
                enableColorVar,
                colorVarInt,
                placement_mode=placement_mode,
                clip_mode=clipMode,
            )
            if tile and not tile.isNull():
                tile_parts.append(tile)
                tile_count += 1

        if not tile_parts:
            return Part.makeCompound([]), 0

        result = tile_parts[0]
        for part in tile_parts[1:]:
            result = result.fuse(part)
        return result, tile_count

    elif distributionMode == "ConcentricDistribution":
        tile_parts = []
        tile_count = 0
        center_x = bounding_box.Center.x + offsetX
        center_y = bounding_box.Center.y + offsetY

        for ring in range(concentricCount):
            if tile_count >= maxTiles:
                break
            radius = (ring + 1) * concentricSpacing
            steps = max(4, int(2 * math.pi * radius / concentricSpacing))
            for i in range(steps):
                if tile_count >= maxTiles:
                    break
                angle = 2 * math.pi * i / steps
                tile_x = center_x + radius * math.cos(angle)
                tile_y = center_y + radius * math.sin(angle)
                tile, color = make_tile_and_clip(
                    baseShape,
                    normed_pattern,
                    tile_x,
                    tile_y,
                    patternScale,
                    tile_rotation_rad,
                    randomizePlacement,
                    randomOffsetRange,
                    randRotMin,
                    randRotMax,
                    randomScaleMin,
                    randomScaleMax,
                    showFaces,
                    shapeDistortion,
                    enableColorVar,
                    colorVarInt,
                    placement_mode=placement_mode,
                    clip_mode=clipMode,
                )
                if tile and not tile.isNull():
                    tile_parts.append(tile)
                    tile_count += 1

        if not tile_parts:
            return Part.makeCompound([]), 0

        result = tile_parts[0]
        for part in tile_parts[1:]:
            result = result.fuse(part)
        return result, tile_count

    elif distributionMode == "RandomDistribution":
        tile_parts = []
        tile_count = 0
        target_count = int(randomCount * densityFactor)

        for _ in range(target_count):
            if tile_count >= maxTiles:
                break
            tile_x = bounding_box.XMin + random.random() * bounding_box.XLength
            tile_y = bounding_box.YMin + random.random() * bounding_box.YLength
            tile, color = make_tile_and_clip(
                baseShape,
                normed_pattern,
                tile_x,
                tile_y,
                patternScale,
                tile_rotation_rad,
                randomizePlacement,
                randomOffsetRange,
                randRotMin,
                randRotMax,
                randomScaleMin,
                randomScaleMax,
                showFaces,
                shapeDistortion,
                enableColorVar,
                colorVarInt,
                placement_mode=placement_mode,
                clip_mode=clipMode,
            )
            if tile and not tile.isNull():
                tile_parts.append(tile)
                tile_count += 1

        if not tile_parts:
            return Part.makeCompound([]), 0

        result = tile_parts[0]
        for part in tile_parts[1:]:
            result = result.fuse(part)
        return result, tile_count

    else:
        return Part.makeCompound([]), 0
