# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

# ******************************************************************************
# *                                                                            *
# *   FreeCAD is free software: you can redistribute it and/or modify          *
# *   it under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the     *
# *   License, or (at your option) any later version.                          *
# *                                                                            *
# *   FreeCAD is distributed in the hope that it will be useful, but           *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
# *   GNU Lesser General Public License for more details.                      *
# *                                                                            *
# *   You should have received a copy of the GNU Lesser General Public         *
# *   License along with FreeCAD.  If not, see                                *
# *   <https://www.gnu.org/licenses/>.                                         *
# *                                                                            *
# ******************************************************************************

"""GUI tests for ViewProviderLink color override plumbing.

To run tests:
    FreeCAD -t TestViewProviderLink
"""

import os
import sys
import tempfile
import unittest
from pathlib import Path


def _configure_software_gl():
    if not sys.platform.startswith("linux"):
        return

    force = os.environ.get("FC_VISUAL_FORCE_SOFTWARE_GL", "1").strip().lower()
    if force in ("", "0", "false", "no"):
        return

    os.environ.setdefault("LIBGL_ALWAYS_SOFTWARE", "1")
    os.environ.setdefault("MESA_LOADER_DRIVER_OVERRIDE", "llvmpipe")


def _require_gui():
    try:
        import FreeCAD  # type: ignore
    except Exception as exc:  # pragma: no cover
        raise RuntimeError("FreeCAD module not available") from exc

    _configure_software_gl()

    if sys.platform.startswith("linux"):
        has_display = bool(os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY"))
        qpa = os.environ.get("QT_QPA_PLATFORM", "").strip().lower()
        headless_qpa = {"offscreen", "minimal", "eglfs", "linuxfb", "vnc"}
        if not has_display and qpa not in headless_qpa:
            raise unittest.SkipTest(
                "No display available (run under xvfb-run or set QT_QPA_PLATFORM=offscreen)"
            )

    try:
        import FreeCADGui  # type: ignore
    except Exception as exc:  # pragma: no cover
        raise unittest.SkipTest("FreeCADGui not available in this build") from exc

    setup_without_gui = getattr(FreeCADGui, "setupWithoutGUI", None)
    if callable(setup_without_gui):
        setup_without_gui()

    home = Path(FreeCAD.getHomePath())
    mod_dir = home / "Mod"
    if mod_dir.is_dir():
        for p in (mod_dir, mod_dir / "Part"):
            ps = str(p)
            if ps not in sys.path:
                sys.path.insert(0, ps)

    try:
        from PySide import QtGui  # type: ignore

        if QtGui.QGuiApplication.instance() is None:
            QtGui.QGuiApplication(sys.argv)
    except Exception as exc:  # pragma: no cover
        raise unittest.SkipTest("Qt (PySide) not available") from exc

    try:
        from pivy import coin  # type: ignore
    except Exception as exc:  # pragma: no cover
        raise unittest.SkipTest("pivy.coin not available") from exc

    return FreeCAD, FreeCADGui, coin


def _instantiate(coin, type_name: str):
    t = coin.SoType.fromName(type_name)
    if t.isBad():
        raise unittest.SkipTest(f"Coin type not registered: {type_name}")
    node = t.createInstance()
    if node is None:
        raise RuntimeError(f"Failed to instantiate {type_name}")
    return node


def _save_active_view_png(
    view, out_path: Path, width: int, height: int, *, view_fn: str = "viewFront"
) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    if hasattr(view, "setAxisCross"):
        view.setAxisCross(False)
    if hasattr(view, "setEnabledNaviCube"):
        view.setEnabledNaviCube(False)
    if hasattr(view, "redraw"):
        view.redraw()
    getattr(view, view_fn)()
    view.fitAll()
    view.saveImage(str(out_path), width, height, "White")


def _mean_rgb(path: Path, x: int, y: int, radius: int = 2) -> tuple[float, float, float]:
    from PySide.QtGui import QImage  # type: ignore

    img = QImage(str(path)).convertToFormat(QImage.Format_ARGB32)
    if img.isNull():
        raise AssertionError(f"unreadable image: {path}")

    r_total = 0
    g_total = 0
    b_total = 0
    count = 0
    for yy in range(max(0, y - radius), min(img.height(), y + radius + 1)):
        for xx in range(max(0, x - radius), min(img.width(), x + radius + 1)):
            pixel = img.pixel(xx, yy)
            r_total += (pixel >> 16) & 0xFF
            g_total += (pixel >> 8) & 0xFF
            b_total += pixel & 0xFF
            count += 1

    if count == 0:
        raise AssertionError(f"sample window around ({x}, {y}) is empty for {path}")

    return (r_total / count, g_total / count, b_total / count)


def _pixel_bbox(path: Path, predicate, region=None):
    from PySide.QtGui import QImage  # type: ignore

    img = QImage(str(path))
    if img.isNull():
        return None

    if region is None:
        region = (0, 0, img.width(), img.height())
    start_x, start_y, end_x, end_y = region
    start_x = max(0, start_x)
    start_y = max(0, start_y)
    end_x = min(img.width(), end_x)
    end_y = min(img.height(), end_y)

    min_x = end_x
    min_y = end_y
    max_x = -1
    max_y = -1

    for y in range(start_y, end_y):
        for x in range(start_x, end_x):
            pixel = img.pixel(x, y)
            r = (pixel >> 16) & 0xFF
            g = (pixel >> 8) & 0xFF
            b = pixel & 0xFF
            if not predicate(r, g, b):
                continue

            min_x = min(min_x, x)
            min_y = min(min_y, y)
            max_x = max(max_x, x)
            max_y = max(max_y, y)

    if max_x < min_x or max_y < min_y:
        return None

    return (min_x, min_y, max_x, max_y)


def _bbox_relative_point(
    bbox: tuple[int, int, int, int], x_ratio: float, y_ratio: float
) -> tuple[int, int]:
    min_x, min_y, max_x, max_y = bbox
    width = max(1, max_x - min_x)
    height = max(1, max_y - min_y)
    x = min_x + int(width * x_ratio)
    y = min_y + int(height * y_ratio)
    return (x, y)


def _make_material(view_object, rgba: tuple[float, float, float, float], transparency: float = 0.0):
    material = view_object.ShapeMaterial
    material.DiffuseColor = rgba
    material.Transparency = transparency
    view_object.ShapeMaterial = material


class TestViewProviderLink(unittest.TestCase):
    def setUp(self):
        self.FreeCAD, self.FreeCADGui, self.coin = _require_gui()
        import Part  # noqa: F401
        import PartGui  # noqa: F401

        self.doc = self.FreeCAD.newDocument("TestViewProviderLink")
        self.FreeCADGui.setActiveDocument(self.doc.Name)
        self.gui_doc = (
            self.FreeCADGui.getDocument(self.doc.Name) or self.FreeCADGui.activeDocument()
        )
        if self.gui_doc is None:
            raise AssertionError(f"failed to resolve Gui document for {self.doc.Name}")

    def tearDown(self):
        if getattr(self, "doc", None) is None:
            return
        try:
            self.FreeCADGui.Selection.clearSelection()
        except Exception:
            # Best-effort cleanup: the GUI selection singleton may already be torn down here.
            pass
        if self.FreeCAD.getDocument(self.doc.Name):
            self.FreeCAD.closeDocument(self.doc.Name)

    def test_apply_element_color_override_api(self):
        root = self.coin.SoSeparator()
        sel_root = _instantiate(self.coin, "SoFCSelectionRoot")
        child = self.coin.SoSeparator()
        leaf = self.coin.SoCube()

        root.addChild(sel_root)
        sel_root.addChild(child)
        child.addChild(leaf)

        search = self.coin.SoSearchAction()
        search.setNode(leaf)
        search.apply(root)
        path = search.getPath()
        self.assertIsNotNone(path, "expected a Coin path to the test leaf node")

        self.assertTrue(hasattr(self.FreeCADGui, "applyElementColorOverride"))
        self.assertTrue(hasattr(self.FreeCADGui, "clearElementColorOverride"))

        self.FreeCADGui.applyElementColorOverride(sel_root, {"Face1": (1.0, 0.1, 0.1, 1.0)})
        self.FreeCADGui.clearElementColorOverride(sel_root)

        self.FreeCADGui.applyElementColorOverride(
            path,
            {"Face": (255, 64, 64, 255), "": 0xFF0000FF},
        )
        self.FreeCADGui.clearElementColorOverride(path)

        with self.assertRaises(TypeError):
            self.FreeCADGui.applyElementColorOverride(42, {"Face1": (1.0, 0.1, 0.1, 1.0)})
        with self.assertRaises(TypeError):
            self.FreeCADGui.applyElementColorOverride(sel_root, {"Face1": "not-a-color"})
        with self.assertRaises(TypeError):
            self.FreeCADGui.applyElementColorOverride(sel_root, {1: (1.0, 0.1, 0.1, 1.0)})
        with self.assertRaises(TypeError):
            self.FreeCADGui.clearElementColorOverride("not-a-node-or-path")

    def test_apply_element_color_override_on_view_provider_link_targets(self):
        box = self.doc.addObject("Part::Box", "Box")
        link = self.doc.addObject("App::Link", "Link")
        link.LinkedObject = box

        self.doc.recompute()
        self.FreeCADGui.updateGui()

        link_view = link.ViewObject.LinkView
        self.assertIsNotNone(link_view, "expected ViewProviderLink to expose a LinkView handle")
        self.assertIsNotNone(
            link.ViewObject.RootNode, "expected ViewProviderLink to expose its Coin root"
        )
        self.assertIsNotNone(
            link_view.RootNode, "expected ViewProviderLink to expose its link root"
        )
        self.assertEqual(link.ViewObject.getElementColors(), {})

        self.FreeCADGui.applyElementColorOverride(
            link.ViewObject.RootNode,
            {"Face1": (1.0, 0.2, 0.2, 1.0)},
        )
        self.FreeCADGui.clearElementColorOverride(link.ViewObject.RootNode)
        self.assertEqual(link.ViewObject.getElementColors(), {})

        search = self.coin.SoSearchAction()
        search.setNode(link_view.RootNode)
        search.apply(link.ViewObject.RootNode)
        path = search.getPath()
        self.assertIsNotNone(path, "expected a Coin path to the link root")

        self.FreeCADGui.applyElementColorOverride(
            path,
            {"Face": (64, 96, 255, 255)},
        )
        self.FreeCADGui.clearElementColorOverride(path)
        self.assertEqual(link.ViewObject.getElementColors(), {})

    def test_face_override_renders_in_view(self):
        box = self.doc.addObject("Part::Box", "Box")
        link = self.doc.addObject("App::Link", "Link")
        link.LinkedObject = box

        self.doc.recompute()
        self.FreeCADGui.updateGui()

        _make_material(box.ViewObject, (0.1, 0.85, 0.15, 1.0))
        box.ViewObject.Visibility = False

        view = self.gui_doc.ActiveView
        self.assertIsNotNone(view, "expected the GUI document to expose an active 3D view")

        width = 512
        height = 512
        out_dir = Path(tempfile.gettempdir()) / "FreeCADTesting" / "ViewProviderLink"
        base_path = out_dir / "base.png"
        override_path = out_dir / "override.png"
        cleared_path = out_dir / "cleared.png"

        _save_active_view_png(view, base_path, width, height)
        br, bg, bb = _mean_rgb(base_path, width // 2, height // 2)
        self.assertGreater(bg, br + 80, f"unexpected base face color: {(br, bg, bb)}")
        self.assertGreater(bg, bb + 80, f"unexpected base face color: {(br, bg, bb)}")

        link.ViewObject.setElementColors({"Face": (0.25, 0.38, 1.0, 1.0)})
        self.FreeCADGui.updateGui()
        _save_active_view_png(view, override_path, width, height)
        or_, og, ob = _mean_rgb(override_path, width // 2, height // 2)
        self.assertGreater(
            ob, or_ + 100, f"link face override did not render blue: {(or_, og, ob)}"
        )
        self.assertGreater(ob, og + 80, f"link face override did not render blue: {(or_, og, ob)}")

        link.ViewObject.setElementColors({})
        self.FreeCADGui.updateGui()
        _save_active_view_png(view, cleared_path, width, height)
        cr, cg, cb = _mean_rgb(cleared_path, width // 2, height // 2)
        self.assertGreater(
            cg, cr + 80, f"clearing face override did not restore green: {(cr, cg, cb)}"
        )
        self.assertGreater(
            cg, cb + 80, f"clearing face override did not restore green: {(cr, cg, cb)}"
        )

    def test_link_renders_per_face_colors_without_triangle_split(self):
        box = self.doc.addObject("Part::Box", "Box")
        link = self.doc.addObject("App::Link", "Link")
        link.LinkedObject = box

        self.doc.recompute()
        self.FreeCADGui.updateGui()

        box.ViewObject.Visibility = False

        view = self.gui_doc.ActiveView
        self.assertIsNotNone(view, "expected the GUI document to expose an active 3D view")

        width = 512
        height = 512
        out_dir = Path(tempfile.gettempdir()) / "FreeCADTesting" / "ViewProviderLink"
        override_path = out_dir / "link-per-face-top.png"

        box.ViewObject.DiffuseColor = [
            (1.0, 0.0, 0.0, 1.0),
            (0.0, 1.0, 0.0, 1.0),
            (0.0, 0.0, 1.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
            (1.0, 0.0, 1.0, 1.0),
            (0.0, 1.0, 1.0, 1.0),
        ]
        self.FreeCADGui.updateGui()

        _save_active_view_png(view, override_path, width, height, view_fn="viewTop")
        bbox = _pixel_bbox(
            override_path,
            lambda r, g, b: min(r, g, b) < 250,
            region=(width // 4, height // 4, width * 3 // 4, height * 3 // 4),
        )
        self.assertIsNotNone(
            bbox, f"linked per-face colors did not render any visible pixels: {override_path}"
        )

        top_left_rgb = _mean_rgb(override_path, *_bbox_relative_point(bbox, 0.35, 0.35), radius=8)
        bottom_right_rgb = _mean_rgb(
            override_path, *_bbox_relative_point(bbox, 0.65, 0.65), radius=8
        )

        for label, rgb in (
            ("top-left triangle", top_left_rgb),
            ("bottom-right triangle", bottom_right_rgb),
        ):
            r, g, b = rgb
            self.assertGreater(
                g,
                r + 40.0,
                f"{label} should keep the linked top face's cyan color (got {rgb})",
            )
            self.assertGreater(
                b,
                r + 40.0,
                f"{label} should keep the linked top face's cyan color (got {rgb})",
            )
            self.assertGreater(
                min(g, b),
                120.0,
                f"{label} should stay visibly cyan through the link render path (got {rgb})",
            )

    def test_link_face_element_override_renders_in_view(self):
        box = self.doc.addObject("Part::Box", "Box")
        link = self.doc.addObject("App::Link", "Link")
        link.LinkedObject = box

        self.doc.recompute()
        self.FreeCADGui.updateGui()

        box.ViewObject.Visibility = False
        box.ViewObject.DiffuseColor = [
            (1.0, 0.0, 0.0, 1.0),
            (0.0, 1.0, 0.0, 1.0),
            (0.0, 0.0, 1.0, 1.0),
            (1.0, 1.0, 0.0, 1.0),
            (1.0, 0.0, 1.0, 1.0),
            (0.0, 1.0, 1.0, 1.0),
        ]
        self.FreeCADGui.updateGui()

        view = self.gui_doc.ActiveView
        self.assertIsNotNone(view, "expected the GUI document to expose an active 3D view")

        width = 512
        height = 512
        out_dir = Path(tempfile.gettempdir()) / "FreeCADTesting" / "ViewProviderLink"
        base_path = out_dir / "link-face6-base.png"
        override_path = out_dir / "link-face6-override.png"

        _save_active_view_png(view, base_path, width, height, view_fn="viewTop")
        br, bg, bb = _mean_rgb(base_path, width // 2, height // 2)
        self.assertGreater(
            bg,
            br + 60.0,
            f"expected the linked top face to start cyan/green-dominant: {(br, bg, bb)}",
        )
        self.assertGreater(
            bb,
            br + 60.0,
            f"expected the linked top face to start cyan/blue-dominant: {(br, bg, bb)}",
        )

        link.ViewObject.setElementColors({"Face6": (1.0, 0.0, 0.0, 1.0)})
        self.FreeCADGui.updateGui()

        _save_active_view_png(view, override_path, width, height, view_fn="viewTop")
        or_, og, ob = _mean_rgb(override_path, width // 2, height // 2)
        self.assertGreater(
            or_,
            og + 80.0,
            f"link Face6 override did not render red on the visible top face: {(or_, og, ob)}",
        )
        self.assertGreater(
            or_,
            ob + 80.0,
            f"link Face6 override did not render red on the visible top face: {(or_, og, ob)}",
        )
