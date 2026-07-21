# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

"""Focused render-pipeline lifecycle and lighting regression tests."""

import unittest
from pathlib import Path

from CoinSnapshotHarness import (
    _QtGuiModule,
    _RENDERER_DRAW_LIST,
    _RENDERER_LEGACY,
    _ViewerSnapshotHarness,
    _images_are_pixel_identical,
    _images_are_similar,
    _mean_rgb,
    _render_png,
    _region_color_count,
    _region_luminance_range,
    _region_pixels_identical,
    _region_pixels_similar,
    _renderer_output_path,
    _require_gui,
    _snapshot_dimensions,
    _snapshot_out_dir,
)
from CoinSnapshotScenes import (
    _build_lighting_equivalence_scene,
    _frame_scene_camera,
    _instantiate,
)


def _save_image(image, path: Path) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not image.save(str(path)):
        raise AssertionError(f"failed to save snapshot image: {path}")
    return path


def _masked_image_difference(
    mask_path: Path,
    lhs_path: Path,
    rhs_path: Path,
    region: tuple[int, int, int, int],
    *,
    background: tuple[int, int, int],
    mask_threshold: int = 12,
    difference_threshold: int = 8,
) -> tuple[int, int, int]:
    if _QtGuiModule is None:
        raise unittest.SkipTest("Qt (PySide) not available")

    QImage = _QtGuiModule.QImage
    mask = QImage(str(mask_path)).convertToFormat(QImage.Format_ARGB32)
    lhs = QImage(str(lhs_path)).convertToFormat(QImage.Format_ARGB32)
    rhs = QImage(str(rhs_path)).convertToFormat(QImage.Format_ARGB32)
    if mask.isNull() or lhs.isNull() or rhs.isNull() or lhs.size() != rhs.size():
        raise AssertionError("cannot compare masked images with mismatched or empty dimensions")

    object_pixels = 0
    changed_pixels = 0
    total_difference = 0
    x0, y0, x1, y1 = region
    for y in range(max(0, y0), min(mask.height(), y1)):
        for x in range(max(0, x0), min(mask.width(), x1)):
            mask_pixel = mask.pixel(x, y)
            mask_rgb = tuple((mask_pixel >> shift) & 0xFF for shift in (16, 8, 0))
            if (
                max(abs(mask_rgb[channel] - background[channel]) for channel in range(3))
                <= mask_threshold
            ):
                continue

            lhs_pixel = lhs.pixel(x, y)
            rhs_pixel = rhs.pixel(x, y)
            lhs_rgb = tuple((lhs_pixel >> shift) & 0xFF for shift in (16, 8, 0))
            rhs_rgb = tuple((rhs_pixel >> shift) & 0xFF for shift in (16, 8, 0))
            difference = sum(abs(lhs_rgb[channel] - rhs_rgb[channel]) for channel in range(3))
            object_pixels += 1
            total_difference += difference
            if (
                max(abs(lhs_rgb[channel] - rhs_rgb[channel]) for channel in range(3))
                > difference_threshold
            ):
                changed_pixels += 1

    return object_pixels, changed_pixels, total_difference


def _make_annotation_stage_root(coin):
    root = coin.SoSeparator()
    camera = coin.SoOrthographicCamera()
    camera.position.setValue(0.0, 0.0, 5.0)
    camera.nearDistance.setValue(0.1)
    camera.farDistance.setValue(20.0)
    camera.height.setValue(2.0)
    root.addChild(camera)
    return root


def _add_stage_cube(coin, parent, color, z, *, scale=(0.75, 0.75, 0.15), transparency=0.0):
    material = coin.SoMaterial()
    material.diffuseColor.setValue(*color)
    if transparency:
        material.transparency.setValue(transparency)
    transform = coin.SoTransform()
    transform.translation.setValue(0.0, 0.0, z)
    transform.scaleFactor.setValue(*scale)
    group = coin.SoSeparator()
    group.addChild(material)
    group.addChild(transform)
    cube = coin.SoCube()
    group.addChild(cube)
    parent.addChild(group)
    return cube


