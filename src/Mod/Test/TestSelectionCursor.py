# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Joao Matos
# SPDX-FileNotice: Part of the FreeCAD project.

"""Regression coverage for rejected preselection cursor restoration."""

import unittest

from CoinSnapshotHarness import (
    _QtCoreModule,
    _QtGuiModule,
    _QtWidgetsModule,
    _pump_gui_events,
    _require_gui,
    _send_mouse_motion,
)


class SelectionCursorTestCase(unittest.TestCase):
    def test_forbidden_cursor_restores_after_preselection_clears(self):
        FreeCAD, FreeCADGui, _ = _require_gui()
        if _QtGuiModule is None or _QtCoreModule is None or _QtWidgetsModule is None:
            raise unittest.SkipTest("Qt bindings are unavailable")

        try:
            import Part
        except Exception as exc:
            raise unittest.SkipTest(f"Part is unavailable: {exc}") from exc

        doc = FreeCAD.newDocument("CoinCursorRestoration")
        gui_doc = FreeCADGui.getDocument(doc.Name)
        try:
            allowed_box = doc.addObject("Part::Box", "AllowedBox")
            allowed_box.Length = 10.0
            allowed_box.Width = 10.0
            allowed_box.Height = 10.0
            allowed_box.Placement.Base = FreeCAD.Vector(-15.0, -5.0, 0.0)

            rejected_box = doc.addObject("Part::Box", "RejectedBox")
            rejected_box.Length = 10.0
            rejected_box.Width = 10.0
            rejected_box.Height = 10.0
            rejected_box.Placement.Base = FreeCAD.Vector(5.0, -5.0, 0.0)
            doc.recompute()

            view = gui_doc.ActiveView
            viewer = view.getViewer()
            viewer.setRenderPipeline("DrawList")
            self.assertEqual(viewer.getRenderPipeline(), "DrawList")
            view.viewTop()
            view.setCameraType("Orthographic")
            view.fitAll()
            view.redraw()
            _pump_gui_events(FreeCADGui, cycles=8)

            class AllowOnlyAllowedBox:
                def allow(self, _doc, obj, _sub):
                    return obj is not None and obj.Name == allowed_box.Name

            FreeCADGui.Selection.addSelectionGate(AllowOnlyAllowedBox())
            graphics_view = view.graphicsView()
            viewport = graphics_view.viewport()
            center = viewport.rect().center()
            allowed_position = center + _QtCoreModule.QPoint(-viewport.width() // 4, 0)
            rejected_position = center + _QtCoreModule.QPoint(viewport.width() // 4, 0)

            _send_mouse_motion(viewport, allowed_position)
            self.assertEqual(
                graphics_view.cursor().shape(),
                _QtCoreModule.Qt.ArrowCursor,
                "an allowed preselection should not show the forbidden cursor",
            )

            # Keep this transition synchronous: the old implementation restored the
            # cursor from GLRenderBelowPath(), which could hide this regression.
            _send_mouse_motion(viewport, rejected_position)
            self.assertEqual(
                graphics_view.cursor().shape(),
                _QtCoreModule.Qt.ForbiddenCursor,
                "a rejected preselection should show the forbidden cursor",
            )

            # A successful programmatic selection must not clear feedback for the
            # currently rejected hover candidate.
            FreeCADGui.Selection.addSelection(allowed_box)
            self.assertEqual(
                graphics_view.cursor().shape(),
                _QtCoreModule.Qt.ForbiddenCursor,
                "a successful selection should not clear a rejected hover cursor",
            )

            _send_mouse_motion(viewport, _QtCoreModule.QPoint(1, 1))
            self.assertEqual(
                graphics_view.cursor().shape(),
                _QtCoreModule.Qt.ArrowCursor,
                "clearing a rejected preselection should restore the arrow cursor",
            )
        finally:
            FreeCADGui.Selection.removeSelectionGate()
            if FreeCAD.getDocument(doc.Name):
                FreeCAD.closeDocument(doc.Name)
