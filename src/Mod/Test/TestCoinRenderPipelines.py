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
from CoinSnapshotScenes import _build_lighting_equivalence_scene, _frame_scene_camera


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


class CoinRenderPipelineTestCase(unittest.TestCase):
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