def _add_transparent_stage_triangle(coin, parent):
    material = coin.SoMaterial()
    material.diffuseColor.setValue(0.95, 0.08, 0.05)
    material.transparency.setValue(0.45)
    coordinates = coin.SoCoordinate3()
    coordinates.point.setValues(
        0,
        3,
        [
            coin.SbVec3f(-0.82, -0.78, 0.0),
            coin.SbVec3f(0.82, -0.78, 0.0),
            coin.SbVec3f(0.0, 0.82, 0.0),
        ],
    )
    faces = coin.SoFaceSet()
    faces.numVertices.setValue(3)
    group = coin.SoSeparator()
    group.addChild(material)
    group.addChild(coordinates)
    group.addChild(faces)
    parent.addChild(group)
    return faces


def _render_annotation_stage_pair(harness, coin, root, out_dir, filename):
    rendered = {}
    for renderer_name in (_RENDERER_LEGACY, _RENDERER_DRAW_LIST):
        path = _renderer_output_path(out_dir, renderer_name, "actual", filename)
        _render_png(harness, coin, root, path, renderer_name, frame_camera=False)
        rendered[renderer_name] = path
    return rendered


def _save_current_frame(harness, path: Path) -> Path:
    return _save_image(harness.capture_framebuffer(), path)


def _path_contains_type(path, type_id):
    if path is None:
        return False
    return any(
        path.getNode(index).getTypeId().isDerivedFrom(type_id) for index in range(path.getLength())
    )


