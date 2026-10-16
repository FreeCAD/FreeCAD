# SPDX-License-Identifier: LGPL-2.1-or-later

"""Snapshot the manual 3D capture, selection, print, and thumbnail procedure.

This test is intended to be executed via `tools/rendering/visual_baselines.py`.
See its docs for details. From the repository root:

  python3 tools/rendering/visual_baselines.py compare --headless --suite VisualView3DRenderSnapshots
"""

import json
import os
import subprocess
from pathlib import Path
import tempfile
import time
import unittest
import zipfile

import FreeCAD as App
import FreeCADGui as Gui

_WIDTH = 300
_HEIGHT = 300
_STYLES = ("as-is", "hidden-line", "wireframe")
_STEPS = (
    *(f"01-live-{style}" for style in _STYLES),
    *(f"02-box-{style}-{state}" for style in _STYLES for state in ("drag", "moved", "finished")),
    "03-polygon-capture",
    "04-capture-lit",
    "04-capture-unlit",
    "04-capture-msaa4",
    "04-capture-no-decorations",
    "05-print-preview",
    "05-print-pdf",
    "06-thumbnail",
    "06-empty-thumbnail",
    *(f"07-hidpi-{kind}-dpr{ratio}" for kind in ("export", "screen") for ratio in (1, 2)),
)


def _changed_pixels(expected, actual, tolerance=8):
    """Ignore tiny channel rounding, but do not hide small leaked decorations."""
    from PySide6.QtGui import QImage

    if expected.size() != actual.size():
        return actual.width() * actual.height()

    expected = expected.convertToFormat(QImage.Format_RGBA8888)
    actual = actual.convertToFormat(QImage.Format_RGBA8888)
    left = bytes(expected.constBits())
    right = bytes(actual.constBits())
    return sum(
        max(abs(left[i + c] - right[i + c]) for c in range(3)) > tolerance
        for i in range(0, len(left), 4)
    )


