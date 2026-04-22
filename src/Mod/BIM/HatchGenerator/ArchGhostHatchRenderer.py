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

if FreeCAD.GuiUp:
    from pivy import coin


class GhostHatchRenderer:
    """
    Renders hatch preview geometry directly as Coin3D scene graph nodes.

    No document objects. No recompute. Instant display.

    Uses the same surface-projection pipeline as CustomHatchFP.execute().

    This version also supports subtraction-mask overlays so the user can see
    which keep-out regions will be removed before recomputing the real hatch.
    """

    _LAYER_STYLES = {
        "hatch": {
            "color": (0.2, 0.6, 1.0),
            "transparency": 0.30,
            "line_width": 1.5,
        },
        "subtractions": {
            "color": (1.0, 0.45, 0.0),
            "transparency": 0.10,
            "line_width": 2.3,
        },
        "subtractions_selected": {
            "color": (1.0, 0.05, 0.05),
            "transparency": 0.0,
            "line_width": 3.0,
        },
    }

    def __init__(self, view_object):
        self.view_object = view_object
        self._roots = {
            "hatch": None,
            "subtractions": None,
            "subtractions_selected": None,
        }
        self._pending_roots = {
            "hatch": None,
            "subtractions": None,
            "subtractions_selected": None,
        }
        self._layer_serials = {
            "hatch": 0,
            "subtractions": 0,
            "subtractions_selected": 0,
        }

    def update_multi(
        self,
        render_pairs,
        subtraction_objects=None,
        selected_object_names=None,
        clip_subtractions_to_faces=True,
    ):
        """
        Render hatch preview for multiple (faces, pattern, source_obj) pairs.

        Optional subtraction_objects are rendered as a second overlay layer.
        selected_object_names, when provided, are emphasized in a stronger red.
        """
        if not FreeCAD.GuiUp:
            return

        self.clear()

        target_faces = []
        if render_pairs:
            hatch_edges, target_faces = self._collect_hatch_edges(render_pairs)
            if hatch_edges:
                self._render_edges(hatch_edges, layer="hatch")

        if subtraction_objects:
            self.update_subtractions(
                subtraction_objects=subtraction_objects,
                target_faces=target_faces if clip_subtractions_to_faces else None,
                selected_object_names=selected_object_names,
                clear_existing=False,
            )

    def update(
        self,
        faces,
        hatch_pattern_obj,
        base_source_obj=None,
        subtraction_objects=None,
        selected_object_names=None,
        clip_subtractions_to_faces=True,
    ):
        """Single-pair convenience wrapper for backward compatibility."""
        self.update_multi(
            [(faces, hatch_pattern_obj, base_source_obj)],
            subtraction_objects=subtraction_objects,
            selected_object_names=selected_object_names,
            clip_subtractions_to_faces=clip_subtractions_to_faces,
        )

    def update_subtractions(
        self,
        subtraction_objects,
        target_faces=None,
        selected_object_names=None,
        clear_existing=True,
    ):
        """
        Render subtraction-mask overlay only.

        subtraction_objects: iterable of document objects with Shape.
        target_faces: optional iterable of faces; when provided, the subtraction
                      shapes are clipped to those faces so the overlay matches
                      the actual cut region more closely.
        selected_object_names: optional iterable of Names to render with a
                               stronger red highlight.
        clear_existing: when True, clears subtraction layers first. Set False
                        when adding overlays on top of an already-rendered hatch.
        """
        if not FreeCAD.GuiUp:
            return

        if clear_existing:
            self.clear_subtractions()

        if not subtraction_objects:
            return

        selected_names = set(selected_object_names or [])
        normal_edges = []
        selected_edges = []

        for obj in subtraction_objects:
            if not obj or not hasattr(obj, "Shape") or obj.Shape.isNull():
                continue

            try:
                world_shape = self._copy_object_shape_to_world_display(obj)
                if not world_shape or world_shape.isNull():
                    continue

                clipped_shape = self._clip_shape_to_faces(world_shape, target_faces)
                if clipped_shape is None or clipped_shape.isNull():
                    clipped_shape = world_shape

                edges = getattr(clipped_shape, "Edges", []) or []
                if not edges:
                    continue

                if obj.Name in selected_names:
                    selected_edges.extend(edges)
                else:
                    normal_edges.extend(edges)
            except Exception as e:
                FreeCAD.Console.PrintWarning(
                    f"Ghost subtraction overlay failed for {getattr(obj, 'Name', 'unknown')}: {e}\n"
                )

        if normal_edges:
            self._render_edges(normal_edges, layer="subtractions")
        if selected_edges:
            self._render_edges(selected_edges, layer="subtractions_selected")

    def clear(self):
        """Remove all ghost geometry from the scene graph and cancel pending injects."""
        self._clear_layers(["hatch", "subtractions", "subtractions_selected"])

    def clear_hatch(self):
        """Remove only the hatch preview layer."""
        self._clear_layers(["hatch"])

    def clear_subtractions(self):
        """Remove only subtraction overlay layers."""
        self._clear_layers(["subtractions", "subtractions_selected"])

    # ------------------------------------------------------------------
    # Hatch preview collection
    # ------------------------------------------------------------------

    def _collect_hatch_edges(self, render_pairs):
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
            from .ArchHatchPatterns import (
                generate_built_in_pattern_shape as generateBuiltInPatternShape,
            )
        except ImportError as e:
            FreeCAD.Console.PrintWarning(f"Ghost hatch: import failed: {e}\n")
            return [], []

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

                for extra in getattr(source_obj, "PatternObjects", None) or []:
                    if extra and hasattr(extra, "Shape") and not extra.Shape.isNull():
                        shape_copy = extra.Shape.copy()
                        if shape_copy and not shape_copy.isNull():
                            all_shapes.append(shape_copy)

                if not all_shapes:
                    return None
                if len(all_shapes) == 1:
                    return all_shapes[0]

                combined = all_shapes[0]
                for extra in all_shapes[1:]:
                    combined = combined.fuse(extra)
                return combined

            if hasattr(source_obj, "Shape") and not source_obj.Shape.isNull():
                return source_obj.Shape.copy()
            return None

        all_edges = []
        all_target_faces = []

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

            all_target_faces.extend(list(faces))

            pattern_shape = _resolve_pattern_shape(hatch_pattern_obj)
            if pattern_shape is None or pattern_shape.isNull():
                continue

            spacing = float(getattr(hatch_pattern_obj, "BaseSpacing", 100.0))
            use_units = getattr(hatch_pattern_obj, "UseUnits", False)
            if use_units:
                try:
                    unit_system = getattr(hatch_pattern_obj, "SelectedUnitSystem", "Metric")
                    spacing = convertBaseSpacingValue(spacing, use_units, unit_system)
                except Exception:
                    pass

            hard_max = int(getattr(hatch_pattern_obj, "MaxTilesAllowed", 1000))
            ghost_max = min(max(hard_max, 200), 1000)

            kwargs = dict(
                patternShape=pattern_shape,
                distributionMode=getattr(hatch_pattern_obj, "DistributionMode", "SeamlessTiling"),
                autoScaleToFitBase=bool(getattr(hatch_pattern_obj, "AutoScaleToFitBase", False)),
                patternScale=float(getattr(hatch_pattern_obj, "PatternScale", 1.0)),
                rotationDeg=(
                    float(getattr(hatch_pattern_obj, "GridRotationDeg", getattr(hatch_pattern_obj, "RotationDeg", 0.0)))
                    if bool(getattr(hatch_pattern_obj, "LinkTileRotationToGrid", True))
                    else float(getattr(hatch_pattern_obj, "TileRotationDeg", getattr(hatch_pattern_obj, "RotationDeg", 0.0)))
                ),
                gridRotationDeg=float(
                    getattr(hatch_pattern_obj, "GridRotationDeg", 0.0)
                ),
                tileRotationDeg=(
                    float(getattr(hatch_pattern_obj, "GridRotationDeg", getattr(hatch_pattern_obj, "RotationDeg", 0.0)))
                    if bool(getattr(hatch_pattern_obj, "LinkTileRotationToGrid", True))
                    else float(getattr(hatch_pattern_obj, "TileRotationDeg", getattr(hatch_pattern_obj, "RotationDeg", 0.0)))
                ),
                baseSpacing=spacing,
                repX=int(getattr(hatch_pattern_obj, "RepetitionsX", 5)),
                repY=int(getattr(hatch_pattern_obj, "RepetitionsY", 5)),
                randRotMin=float(getattr(hatch_pattern_obj, "RandomRotationMin", 0.0)),
                randRotMax=float(getattr(hatch_pattern_obj, "RandomRotationMax", 0.0)),
                randomizePlacement=bool(getattr(hatch_pattern_obj, "RandomizePlacement", False)),
                randomOffsetRange=float(getattr(hatch_pattern_obj, "RandomOffsetRange", 0.0)),
                randomScaleMin=float(getattr(hatch_pattern_obj, "RandomScaleMin", 1.0)),
                randomScaleMax=float(getattr(hatch_pattern_obj, "RandomScaleMax", 1.0)),
                radialCount=int(getattr(hatch_pattern_obj, "RadialCount", 8)),
                radialRadius=float(getattr(hatch_pattern_obj, "RadialRadius", 50.0)),
                concentricCount=int(getattr(hatch_pattern_obj, "ConcentricCount", 5)),
                concentricSpacing=float(getattr(hatch_pattern_obj, "ConcentricSpacing", 10.0)),
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
                spacingVariation=float(getattr(hatch_pattern_obj, "SpacingVariation", 0.0)),
                shapeDistortion=bool(getattr(hatch_pattern_obj, "EnableShapeDistortion", False)),
                apply3D=False,
                placement_mode=getattr(hatch_pattern_obj, "PatternPlacementMode", "Origin"),
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
                        transforms.append((local_base, to_world, preview_offset_x, preview_offset_y))
                    except Exception as e:
                        FreeCAD.Console.PrintWarning(f"Ghost hatch projection failed: {e}\n")

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
                        FreeCAD.Console.PrintWarning(f"Ghost hatch face failed: {e}\n")
                        continue

        return all_edges, all_target_faces

    # ------------------------------------------------------------------
    # Subtraction overlay helpers
    # ------------------------------------------------------------------

    def _placements_are_close(self, pl1, pl2, tol_mm=1.0, tol_rad=0.001):
        if (pl1.Base - pl2.Base).Length > tol_mm:
            return False
        rel_angle = pl1.inverse().multiply(pl2).Rotation.Angle
        return abs(rel_angle) < tol_rad

    def _copy_object_shape_to_world_display(self, obj):
        """
        Copy an object's shape and place it in world coordinates for display.

        This mirrors the two-placement-convention logic used elsewhere in the
        hatch stack, but keeps the implementation lightweight for preview-only
        scene-graph rendering.
        """
        shape_copy = obj.Shape.copy()
        obj_pl = obj.Placement
        shape_pl = shape_copy.Placement

        if self._placements_are_close(shape_pl, obj_pl):
            total_pl = obj_pl
        else:
            total_pl = obj_pl.multiply(shape_pl)

        shape_copy.Placement = FreeCAD.Placement()
        if not total_pl.isIdentity():
            shape_copy.transformShape(total_pl.toMatrix())
        return shape_copy

    def _clip_shape_to_faces(self, shape, target_faces):
        if shape is None or shape.isNull() or not target_faces:
            return shape

        try:
            import Part

            clip_parts = []
            for face in target_faces:
                if face is None:
                    continue
                try:
                    clip_parts.append(face.copy())
                except Exception:
                    try:
                        clip_parts.append(Part.Face(face.OuterWire))
                    except Exception:
                        pass

            if not clip_parts:
                return shape

            if len(clip_parts) == 1:
                clip_shape = clip_parts[0]
            else:
                clip_shape = Part.makeCompound(clip_parts)

            clipped = shape.common(clip_shape)
            if clipped and not clipped.isNull() and getattr(clipped, "Edges", []):
                return clipped
        except Exception:
            pass

        return shape

    # ------------------------------------------------------------------
    # Coin3D scene-graph rendering
    # ------------------------------------------------------------------

    def _render_edges(self, edges, layer="hatch"):
        """Build a Coin3D line set from edges and inject it into the scene graph."""
        if not edges:
            return

        style = self._LAYER_STYLES[layer]
        separator = coin.SoSeparator()

        material = coin.SoMaterial()
        material.diffuseColor.setValue(*style["color"])
        material.transparency.setValue(style["transparency"])
        separator.addChild(material)

        draw_style = coin.SoDrawStyle()
        draw_style.lineWidth = style["line_width"]
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
        self._layer_serials[layer] += 1
        render_serial = self._layer_serials[layer]
        self._pending_roots[layer] = separator

        def _inject():
            try:
                if render_serial != self._layer_serials[layer]:
                    return
                if self._pending_roots[layer] is not separator:
                    return

                old_root = self._roots.get(layer)
                if old_root is not None:
                    try:
                        view_object.RootNode.removeChild(old_root)
                    except Exception:
                        pass

                view_object.RootNode.addChild(separator)
                self._roots[layer] = separator
                self._pending_roots[layer] = None
            except Exception as e:
                FreeCAD.Console.PrintWarning(f"Ghost hatch inject failed ({layer}): {e}\n")

        from PySide import QtCore

        QtCore.QTimer.singleShot(0, _inject)

    def _clear_layers(self, layers):
        for layer in layers:
            self._layer_serials[layer] += 1
            self._pending_roots[layer] = None

            root = self._roots.get(layer)
            if root is not None:
                try:
                    self.view_object.RootNode.removeChild(root)
                except Exception:
                    pass
                self._roots[layer] = None