class CoinRenderPipelineTestCase(unittest.TestCase):
    def test_3d_annotation_self_depth_matches_between_pipelines(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if not {_RENDERER_LEGACY, _RENDERER_DRAW_LIST}.issubset(
                set(harness.render_pipelines())
            ):
                raise unittest.SkipTest("3D annotation depth regression requires both pipelines")

            root = coin.SoSeparator()
            camera = coin.SoOrthographicCamera()
            camera.position.setValue(0.0, 0.0, 5.0)
            camera.nearDistance.setValue(0.1)
            camera.farDistance.setValue(20.0)
            camera.height.setValue(2.0)
            root.addChild(camera)

            # Main geometry must be covered by the delayed stage after its
            # depth barrier, while the two annotation cubes must still
            # self-occlude in front-to-back order.
            main_material = coin.SoMaterial()
            main_material.diffuseColor.setValue(0.15, 0.75, 0.20)
            main_transform = coin.SoTransform()
            main_transform.scaleFactor.setValue(1.3, 1.3, 0.12)
            main_group = coin.SoSeparator()
            main_group.addChild(main_material)
            main_group.addChild(main_transform)
            main_group.addChild(coin.SoCube())
            root.addChild(main_group)

            annotation = _instantiate(coin, "So3DAnnotation")
            front_material = coin.SoMaterial()
            front_material.diffuseColor.setValue(0.90, 0.10, 0.08)
            front_transform = coin.SoTransform()
            front_transform.translation.setValue(0.0, 0.0, 0.28)
            front_transform.scaleFactor.setValue(0.82, 0.82, 0.22)
            front_group = coin.SoSeparator()
            front_group.addChild(front_material)
            front_group.addChild(front_transform)
            front_group.addChild(coin.SoCube())

            back_material = coin.SoMaterial()
            back_material.diffuseColor.setValue(0.08, 0.18, 0.92)
            back_transform = coin.SoTransform()
            back_transform.translation.setValue(0.0, 0.0, -0.28)
            back_transform.scaleFactor.setValue(0.62, 0.62, 0.22)
            back_group = coin.SoSeparator()
            back_group.addChild(back_material)
            back_group.addChild(back_transform)
            back_group.addChild(coin.SoCube())

            annotation.addChild(front_group)
            annotation.addChild(back_group)
            root.addChild(annotation)

            harness.viewer.setBackgroundColor(1.0, 1.0, 1.0)
            harness.viewer.setGradientBackground("NONE")
            harness.viewer.setOverrideMode("No Shading")

            rendered = {}
            for renderer_name in (_RENDERER_LEGACY, _RENDERER_DRAW_LIST):
                path = _renderer_output_path(
                    out_dir,
                    renderer_name,
                    "actual",
                    "So3DAnnotationSelfDepth.png",
                )
                _render_png(harness, coin, root, path, renderer_name, frame_camera=False)
                rendered[renderer_name] = path

            QImage = _QtGuiModule.QImage
            for renderer_name, path in rendered.items():
                image = QImage(str(path)).convertToFormat(QImage.Format_ARGB32)
                red = 0
                blue = 0
                for y in range(height // 2 - 8, height // 2 + 8):
                    for x in range(width // 2 - 8, width // 2 + 8):
                        color = image.pixelColor(x, y)
                        if color.red() > color.blue() * 1.8 and color.red() > 140:
                            red += 1
                        if color.blue() > color.red() * 1.8 and color.blue() > 140:
                            blue += 1
                self.assertGreater(
                    red, 100, f"front annotation cube is not visible: {renderer_name}"
                )
                self.assertLess(
                    blue, red // 4, f"back cube bypassed annotation depth: {renderer_name}"
                )
        finally:
            harness.close()

    def test_3d_annotation_bypasses_main_depth(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if not {_RENDERER_LEGACY, _RENDERER_DRAW_LIST}.issubset(
                set(harness.render_pipelines())
            ):
                raise unittest.SkipTest("3D annotation depth regression requires both pipelines")

            root = _make_annotation_stage_root(coin)
            main_group = coin.SoSeparator()
            _add_stage_cube(coin, main_group, (0.08, 0.72, 0.16), 0.45, scale=(0.9, 0.9, 0.16))
            root.addChild(main_group)

            annotation = _instantiate(coin, "So3DAnnotation")
            _add_stage_cube(coin, annotation, (0.92, 0.08, 0.05), 0.0, scale=(0.55, 0.55, 0.12))
            root.addChild(annotation)

            harness.viewer.setBackgroundColor(1.0, 1.0, 1.0)
            harness.viewer.setGradientBackground("NONE")
            harness.viewer.setOverrideMode("No Shading")
            rendered = _render_annotation_stage_pair(
                harness, coin, root, out_dir, "So3DAnnotationMainDepthBypass.png"
            )

            for renderer_name, path in rendered.items():
                red, green, _blue = _mean_rgb(path, width // 2, height // 2, radius=5)
                self.assertGreater(
                    red,
                    green + 45.0,
                    f"after-main annotation did not bypass main depth: {renderer_name}",
                )
        finally:
            harness.close()

    def test_3d_annotation_transparency_matches_between_pipelines(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if not {_RENDERER_LEGACY, _RENDERER_DRAW_LIST}.issubset(
                set(harness.render_pipelines())
            ):
                raise unittest.SkipTest("transparent annotation regression requires both pipelines")

            root = _make_annotation_stage_root(coin)
            main_group = coin.SoSeparator()
            _add_stage_cube(coin, main_group, (0.08, 0.55, 0.82), -0.30, scale=(0.9, 0.9, 0.12))
            root.addChild(main_group)
            annotation = _instantiate(coin, "So3DAnnotation")
            _add_transparent_stage_triangle(coin, annotation)
            root.addChild(annotation)

            harness.viewer.setBackgroundColor(1.0, 1.0, 1.0)
            harness.viewer.setGradientBackground("NONE")
            harness.viewer.setOverrideMode("No Shading")
            rendered = _render_annotation_stage_pair(
                harness, coin, root, out_dir, "So3DAnnotationTransparent.png"
            )
            object_region = (width // 4, height // 4, width * 3 // 4, height * 3 // 4)

            self.assertTrue(
                _region_pixels_similar(
                    rendered[_RENDERER_LEGACY],
                    rendered[_RENDERER_DRAW_LIST],
                    object_region,
                    tolerance=45,
                    max_mismatched_fraction=0.08,
                ),
                "transparent after-main annotation differs between LegacyGL and DrawList",
            )
            red, green, _blue = _mean_rgb(
                rendered[_RENDERER_DRAW_LIST], width // 2, height // 2, radius=5
            )
            self.assertGreater(red, 70.0, "transparent after-main annotation was not rendered")
            self.assertGreater(green, 35.0, "transparent annotation lost its underlying geometry")
        finally:
            harness.close()

    def test_main_selection_respects_depth_before_after_main_clear(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if _RENDERER_DRAW_LIST not in harness.render_pipelines():
                raise unittest.SkipTest("main selection depth regression requires DrawList")

            root = _make_annotation_stage_root(coin)
            front_group = coin.SoSeparator()
            _add_stage_cube(coin, front_group, (0.08, 0.72, 0.16), 0.45, scale=(0.9, 0.9, 0.16))
            root.addChild(front_group)
            rear_group = coin.SoSeparator()
            _add_stage_cube(coin, rear_group, (0.08, 0.16, 0.84), -0.35, scale=(0.9, 0.9, 0.16))
            root.addChild(rear_group)

            # Keep a delayed subtree in the frame so the after-main depth barrier is active,
            # without covering the center used to verify main-scene occlusion.
            annotation = _instantiate(coin, "So3DAnnotation")
            aid_group = coin.SoSeparator()
            aid_translation = coin.SoTransform()
            aid_translation.translation.setValue(-0.78, 0.78, 0.0)
            aid_group.addChild(aid_translation)
            _add_stage_cube(coin, aid_group, (0.92, 0.30, 0.05), 0.0, scale=(0.10, 0.10, 0.10))
            annotation.addChild(aid_group)
            root.addChild(annotation)

            harness.viewer.setBackgroundColor(1.0, 1.0, 1.0)
            harness.viewer.setGradientBackground("NONE")
            harness.viewer.setOverrideMode("No Shading")
            initial = _renderer_output_path(
                out_dir, _RENDERER_DRAW_LIST, "actual", "MainSelectionDepthInitial.png"
            )
            _render_png(harness, coin, root, initial, _RENDERER_DRAW_LIST, frame_camera=False)

            manager = harness.viewer.getSoRenderManager()
            select = getattr(manager, "setDrawListSelection", None)
            if not callable(select):
                raise unittest.SkipTest("DrawList selection API is not exposed by this build")
            self.assertTrue(
                select(2, coin.SbColor4f(1.0, 0.0, 1.0, 1.0), False),
                "the rear main object was not present in the DrawList pick LUT",
            )
            harness.view.redraw()
            harness.flush(cycles=8)
            selected = _save_current_frame(
                harness,
                _renderer_output_path(
                    out_dir, _RENDERER_DRAW_LIST, "actual", "MainSelectionDepthSelected.png"
                ),
            )
            red, green, blue = _mean_rgb(selected, width // 2, height // 2, radius=5)
            self.assertGreater(green, red + 35.0, "main selection bypassed the front object depth")
            self.assertGreater(green, blue + 35.0, "main selection bypassed the front object depth")
        finally:
            harness.close()

    def test_after_main_selection_runs_after_depth_clear(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if _RENDERER_DRAW_LIST not in harness.render_pipelines():
                raise unittest.SkipTest("after-main selection regression requires DrawList")

            root = _make_annotation_stage_root(coin)
            main_group = coin.SoSeparator()
            _add_stage_cube(coin, main_group, (0.06, 0.70, 0.18), 0.35, scale=(0.9, 0.9, 0.16))
            root.addChild(main_group)
            annotation = _instantiate(coin, "So3DAnnotation")
            _add_stage_cube(coin, annotation, (0.88, 0.08, 0.04), 0.0, scale=(0.55, 0.55, 0.12))
            root.addChild(annotation)

            harness.viewer.setBackgroundColor(1.0, 1.0, 1.0)
            harness.viewer.setGradientBackground("NONE")
            harness.viewer.setOverrideMode("No Shading")
            initial = _renderer_output_path(
                out_dir, _RENDERER_DRAW_LIST, "actual", "AfterMainSelectionInitial.png"
            )
            _render_png(harness, coin, root, initial, _RENDERER_DRAW_LIST, frame_camera=False)

            manager = harness.viewer.getSoRenderManager()
            select = getattr(manager, "setDrawListSelection", None)
            if not callable(select):
                raise unittest.SkipTest("DrawList selection API is not exposed by this build")
            self.assertTrue(
                select(2, coin.SbColor4f(1.0, 1.0, 0.0, 1.0), False),
                "the after-main object was not present in the DrawList pick LUT",
            )
            harness.view.redraw()
            harness.flush(cycles=8)
            selected = _save_current_frame(
                harness,
                _renderer_output_path(
                    out_dir, _RENDERER_DRAW_LIST, "actual", "AfterMainSelectionSelected.png"
                ),
            )
            # Whole-object selection is represented by a wireframe bounding box. Sample its
            # top edge, where a pre-clear selection would still fail the main-scene depth test.
            edge_y = height // 2 - int(height * 0.27)
            red, green, blue = _mean_rgb(selected, width // 2, edge_y, radius=2)
            self.assertGreater(red, 100.0, "after-main selection was not rendered")
            self.assertGreater(
                green, 80.0, "after-main selection did not run after the depth clear"
            )
            self.assertLess(blue, 100.0, "after-main selection color was not applied")
        finally:
            harness.close()

    def test_draw_list_cpu_picking_keeps_annotation_aid_interactive(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if _RENDERER_DRAW_LIST not in harness.render_pipelines():
                raise unittest.SkipTest("annotation interaction regression requires DrawList")

            root = _make_annotation_stage_root(coin)
            annotation = _instantiate(coin, "So3DAnnotation")
            indicator = _instantiate(coin, "SoFCPlacementIndicatorKit")
            indicator.parts.setValue(31)
            indicator.axes.setValue(7)
            indicator.axisLength.setValue(0.8)
            indicator.scaleFactor.setValue(70.0)
            indicator.axisLabels.setValues(0, 3, ["X", "Y", "Z"])
            annotation.addChild(indicator)
            root.addChild(annotation)

            harness.viewer.setBackgroundColor(1.0, 1.0, 1.0)
            harness.viewer.setGradientBackground("NONE")
            harness.viewer.setOverrideMode("No Shading")
            rendered = _renderer_output_path(
                out_dir, _RENDERER_DRAW_LIST, "actual", "So3DAnnotationPlacementAid.png"
            )
            _render_png(harness, coin, root, rendered, _RENDERER_DRAW_LIST, frame_camera=False)

            pick = coin.SoRayPickAction(coin.SbViewportRegion(width, height))
            pick.setPoint(coin.SbVec2s(width // 2, height // 2))
            pick.setRadius(14.0)
            pick.setPickAll(True)
            pick.apply(root)
            picked = pick.getPickedPoint()
            self.assertIsNotNone(picked, "CPU ray picking did not hit the placement indicator")
            self.assertTrue(
                _path_contains_type(picked.getPath(), annotation.getTypeId()),
                "CPU picking did not retain the So3DAnnotation path for the placement aid",
            )
            # The DrawList GPU ID buffer intentionally excludes after-main commands for now;
            # this test verifies the existing CPU event/ray-picking path remains interactive.
        finally:
            harness.close()

    def test_render_type_transitions_resynchronize_native_redraw(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if _RENDERER_LEGACY not in harness.render_pipelines():
                raise unittest.SkipTest("render-type state regression requires LegacyGL")

            root, _light = _build_lighting_equivalence_scene(coin)
            root.ref()
            try:
                _frame_scene_camera(coin, root, width, height)
                harness.set_render_pipeline(_RENDERER_LEGACY)
                harness.viewer.setOverrideMode("Shaded")
                harness.viewer.setSceneGraph(root)
                harness.view.redraw()
                harness.flush(cycles=8)

                def capture(suffix: str) -> Path:
                    return _save_image(
                        harness.capture_framebuffer(),
                        _renderer_output_path(
                            out_dir, _RENDERER_LEGACY, "actual", f"RenderType{suffix}.png"
                        ),
                    )

                native_before = capture("NativeBefore")
                object_region = (
                    width // 5,
                    height // 5,
                    width * 4 // 5,
                    height * 4 // 5,
                )
                for render_type in ("Image", "Framebuffer"):
                    harness.viewer.setRenderType(render_type)
                    harness.view.redraw()
                    harness.flush(cycles=8)
                    capture(render_type)

                    harness.viewer.setRenderType("Native")
                    harness.view.redraw()
                    harness.flush(cycles=8)
                    native_after = capture(f"NativeAfter{render_type}")
                    self.assertTrue(
                        _region_pixels_identical(native_before, native_after, object_region),
                        f"Native redraw changed after {render_type} presentation",
                    )
            finally:
                root.unref()
        finally:
            harness.close()

    def test_render_to_image_resynchronizes_native_redraw(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if _RENDERER_LEGACY not in harness.render_pipelines():
                raise unittest.SkipTest("renderToImage state regression requires LegacyGL")

            root, _light = _build_lighting_equivalence_scene(coin)
            root.ref()
            try:
                _frame_scene_camera(coin, root, width, height)
                harness.set_render_pipeline(_RENDERER_LEGACY)
                harness.viewer.setOverrideMode("Shaded")
                harness.viewer.setSceneGraph(root)
                harness.view.redraw()
                harness.flush(cycles=8)

                frame_a = _save_image(
                    harness.capture_framebuffer(),
                    _renderer_output_path(
                        out_dir, _RENDERER_LEGACY, "actual", "RenderToImageNativeA.png"
                    ),
                )
                captured = harness.viewer.renderToImage(samples=0)
                self.assertFalse(captured.isNull(), "renderToImage returned an empty image")
                _save_image(
                    captured,
                    _renderer_output_path(
                        out_dir, _RENDERER_LEGACY, "actual", "RenderToImageCapture.png"
                    ),
                )

                harness.view.redraw()
                harness.flush(cycles=8)
                frame_b = _save_image(
                    harness.capture_framebuffer(),
                    _renderer_output_path(
                        out_dir, _RENDERER_LEGACY, "actual", "RenderToImageNativeB.png"
                    ),
                )
            finally:
                root.unref()

            object_region = (width // 5, height // 5, width * 4 // 5, height * 4 // 5)
            self.assertTrue(
                _region_pixels_identical(frame_a, frame_b, object_region),
                "Native redraw changed the object after renderToImage()",
            )
        finally:
            harness.close()

    def test_legacy_solid_background_preserves_lighting(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if _RENDERER_LEGACY not in harness.render_pipelines():
                raise unittest.SkipTest("LegacyGL is unavailable")

            root, _light = _build_lighting_equivalence_scene(coin)
            path = _renderer_output_path(
                out_dir, _RENDERER_LEGACY, "actual", "SolidBackgroundLighting.png"
            )
            harness.viewer.setBackgroundColor(0.92, 0.92, 0.92)
            harness.viewer.setGradientBackground("NONE")
            harness.viewer.setOverrideMode("Shaded")
            _render_png(harness, coin, root, path, _RENDERER_LEGACY)

            object_region = (width // 5, height // 5, width * 4 // 5, height * 4 // 5)
            self.assertGreater(
                _region_color_count(path, object_region, background=(235, 235, 235)),
                2,
                f"solid-background object has no meaningful color variation: {path}",
            )
            low, high = _region_luminance_range(path, object_region, background=(235, 235, 235))
            self.assertGreater(
                high - low,
                20,
                f"solid-background object is effectively flat-lit ({low}..{high}): {path}",
            )
        finally:
            harness.close()

    def test_axis_cross_does_not_contaminate_main_scene(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if not {_RENDERER_LEGACY, _RENDERER_DRAW_LIST}.issubset(
                set(harness.render_pipelines())
            ):
                raise unittest.SkipTest("axis-cross regression requires both render pipelines")

            root, _light = _build_lighting_equivalence_scene(coin)
            harness.viewer.setOverrideMode("Shaded")

            # Install the scene once. Reinstalling it for every frame bypasses the retained
            # scene/draw-list path this regression is intended to exercise.
            root.ref()
            try:
                _frame_scene_camera(coin, root, width, height)
                harness.viewer.setSceneGraph(root)

                def render(renderer_name: str, suffix: str) -> Path:
                    path = _renderer_output_path(
                        out_dir, renderer_name, "actual", f"AxisCrossContamination{suffix}.png"
                    )
                    harness.set_render_pipeline(renderer_name)
                    harness.view.redraw()
                    harness.flush(cycles=8)
                    image = harness.capture_framebuffer()
                    path.parent.mkdir(parents=True, exist_ok=True)
                    if not image.save(str(path)):
                        raise AssertionError(f"failed to save snapshot image: {path}")
                    return path

                object_region = (width // 5, height // 5, width * 4 // 5, height * 4 // 5)
                harness.view.setCornerCrossVisible(False)
                disabled_legacy = render(_RENDERER_LEGACY, "DisabledLegacy")

                harness.view.setCornerCrossVisible(True)
                enabled_legacy = render(_RENDERER_LEGACY, "EnabledLegacy")
                next_legacy = render(_RENDERER_LEGACY, "NextLegacy")

                self.assertTrue(
                    _region_pixels_identical(disabled_legacy, next_legacy, object_region),
                    "enabling the axis cross changed the next LegacyGL main-scene object",
                )

                harness.view.setCornerCrossVisible(False)
                disabled_draw_list = render(_RENDERER_DRAW_LIST, "DisabledDrawList")
                harness.view.setCornerCrossVisible(True)
                enabled_draw_list = render(_RENDERER_DRAW_LIST, "EnabledDrawList")
                next_draw_list = render(_RENDERER_DRAW_LIST, "NextDrawList")

                self.assertTrue(
                    _region_pixels_identical(disabled_draw_list, next_draw_list, object_region),
                    "enabling the axis cross changed the next DrawList main-scene object",
                )
                # Fixed-function and retained rendering quantize the same lit faces
                # differently; tolerate that bounded rasterization difference while
                # still requiring the object silhouette and lighting pattern to match.
                self.assertTrue(
                    _region_pixels_similar(
                        disabled_legacy,
                        disabled_draw_list,
                        object_region,
                        tolerance=40,
                        max_mismatched_fraction=0.01,
                    ),
                    "LegacyGL and DrawList lighting differ in the main object region",
                )
                self.assertTrue(
                    _region_pixels_similar(
                        enabled_legacy,
                        enabled_draw_list,
                        object_region,
                        tolerance=40,
                        max_mismatched_fraction=0.01,
                    ),
                    "axis-cross rendering changed LegacyGL/DrawList main-scene equivalence",
                )
            finally:
                root.unref()
        finally:
            harness.close()

    def test_gradient_background_keeps_lit_main_scene(self):
        FreeCAD, FreeCADGui, coin = _require_gui()
        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if _RENDERER_LEGACY not in harness.render_pipelines():
                raise unittest.SkipTest("LegacyGL is unavailable")

            root, _light = _build_lighting_equivalence_scene(coin)
            root.ref()
            try:
                _frame_scene_camera(coin, root, width, height)
                harness.set_render_pipeline(_RENDERER_LEGACY)
                harness.viewer.setOverrideMode("Shaded")
                harness.viewer.setBackgroundColor(1.0, 1.0, 1.0)
                harness.viewer.setGradientBackground("NONE")
                harness.viewer.setSceneGraph(root)
                harness.view.redraw()
                harness.flush(cycles=8)
                solid = _save_image(
                    harness.capture_framebuffer(),
                    _renderer_output_path(
                        out_dir, _RENDERER_LEGACY, "actual", "GradientBackgroundSolidMask.png"
                    ),
                )

                harness.viewer.setGradientBackground("LINEAR")
                harness.viewer.setGradientBackgroundColor((0.10, 0.10, 0.25), (0.85, 0.85, 1.0))
                harness.view.redraw()
                harness.flush(cycles=8)
                shaded = _save_image(
                    harness.capture_framebuffer(),
                    _renderer_output_path(
                        out_dir, _RENDERER_LEGACY, "actual", "GradientBackgroundShaded.png"
                    ),
                )

                harness.viewer.setOverrideMode("No Shading")
                harness.view.redraw()
                harness.flush(cycles=8)
                unshaded = _save_image(
                    harness.capture_framebuffer(),
                    _renderer_output_path(
                        out_dir, _RENDERER_LEGACY, "actual", "GradientBackgroundUnshaded.png"
                    ),
                )
            finally:
                root.unref()

            object_region = (width // 5, height // 5, width * 4 // 5, height * 4 // 5)
            object_pixels, changed_pixels, total_difference = _masked_image_difference(
                solid,
                shaded,
                unshaded,
                object_region,
                background=(255, 255, 255),
            )
            self.assertGreater(
                object_pixels, 100, "solid-background reference did not find the object"
            )
            self.assertGreater(
                changed_pixels,
                object_pixels * 0.10,
                "shaded and unshaded output did not differ across enough object pixels",
            )
            self.assertGreater(
                total_difference / object_pixels,
                4.0,
                "shaded and unshaded object output is effectively identical",
            )
        finally:
            harness.close()

    def test_viewer_runtime_pipeline_switch(self):
        FreeCAD, FreeCADGui, coin = _require_gui()

        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        try:
            if not {_RENDERER_LEGACY, _RENDERER_DRAW_LIST}.issubset(
                set(harness.render_pipelines())
            ):
                raise unittest.SkipTest("runtime pipeline switching requires both render pipelines")

            root, _light = _build_lighting_equivalence_scene(coin)
            root.ref()
            try:
                _frame_scene_camera(coin, root, width, height)
                harness.viewer.setSceneGraph(root)

                def render(renderer_name: str, suffix: str) -> Path:
                    harness.set_render_pipeline(renderer_name)
                    harness.view.redraw()
                    harness.flush(cycles=8)
                    image = harness.capture_framebuffer()
                    self.assertFalse(image.isNull(), f"{renderer_name} runtime render is empty")
                    self.assertEqual((image.width(), image.height()), (width, height))
                    return _save_image(
                        image,
                        _renderer_output_path(
                            out_dir, renderer_name, "actual", f"RuntimePipeline{suffix}.png"
                        ),
                    )

                legacy_a = render(_RENDERER_LEGACY, "LegacyA")
                render(_RENDERER_DRAW_LIST, "DrawList")
                legacy_b = render(_RENDERER_LEGACY, "LegacyB")
            finally:
                root.unref()

            self.assertEqual(harness.viewer.getRenderPipeline(), "LegacyGL")
            object_region = (width // 5, height // 5, width * 4 // 5, height * 4 // 5)
            self.assertTrue(
                _region_pixels_similar(
                    legacy_a,
                    legacy_b,
                    object_region,
                    tolerance=8,
                    max_mismatched_fraction=0.01,
                ),
                "LegacyGL object output changed after the DrawList round trip",
            )
        finally:
            harness.close()

    def test_viewer_lighting_matches_legacy_and_updates(self):
        FreeCAD, FreeCADGui, coin = _require_gui()

        width, height = _snapshot_dimensions()
        out_dir = _snapshot_out_dir()
        harness = _ViewerSnapshotHarness(FreeCAD, FreeCADGui, width, height)
        render_pipelines = set(harness.render_pipelines())
        if not {_RENDERER_LEGACY, _RENDERER_DRAW_LIST}.issubset(render_pipelines):
            harness.close()
            raise unittest.SkipTest("lighting equivalence requires both render pipelines")

        def render(
            renderer_name: str,
            override_mode: str,
            suffix: str,
            direction=None,
        ) -> Path:
            root, light = _build_lighting_equivalence_scene(coin)
            if direction is not None:
                light.direction.setValue(direction)
            harness.viewer.setOverrideMode(override_mode)
            path = _renderer_output_path(
                out_dir,
                renderer_name,
                "actual",
                f"ViewerLighting{suffix}.png",
            )
            _render_png(harness, coin, root, path, renderer_name, frame_camera=True)
            return path

        try:
            lit = {
                renderer_name: render(renderer_name, "Shaded", "Lit")
                for renderer_name in (_RENDERER_LEGACY, _RENDERER_DRAW_LIST)
            }
            unlit = {
                renderer_name: render(renderer_name, "No Shading", "Unlit")
                for renderer_name in (_RENDERER_LEGACY, _RENDERER_DRAW_LIST)
            }

            self.assertTrue(
                _images_are_pixel_identical(unlit[_RENDERER_LEGACY], unlit[_RENDERER_DRAW_LIST]),
                "legacy and draw-list unlit output diverged",
            )
            self.assertFalse(
                _images_are_pixel_identical(lit[_RENDERER_DRAW_LIST], unlit[_RENDERER_DRAW_LIST]),
                "draw-list lighting mode did not affect the rendered output",
            )

            # Coin's legacy action only enables fixed-function lighting for a
            # compatibility-profile context.  Core-profile contexts therefore
            # provide a valid unlit legacy reference, but cannot provide a
            # meaningful lit image for a cross-pipeline comparison.
            legacy_lighting_available = not _images_are_pixel_identical(
                lit[_RENDERER_LEGACY], unlit[_RENDERER_LEGACY]
            )
            if legacy_lighting_available:
                self.assertTrue(
                    _images_are_similar(
                        lit[_RENDERER_LEGACY],
                        lit[_RENDERER_DRAW_LIST],
                        tolerance=40,
                        max_mismatched_fraction=0.01,
                    ),
                    "legacy and draw-list lit output diverged",
                )

            changed_direction = coin.SbVec3f(-0.70, 0.20, -1.0)
            changed_draw_list = render(
                _RENDERER_DRAW_LIST,
                "Shaded",
                "LightChangedDrawList",
                changed_direction,
            )
            changed_legacy = render(
                _RENDERER_LEGACY,
                "Shaded",
                "LightChangedLegacy",
                changed_direction,
            )

            self.assertFalse(
                _images_are_pixel_identical(lit[_RENDERER_DRAW_LIST], changed_draw_list),
                "draw-list retained stale lighting after the directional light changed",
            )
            if legacy_lighting_available:
                self.assertTrue(
                    _images_are_similar(
                        changed_draw_list,
                        changed_legacy,
                        tolerance=40,
                        max_mismatched_fraction=0.01,
                    ),
                    "legacy and draw-list diverged after the directional light changed",
                )
        finally:
            harness.viewer.setOverrideMode("As Is")
            harness.close()
