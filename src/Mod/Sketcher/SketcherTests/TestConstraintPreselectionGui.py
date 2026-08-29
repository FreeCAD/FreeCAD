# SPDX-License-Identifier: LGPL-2.1-or-later

import math
import time
import unittest

import FreeCAD
import FreeCADGui
import Part
import Sketcher
import SketcherGui
from PySide6 import QtCore, QtGui, QtWidgets


class SketcherGuiTestCases(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not FreeCAD.GuiUp:
            raise unittest.SkipTest("Cannot run GUI tests in a CLI environment.")

        FreeCADGui.getMainWindow().show()
        cls.pump_gui_events()

    @staticmethod
    def pump_gui_events(iterations=6, delay=0.01):
        app = QtWidgets.QApplication.instance()
        for _ in range(iterations):
            app.processEvents(QtCore.QEventLoop.AllEvents, int(delay * 1000))
            if delay > 0.0:
                time.sleep(delay)

    @staticmethod
    def build_issue_20811_sketch(sketch):
        # Mirrors ConstraintSelect.FCStd from issue #20811 without requiring a binary fixture.
        first_line = sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(0.0, 0.0, 0.0),
                FreeCAD.Vector(0.0, 24.181930730600406, 0.0),
            ),
            False,
        )
        second_line = sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(0.0, 24.181930730600406, 0.0),
                FreeCAD.Vector(14.77492472860838, 30.362974758199655, 0.0),
            ),
            False,
        )
        arc = sketch.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(
                    FreeCAD.Vector(20.44726296326554, 16.80404036490776, 0.0),
                    FreeCAD.Vector(0.0, 0.0, 1.0),
                    14.697623036734457,
                ),
                0.0,
                1.96702,
            ),
            False,
        )
        third_line = sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(35.144886, 16.80404036490776, 0.0),
                FreeCAD.Vector(35.144886, 14.202975, 0.0),
            ),
            False,
        )
        fourth_line = sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(35.144886, 14.202975, 0.0),
                FreeCAD.Vector(0.0, 0.0, 0.0),
            ),
            False,
        )

        sketch.addConstraint(Sketcher.Constraint("Coincident", -1, 1, first_line, 1))
        sketch.addConstraint(Sketcher.Constraint("PointOnObject", first_line, 2, -2))
        sketch.addConstraint(Sketcher.Constraint("Coincident", first_line, 2, second_line, 1))
        tangent_one = sketch.addConstraint(Sketcher.Constraint("Tangent", second_line, 2, arc, 2))
        tangent_two = sketch.addConstraint(Sketcher.Constraint("Tangent", arc, 1, third_line, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", third_line, 2, fourth_line, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", fourth_line, 2, first_line, 1))
        sketch.addConstraint(Sketcher.Constraint("Vertical", third_line))

        return tangent_one, tangent_two

    def use_attached_issue_sketch(self):
        FreeCADGui.ActiveDocument.resetEdit()
        self.doc.removeObject(self.sketch.Name)
        body = self.doc.addObject("PartDesign::Body", "Body")
        sketch = self.doc.addObject("Sketcher::SketchObject", "IssueSketch")
        body.addObject(sketch)
        sketch.AttachmentSupport = [(body.Origin.OriginFeatures[3], "")]
        sketch.MapMode = "FlatFace"
        self.sketch = sketch
        self.doc.recompute()
        FreeCADGui.ActiveDocument.setEdit(sketch.Name)
        self.pump_gui_events(6)
        self.view = FreeCADGui.ActiveDocument.ActiveView

    @staticmethod
    def classify_preselection(info, expected_constraint_name):
        if not info or not info["ObjectName"]:
            return "none"

        names = info.get("SubElementNames") or []
        if expected_constraint_name in names:
            return "target_constraint"
        if any(name.startswith("Constraint") for name in names):
            return "other_constraint"
        if any(name.startswith("Vertex") for name in names):
            return "vertex"
        if any(name.startswith("Edge") for name in names):
            return "edge"
        if any(name.endswith("_Axis") for name in names):
            return "axis"
        return "other"

    @staticmethod
    def project_coin_path_point_to_viewport(view_volume, viewport_region, point):
        projected = view_volume.projectToScreen(point)
        origin_x, origin_y = viewport_region.getViewportOriginPixels()
        width, height = viewport_region.getViewportSizePixels()
        return (
            projected[0] * width + origin_x,
            projected[1] * height + origin_y,
        )

    def get_rendered_constraint_icon_geometry(self, constraint_id):
        """Return Coin's current screen geometry for a rendered constraint icon.

        The current camera view volume is supplied to SoGetMatrixAction on a
        scenegraph copy, without refreshing the live SoZoomTranslation cache
        used by the picker. The actual visible icon is selected when multiple
        constraints share one image.
        """
        from pivy import coin

        viewer = self.view.getViewer()
        scene_root = viewer.getSoRenderManager().getSceneGraph()
        matrix_root = scene_root.copy(True)
        matrix_root.ref()
        search = coin.SoSearchAction()
        search.setName("ConstraintGroup")
        search.setSearchingAll(True)
        search.apply(matrix_root)
        self.assertTrue(search.isFound(), "Could not find ConstraintGroup in the scene graph")

        group_path = search.getPath()
        group = group_path.getTail()
        separator = None
        icon = None
        visible_ids = None
        target_id = str(constraint_id)
        for separator_index in range(group.getNumChildren()):
            candidate_separator = group.getChild(separator_index)
            if not candidate_separator.isOfType(coin.SoSeparator.getClassTypeId()):
                continue

            for candidate_icon_index in (2, 5):
                if candidate_icon_index + 1 >= candidate_separator.getNumChildren():
                    continue
                candidate_icon = candidate_separator.getChild(candidate_icon_index)
                candidate_info = candidate_separator.getChild(candidate_icon_index + 1)
                if not candidate_icon.isOfType(coin.SoImage.getClassTypeId()):
                    continue
                if not candidate_info.isOfType(coin.SoInfo.getClassTypeId()):
                    continue

                ids = candidate_info.string.getValue().getString().split(",")
                icon_size = None
                try:
                    icon_size = candidate_icon.image.getValue()[1]
                    if candidate_icon.width.getValue() != -1:
                        icon_size[0] = candidate_icon.width.getValue()
                    if candidate_icon.height.getValue() != -1:
                        icon_size[1] = candidate_icon.height.getValue()
                except (UnicodeDecodeError, RuntimeError):
                    # Coin may expose image metadata that cannot be decoded by
                    # every backend, so treat the icon size as unknown.
                    icon_size = None
                if target_id in ids and (
                    icon_size is None or (icon_size[0] > 0 and icon_size[1] > 0)
                ):
                    separator = candidate_separator
                    icon = candidate_icon
                    visible_ids = ids
                    break
            if icon is not None:
                break

        self.assertIsNotNone(
            icon,
            f"Could not find a visible icon for Constraint{constraint_id + 1}",
        )

        viewport_region = viewer.getSoRenderManager().getViewportRegion()
        camera = viewer.getSoRenderManager().getCamera()
        self.assertIsNotNone(camera, "Could not find the active camera")
        render_viewport_region = coin.SbViewportRegion(viewport_region)
        view_volume = camera.getViewVolume(viewport_region, render_viewport_region)
        icon_path = group_path.copy()
        icon_path.append(separator)
        icon_path.append(icon)
        icon_path.ref()
        try:
            matrix_action = coin.SoGetMatrixAction(viewport_region)
            coin.SoViewVolumeElement.set(matrix_action.getState(), icon, view_volume)
            coin.SoViewportRegionElement.set(matrix_action.getState(), render_viewport_region)
            matrix_action.apply(icon_path)
            world_center = matrix_action.getMatrix().multVecMatrix(coin.SbVec3f(0.0, 0.0, 0.0))

            bbox_action = coin.SoGetBoundingBoxAction(viewport_region)
            coin.SoViewVolumeElement.set(bbox_action.getState(), icon, view_volume)
            coin.SoViewportRegionElement.set(bbox_action.getState(), render_viewport_region)
            bbox_action.apply(icon_path)
            lower_bound = coin.SbVec3f()
            upper_bound = coin.SbVec3f()
            bbox_action.getBoundingBox().getBounds(lower_bound, upper_bound)
        finally:
            icon_path.unref()

        screen_center = self.project_coin_path_point_to_viewport(
            view_volume, render_viewport_region, world_center
        )
        lower_screen = self.project_coin_path_point_to_viewport(
            view_volume, render_viewport_region, lower_bound
        )
        upper_screen = self.project_coin_path_point_to_viewport(
            view_volume, render_viewport_region, upper_bound
        )
        image_width = abs(upper_screen[0] - lower_screen[0])
        image_height = abs(upper_screen[1] - lower_screen[1])
        # Merged constraint images stack one rendered constraint entry per row.
        target_row = visible_ids.index(target_id)
        row_height = image_height / len(visible_ids)
        target_center_y = screen_center[1] + image_height / 2.0 - row_height * (target_row + 0.5)
        geometry_values = (*screen_center, image_width, row_height, target_center_y)
        if not all(math.isfinite(value) for value in geometry_values):
            matrix_root.unref()
            raise RuntimeError(
                f"Rendered constraint icon geometry is not finite: {geometry_values}; "
                f"world_center={tuple(world_center)}, lower={tuple(lower_bound)}, "
                f"upper={tuple(upper_bound)}, viewport={render_viewport_region.getViewportSizePixels()}, "
                f"vv={(view_volume.getWidth(), view_volume.getHeight(), view_volume.getDepth())}"
            )

        rendered_center = (
            int(round(screen_center[0])),
            int(round(target_center_y)),
        )
        ray_pick = coin.SoRayPickAction(viewport_region)
        ray_pick.setPoint(coin.SbVec2s(*rendered_center))
        ray_pick.setRadius(0.0)
        ray_pick.setPickAll(True)
        ray_pick.apply(matrix_root)
        icon_is_coin_pickable = any(
            any(
                picked_point.getPath().getNode(node_index) == icon
                for node_index in range(picked_point.getPath().getLength())
            )
            for picked_point in ray_pick.getPickedPointList()
        )
        if not icon_is_coin_pickable:
            matrix_root.unref()
            raise RuntimeError(
                f"Coin did not pick the rendered constraint icon at {rendered_center}"
            )

        matrix_root.unref()
        return (
            rendered_center,
            (image_width, row_height),
        )

    def get_stable_rendered_constraint_icon_geometry(self, constraint_id, timeout=2.0):
        deadline = time.monotonic() + timeout
        previous = None
        last_error = None

        while time.monotonic() < deadline:
            try:
                geometry = self.get_rendered_constraint_icon_geometry(constraint_id)
            except (AssertionError, RuntimeError) as error:
                last_error = error
            else:
                if previous is not None:
                    previous_center, previous_size = previous
                    center, size = geometry
                    if (
                        center != (0, 0)
                        and all(value > 0.0 for value in size)
                        and center == previous_center
                        and all(
                            math.isclose(left, right, abs_tol=0.01)
                            for left, right in zip(size, previous_size)
                        )
                    ):
                        return geometry
                previous = geometry

            self.view.redraw()
            self.pump_gui_events(1)

        if last_error is not None:
            message = f"Rendered constraint icon did not stabilize: {last_error}"
        else:
            message = f"Rendered constraint icon did not stabilize: {previous}"
        raise AssertionError(message)

    def rendered_center_to_qpoint(self, rendered_center):
        viewport = self.view.graphicsView().viewport()
        width, height = self.view.getSize()
        scale = viewport.devicePixelRatioF()
        return QtCore.QPoint(
            int(round(rendered_center[0] / scale)),
            int(round((height - rendered_center[1] - 1) / scale)),
        )

    def move_mouse_to_rendered_center(self, rendered_center):
        viewport = self.view.graphicsView().viewport()
        position = self.rendered_center_to_qpoint(rendered_center)
        event = QtGui.QMouseEvent(
            QtCore.QEvent.Type.MouseMove,
            position,
            viewport.mapToGlobal(position),
            QtCore.Qt.MouseButton.NoButton,
            QtCore.Qt.MouseButton.NoButton,
            QtCore.Qt.KeyboardModifier.NoModifier,
        )
        QtWidgets.QApplication.sendEvent(viewport, event)
        self.pump_gui_events(12)

    def preselect_constraint_within_rendered_icon(
        self, rendered_center, rendered_size, expected_constraint_name
    ):
        max_offset_x = max(0, int(math.floor(rendered_size[0] / 2.0)) - 2)
        max_offset_y = max(0, int(math.floor(rendered_size[1] / 2.0)) - 2)
        offsets = []
        for offset_y in (0, -max_offset_y, max_offset_y):
            for offset_x in (0, -max_offset_x, max_offset_x):
                offset = (offset_x, offset_y)
                if offset not in offsets:
                    offsets.append(offset)

        attempts = []
        for offset_x, offset_y in offsets:
            probe = (rendered_center[0] + offset_x, rendered_center[1] + offset_y)
            FreeCADGui.Selection.clearPreselection()
            self.move_mouse_to_rendered_center(probe)
            preselection = FreeCADGui.Selection.getPreselection()
            names = tuple(preselection.SubElementNames or [])
            attempts.append((probe, preselection.ObjectName, names))
            if preselection.ObjectName == self.sketch.Name and expected_constraint_name in names:
                return probe, attempts

        return None, attempts

    @classmethod
    def configure_view_state(cls, view, tilt=None):
        view.viewTop()
        cls.pump_gui_events()
        view.fitAll()
        cls.pump_gui_events()

        if tilt is not None:
            base_rotation = view.getCameraOrientation()
            view.setCameraOrientation(tilt.multiply(base_rotation))
            cls.pump_gui_events()
            view.fitAll()
            cls.pump_gui_events()

    @classmethod
    def get_stable_viewport_point(cls, view, world_point):
        deadline = time.monotonic() + 2.0
        previous = None
        point = (0, 0)
        while time.monotonic() < deadline:
            point = tuple(int(value) for value in view.getPointOnViewport(world_point))
            if point != (0, 0) and point == previous:
                return point
            if point == (0, 0):
                view.fitAll()
            view.redraw()
            previous = point
            cls.pump_gui_events(1)
        raise AssertionError(f"Viewport point did not stabilize: {point}")

    def setUp(self):
        self.doc = FreeCAD.newDocument("SketchGuiTest")
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        self.doc.recompute()

        FreeCADGui.getMainWindow().show()
        self.pump_gui_events()
        FreeCADGui.ActiveDocument.setEdit(self.sketch.Name)
        self.pump_gui_events()

        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.pump_gui_events(12)
        graphics_view = self.view.graphicsView()
        graphics_view.resize(600, 600)
        self.pump_gui_events(8)
        self.view.viewTop()
        self.pump_gui_events(4)
        self.view.fitAll()
        self.pump_gui_events(8)

        self.viewer = self.view.getViewer()
        self.original_pick_radius = self.viewer.getPickRadius()
        self.viewer.setPickRadius(3.0)

    def tearDown(self):
        self.viewer.setPickRadius(self.original_pick_radius)
        FreeCADGui.Selection.clearPreselection()
        FreeCADGui.Selection.clearSelection()
        if FreeCADGui.ActiveDocument:
            FreeCADGui.ActiveDocument.resetEdit()
        self.pump_gui_events()

        if self.doc is not None:
            document_name = self.doc.Name
            self.doc = None
            FreeCAD.closeDocument(document_name)
            self.pump_gui_events()

    def testConstraintIconMousePreselectionMatchesRenderedPosition(self):
        self.use_attached_issue_sketch()
        _, tangent_id = self.build_issue_20811_sketch(self.sketch)
        expected_name = f"Constraint{tangent_id + 1}"
        self.doc.recompute()
        self.pump_gui_events()

        self.configure_view_state(self.view)
        viewer = self.view.getViewer()
        old_pick_radius = viewer.getPickRadius()
        viewer.setPickRadius(0.5)
        graphics_view = self.view.graphicsView()
        original_size = graphics_view.size()
        main_window = FreeCADGui.getMainWindow()
        original_window_size = main_window.size()
        failures = []

        try:
            for size in ((900, 400), (600, 600), (600, 860), (400, 900)):
                graphics_view.resize(*size)
                self.pump_gui_events(12)
                self.view.fitAll()
                self.pump_gui_events(12)

                width, height = self.view.getSize()
                rendered_center, rendered_size = self.get_stable_rendered_constraint_icon_geometry(
                    tangent_id
                )
                selected_probe, attempts = self.preselect_constraint_within_rendered_icon(
                    rendered_center, rendered_size, expected_name
                )
                if selected_probe is None:
                    failures.append(
                        f"viewport={width}x{height}, rendered_center={rendered_center}, "
                        f"rendered_size={rendered_size}, attempts={attempts}"
                    )
        finally:
            viewer.setPickRadius(old_pick_radius)
            QtWidgets.QApplication.sendEvent(
                graphics_view.viewport(),
                QtCore.QEvent(QtCore.QEvent.Type.Leave),
            )
            FreeCADGui.Selection.clearPreselection()
            graphics_view.resize(original_size)
            main_window.resize(original_window_size)
            self.pump_gui_events(4)
            self.view.fitAll()
            self.pump_gui_events(4)

        self.assertFalse(failures, "\n".join(failures))

    def testConstraintIconMousePreselectionUpdatesAfterViewportAspectRatioChange(self):
        self.use_attached_issue_sketch()
        _, tangent_id = self.build_issue_20811_sketch(self.sketch)
        expected_name = f"Constraint{tangent_id + 1}"
        self.doc.recompute()
        self.pump_gui_events(12)

        self.configure_view_state(self.view)
        viewer = self.view.getViewer()
        old_pick_radius = viewer.getPickRadius()
        viewer.setPickRadius(0.5)
        graphics_view = self.view.graphicsView()
        original_size = graphics_view.size()
        main_window = FreeCADGui.getMainWindow()
        original_window_size = main_window.size()
        failures = []

        try:
            for state, size in (
                ("square_before", (600, 600)),
                ("tall", (400, 900)),
                ("square_after", (600, 600)),
            ):
                graphics_view.resize(*size)
                self.pump_gui_events(12)
                if state == "square_before":
                    self.view.fitAll()
                    self.pump_gui_events(12)

                width, height = self.view.getSize()
                rendered_center, rendered_size = self.get_stable_rendered_constraint_icon_geometry(
                    tangent_id
                )
                selected_probe, attempts = self.preselect_constraint_within_rendered_icon(
                    rendered_center, rendered_size, expected_name
                )
                if selected_probe is None:
                    failures.append(
                        f"state={state}, viewport={width}x{height}, "
                        f"aspect={width / height:.3f}, rendered_center={rendered_center}, "
                        f"rendered_size={rendered_size}, attempts={attempts}"
                    )
        finally:
            viewer.setPickRadius(old_pick_radius)
            QtWidgets.QApplication.sendEvent(
                graphics_view.viewport(),
                QtCore.QEvent(QtCore.QEvent.Type.Leave),
            )
            FreeCADGui.Selection.clearPreselection()
            graphics_view.resize(original_size)
            main_window.resize(original_window_size)
            self.pump_gui_events(4)
            self.view.fitAll()
            self.pump_gui_events(4)

        self.assertFalse(failures, "\n".join(failures))

    def testNearestOverlappingConstraintIconPreselection(self):
        first_line = self.sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(-10.0, 0.0, 0.0),
                FreeCAD.Vector(-10.0, 20.0, 0.0),
            ),
            False,
        )
        second_line = self.sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(10.0, 0.0, 0.0),
                FreeCAD.Vector(10.0, 20.0, 0.0),
            ),
            False,
        )
        first_constraint = self.sketch.addConstraint(Sketcher.Constraint("Vertical", first_line))
        second_constraint = self.sketch.addConstraint(Sketcher.Constraint("Vertical", second_line))
        self.doc.recompute()
        self.pump_gui_events(8)
        self.view.graphicsView().resize(600, 600)
        self.pump_gui_events(8)
        self.configure_view_state(self.view)
        self.pump_gui_events(12)

        viewer = self.view.getViewer()
        viewer.setPickRadius(5.0)

        from pivy import coin

        search = coin.SoSearchAction()
        search.setName("ConstraintGroup")
        search.setSearchingAll(True)
        search.apply(self.view.getViewer().getSoRenderManager().getSceneGraph())
        self.assertTrue(search.isFound(), "Could not find ConstraintGroup in the scene graph")
        group = search.getPath().getTail()

        first_translation = group.getChild(first_constraint).getChild(1)
        second_translation = group.getChild(second_constraint).getChild(1)
        first_translation.translation = coin.SbVec3f(0.0, 0.0, 0.0)
        second_translation.translation = coin.SbVec3f(0.0, 0.0, 0.0)

        first_translation.abPos = coin.SbVec3f(0.0, 0.0, 0.004)
        reference_delta = 100.0
        first_reference = self.get_stable_viewport_point(self.view, FreeCAD.Vector(0.0, 0.0, 0.0))
        second_reference = self.get_stable_viewport_point(
            self.view, FreeCAD.Vector(reference_delta, 0.0, 0.0)
        )
        pixels_per_unit = abs(second_reference[0] - first_reference[0]) / reference_delta
        self.assertGreater(pixels_per_unit, 0.0)
        pixel_delta = 10.0 / pixels_per_unit

        second_translation.abPos = coin.SbVec3f(pixel_delta, 0.0, 0.004)
        self.pump_gui_events(8)
        first_center = self.get_stable_rendered_constraint_icon_geometry(first_constraint)[0]
        second_center = self.get_stable_rendered_constraint_icon_geometry(second_constraint)[0]
        distance = abs(second_center[0] - first_center[0])
        self.assertTrue(
            8.0 <= distance <= 12.0,
            f"Could not place icons in overlapping hit regions: distance={distance}, "
            f"viewport={self.view.getSize()}",
        )
        probe = (
            int(round(second_center[0] - 4.0)),
            int(round(second_center[1])),
        )
        info = SketcherGui.getActiveSketchPreselection(probe)
        expected = f"Constraint{second_constraint + 1}"

        self.assertEqual(
            self.classify_preselection(info, expected),
            "target_constraint",
            f"first_center={first_center}, second_center={second_center}, "
            f"probe={probe}, preselection={info}",
        )

    def testConstraintIconPreselectionHonorsSelectionRadius(self):
        line_id = self.sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(-20.0, -10.0, 0.0),
                FreeCAD.Vector(-20.0, 10.0, 0.0),
            ),
            False,
        )
        constraint_id = self.sketch.addConstraint(Sketcher.Constraint("Vertical", line_id))
        expected_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()
        self.pump_gui_events(12)

        self.configure_view_state(self.view)
        viewer = self.view.getViewer()
        old_pick_radius = viewer.getPickRadius()
        viewer.setPickRadius(0.5)
        graphics_view = self.view.graphicsView()
        original_size = graphics_view.size()
        main_window = FreeCADGui.getMainWindow()
        original_window_size = main_window.size()

        try:
            graphics_view.resize(600, 600)
            self.pump_gui_events(12)
            self.view.fitAll()
            self.pump_gui_events(12)

            from pivy import coin

            search = coin.SoSearchAction()
            search.setName("ConstraintGroup")
            search.setSearchingAll(True)
            search.apply(viewer.getSoRenderManager().getSceneGraph())
            self.assertTrue(search.isFound(), "Could not find ConstraintGroup in the scene graph")
            icon_translation = search.getPath().getTail().getChild(constraint_id).getChild(1)
            icon_translation.translation = coin.SbVec3f(0.0, 0.0, 0.0)
            icon_translation.abPos = coin.SbVec3f(20.0, 20.0, 0.004)
            self.pump_gui_events(8)

            rendered_center, rendered_size = self.get_stable_rendered_constraint_icon_geometry(
                constraint_id
            )
            half_width = rendered_size[0] / 2.0

            probes = (
                (0.5, 0.0, "target_constraint"),
                (0.5, 2.0, "none"),
                (5.0, 4.0, "target_constraint"),
                (5.0, 7.0, "none"),
                (35.0, 20.0, "target_constraint"),
                (35.0, 38.0, "none"),
            )
            results = []
            for pick_radius, distance_outside, expected in probes:
                viewer.setPickRadius(pick_radius)
                self.pump_gui_events(2)
                probe_x = rendered_center[0]
                if distance_outside:
                    probe_x += half_width + distance_outside
                probe = (
                    int(round(probe_x)),
                    rendered_center[1],
                )
                FreeCADGui.Selection.clearPreselection()
                self.move_mouse_to_rendered_center(probe)
                preselection = FreeCADGui.Selection.getPreselection()
                names = preselection.SubElementNames or []
                actual = (
                    "target_constraint"
                    if preselection.ObjectName == self.sketch.Name and expected_name in names
                    else "none"
                )
                results.append((pick_radius, distance_outside, probe, expected, actual, names))

            detail = (
                f"rendered_center={rendered_center}, rendered_size={rendered_size}, "
                f"results={results}"
            )
            self.assertEqual(
                [result[4] for result in results],
                [result[3] for result in results],
                detail,
            )
        finally:
            viewer.setPickRadius(old_pick_radius)
            QtWidgets.QApplication.sendEvent(
                graphics_view.viewport(),
                QtCore.QEvent(QtCore.QEvent.Type.Leave),
            )
            FreeCADGui.Selection.clearPreselection()
            graphics_view.resize(original_size)
            main_window.resize(original_window_size)
            self.pump_gui_events(4)
            self.view.fitAll()
            self.pump_gui_events(4)

    def testPointPreselectionHonorsSelectionRadius(self):
        point = FreeCAD.Vector(20.0, 20.0, 0.0)
        self.sketch.addGeometry(Part.Point(point), False)
        self.doc.recompute()
        self.pump_gui_events(8)

        self.configure_view_state(self.view)
        viewer = self.view.getViewer()
        old_pick_radius = viewer.getPickRadius()
        viewer.setPickRadius(0.5)

        try:
            center = self.get_stable_viewport_point(self.view, point)
            probes = (
                (0.5, 6, "none"),
                (5.0, 8, "vertex"),
                (35.0, 25, "vertex"),
                (35.0, 45, "none"),
            )
            results = []
            for pick_radius, offset, expected in probes:
                viewer.setPickRadius(pick_radius)
                self.pump_gui_events(2)
                info = SketcherGui.getActiveSketchPreselection((center[0] + offset, center[1]))
                actual = self.classify_preselection(info, "Constraint0")
                results.append((pick_radius, offset, expected, actual, info))

            self.assertEqual(
                [result[3] for result in results],
                [result[2] for result in results],
                f"center={center}, results={results}",
            )
        finally:
            viewer.setPickRadius(old_pick_radius)
            FreeCADGui.Selection.clearPreselection()
            self.pump_gui_events(4)

    def testCurvePreselectionHonorsSelectionRadius(self):
        start = FreeCAD.Vector(20.0, 20.0, 0.0)
        end = FreeCAD.Vector(40.0, 20.0, 0.0)
        self.sketch.addGeometry(Part.LineSegment(start, end), False)
        self.doc.recompute()
        self.pump_gui_events(8)

        self.configure_view_state(self.view)
        viewer = self.view.getViewer()
        old_pick_radius = viewer.getPickRadius()
        viewer.setPickRadius(0.5)

        try:
            midpoint = FreeCAD.Vector(30.0, 20.0, 0.0)
            center = self.get_stable_viewport_point(self.view, midpoint)
            probes = (
                (0.5, 2, "none"),
                (5.0, 4, "edge"),
                (35.0, 20, "edge"),
                (35.0, 38, "none"),
            )
            results = []
            for pick_radius, offset, expected in probes:
                viewer.setPickRadius(pick_radius)
                self.pump_gui_events(2)
                info = SketcherGui.getActiveSketchPreselection((center[0], center[1] + offset))
                actual = self.classify_preselection(info, "Constraint0")
                results.append((pick_radius, offset, expected, actual, info))

            self.assertEqual(
                [result[3] for result in results],
                [result[2] for result in results],
                f"center={center}, results={results}",
            )
        finally:
            viewer.setPickRadius(old_pick_radius)
            FreeCADGui.Selection.clearPreselection()
            self.pump_gui_events(4)

    def testPointMarkerWinsOverOverlappingConstraintLabel(self):
        start_point = FreeCAD.Vector(80.0, 100.0, 0.0)
        end_point = FreeCAD.Vector(120.0, 140.0, 0.0)
        marker_point = FreeCAD.Vector(92.0, 88.0, 0.0)

        line_id = self.sketch.addGeometry(
            Part.LineSegment(start_point, end_point),
            False,
        )
        self.sketch.addGeometry(Part.Point(marker_point), False)
        self.doc.recompute()
        self.pump_gui_events()

        self.configure_view_state(self.view)

        marker_coin = self.get_stable_viewport_point(self.view, marker_point)

        vertex_offsets = []
        for dy in range(-12, 13, 2):
            for dx in range(-12, 13, 2):
                probe_coin = (marker_coin[0] + dx, marker_coin[1] + dy)
                probe_info = SketcherGui.getActiveSketchPreselection(probe_coin)
                probe_kind = self.classify_preselection(probe_info, "Constraint0")
                if probe_kind == "vertex":
                    vertex_offsets.append((dx, dy))

        constraint_id = self.sketch.addConstraint(
            Sketcher.Constraint("Distance", line_id, 1, line_id, 2, 40.0)
        )
        self.sketch.setLabelDistance(constraint_id, -12.0 * math.sqrt(2.0))
        self.sketch.setLabelPosition(constraint_id, 0.0)
        self.expected_constraint_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()
        self.pump_gui_events()

        marker_info = SketcherGui.getActiveSketchPreselection(marker_coin)
        marker_kind = self.classify_preselection(marker_info, self.expected_constraint_name)

        probe_results = []
        for dx, dy in vertex_offsets:
            probe_coin = (marker_coin[0] + dx, marker_coin[1] + dy)
            probe_info = SketcherGui.getActiveSketchPreselection(probe_coin)
            probe_kind = self.classify_preselection(probe_info, self.expected_constraint_name)
            probe_results.append((dx, dy, probe_kind, probe_info))

        unexpected_probe_results = [result for result in probe_results if result[2] != "vertex"]

        detail = (
            f"marker_info={marker_info}, vertex_offsets={vertex_offsets}, "
            f"probe_results={probe_results}, marker_coin={marker_coin}"
        )

        self.assertGreater(len(vertex_offsets), 0, detail)
        self.assertEqual(marker_kind, "vertex", detail)
        self.assertEqual(unexpected_probe_results, [], detail)

    def testCurveWinsOverOverlappingDistanceDimensionLine(self):
        start_point = FreeCAD.Vector(80.0, 100.0, 0.0)
        end_point = FreeCAD.Vector(130.0, 100.0, 0.0)
        midpoint = (start_point + end_point) * 0.5

        line_id = self.sketch.addGeometry(
            Part.LineSegment(start_point, end_point),
            False,
        )
        self.doc.recompute()
        self.pump_gui_events()

        self.configure_view_state(self.view)

        midpoint_coin = self.get_stable_viewport_point(self.view, midpoint)

        edge_offsets = []
        for dy in range(-10, 11, 2):
            for dx in range(-16, 17, 2):
                probe_coin = (midpoint_coin[0] + dx, midpoint_coin[1] + dy)
                probe_info = SketcherGui.getActiveSketchPreselection(probe_coin)
                probe_kind = self.classify_preselection(probe_info, "Constraint0")
                if probe_kind == "edge":
                    edge_offsets.append((dx, dy))

        constraint_id = self.sketch.addConstraint(
            Sketcher.Constraint(
                "Distance",
                line_id,
                1,
                line_id,
                2,
                start_point.distanceToPoint(end_point),
            )
        )
        self.sketch.setLabelDistance(constraint_id, 0.0)
        self.sketch.setLabelPosition(constraint_id, 12.0)
        self.expected_constraint_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()
        self.pump_gui_events()

        probe_results = []
        for dx, dy in edge_offsets:
            probe_coin = (midpoint_coin[0] + dx, midpoint_coin[1] + dy)
            probe_info = SketcherGui.getActiveSketchPreselection(probe_coin)
            probe_kind = self.classify_preselection(probe_info, self.expected_constraint_name)
            probe_results.append((dx, dy, probe_kind, probe_info))

        unexpected_probe_results = [result for result in probe_results if result[2] != "edge"]

        detail = (
            f"edge_offsets={edge_offsets}, probe_results={probe_results}, "
            f"midpoint_coin={midpoint_coin}"
        )

        self.assertGreater(len(edge_offsets), 0, detail)
        self.assertEqual(unexpected_probe_results, [], detail)

    def testDistanceDatumTextWinsOverOverlappingCurve(self):
        start_point = FreeCAD.Vector(80.0, 100.0, 0.0)
        end_point = FreeCAD.Vector(130.0, 100.0, 0.0)
        midpoint = (start_point + end_point) * 0.5

        line_id = self.sketch.addGeometry(
            Part.LineSegment(start_point, end_point),
            False,
        )
        self.doc.recompute()
        self.pump_gui_events()

        self.configure_view_state(self.view)

        midpoint_coin = tuple(int(value) for value in self.view.getPointOnViewport(midpoint))

        before_info = SketcherGui.getActiveSketchPreselection(midpoint_coin)
        before_kind = self.classify_preselection(before_info, "Constraint0")

        constraint_id = self.sketch.addConstraint(
            Sketcher.Constraint(
                "Distance",
                line_id,
                1,
                line_id,
                2,
                start_point.distanceToPoint(end_point),
            )
        )
        self.sketch.setLabelDistance(constraint_id, 0.0)
        self.sketch.setLabelPosition(constraint_id, 0.0)
        self.expected_constraint_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()
        self.pump_gui_events()

        text_coin = self.find_constraint_probe_viewport_point(
            self.view,
            midpoint,
            self.expected_constraint_name,
            span=32,
            step=2,
        )
        after_info = (
            SketcherGui.getActiveSketchPreselection(text_coin) if text_coin is not None else None
        )
        after_kind = self.classify_preselection(after_info, self.expected_constraint_name)

        detail = (
            f"before_info={before_info}, after_info={after_info}, "
            f"midpoint_coin={midpoint_coin}, text_coin={text_coin}"
        )

        self.assertEqual(before_kind, "edge", detail)
        self.assertIsNotNone(text_coin, detail)
        self.assertEqual(after_kind, "target_constraint", detail)

    def testAngleDatumTextWinsOverHorizontalAxis(self):
        first_line = self.sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(0.0, 0.0, 0.0),
                FreeCAD.Vector(50.0, 50.0 * math.tan(math.radians(30.0)), 0.0),
            ),
            False,
        )
        second_line = self.sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(0.0, 0.0, 0.0),
                FreeCAD.Vector(50.0, -50.0 * math.tan(math.radians(30.0)), 0.0),
            ),
            False,
        )
        self.doc.recompute()
        self.pump_gui_events()

        self.configure_view_state(self.view)

        # The angle bisector is the horizontal axis, so the label text will be centered at
        # x=20, y=0.
        text_center = FreeCAD.Vector(20.0, 0.0, 0.0)
        before_info = SketcherGui.getActiveSketchPreselection(
            tuple(int(value) for value in self.view.getPointOnViewport(text_center))
        )
        before_kind = self.classify_preselection(before_info, "Constraint0")

        constraint_id = self.sketch.addConstraint(
            Sketcher.Constraint(
                "Angle",
                first_line,
                1,
                second_line,
                1,
                math.radians(-60.0),
            )
        )
        self.sketch.setLabelDistance(constraint_id, 10.0)
        self.expected_constraint_name = f"Constraint{constraint_id + 1}"
        self.doc.recompute()
        self.pump_gui_events()

        text_coin = self.find_constraint_probe_viewport_point(
            self.view,
            text_center,
            self.expected_constraint_name,
            span=32,
            step=2,
        )
        info = SketcherGui.getActiveSketchPreselection(text_coin) if text_coin is not None else None
        kind = self.classify_preselection(info, self.expected_constraint_name)

        detail = (
            f"before_info={before_info}, before_kind={before_kind}, "
            f"info={info}, kind={kind}, text_center={text_center}, text_coin={text_coin}"
        )

        self.assertEqual(before_kind, "axis", detail)
        self.assertIsNotNone(text_coin, detail)
        self.assertEqual(kind, "target_constraint", detail)
