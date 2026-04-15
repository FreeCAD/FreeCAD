#!/usr/bin/env python
# -*- coding: utf-8 -*-
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
# ***************************************************************************/

import FreeCAD

if FreeCAD.GuiUp:
    from pivy import coin


class GhostHatchRenderer:
    """
    Renders hatch patterns directly as Coin3D scene graph nodes.

    No document objects. No recompute. Instant display.

    Uses the same surface-projection pipeline as CustomHatchFP.execute().
    """

    def __init__(self, view_object):
        self.view_object = view_object
        self.root = None
        self._pending_root = None  # Track pending injection to cancel stale renders
        self._render_serial = 0    # Increment to invalidate stale async calls
        self._color = (0.2, 0.6, 1.0)

    def update_multi(self, render_pairs):
        """Render hatch for multiple (faces, pattern, source_obj) pairs."""
        if not FreeCAD.GuiUp:
            return

        self.clear()
        if not render_pairs:
            return

        try:
            import Part
            from .ArchHatchCore import buildHatchShape
            from .ArchHatch import (
                build_surface_transforms,
                project_shape_to_local,
                unproject_shape_to_world,
                convertBaseSpacingValue,
                get_face_local_frame,
                get_preferred_face_normal_from_source,
            )
            from .ArchHatchPatterns import generate_built_in_pattern_shape as generateBuiltInPatternShape


        except ImportError as e:
            FreeCAD.Console.PrintWarning(f"Ghost hatch: import failed: {e}\n")
            return

        def _resolve_pattern_shape(source_obj):
            if not source_obj:
                return None

            if getattr(source_obj, "HatchRole", "Applied") == "Definition":
                pattern_type = getattr(source_obj, "PatternType", "CustomObject")
                if pattern_type != "CustomObject":
                    try:
                        return generateBuiltInPatternShape(pattern_type)
                    except Exception:
                        return None

                all_shapes = []
                pattern_obj = getattr(source_obj, "PatternObject", None)
                if pattern_obj and hasattr(pattern_obj, "Shape") and not pattern_obj.Shape.isNull():
                    shape_copy = pattern_obj.Shape.copy()
                    if shape_copy and not shape_copy.isNull():
                        all_shapes.append(shape_copy)

                for extra in (getattr(source_obj, "PatternObjects", None) or []):
                    if extra and hasattr(extra, "Shape") and not extra.Shape.isNull():
                        shape_copy = extra.Shape.copy()
                        if shape_copy and not shape_copy.isNull():
                            all_shapes.append(shape_copy)

                if not all_shapes:
                    return None
                if len(all_shapes) == 1:
                    return all_shapes[0]

                fused = all_shapes[0]
                for extra in all_shapes[1:]:
                    fused = fused.fuse(extra)
                return fused

            if hasattr(source_obj, "Shape") and not source_obj.Shape.isNull():
                return source_obj.Shape.copy()
            return None

        all_edges = []

        for pair in render_pairs:
            if not pair:
                continue

            if len(pair) >= 3:
                faces, hatch_pattern_obj, base_source_obj = pair[0], pair[1], pair[2]
            else:
                faces, hatch_pattern_obj = pair[0], pair[1]
                base_source_obj = None

            if not faces or not hatch_pattern_obj:
                continue

            pattern_shape = _resolve_pattern_shape(hatch_pattern_obj)
            if pattern_shape is None or pattern_shape.isNull():
                continue

            spacing = float(getattr(hatch_pattern_obj, "BaseSpacing", 100.0))
            use_units = getattr(hatch_pattern_obj, "UseUnits", False)
            if use_units:
                try:
                    unit_system = getattr(
                        hatch_pattern_obj, "SelectedUnitSystem", "Metric"
                    )
                    spacing = convertBaseSpacingValue(spacing, use_units, unit_system)
                except Exception:
                    pass

            hard_max = int(getattr(hatch_pattern_obj, "MaxTilesAllowed", 1000))
            ghost_max = min(max(hard_max, 200), 600)

            kwargs = dict(
                patternShape=pattern_shape,
                distributionMode=getattr(
                    hatch_pattern_obj, "DistributionMode", "SeamlessTiling"
                ),
                autoScaleToFitBase=bool(
                    getattr(hatch_pattern_obj, "AutoScaleToFitBase", False)
                ),
                patternScale=float(getattr(hatch_pattern_obj, "PatternScale", 1.0)),
                rotationDeg=float(getattr(hatch_pattern_obj, "RotationDeg", 0.0)),
                baseSpacing=spacing,
                repX=int(getattr(hatch_pattern_obj, "RepetitionsX", 5)),
                repY=int(getattr(hatch_pattern_obj, "RepetitionsY", 5)),
                randRotMin=float(getattr(hatch_pattern_obj, "RandomRotationMin", 0.0)),
                randRotMax=float(getattr(hatch_pattern_obj, "RandomRotationMax", 0.0)),
                randomizePlacement=bool(
                    getattr(hatch_pattern_obj, "RandomizePlacement", False)
                ),
                randomOffsetRange=float(
                    getattr(hatch_pattern_obj, "RandomOffsetRange", 0.0)
                ),
                randomScaleMin=float(getattr(hatch_pattern_obj, "RandomScaleMin", 1.0)),
                randomScaleMax=float(getattr(hatch_pattern_obj, "RandomScaleMax", 1.0)),
                radialCount=int(getattr(hatch_pattern_obj, "RadialCount", 8)),
                radialRadius=float(getattr(hatch_pattern_obj, "RadialRadius", 50.0)),
                concentricCount=int(
                    getattr(hatch_pattern_obj, "ConcentricCount", 5)
                ),
                concentricSpacing=float(
                    getattr(hatch_pattern_obj, "ConcentricSpacing", 10.0)
                ),
                randomCount=int(getattr(hatch_pattern_obj, "RandomCount", 30)),
                offsetX=float(getattr(hatch_pattern_obj, "PatternOffsetX", 0.0)),
                offsetY=float(getattr(hatch_pattern_obj, "PatternOffsetY", 0.0)),
                scaleMode=getattr(hatch_pattern_obj, "ScaleMode", "Absolute"),
                tileShape=(
                    getattr(getattr(hatch_pattern_obj, "BaseTileObject", None), "Shape", None)
                    if getattr(hatch_pattern_obj, "BaseTileObject", None)
                    else None
                ),
                tileVisibility=False,
                showFaces=False,
                maxTiles=ghost_max,
                densityFactor=float(getattr(hatch_pattern_obj, "DensityFactor", 1.0)),
                enableColorVar=False,
                colorVarInt=0.0,
                spacingVariation=float(
                    getattr(hatch_pattern_obj, "SpacingVariation", 0.0)
                ),
                shapeDistortion=bool(
                    getattr(hatch_pattern_obj, "EnableShapeDistortion", False)
                ),
                apply3D=False,
                placement_mode=getattr(
                    hatch_pattern_obj, "PatternPlacementMode", "Origin"
                ),
                clipMode=getattr(hatch_pattern_obj, "ClipMode", "BooleanOnly"),
            )

            preferred_normal = get_preferred_face_normal_from_source(base_source_obj)

            grouped = {}
            for face in faces:
                try:
                    center, _x_axis, _y_axis, normal = get_face_local_frame(
                        face,
                        preferred_normal=preferred_normal,
                    )
                    plane_d = round(normal.dot(center), 6)
                    signature = (
                        round(normal.x, 6),
                        round(normal.y, 6),
                        round(normal.z, 6),
                        plane_d,
                    )
                    grouped.setdefault(signature, []).append(face)
                except Exception:
                    grouped.setdefault(("fallback", id(face)), []).append(face)

            for _signature, face_group in grouped.items():
                projected = []
                transforms = []

                # FIX: Use raw offsets directly (same as final execution path)
                preview_offset_x = float(getattr(hatch_pattern_obj, "PatternOffsetX", 0.0))
                preview_offset_y = float(getattr(hatch_pattern_obj, "PatternOffsetY", 0.0))

                for face in face_group:
                    try:
                        to_local, to_world = build_surface_transforms(
                            face,
                            preferred_normal=preferred_normal,
                        )

                        local_base = project_shape_to_local(face, to_local)

                        if local_base.Faces:
                            try:
                                local_base = Part.Face(local_base.Faces[0].OuterWire)
                            except Exception:
                                try:
                                    local_base = local_base.Faces[0]
                                except Exception:
                                    pass

                        projected.append(local_base)
                        transforms.append(
                            (local_base, to_world, preview_offset_x, preview_offset_y)
                        )
                    except Exception as e:
                        FreeCAD.Console.PrintWarning(
                            f"Ghost hatch projection failed: {e}\n"
                        )

                if not transforms:
                    continue

                override_bb = None
                try:
                    if len(projected) == 1:
                        override_bb = projected[0].BoundBox
                    else:
                        override_bb = Part.makeCompound(projected).BoundBox
                except Exception:
                    override_bb = None

                for local_base, to_world, offset_x, offset_y in transforms:
                    try:
                        local_kwargs = dict(kwargs)
                        local_kwargs["offsetX"] = offset_x
                        local_kwargs["offsetY"] = offset_y

                        hatch_local, _tiles = buildHatchShape(
                            baseShape=local_base,
                            overrideBB=override_bb,
                            **local_kwargs,
                        )
                        if hatch_local and not hatch_local.isNull():
                            hatch_world = unproject_shape_to_world(hatch_local, to_world)
                            all_edges.extend(hatch_world.Edges)
                    except Exception as e:
                        FreeCAD.Console.PrintWarning(
                            f"Ghost hatch face failed: {e}\n"
                        )
                        continue

        if not all_edges:
            return

        self._render_edges(all_edges)

    def update(self, faces, hatch_pattern_obj, base_source_obj=None):
        """Single-pair convenience wrapper for backward compatibility."""
        self.update_multi([(faces, hatch_pattern_obj, base_source_obj)])

    def _render_edges(self, edges):
        """Build coin3D line set from edges and inject into scene graph."""
        separator = coin.SoSeparator()

        material = coin.SoMaterial()
        material.diffuseColor.setValue(*self._color)
        material.transparency.setValue(0.3)
        separator.addChild(material)

        draw_style = coin.SoDrawStyle()
        draw_style.lineWidth = 1.5
        separator.addChild(draw_style)

        coordinates = coin.SoCoordinate3()
        line_set = coin.SoIndexedLineSet()
        points = []
        indices = []
        idx = 0

        for edge in edges:
            try:
                points_edge = edge.discretize(10)
                for point in points_edge:
                    points.append((point.x, point.y, point.z))
                indices.extend(list(range(idx, idx + len(points_edge))) + [-1])
                idx += len(points_edge)
            except Exception:
                continue

        if not points:
            return

        coordinates.point.setValues(0, len(points), points)
        line_set.coordIndex.setValues(0, len(indices), indices)
        separator.addChild(coordinates)
        separator.addChild(line_set)

        view_object = self.view_object
        self._render_serial += 1
        render_serial = self._render_serial
        self._pending_root = separator

        def _inject():
            try:
                # If a newer render/clear happened meanwhile, discard this stale node.
                if render_serial != self._render_serial:
                    return
                if self._pending_root is not separator:
                    return

                view_object.RootNode.addChild(separator)
                self.root = separator
                self._pending_root = None
            except Exception as e:
                FreeCAD.Console.PrintWarning(f"Ghost hatch inject failed: {e}\n")

        from PySide import QtCore
        QtCore.QTimer.singleShot(0, _inject)

    def clear(self):
        """Remove ghost geometry from the scene graph and cancel pending injects."""
        self._render_serial += 1
        self._pending_root = None

        if self.root is not None:
            try:
                self.view_object.RootNode.removeChild(self.root)
            except Exception:
                pass
            self.root = None