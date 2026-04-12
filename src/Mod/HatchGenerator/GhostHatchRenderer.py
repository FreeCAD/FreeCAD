# GhostHatchRenderer.py
# Coin3D ghost rendering for fast hatch preview
# No document objects, no recompute, instant display

import FreeCAD

if FreeCAD.GuiUp:
    from pivy import coin


class GhostHatchRenderer:
    """
    Renders hatch patterns directly as Coin3D scene graph nodes.

    No document objects. No recompute. No DAG. Instant display.

    Uses the same surface-projection pipeline as CustomHatchFP.execute():
      1. buildSurfaceTransforms(face) → to_local, to_world matrices
      2. projectShapeToLocal(face_shape, to_local) → flat 2D base
      3. buildHatchShape(flat_base, pattern_shape, ...) → flat hatch
      4. unprojectShapeToWorld(flat_hatch, to_world) → 3D hatch on face
      5. Convert edges to coin3D coordinates
      6. Inject into ViewProvider RootNode

    This is why the ghost appears ON the face surface, not at z=0.
    """

    def __init__(self, vobj):
        self.vobj  = vobj
        self.root  = None
        self._color = (0.2, 0.6, 1.0)   # blue to distinguish from hard hatch

    # =========================================================================
    # FIX 2+3+4 — update_multi: multi-material support + proper spacing
    # =========================================================================
    def update_multi(self, render_pairs):
        """
        Render hatch for multiple (faces, pattern) pairs.
        Called deferred via QTimer so coin3D scene graph is unlocked.

        Parameters
        ----------
        render_pairs : list of (face_list, hatch_pattern_obj)
        """
        if not FreeCAD.GuiUp:
            return

        self.clear()

        if not render_pairs:
            return

        try:
            from HatchCore import buildHatchShape
            from HatchGenerator import (buildSurfaceTransforms,
                                         projectShapeToLocal,
                                         unprojectShapeToWorld,
                                         convertBaseSpacingValue)
        except ImportError as e:
            FreeCAD.Console.PrintWarning(f"Ghost hatch: import failed: {e}\n")
            return

        all_edges = []

        for faces, hatch_pattern_obj in render_pairs:
            if not faces or not hatch_pattern_obj:
                continue
            if not hasattr(hatch_pattern_obj, "Shape") or \
               hatch_pattern_obj.Shape.isNull():
                continue

            pattern_shape = hatch_pattern_obj.Shape.copy()
            if pattern_shape.isNull():
                continue

            # ================================================================
            # FIX 4 — Read spacing the same way execute() does
            # ================================================================
            spacing = float(getattr(hatch_pattern_obj, "BaseSpacing", 100.0))
            use_units = getattr(hatch_pattern_obj, "UseUnits", False)
            if use_units:
                try:
                    unit_system = getattr(hatch_pattern_obj, "SelectedUnitSystem", "Metric")
                    spacing = convertBaseSpacingValue(spacing, use_units, unit_system)
                except Exception:
                    pass

            scale    = float(getattr(hatch_pattern_obj, "PatternScale", 1.0))
            rot      = float(getattr(hatch_pattern_obj, "RotationDeg", 0.0))
            distmode = getattr(hatch_pattern_obj, "DistributionMode", "SeamlessTiling")

            for face in faces:
                try:
                    # ── Surface projection pipeline ────────────────────────────
                    to_local, to_world = buildSurfaceTransforms(face)
                    local_base = projectShapeToLocal(face, to_local)

                    hatch_local, _tiles = buildHatchShape(
                        baseShape     = local_base,
                        patternShape  = pattern_shape,
                        distributionMode = distmode,
                        patternScale  = scale,
                        rotationDeg   = rot,
                        baseSpacing   = spacing,
                        maxTiles      = 50,
                        showFaces     = False,
                    )

                    if hatch_local and not hatch_local.isNull():
                        hatch_world = unprojectShapeToWorld(hatch_local, to_world)
                        all_edges.extend(hatch_world.Edges)

                except Exception as e:
                    FreeCAD.Console.PrintWarning(
                        f"Ghost hatch face failed: {e}\n"
                    )
                    continue

        if not all_edges:
            return

        self._render_edges(all_edges)

    # =========================================================================
    # update — Single-pair convenience wrapper for backward compatibility
    # =========================================================================
    def update(self, faces, hatch_pattern_obj):
        """
        Single-pair convenience wrapper for backward compatibility.
        """
        self.update_multi([(faces, hatch_pattern_obj)])

    def _render_edges(self, edges):
        """Convert Part edges to a Coin3D line set and inject into scene."""
        sep     = coin.SoSeparator()
        mat     = coin.SoMaterial()
        mat.diffuseColor.setValue(*self._color)
        mat.transparency.setValue(0.3)
        sep.addChild(mat)

        ds = coin.SoDrawStyle()
        ds.lineWidth = 1.5
        sep.addChild(ds)

        coords  = coin.SoCoordinate3()
        lineset = coin.SoIndexedLineSet()
        pts     = []
        indices = []
        idx     = 0

        for edge in edges:
            try:
                # Discretize edge to polyline points
                pts_edge = edge.discretize(10)
                for pt in pts_edge:
                    pts.append((pt.x, pt.y, pt.z))
                indices.extend(list(range(idx, idx + len(pts_edge))) + [-1])
                idx += len(pts_edge)
            except Exception:
                continue

        if not pts:
            return

        coords.point.setValues(0, len(pts), pts)
        lineset.coordIndex.setValues(0, len(indices), indices)
        sep.addChild(coords)
        sep.addChild(lineset)

        try:
            self.vobj.RootNode.addChild(sep)
            self.root = sep
        except Exception as e:
            FreeCAD.Console.PrintWarning(f"Ghost hatch render failed: {e}\n")

    def clear(self):
        """Remove ghost geometry from the scene graph."""
        if self.root is not None:
            try:
                self.vobj.RootNode.removeChild(self.root)
            except Exception:
                pass
            self.root = None
            