class View3DRenderSnapshots(unittest.TestCase):
    """Use production documents, commands, image export, printing, and saving."""

    @classmethod
    def setUpClass(cls):
        if not App.GuiUp:
            raise unittest.SkipTest("Use FreeCAD -t; a GUI document is required")

        from PySide6 import QtCore, QtGui, QtTest, QtWidgets
        import VisualTestSupport as visual
        import Part  # Register the same primitives used by the manual procedure.

        cls.qt = QtCore
        cls.images = QtGui
        cls.events = QtTest.QTest
        cls.widgets = QtWidgets
        cls.visual = visual
        cls.part = Part
        visual.require_gui()

        cls.output = Path(
            os.environ.get(
                "FC_VISUAL_OUT_DIR",
                str(Path(tempfile.gettempdir()) / "FreeCADTesting" / "View3DRenderSnapshots"),
            )
        )
        cls.output.mkdir(parents=True, exist_ok=True)
        relative = Path("tests/visual/baselines/VisualView3DRenderSnapshots")
        default = next(
            (
                parent / relative
                for parent in Path(__file__).resolve().parents
                if (parent / relative).is_dir()
            ),
            Path(App.getHomePath()) / relative,
        )
        cls.baselines = Path(os.environ.get("FC_VISUAL_BASELINE_DIR", str(default)))
        cls.mode = os.environ.get("FC_VISUAL_MODE", "compare")
        if cls.mode not in ("record", "compare", "update"):
            raise ValueError(f"Unknown FC_VISUAL_MODE: {cls.mode}")
        if cls.mode == "compare" and not any(cls.baselines.glob("*.png")):
            raise unittest.SkipTest("No approved 3D render baselines; use the baseline manager")
        cls.snapshots = {}
        cls.checks = []

    @classmethod
    def tearDownClass(cls):
        if os.environ.get("FC_VISUAL_HIDPI_WORKER"):
            return

        missing = sorted(set(_STEPS) - cls.snapshots.keys())
        report = {
            "label": os.environ.get("FC_VISUAL_LABEL", ""),
            "freecad_version": App.Version(),
            "qt_version": cls.qt.qVersion(),
            "platform": cls.widgets.QApplication.platformName(),
            "mode": cls.mode,
            "snapshots": cls.snapshots,
            "checks": cls.checks,
            "missing": missing,
            "success": not missing
            and all(item["passed"] for item in cls.snapshots.values())
            and all(item["passed"] for item in cls.checks),
        }
        (cls.output / "results.json").write_text(json.dumps(report, indent=2) + "\n")

    def setUp(self):
        self._preference("View", "Int", "AntiAliasing", 0, 0)
        self._preference("View", "String", "SavePicture", "FramebufferObject", "")
        self._preference("View", "Bool", "EnableSelection", True, True)
        self._preference("Document", "Bool", "SaveThumbnail", True, True)
        self._preference("Document", "Int", "ThumbnailSize", 256, 256)
        self._preference("Document", "Bool", "AddThumbnailLogo", False, False)
        self.temp = tempfile.TemporaryDirectory(prefix="freecad-render-scene-")
        self.addCleanup(self.temp.cleanup)

        self.harness = self.visual.ViewerSnapshotHarness(_WIDTH, _HEIGHT)
        self.addCleanup(self.harness.close)
        self.addCleanup(Gui.Selection.clearSelection)
        self.addCleanup(Gui.Selection.clearPreselection)
        self.doc = self.harness.doc
        self.view = self.harness.view
        self.viewer = self.harness.viewer
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.ViewObject.ShapeColor = (0.85, 0.2, 0.15)
        self.cylinder = self.doc.addObject("Part::Cylinder", "Cylinder")
        self.cylinder.Radius = 4
        self.cylinder.Height = 16
        self.cylinder.Placement.Base = App.Vector(7, 4, 0)
        self.cylinder.ViewObject.ShapeColor = (0.15, 0.35, 0.85)
        self.cylinder.ViewObject.Transparency = 50
        self.doc.recompute()

        self.graphics = self.view.graphicsView()
        self.graphics.setFrameShape(self.widgets.QFrame.NoFrame)
        self.graphics.setHorizontalScrollBarPolicy(self.qt.Qt.ScrollBarAlwaysOff)
        self.graphics.setVerticalScrollBarPolicy(self.qt.Qt.ScrollBarAlwaysOff)
        self.graphics.setFixedSize(_WIDTH, _HEIGHT)
        self.viewport = self.graphics.viewport()
        self.addCleanup(self._drop_widgets)
        self.view.setNavigationType("Gui::CADNavigationStyle")
        self.view.setAnimationEnabled(False)
        self.view.setCameraType("Orthographic")
        self.view.viewAxonometric()
        self.viewer.setBackgroundColor(0.92, 0.94, 0.97)
        self.viewer.setGradientBackground("NONE")
        self.viewer.setEnabledNaviCube(True)
        self.view.setAxisCross(True)
        self.view.setCornerCrossVisible(True)
        self.view.setCornerCrossSize(10)
        self.view.fitAll()
        Gui.Selection.clearSelection()
        Gui.Selection.clearPreselection()
        self.addCleanup(self._cancel_selection)
        self._settle()
        self.viewport.setFocus()
        self._park_pointer()

    def _preference(self, group, kind, key, value, default):
        params = App.ParamGet(f"User parameter:BaseApp/Preferences/{group}")
        setter = getattr(params, f"Set{kind}")
        previous = getattr(params, f"Get{kind}")(key, default)
        self.addCleanup(setter, key, previous)
        setter(key, value)

    def _drop_widgets(self):
        # Failed subtests retain self; release wrappers before the C++ view is destroyed.
        self.viewport = None
        self.graphics = None
        self.viewer = None
        self.view = None

    def _cancel_selection(self):
        self.events.keyClick(self.viewport, self.qt.Qt.Key_Escape)

    def _park_pointer(self):
        self.images.QCursor.setPos(self.viewport.mapToGlobal(self.qt.QPoint(2, 2)))
        Gui.Selection.clearPreselection()

    def _settle(self):
        for _ in range(3):
            Gui.updateGui()
            self.widgets.QApplication.processEvents()
            time.sleep(0.02)
        self.viewport.repaint()

    def _frame(self):
        self.viewport.repaint()
        image = self.viewer.grabFramebuffer()
        self.assertFalse(image.isNull(), "Viewport framebuffer is empty")
        ratio = self.viewport.devicePixelRatioF()
        self.assertEqual(image.size(), self.qt.QSize(round(_WIDTH * ratio), round(_HEIGHT * ratio)))
        return image

    def _snapshot(self, name, image=None):
        if image is None:
            image = self._frame()
        actual = self.output / "actual" / f"{name}.png"
        actual.parent.mkdir(parents=True, exist_ok=True)
        self.assertFalse(image.isNull(), f"Null snapshot: {name}")
        self.assertTrue(image.save(str(actual)), f"Cannot save {actual}")
        result = {"file": str(actual.relative_to(self.output)), "passed": True}
        self.snapshots[name] = result

        if self.mode in ("record", "update"):
            if os.environ.get("FC_VISUAL_UPDATE_BASELINE") == "1":
                self.baselines.mkdir(parents=True, exist_ok=True)
                (self.baselines / actual.name).write_bytes(actual.read_bytes())
            return image

        with self.subTest(snapshot=name):
            baseline = self.baselines / f"{name}.png"
            result["passed"] = baseline.is_file()
            self.assertTrue(result["passed"], f"Missing baseline: {baseline}")
            expected = self.output / "expected" / baseline.name
            expected.parent.mkdir(parents=True, exist_ok=True)
            expected.write_bytes(baseline.read_bytes())
            passed, message = self.visual.compare_images(
                expected,
                actual,
                self.output / "diff" / baseline.name,
                tolerance=8,
                ignore_alpha=False,
                max_mismatched_pixels=int(image.width() * image.height() * 0.002),
            )
            result.update(passed=passed, message=message)
            self.assertTrue(passed, f"{name}: {message}")
        return image

    def _check(self, name, condition, message):
        self.checks.append({"name": name, "passed": bool(condition), "message": message})
        with self.subTest(check=name):
            self.assertTrue(condition, message)

    def _same(self, name, expected, actual):
        count = _changed_pixels(expected, actual)
        self._check(name, count == 0, f"{count} changed pixels; expected identical RGB")

    def _xfail(self, name, condition, message):
        self.checks.append(
            {"name": name, "passed": not condition, "expected_failure": True, "message": message}
        )
        with self.subTest(xfail=name):
            self.assertFalse(condition, f"Unexpected pass: {name}; remove xfail. {message}")

    def _style(self, name):
        actions = {"as-is": 0, "wireframe": 2, "hidden-line": 3}
        Gui.runCommand("Std_DrawStyle", actions[name])
        self._settle()

    def _mouse(self, event_type, point, button, buttons):
        event = self.images.QMouseEvent(
            event_type,
            self.qt.QPointF(point),
            self.qt.QPointF(self.viewport.mapToGlobal(point)),
            button,
            buttons,
            self.qt.Qt.NoModifier,
        )
        self.widgets.QApplication.sendEvent(self.viewport, event)

    def _move(self, point):
        self._mouse(self.qt.QEvent.MouseMove, point, self.qt.Qt.NoButton, self.qt.Qt.LeftButton)
        self._settle()

    def _drag_start(self):
        Gui.runCommand("Std_BoxSelection")
        start = self.qt.QPoint(10, 10)
        self._mouse(
            self.qt.QEvent.MouseButtonPress, start, self.qt.Qt.LeftButton, self.qt.Qt.LeftButton
        )

    def test_01_live_snapshots(self):
        for style in _STYLES:
            self._style(style)
            native = self._frame()
            path = Path(self.temp.name) / "live.png"
            self.view.saveImage(str(path), _WIDTH, _HEIGHT, "Current", "$MIBA", 0)
            image = self._snapshot(f"01-live-{style}", self.images.QImage(str(path)))
            name = f"live-{style}-matches-viewport"
            self._same(name, native, image)

    def test_02_box_selection(self):
        for style in _STYLES:
            self._style(style)
            Gui.Selection.clearSelection()
            self._park_pointer()
            before = self._frame()
            self._drag_start()
            self._move(self.qt.QPoint(_WIDTH // 2, _HEIGHT // 2))
            drag = self._snapshot(f"02-box-{style}-drag")
            self._move(self.qt.QPoint(_WIDTH - 10, _HEIGHT - 10))
            moved = self._snapshot(f"02-box-{style}-moved")
            self._check(
                f"box-{style}-outline-moves",
                _changed_pixels(drag, moved) > 50,
                "Selection outline did not move",
            )
            self._mouse(
                self.qt.QEvent.MouseButtonRelease,
                self.qt.QPoint(_WIDTH - 10, _HEIGHT - 10),
                self.qt.Qt.LeftButton,
                self.qt.Qt.NoButton,
            )
            self._settle()
            self._snapshot(f"02-box-{style}-finished")
            selected = {obj.Name for obj in Gui.Selection.getSelection()}
            self._check(
                f"box-{style}-selects-objects",
                selected == {"Box", "Cylinder"},
                f"Selected: {sorted(selected)}",
            )
            Gui.Selection.clearSelection()
            self._drag_start()
            self._move(self.qt.QPoint(_WIDTH * 11 // 16, _HEIGHT * 3 // 4))
            self._cancel_selection()
            self._park_pointer()
            self._settle()
            cancelled = self._frame()
            self._same(f"box-{style}-cancel-restores-scene", before, cancelled)

    def test_03_polygon_capture(self):
        # Regression reproducer for https://github.com/FreeCAD/FreeCAD/issues/31948
        import Mesh
        import MeshGui  # Register the triangle-deletion command.

        mesh = self.doc.addObject("Mesh::Feature", "CutMesh")
        mesh.Mesh = Mesh.Mesh([[(0, 0, 0), (1, 0, 0), (0, 1, 0)]])
        Gui.Selection.addSelection(mesh)
        self._settle()
        # Enter Image mode before the pending background change reaches the viewport.
        self.viewer.setBackgroundColor(0, 1, 0)
        Gui.runCommand("Mesh_PolyCut")
        self._settle()
        frozen = self._snapshot("03-polygon-capture")
        self._xfail(
            "polygon-capture-is-fresh",
            frozen.pixelColor(20, 20) == self.images.QColor(0, 255, 0),
            f"Captured background: {frozen.pixelColor(20, 20).getRgb()}",
        )

        self.viewer.setBackgroundColor(1, 0, 0)
        self._settle()
        self._same("polygon-capture-stays-frozen", frozen, self._frame())

    def test_04_scene_captures(self):
        camera = self.view.getCamera()
        lit = self._snapshot(
            "04-capture-lit",
            self.viewer.renderToImage(
                width=_WIDTH, height=_HEIGHT, samples=0, includeViewerLighting=True
            ),
        )
        unlit = self._snapshot(
            "04-capture-unlit",
            self.viewer.renderToImage(
                width=_WIDTH, height=_HEIGHT, samples=0, includeViewerLighting=False
            ),
        )
        self._snapshot(
            "04-capture-msaa4",
            self.viewer.renderToImage(
                width=_WIDTH, height=_HEIGHT, samples=4, includeViewerLighting=True
            ),
        )
        self._check(
            "lighting-choice-affects-capture",
            _changed_pixels(lit, unlit) > 1000,
            "Removing viewer lighting did not affect the model",
        )

        self.view.setAxisCross(False)
        self.view.setCornerCrossVisible(False)
        self.viewer.setEnabledNaviCube(False)
        self._settle()
        self._snapshot(
            "04-capture-no-decorations",
            self.viewer.renderToImage(
                width=_WIDTH, height=_HEIGHT, samples=0, includeViewerLighting=True
            ),
        )
        self.view.setAxisCross(True)
        self.view.setCornerCrossVisible(True)
        self.viewer.setEnabledNaviCube(True)
        self._settle()
        self._check(
            "capture-preserves-camera",
            self.view.getCamera() == camera,
            "Scene capture changed the viewer camera",
        )

    def _print_preview(self, name):
        from PySide6.QtPrintSupport import QPrintPreviewDialog, QPrintPreviewWidget

        captured = []
        errors = []
        timer = self.qt.QTimer()
        deadline = time.monotonic() + 10

        def capture():
            dialog = next(
                (
                    widget
                    for widget in self.widgets.QApplication.topLevelWidgets()
                    if isinstance(widget, QPrintPreviewDialog) and widget.isVisible()
                ),
                None,
            )
            if dialog is None and time.monotonic() < deadline:
                return

            timer.stop()
            if dialog is None:
                errors.append(AssertionError("Print Preview did not appear"))
                modal = self.widgets.QApplication.activeModalWidget()
                if modal is not None:
                    modal.close()
                return

            try:
                preview = dialog.findChild(QPrintPreviewWidget)
                self.assertIsNotNone(preview, "Print dialog has no preview widget")
                preview.setFixedSize(_WIDTH, _HEIGHT)
                preview.setZoomMode(QPrintPreviewWidget.FitInView)
                self._settle()
                captured.append(self._snapshot(name, preview.grab().toImage()))
            except Exception as exc:
                errors.append(exc)
            finally:
                dialog.close()

        timer.timeout.connect(capture)
        timer.start(20)
        try:
            Gui.runCommand("Std_PrintPreview")
        finally:
            timer.stop()
        if errors:
            raise errors[0]
        self.assertTrue(captured, "Command did not open Print Preview")
        return captured[0]

    def test_05_printing(self):
        camera = self.view.getCamera()
        self._print_preview("05-print-preview")
        self._check(
            "preview-preserves-camera",
            self.view.getCamera() == camera,
            "Print Preview changed the viewer camera",
        )

        from PySide6.QtPdf import QPdfDocument

        pdf = self.output / "print.pdf"
        Gui.export([self.box, self.cylinder], str(pdf))
        document = QPdfDocument(None)
        document.load(str(pdf))
        self.assertEqual(document.pageCount(), 1, "PDF export did not produce one page")
        image = document.render(0, self.qt.QSize(_WIDTH, _HEIGHT))
        self._snapshot("05-print-pdf", image)
        document.close()
        # PDF export intentionally calls viewAll(); restore before the final viewport snapshot.
        self.view.setCamera(camera)
        self._settle()

    def test_06_thumbnails(self):
        self.view.viewBottom()
        self._settle()
        before = self._frame()
        camera = self.view.getCamera()
        archive_path = Path(self.temp.name) / "RenderCheck.FCStd"
        self.doc.saveAs(str(archive_path))
        self._check(
            "thumbnail-preserves-camera",
            self.view.getCamera() == camera,
            "Saving a thumbnail changed the viewer camera",
        )
        with zipfile.ZipFile(archive_path) as archive:
            image = self.images.QImage.fromData(archive.read("thumbnails/Thumbnail.png"))
        self._snapshot("06-thumbnail", image)
        self._check(
            "thumbnail-size",
            image.size() == self.qt.QSize(256, 256),
            f"Thumbnail size: {image.size()}",
        )
        self._settle()
        after = self._frame()
        self._same("thumbnail-preserves-viewport", before, after)

        self.doc.removeObject(self.box.Name)
        self.doc.removeObject(self.cylinder.Name)
        self.doc.recompute()
        self.doc.save()
        with zipfile.ZipFile(archive_path) as archive:
            empty = self.images.QImage.fromData(archive.read("thumbnails/Thumbnail.png"))
        self._snapshot("06-empty-thumbnail", empty)
        self._check(
            "empty-thumbnail-is-transparent",
            all(
                empty.pixelColor(x, y).alpha() == 0
                for y in range(empty.height())
                for x in range(empty.width())
            ),
            "Empty document thumbnail still contains visible pixels",
        )

    def test_07_hidpi(self):
        captures = {}
        for ratio in (1, 2):
            output = self.output / "hidpi" / f"dpr{ratio}"
            output.mkdir(parents=True, exist_ok=True)
            env = os.environ.copy()
            env.update(
                QT_SCALE_FACTOR=str(ratio),
                FC_VISUAL_HIDPI_WORKER=str(ratio),
                FC_VISUAL_OUT_DIR=str(output),
                FC_VISUAL_MODE="record",
            )
            env.pop("FC_VISUAL_UPDATE_BASELINE", None)
            command = [
                self.qt.QCoreApplication.applicationFilePath(),
                "--user-cfg",
                str(output / "user.cfg"),
                "--system-cfg",
                str(output / "system.cfg"),
                "--python-path",
                str(Path(__file__).resolve().parent),
                "-t",
                "VisualView3DRenderSnapshots",
            ]
            # Qt reads device scaling at startup, so each DPR needs its own process.
            log = output / "capture.log"
            with log.open("w") as stream:
                result = subprocess.run(
                    command,
                    env=env,
                    stdout=stream,
                    stderr=subprocess.STDOUT,
                    timeout=60,
                    check=False,
                )
            self.assertEqual(result.returncode, 0, log.read_text())
            for kind in ("export", "screen"):
                name = f"07-hidpi-{kind}-dpr{ratio}"
                captures[kind, ratio] = self._snapshot(
                    name, self.images.QImage(str(output / f"{kind}.png"))
                )

        for ratio in (1, 2):
            with self.subTest(capture="export", dpr=ratio):
                self.assertEqual(captures["export", ratio].size(), self.qt.QSize(_WIDTH, _HEIGHT))
            with self.subTest(capture="screen", dpr=ratio):
                self.assertEqual(
                    captures["screen", ratio].size(), self.qt.QSize(_WIDTH * ratio, _HEIGHT * ratio)
                )

        # Normalize only screen captures; fixed-size exports must match directly.
        for kind in ("export", "screen"):
            image = captures[kind, 2]
            if kind == "screen":
                image = image.scaled(
                    _WIDTH, _HEIGHT, self.qt.Qt.IgnoreAspectRatio, self.qt.Qt.SmoothTransformation
                )
                self.assertTrue(image.save(str(self.output / "hidpi" / "screen-normalized.png")))
            count = _changed_pixels(captures[kind, 1], image, tolerance=8)
            limit = _WIDTH * _HEIGHT * 0.002
            check = self._check if kind == "export" else self._xfail
            check(
                f"hidpi-{kind}-parity",
                count <= limit,
                f"{count} changed pixels; expected at most {limit}",
            )

    def _capture_hidpi(self):
        from pivy import coin
        from SoSwitchMarker import SoSwitchMarker

        part = self.doc.addObject("App::Part", "HiDpiPart")
        part.addObject(self.box)
        part.addObject(self.cylinder)
        datum = self.doc.addObject("App::LocalCoordinateSystem", "Datum")
        part.addObject(datum)
        datum.Placement.Base = App.Vector(-8, 0, 0)
        datum.ViewObject.show()
        for feature in datum.OriginFeatures:
            feature.ViewObject.Visibility = feature.isDerivedFrom("App::Line")

        joint = self.doc.addObject("App::FeaturePython", "JointMarker")
        part.addObject(joint)
        marker = SoSwitchMarker(joint.ViewObject)
        marker.whichChild = coin.SO_SWITCH_ALL
        marker.transform.translation.setValue(14, 0, 10)
        joint.ViewObject.RootNode.addChild(marker)
        self.addCleanup(joint.ViewObject.RootNode.removeChild, marker)
        self.doc.recompute()
        self.view.setAxisCross(False)
        self.view.setCornerCrossVisible(False)
        self.viewer.setEnabledNaviCube(False)
        self._settle()

        ratio = int(os.environ["FC_VISUAL_HIDPI_WORKER"])
        self.assertEqual(self.viewport.devicePixelRatioF(), ratio)
        self.assertTrue(self._frame().save(str(self.output / "screen.png")))
        self.view.saveImage(str(self.output / "export.png"), _WIDTH, _HEIGHT, "Current", "$MIBA", 0)


def load_tests(loader, tests, pattern):
    if os.environ.get("FC_VISUAL_HIDPI_WORKER"):
        return unittest.TestSuite([View3DRenderSnapshots("_capture_hidpi")])
    return tests
