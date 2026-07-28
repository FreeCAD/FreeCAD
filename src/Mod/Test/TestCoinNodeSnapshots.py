# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

"""Visual snapshot comparisons for selected Coin/Inventor nodes."""

import importlib
import importlib.util
import os
import shutil
import sys
import unittest

from CoinSnapshotHarness import (
    _ViewerSnapshotHarness,
    _baseline_dir,
    _comparison_baseline_path,
    _compare_images,
    _env_truthy,
    _images_are_pixel_identical,
    _logical_viewport_size,
    _mean_rgb,
    _non_background_pixel_count,
    _pixel_bbox,
    _bbox_relative_point,
    _render_pipelines_for_support,
    _render_png,
    _renderer_output_path,
    _require_gui,
    _snapshot_dimensions,
    _snapshot_out_dir,
    _update_baseline_path,
)
from CoinSnapshotScenes import (
    ALL_RENDERERS,
    SnapshotRuntime,
    _DEFAULT_FONT_FAMILY,
    _SO_DATUM_LABEL_TYPES,
    _instantiate,
    _make_so_datum_label_symmetric_scene,
    _make_top_view_scene,
    frame_snapshot_camera,
    get_snapshot_fixture,
    selected_snapshot_nodes,
)


class CoinNodeSnapshotTestCase(unittest.TestCase):
    def test_snapshot_logical_size_rounds_fractional_dpr(self):
        expected = {
            1.0: (512, 512),
            1.25: (410, 410),
            1.5: (341, 341),
            1.75: (293, 293),
            2.0: (256, 256),
        }
        for dpr, size in expected.items():
            with self.subTest(dpr=dpr):
                self.assertEqual(_logical_viewport_size(512, 512, dpr), size)

    def test_so_reg_point_unpickable_regression(self):
        _, _, coin = _require_gui()

        root = coin.SoSeparator()
        probe = _instantiate(coin, "SoRegPoint")
        probe.base.setValue(0.0, 0.0, 0.0)
        probe.normal.setValue(0.0, 0.0, 1.0)
        probe.length.setValue(1.0)
        probe.text.setValue("probe")
        root.addChild(probe)

        pick = coin.SoRayPickAction(coin.SbViewportRegion(512, 512))
        pick.setRadius(8.0)
        pick.setPickAll(True)
        pick.setRay(coin.SbVec3f(0.0, 0.0, 5.0), coin.SbVec3f(0.0, 0.0, -1.0), 0.0, 10.0)
        pick.apply(root)

        self.assertIsNone(
            pick.getPickedPoint(),
            "SoRegPoint should stay unpickable so manual-alignment markers do not intercept clicks",
        )

    def test_so_datum_label_ignores_parent_cull_face(self):
        FreeCAD, FreeCADGui, coin = _require_gui()

        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            for renderer_name in _render_pipelines_for_support(
                harness,
                ALL_RENDERERS,
                label="SoDatumLabelCullFaceRegression",
            ):
                with self.subTest(renderer=renderer_name):
                    actual_path = _renderer_output_path(
                        out_dir,
                        renderer_name,
                        "actual",
                        "SoDatumLabelCullFaceRegression.png",
                    )

                    root = coin.SoSeparator()

                    cam = coin.SoOrthographicCamera()
                    cam.position.setValue(0.0, 0.0, 2.0)
                    cam.nearDistance.setValue(1.0)
                    cam.farDistance.setValue(5.0)
                    cam.height.setValue(2.0)
                    root.addChild(cam)

                    light = coin.SoDirectionalLight()
                    root.addChild(light)

                    callback = coin.SoCallback()

                    def _enable_backface_culling(userdata, action):
                        if not action.getTypeId().isDerivedFrom(
                            coin.SoGLRenderAction.getClassTypeId()
                        ):
                            return
                        coin.SoLazyElement.setBackfaceCulling(action.getState(), True)

                    callback.setCallback(_enable_backface_culling, None)
                    root.addChild(callback)

                    transform = coin.SoTransform()
                    transform.rotation.setValue(
                        coin.SbRotation(coin.SbVec3f(0.0, 1.0, 0.0), 3.141592653589793)
                    )
                    root.addChild(transform)

                    label = _instantiate(coin, "SoDatumLabel")
                    label.string.setValue("CullFace")
                    label.textColor.setValue(0.0, 0.4, 0.9)
                    label.name.setValue(_DEFAULT_FONT_FAMILY)
                    label.size.setValue(28)
                    label.lineWidth.setValue(1.0)
                    label.sampling.setValue(2.0)
                    label.datumtype.setValue(_SO_DATUM_LABEL_TYPES["DISTANCE"])
                    label.param1.setValue(0.18)
                    label.param2.setValue(0.0)
                    # fmt: off
                    label.pnts.setValues(0, 2, [
                        coin.SbVec3f(-0.08, -0.02, 0.0), coin.SbVec3f(0.08, 0.02, 0.0)
                    ])
                    # fmt: on
                    root.addChild(label)

                    _render_png(harness, coin, root, actual_path, renderer_name, frame_camera=False)
                    self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
                    self.assertGreater(
                        _non_background_pixel_count(actual_path),
                        50,
                        f"SoDatumLabel text disappeared under inherited face culling: {actual_path}",
                    )
        finally:
            harness.close()

    def test_so_string_label_anchor_regression(self):
        FreeCAD, FreeCADGui, coin = _require_gui()

        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            for renderer_name in _render_pipelines_for_support(
                harness,
                ALL_RENDERERS,
                label="SoStringLabelAnchorRegression",
            ):
                with self.subTest(renderer=renderer_name):
                    actual_path = _renderer_output_path(
                        out_dir,
                        renderer_name,
                        "actual",
                        "SoStringLabelAnchorRegression.png",
                    )

                    root = coin.SoSeparator()

                    cam = coin.SoOrthographicCamera()
                    cam.position.setValue(0.0, 0.0, 2.0)
                    cam.nearDistance.setValue(1.0)
                    cam.farDistance.setValue(5.0)
                    cam.height.setValue(2.0)
                    root.addChild(cam)

                    light = coin.SoDirectionalLight()
                    root.addChild(light)

                    font = coin.SoFont()
                    font.name.setValue(_DEFAULT_FONT_FAMILY)
                    font.size.setValue(28.0)
                    root.addChild(font)

                    marker_material = coin.SoMaterial()
                    marker_material.diffuseColor.setValue(1.0, 0.0, 0.0)
                    root.addChild(marker_material)

                    marker_style = coin.SoDrawStyle()
                    marker_style.pointSize.setValue(11.0)
                    root.addChild(marker_style)

                    marker_coords = coin.SoCoordinate3()
                    marker_coords.point.setValues(0, 1, [coin.SbVec3f(0.0, 0.0, 0.0)])
                    root.addChild(marker_coords)

                    marker = coin.SoPointSet()
                    marker.numPoints.setValue(1)
                    root.addChild(marker)

                    text_material = coin.SoMaterial()
                    text_material.diffuseColor.setValue(0.05, 0.05, 0.05)
                    root.addChild(text_material)

                    label = _instantiate(coin, "SoStringLabel")
                    label.string.setValues(0, 2, ["OOOO", "OOOO"])
                    label.name.setValue(_DEFAULT_FONT_FAMILY)
                    label.size.setValue(28)
                    label.textColor.setValue(0.0, 0.0, 0.0)
                    root.addChild(label)

                    _render_png(harness, coin, root, actual_path, renderer_name, frame_camera=False)
                    self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")

                    marker_bbox = _pixel_bbox(
                        actual_path,
                        lambda r, g, b, _a: r >= 200 and g <= 80 and b <= 80,
                    )
                    text_bbox = _pixel_bbox(
                        actual_path,
                        lambda r, g, b, _a: max(r, g, b) <= 100
                        and (max(r, g, b) - min(r, g, b)) <= 24,
                    )

                    self.assertIsNotNone(marker_bbox, f"origin marker not visible: {actual_path}")
                    self.assertIsNotNone(text_bbox, f"text pixels not visible: {actual_path}")

                    marker_min_x, marker_min_y, marker_max_x, marker_max_y = marker_bbox
                    text_min_x, text_min_y, text_max_x, _text_max_y = text_bbox

                    marker_center_x = (marker_min_x + marker_max_x) / 2.0
                    marker_center_y = (marker_min_y + marker_max_y) / 2.0
                    text_center_x = (text_min_x + text_max_x) / 2.0

                    self.assertLessEqual(
                        abs(text_center_x - marker_center_x),
                        6.0,
                        (
                            "SoStringLabel should stay horizontally centered on its projected origin "
                            f"(marker bbox={marker_bbox}, text bbox={text_bbox}, image={actual_path})"
                        ),
                    )
                    self.assertGreaterEqual(
                        text_min_y,
                        marker_center_y - 2.0,
                        (
                            "SoStringLabel should extend downward from its projected origin "
                            f"(marker bbox={marker_bbox}, text bbox={text_bbox}, image={actual_path})"
                        ),
                    )
        finally:
            harness.close()

    def test_so_datum_label_symmetric_regression(self):
        FreeCAD, FreeCADGui, coin = _require_gui()

        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            for renderer_name in _render_pipelines_for_support(
                harness,
                ALL_RENDERERS,
                label="SoDatumLabelSymmetricRegression",
            ):
                with self.subTest(renderer=renderer_name):
                    actual_path = _renderer_output_path(
                        out_dir,
                        renderer_name,
                        "actual",
                        "SoDatumLabelSymmetricRegression.png",
                    )

                    root = _make_so_datum_label_symmetric_scene(coin)
                    _render_png(harness, coin, root, actual_path, renderer_name, frame_camera=False)
                    self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
                    self.assertGreater(
                        _non_background_pixel_count(actual_path),
                        80,
                        f"SoDatumLabel symmetric glyph did not render: {actual_path}",
                    )

                    glyph_bbox = _pixel_bbox(
                        actual_path,
                        lambda r, g, b, a: a >= 1 and b >= 120 and b > g + 20 and b > r + 40,
                    )
                    self.assertIsNotNone(
                        glyph_bbox,
                        f"SoDatumLabel symmetric glyph color not visible: {actual_path}",
                    )

                    min_x, min_y, max_x, max_y = glyph_bbox
                    self.assertGreaterEqual(
                        max_x - min_x,
                        100,
                        f"SoDatumLabel symmetric glyph is unexpectedly narrow: {actual_path}",
                    )

                    glyph_center_x = (min_x + max_x) / 2.0
                    glyph_center_y = (min_y + max_y) / 2.0
                    self.assertLessEqual(
                        abs(glyph_center_x - (width / 2.0)),
                        width * 0.20,
                        (
                            "SoDatumLabel symmetric glyph drifted too far from the authored scene "
                            f"center (bbox={glyph_bbox}, image={actual_path})"
                        ),
                    )
                    self.assertLessEqual(
                        abs(glyph_center_y - (height / 2.0)),
                        height * 0.20,
                        (
                            "SoDatumLabel symmetric glyph drifted too far from the authored scene "
                            f"center (bbox={glyph_bbox}, image={actual_path})"
                        ),
                    )
        finally:
            harness.close()

    def test_so_brep_face_set_per_part_material_binding_regression(self):
        FreeCAD, FreeCADGui, coin = _require_gui()

        if importlib.util.find_spec("PartGui") is not None:
            importlib.import_module("PartGui")

        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            for renderer_name in _render_pipelines_for_support(
                harness,
                ALL_RENDERERS,
                label="SoBrepFaceSetPerPartMaterialBindingRegression",
            ):
                with self.subTest(renderer=renderer_name):
                    actual_path = _renderer_output_path(
                        out_dir,
                        renderer_name,
                        "actual",
                        "SoBrepFaceSetPerPartMaterialBindingRegression.png",
                    )

                    root = coin.SoSeparator()

                    cam = coin.SoOrthographicCamera()
                    cam.position.setValue(0.0, 0.0, 2.0)
                    cam.nearDistance.setValue(1.0)
                    cam.farDistance.setValue(5.0)
                    cam.height.setValue(2.0)
                    root.addChild(cam)

                    root.addChild(coin.SoDirectionalLight())

                    light_model = coin.SoLightModel()
                    light_model.model.setValue(coin.SoLightModel.BASE_COLOR)
                    root.addChild(light_model)

                    coords = coin.SoCoordinate3()
                    # fmt: off
                    coords.point.setValues(0, 8, [
                        coin.SbVec3f(-0.85, -0.55, 0.0),
                        coin.SbVec3f(-0.10, -0.55, 0.0),
                        coin.SbVec3f(-0.10, 0.55, 0.0),
                        coin.SbVec3f(-0.85, 0.55, 0.0),
                        coin.SbVec3f(0.10, -0.55, 0.0),
                        coin.SbVec3f(0.85, -0.55, 0.0),
                        coin.SbVec3f(0.85, 0.55, 0.0),
                        coin.SbVec3f(0.10, 0.55, 0.0),
                    ])
                    root.addChild(coords)

                    material = coin.SoMaterial()
                    material.diffuseColor.setValues(0, 2, [
                        coin.SbColor(0.90, 0.15, 0.15),
                        coin.SbColor(0.15, 0.75, 0.20),
                    ])
                    # fmt: on
                    root.addChild(material)

                    material_binding = coin.SoMaterialBinding()
                    material_binding.value = coin.SoMaterialBinding.PER_PART
                    root.addChild(material_binding)

                    faces = _instantiate(coin, "SoBrepFaceSet")
                    # fmt: off
                    faces.coordIndex.setValues(0, 16, [
                        0, 1, 2, -1,  0, 2, 3, -1,
                        4, 5, 6, -1,  4, 6, 7, -1,
                    ])
                    # fmt: on
                    faces.partIndex.setValues(0, 2, [2, 2])
                    root.addChild(faces)

                    _render_png(harness, coin, root, actual_path, renderer_name, frame_camera=False)
                    self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
                    self.assertGreater(
                        _non_background_pixel_count(actual_path),
                        1000,
                        f"SoBrepFaceSet per-part regression render seems empty: {actual_path}",
                    )

                    left_rgb = _mean_rgb(
                        actual_path, int(width * 0.28), int(height * 0.50), radius=8
                    )
                    right_rgb = _mean_rgb(
                        actual_path, int(width * 0.72), int(height * 0.50), radius=8
                    )

                    r, g, b = left_rgb
                    self.assertGreater(
                        r,
                        g + 60.0,
                        f"left quad should keep part 0's red material (got {left_rgb}, image={actual_path})",
                    )
                    self.assertGreater(
                        r,
                        b + 60.0,
                        f"left quad should keep part 0's red material (got {left_rgb}, image={actual_path})",
                    )
                    self.assertGreater(
                        r,
                        120.0,
                        f"left quad should stay visibly red (got {left_rgb}, image={actual_path})",
                    )

                    r, g, b = right_rgb
                    self.assertGreater(
                        g,
                        r + 60.0,
                        f"right quad should keep part 1's green material (got {right_rgb}, image={actual_path})",
                    )
                    self.assertGreater(
                        g,
                        b + 60.0,
                        f"right quad should keep part 1's green material (got {right_rgb}, image={actual_path})",
                    )
                    self.assertGreater(
                        g,
                        120.0,
                        f"right quad should stay visibly green (got {right_rgb}, image={actual_path})",
                    )
        finally:
            harness.close()

    def test_so_brep_face_set_partial_render_transparency_regression(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        importlib.import_module("Part")
        importlib.import_module("PartGui")

        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        doc = FreeCAD.newDocument("SoBrepFaceSetPartialRenderTransparencyRegression")
        try:
            box = doc.addObject("Part::Box", "Box")
            doc.recompute()
            FreeCADGui.setActiveDocument(doc.Name)
            gui_doc = FreeCADGui.getDocument(doc.Name) or FreeCADGui.activeDocument()
            if gui_doc is None:
                raise AssertionError(f"failed to resolve Gui document for {doc.Name}")

            box.ViewObject.DiffuseColor = [
                (0.90, 0.18, 0.18, 1.0),
                (0.18, 0.78, 0.22, 1.0),
                (0.18, 0.38, 0.92, 1.0),
                (0.94, 0.82, 0.18, 1.0),
                (0.82, 0.28, 0.86, 1.0),
                (0.92, 0.18, 0.18, 1.0),
            ]
            box.ViewObject.Transparency = 35
            box.ViewObject.partialRender(["Face6"], False)
            FreeCADGui.updateGui()

            harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
            try:
                for renderer_name in _render_pipelines_for_support(
                    harness,
                    ALL_RENDERERS,
                    label="SoBrepFaceSetPartialRenderTransparencyRegression",
                ):
                    with self.subTest(renderer=renderer_name):
                        actual_path = _renderer_output_path(
                            out_dir,
                            renderer_name,
                            "actual",
                            "SoBrepFaceSetPartialRenderTransparencyRegression.png",
                        )

                        root = _make_top_view_scene(
                            coin,
                            box.ViewObject.RootNode,
                            center=(5.0, 5.0, 5.0),
                            camera_height=14.0,
                        )
                        _render_png(
                            harness, coin, root, actual_path, renderer_name, frame_camera=False
                        )

                        self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
                        self.assertGreater(
                            _non_background_pixel_count(actual_path),
                            1000,
                            f"partial transparent face render seems empty: {actual_path}",
                        )

                        bbox = _pixel_bbox(actual_path, lambda r, g, b, _a: min(r, g, b) < 250)
                        self.assertIsNotNone(
                            bbox,
                            f"partial transparent face render produced no visible pixels: {actual_path}",
                        )

                        top_left_rgb = _mean_rgb(
                            actual_path, *_bbox_relative_point(bbox, 0.35, 0.35), radius=8
                        )
                        bottom_right_rgb = _mean_rgb(
                            actual_path, *_bbox_relative_point(bbox, 0.65, 0.65), radius=8
                        )

                        for label, rgb in (
                            ("top-left triangle", top_left_rgb),
                            ("bottom-right triangle", bottom_right_rgb),
                        ):
                            r, g, b = rgb
                            self.assertGreater(
                                r,
                                g + 30.0,
                                f"{label} should keep the partially rendered top face's red color (got {rgb}, image={actual_path})",
                            )
                            self.assertGreater(
                                r,
                                b + 30.0,
                                f"{label} should keep the partially rendered top face's red color (got {rgb}, image={actual_path})",
                            )
                            self.assertGreater(
                                r,
                                150.0,
                                f"{label} should stay visibly red even with transparency (got {rgb}, image={actual_path})",
                            )
            finally:
                harness.close()
        finally:
            if getattr(FreeCADGui, "Selection", None) is not None:
                FreeCADGui.Selection.clearSelection()
            if FreeCAD.getDocument(doc.Name):
                FreeCAD.closeDocument(doc.Name)

    def test_coin_node_snapshots(self):
        """Render each configured node and compare against baseline images."""
        is_ci = bool(os.environ.get("CI", "").strip())
        is_linux = sys.platform.startswith("linux")
        smoke_mode = is_ci and not is_linux

        FreeCAD, FreeCADGui, coin = _require_gui()

        node_types = selected_snapshot_nodes()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        update_baseline = _env_truthy("FC_VISUAL_UPDATE_BASELINE")
        try:
            baseline_dir = _baseline_dir(create=update_baseline)
        except FileNotFoundError as exc:
            raise unittest.SkipTest(str(exc)) from exc

        if not baseline_dir.is_dir():
            self.fail(
                f"baseline directory not found: {baseline_dir} "
                "(set FC_VISUAL_BASELINE_DIR or run with FC_VISUAL_UPDATE_BASELINE=1)"
            )

        tolerance = int(os.environ.get("FC_VISUAL_TOLERANCE", "8"))
        tolerance = max(0, min(tolerance, 255))
        ignore_alpha = _env_truthy("FC_VISUAL_IGNORE_ALPHA", "1")
        max_mismatch_pct = float(os.environ.get("FC_VISUAL_MAX_MISMATCH_PCT", "0.20"))
        max_mismatch_pct = max(0.0, min(max_mismatch_pct, 100.0))
        max_mismatched_pixels = int((width * height) * (max_mismatch_pct / 100.0))

        did_render = False
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            for type_name in node_types:
                fixture = get_snapshot_fixture(type_name)
                render_pipelines = harness.supported_pipelines(fixture)
                for renderer_name in render_pipelines:
                    with self.subTest(renderer=renderer_name, node=type_name):
                        fixture = get_snapshot_fixture(type_name)
                        runtime = SnapshotRuntime(coin, width, height, type_name)
                        root = fixture.build(runtime)

                        actual_path = _renderer_output_path(
                            out_dir, renderer_name, "actual", f"{type_name}.png"
                        )
                        expected_path = _renderer_output_path(
                            out_dir, renderer_name, "expected", f"{type_name}.png"
                        )
                        diff_path = _renderer_output_path(
                            out_dir, renderer_name, "diff", f"{type_name}.png"
                        )
                        baseline_path = _comparison_baseline_path(
                            baseline_dir, renderer_name, type_name
                        )
                        update_path = _update_baseline_path(baseline_dir, renderer_name, type_name)

                        frame_snapshot_camera(runtime, root, fixture.framing_policy)

                        _render_png(
                            harness,
                            coin,
                            root,
                            actual_path,
                            renderer_name,
                            frame_camera=False,
                        )
                        self.assertTrue(actual_path.exists(), f"missing snapshot: {actual_path}")
                        self.assertGreater(
                            actual_path.stat().st_size, 0, f"empty snapshot: {actual_path}"
                        )

                        did_render = True
                        self.assertGreater(
                            _non_background_pixel_count(actual_path),
                            10,
                            f"snapshot seems empty (all background): {actual_path}",
                        )

                        if type_name == "SoRegPoint":
                            label_bbox = _pixel_bbox(
                                actual_path,
                                lambda r, g, b, _a: r >= 200 and g <= 180 and b <= 160,
                            )
                            self.assertIsNotNone(
                                label_bbox,
                                f"SoRegPoint marker and label are not visible: {actual_path}",
                            )
                            _min_x, min_y, max_x, _max_y = label_bbox
                            self.assertLessEqual(
                                min_y,
                                int(height * 0.10),
                                f"SoRegPoint label is missing or displaced: {actual_path}",
                            )
                            self.assertGreaterEqual(
                                max_x,
                                int(width * 0.95),
                                f"SoRegPoint label is missing or clipped: {actual_path}",
                            )

                        if update_baseline:
                            if update_path.exists() and _images_are_pixel_identical(
                                update_path, actual_path
                            ):
                                continue
                            update_path.parent.mkdir(parents=True, exist_ok=True)
                            shutil.copyfile(actual_path, update_path)
                            continue

                        if not baseline_path.exists():
                            if smoke_mode:
                                continue
                            self.fail(
                                f"missing baseline: {baseline_path} (run with FC_VISUAL_UPDATE_BASELINE=1)"
                            )

                        expected_path.parent.mkdir(parents=True, exist_ok=True)
                        shutil.copyfile(baseline_path, expected_path)

                        ok, msg = _compare_images(
                            expected_path,
                            actual_path,
                            diff_path,
                            tolerance=tolerance,
                            ignore_alpha=ignore_alpha,
                            max_mismatched_pixels=max_mismatched_pixels,
                        )
                        if smoke_mode:
                            if not ok:
                                print(f"SMOKE mismatch for {renderer_name}/{type_name}: {msg}")
                        else:
                            self.assertTrue(ok, msg)
        finally:
            harness.close()
        self.assertTrue(did_render, "No snapshots were rendered")
