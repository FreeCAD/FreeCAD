# SPDX-License-Identifier: LGPL-2.1-or-later
"""Regression coverage for operation sequences and evaluator invariants."""

import importlib
import tempfile
from pathlib import Path
import json
import math
import sys
import unittest
from types import SimpleNamespace
from unittest.mock import patch

import FreeCAD as App
import Part
import Forms
from Forms import topology, brep
from Forms.cage import ControlCage
from Forms.tmesh import HierarchicalTMesh
from Forms.edit_journal import EditJournal
from Forms.preview import mesh_preview


class ReviewTest(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("FormsReviewRegression")
        from Forms.preferences import preferences
        pref = preferences()
        self.previous_preferences = {key: pref.GetBool(key, False)
                                     for key in ("MeshPreview", "GreedySelection")}

    def tearDown(self):
        App.closeDocument(self.doc.Name)
        from Forms.preferences import preferences
        for key, value in self.previous_preferences.items():
            preferences().SetBool(key, value)

    def test_base_rewrites_reject_local_edits_without_mutation(self):
        obj = Forms.create_box(self.doc)
        Forms.subdivide_faces(obj, [0])
        points = list(obj.LocalControlPoints)
        points[0] += App.Vector(0, 0, 7)
        obj.LocalControlPoints = points
        self.doc.recompute()
        before = obj.dumpContent(0)
        edge = next(iter(ControlCage.from_object(obj).edge_counts()))
        for operation, args in (
            (Forms.insert_edge_loop, (edge,)), (Forms.erase_and_fill, ([0],)),
            (Forms.fill_holes, ([edge],)), (Forms.bridge_boundaries, ([edge],)),
        ):
            with self.subTest(operation=operation.__name__):
                with self.assertRaisesRegex(ValueError, "existing local edits"):
                    operation(obj, *args)
                self.assertEqual(before, obj.dumpContent(0))

    def test_retained_control_locations_survive_dissolve_delete_and_restore(self):
        for dissolve in (False, True):
            with self.subTest(dissolve=dissolve):
                obj = Forms.create_box(self.doc)
                _, children = Forms.subdivide_faces(obj, [0])
                self.doc.recompute()
                if dissolve:
                    mesh = HierarchicalTMesh.decode(obj.TMeshData)
                    counts = {}
                    for child in children:
                        for side in mesh.faces[child].sides:
                            for a, b in zip(side, side[1:]):
                                edge = tuple(sorted((a, b)))
                                counts[edge] = counts.get(edge, 0) + 1
                    Forms.dissolve_edges(obj, [e for e, count in counts.items() if count == 2])
                else:
                    Forms.delete_faces(obj, children)
                self.doc.recompute()
                self.assertFalse(obj.Shape.isNull(), obj.ConversionStatus)
                self.assertTrue(obj.Shape.isValid())
                self.assertEqual(bool(obj.Shape.Solids), dissolve)
                mesh = HierarchicalTMesh.decode(obj.TMeshData)
                self.assertTrue(all(mesh.parameter_locations().values()))
                copy = self.doc.copyObject(obj, False)
                copy.touch()
                self.doc.recompute()
                self.assertFalse(copy.Shape.isNull(), copy.ConversionStatus)

    def test_local_budget_rejects_before_uniform_evaluation(self):
        vertices, faces = topology.box_control_cage(20, 20, 20, 2, 2, 2)
        mesh = HierarchicalTMesh.from_quad_cage(vertices, faces)
        for _ in range(5):
            mesh, _ = mesh.subdivide_grid([0], 1, 1)
        with patch.object(brep, "catmull_clark_step_details") as step:
            with self.assertRaisesRegex(ValueError, "sampling limit"):
                brep._tmesh_refinement(vertices, faces, mesh)
            step.assert_not_called()

    def test_oversized_edit_keeps_the_document_unchanged(self):
        obj = Forms.create_box(self.doc)
        before = (obj.CageMode, list(obj.ControlPoints), obj.TMeshData, list(obj.LocalControlPoints))
        with self.assertRaises(ValueError):
            Forms.subdivide_faces(obj, [0], 32, 32)
        self.assertEqual(before, (obj.CageMode, list(obj.ControlPoints), obj.TMeshData, list(obj.LocalControlPoints)))

    def test_pipe_evaluates_new_local_controls(self):
        path = self.doc.addObject("Part::Feature", "Path")
        path.Shape = Part.makePolygon([App.Vector(), App.Vector(0, 0, 30)])
        obj = Forms.create_pipe(self.doc, path)
        original_count = len(obj.Shape.Faces)
        Forms.subdivide_faces(obj, [0])
        self.doc.recompute()
        self.assertEqual(len(obj.Shape.Faces), original_count + 3)
        volume = obj.Shape.Volume
        points = list(obj.LocalControlPoints)
        points[0] += App.Vector(1, 1, 1)
        obj.LocalControlPoints = points
        self.doc.recompute()
        self.assertTrue(obj.Shape.isValid())
        self.assertNotAlmostEqual(obj.Shape.Volume, volume, places=5)

    def test_unsupported_local_crease_is_rejected_without_storing_it(self):
        obj = Forms.create_box(self.doc)
        Forms.subdivide_faces(obj, [0])
        mesh = HierarchicalTMesh.decode(obj.TMeshData)
        seam = next(e for e in mesh.atomic_edges() if min(e) >= len(obj.ControlPoints))
        before = list(obj.EdgeSharpness)
        with self.assertRaisesRegex(ValueError, "not supported"):
            Forms.set_edge_crease(obj, [seam], 10)
        self.assertEqual(obj.EdgeSharpness, before)

    def test_match_uses_form_and_parent_placements(self):
        parent = self.doc.addObject("App::Part", "SupportParent")
        parent.Placement = App.Placement(App.Vector(4, 9, 12), App.Rotation(App.Vector(0, 1, 0), 25))
        support = self.doc.addObject("Part::Feature", "Support")
        support.Shape = Part.makePlane(20, 20)
        parent.addObject(support)
        obj = Forms.create_face(self.doc)
        obj.Placement = App.Placement(App.Vector(0, 0, 50), App.Rotation(App.Vector(0, 0, 1), 30))
        Forms.match_boundary(obj, ControlCage.from_object(obj).boundary_edges,
                             (support, ["Face1"]), "Connected")
        self.doc.recompute()
        world_support = support.Shape.copy()
        world_support.Placement = parent.Placement * support.Placement
        self.assertLess(obj.Shape.distToShape(world_support)[0], 1.0e-6)
        for index in obj.MatchBoundary:
            point = obj.Placement.multVec(obj.ControlPoints[index])
            self.assertLess(Part.Vertex(point).distToShape(world_support)[0], 1.0e-6)

    def test_finite_crease_limit_is_invariant_under_subdivision(self):
        vertices, faces = topology.box_control_cage(20, 20, 20)
        edges = topology.cage_edges(faces)
        for sharpness in (.5, 1., 2.5, 9.5, 10.):
            with self.subTest(sharpness=sharpness):
                corners = [sharpness] + [0.] * (len(vertices) - 1)
                creases = {edges[0]: sharpness, edges[1]: sharpness}
                before = topology.catmull_clark_limit_points(vertices, faces, creases, corners)
                vv, ff, old, _, _, ee, cc = topology.catmull_clark_step_details(
                    vertices, faces, creases, corners)
                after = topology.catmull_clark_limit_points(vv, ff, ee, cc)
                for i, value in enumerate(before):
                    self.assertLess(math.dist(value, after[old[i]]), 1.0e-10)

    def test_explicit_corner_wins_over_two_sharp_edges(self):
        vertices, faces = topology.box_control_cage(20, 20, 20)
        radial = [edge for edge in topology.cage_edges(faces) if 0 in edge][:2]
        result = topology.catmull_clark_step_details(
            vertices, faces, dict.fromkeys(radial, 10), [10.] + [0.] * (len(vertices) - 1))
        self.assertEqual(result[0][result[2][0]], vertices[0])

    def test_blender_crease_conversion_matches_its_evaluator(self):
        with patch.dict(sys.modules, {"bpy": SimpleNamespace()}):
            extract = importlib.import_module("Forms.blender_extract")
            create = importlib.import_module("Forms.blender_create")
        self.assertAlmostEqual(extract._sharpness(.5), 2.5)
        self.assertAlmostEqual(create._crease(2.5), .5)
        for value in (0, .1, .5, .9, 1):
            self.assertAlmostEqual(create._crease(extract._sharpness(value)), value)

    def test_blender_subdivision_conformance(self):
        from Forms.blender_bridge import find_blender_executable, run_blender_script
        executable = find_blender_executable()
        if not executable:
            self.skipTest("Blender integration requires an installed Blender")
        vertices, faces = topology.box_control_cage(2, 2, 2)
        radial = [edge for edge in topology.cage_edges(faces) if 0 in edge]
        cases = []
        for weights, corner in [([], 0), ([.5, .5], 0), ([1, 1], 0),
                                ([2.5, 2.5], 0), ([10, 10], 0),
                                ([.2, .7, 1.2], .3), ([10, 10], 10), ([], .5)]:
            cases.append(dict(vertices=vertices, faces=faces,
                              edges=[[*edge, value] for edge, value in zip(radial, weights)],
                              corners=[corner] + [0] * (len(vertices) - 1)))
        with tempfile.TemporaryDirectory(prefix="forms-conformance-") as directory:
            path = Path(directory) / "cases.json"
            path.write_text(json.dumps(cases), encoding="utf-8")
            run_blender_script(Path(__file__).with_name("BlenderConformance.py"), [path],
                               executable=executable, timeout=60)
            outputs = json.loads(path.with_suffix(".out.json").read_text(encoding="utf-8"))
        for case, output in zip(cases, outputs):
            with self.subTest(edges=case["edges"], corner=case["corners"][0]):
                vv, ff = case["vertices"], case["faces"]
                ee = {tuple(edge[:2]): edge[2] for edge in case["edges"]}
                cc = case["corners"]
                for _ in range(2):
                    vv, ff, _, _, _, ee, cc = topology.catmull_clark_step_details(vv, ff, ee, cc)
                self.assertEqual(len(vv), len(output))
                self.assertLess(max(min(math.dist(point, target) for target in output)
                                    for point in vv), 2.0e-6)

    @unittest.skipUnless(App.GuiUp, "GUI lifecycle requires FreeCADGui")
    def test_gui_drag_preview_and_cancel_release_resources(self):
        import FreeCADGui as Gui
        import gc
        import weakref
        from PySide import QtCore, QtWidgets
        from Forms.edit import active_form_session
        from Forms.preferences import preferences
        preferences().SetBool("MeshPreview", True)
        obj = Forms.create_box(self.doc)
        self.doc.UndoMode = 1
        original = list(obj.ControlPoints)
        gui = Gui.getDocument(self.doc.Name)
        for _ in range(3):
            gui.setEdit(obj, 0)
            session = active_form_session(obj)
            self.assertIsNotNone(session)
            session._restore_control_selection({0}, set())
            session.dragger_started(session.dragger)
            session.syncing = True
            session.dragger.translation.setValue(tuple(session.base_center + App.Vector(1, 0, 0)))
            session.syncing = False
            with patch.object(brep, "_make_surface", side_effect=AssertionError("BRep during motion")):
                session.dragger_moved(session.dragger)
            preview = weakref.ref(session.motion_preview)
            self.assertIsNotNone(preview())
            session.dragger_finished(session.dragger)
            self.assertIsNone(session.motion_preview)
            session.reject()
            self.assertTrue(session.cleaned)
            self.assertIsNone(session.edit_backup)
            self.assertEqual(original, obj.ControlPoints)
            ref = weakref.ref(session)
            del session
            QtWidgets.QApplication.sendPostedEvents(None, QtCore.QEvent.DeferredDelete)
            QtWidgets.QApplication.processEvents()
            gc.collect()
            self.assertIsNone(preview())
            self.assertIsNone(ref(), "Closed edit session retained by a GUI callback")

    @unittest.skipUnless(App.GuiUp, "Preferences and selection require FreeCADGui")
    def test_gui_preference_defaults_and_background_clear(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets
        from pivy import coin
        from Forms.edit import active_form_session
        from Forms.preferences import preferences, PreferencesPage
        pref = preferences()
        pref.RemBool("MeshPreview")
        pref.RemBool("GreedySelection")
        page = PreferencesPage()
        page.loadSettings()
        self.assertFalse(page.form.meshPreview.isChecked())
        self.assertFalse(page.form.greedySelection.isChecked())
        page.form.meshPreview.setChecked(True)
        page.form.greedySelection.setChecked(True)
        page.saveSettings()
        self.assertTrue(pref.GetBool("MeshPreview"))
        self.assertTrue(pref.GetBool("GreedySelection"))
        obj = Forms.create_box(self.doc)
        gui = Gui.getDocument(self.doc.Name)
        for greedy in (False, True):
            pref.SetBool("GreedySelection", greedy)
            with patch.object(Gui.Selection, "setSelectionStyle", wraps=Gui.Selection.setSelectionStyle) as style:
                gui.setEdit(obj, 0)
                session = active_form_session(obj)
                style.assert_called_with(int(greedy))
                Gui.Selection.addSelection(obj, "Edge1")
                self.assertTrue(Gui.Selection.getSelectionEx())
                event = coin.SoMouseButtonEvent()
                event.setButton(coin.SoMouseButtonEvent.BUTTON1)
                event.setPosition(coin.SbVec2s(10, 10))
                callback = SimpleNamespace(getEvent=lambda: event, getPickedPoint=lambda: None)
                event.setState(coin.SoButtonEvent.DOWN)
                session._selection_mouse_event(callback)
                event.setState(coin.SoButtonEvent.UP)
                session._selection_mouse_event(callback)
                QtWidgets.QApplication.processEvents()
                self.assertFalse(Gui.Selection.getSelectionEx())
                self.assertFalse(session.selected)
                session.reject()
                style.assert_called_with(0)
        page.form.deleteLater()

    @unittest.skipUnless(App.GuiUp, "Edge clicking requires FreeCADGui")
    def test_gui_second_edge_click_preempts_whole_object_selection(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets
        from pivy import coin
        from Forms.edit import active_form_session
        from Forms.preferences import preferences
        obj = Forms.create_box(self.doc)
        for greedy in (False, True):
            preferences().SetBool("GreedySelection", greedy)
            Gui.getDocument(self.doc.Name).setEdit(obj, 0)
            session = active_form_session(obj)
            for second_edge in ("Edge2", "Edge1"):
                Gui.Selection.clearSelection()
                event = coin.SoMouseButtonEvent()
                event.setButton(coin.SoMouseButtonEvent.BUTTON1)
                event.setPosition(coin.SbVec2s(10, 10))
                event.setState(coin.SoButtonEvent.UP)
                handled = []
                callback = SimpleNamespace(getEvent=lambda: event,
                    getPickedPoint=lambda: object(), setHandled=lambda: handled.append(True))
                event.setState(coin.SoButtonEvent.DOWN)
                session._selection_mouse_event(callback)
                Gui.Selection.addSelection(obj, "Edge1")
                if second_edge == "Edge2":
                    event.setPosition(coin.SbVec2s(100, 100))
                pick_view = SimpleNamespace(getObjectInfo=lambda position: {
                    "Document": self.doc.Name, "Object": obj.Name, "Component": "Face1"})
                with patch.object(session, "view", pick_view):
                    event.setState(coin.SoButtonEvent.DOWN)
                    session._selection_mouse_event(callback)
                    self.assertEqual(session.selection_enabled_field.getValue(), second_edge != "Edge1")
                    event.setState(coin.SoButtonEvent.UP)
                    session._selection_mouse_event(callback)
                self.assertTrue(session.selection_enabled_field.getValue())
                self.assertEqual(bool(handled), second_edge == "Edge1")
                QtWidgets.QApplication.processEvents()
                selections = Gui.Selection.getSelectionEx()
                self.assertEqual(len(selections), 1)
                names = selections[0].SubElementNames
                self.assertTrue(names, "Repeated edge click selected the whole object")
                if second_edge == "Edge1":
                    self.assertGreater(len(names), 1)
                else:
                    self.assertEqual(len(names), 1)
            session.reject()

    @unittest.skipUnless(App.GuiUp, "Native picking requires FreeCADGui")
    def test_gui_double_click_after_topology_edits(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets
        from pivy import coin
        from Forms.edit import active_form_session
        from Forms.preferences import preferences
        for operation in ("delete", "insert"):
            for greedy in (False, True):
                with self.subTest(operation=operation, greedy=greedy):
                    preferences().SetBool("GreedySelection", greedy)
                    obj = Forms.create_box(self.doc)
                    Gui.getDocument(self.doc.Name).setEdit(obj, 0)
                    session = active_form_session(obj)
                    if operation == "delete":
                        Forms.delete_faces(obj, [0])
                    else:
                        from Forms.operations import insert_edge_on_face
                        insert_edge_on_face(obj, 0)
                    self.doc.recompute()
                    session.topology_changed()
                    session.view.viewAxonometric()
                    session.view.fitAll()
                    QtWidgets.QApplication.processEvents()
                    manager = session.viewer.getSoRenderManager()

                    def click(position):
                        for state in (coin.SoButtonEvent.DOWN, coin.SoButtonEvent.UP):
                            event = coin.SoMouseButtonEvent()
                            event.setButton(coin.SoMouseButtonEvent.BUTTON1)
                            event.setPosition(coin.SbVec2s(*position))
                            event.setState(state)
                            action = coin.SoHandleEventAction(manager.getViewportRegion())
                            action.setPickRadius(session.viewer.getPickRadius())
                            action.setEvent(event)
                            action.apply(manager.getSceneGraph())

                    checked = 0
                    selected_loop = False
                    for edge in obj.Shape.Edges:
                        middle = edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2)
                        position = session.view.getPointOnScreen(middle)
                        Gui.Selection.clearSelection()
                        click(position)
                        previous = session.last_added_edge
                        if previous is None:
                            continue
                        control_edge = session._control_edge_for_subelement(previous[2])
                        if control_edge is None:
                            continue
                        click(position)
                        QtWidgets.QApplication.processEvents()
                        selected = session._selected_control_edges()
                        self.assertIn(control_edge, selected)
                        selected_loop = selected_loop or len(selected) > 1
                        self.assertTrue(all(s.SubElementNames for s in Gui.Selection.getSelectionEx()))
                        checked += 1
                        if checked >= 3:
                            break
                    self.assertGreater(checked, 0, "No visible editable edge was exercised")
                    self.assertTrue(selected_loop, "Double clicks never expanded an edge chain")
                    session.reject()
                    self.doc.removeObject(obj.Name)

    @unittest.skipUnless(App.GuiUp, "Dragging requires FreeCADGui")
    def test_gui_default_drag_keeps_the_smooth_brep(self):
        import FreeCADGui as Gui
        from Forms.edit import active_form_session
        from Forms.preferences import preferences
        preferences().RemBool("MeshPreview")
        obj = Forms.create_box(self.doc)
        Gui.getDocument(self.doc.Name).setEdit(obj, 0)
        session = active_form_session(obj)
        session._restore_control_selection({0}, set())
        session.dragger_started(session.dragger)
        session.syncing = True
        session.dragger.translation.setValue(tuple(session.base_center + App.Vector(1, 0, 0)))
        session.syncing = False
        with patch.object(brep, "_make_surface", wraps=brep._make_surface) as fitting:
            session.dragger_moved(session.dragger)
            self.assertGreater(fitting.call_count, 0)
        self.assertIsNone(session.motion_preview)
        session.dragger_finished(session.dragger)
        session.reject()

    def test_straighten_finds_the_dominant_eigenvector(self):
        points = [(math.sqrt(1.5), 0, 0), (-math.sqrt(1.5), 0, 0), (0, 1, 1), (0, -1, -1)]
        center, axis = topology._largest_covariance_axis(points)
        self.assertAlmostEqual(abs(axis[1] + axis[2]), math.sqrt(2))
        self.assertAlmostEqual(axis[0], 0)

    def _split_form(self):
        obj = Forms.create_cylinder(self.doc)
        obj.HeightSegments = 3
        self.doc.recompute()
        cage = ControlCage.from_object(obj)
        for edge in topology.cage_edges(cage.faces):
            seam = topology.cage_edge_loop(cage.faces, edge)
            try:
                cage.split_along_edges(seam)
            except ValueError:
                continue
            return obj, seam
        self.fail("No separating edge loop")

    def test_cancel_unweld_removes_created_object_and_is_undoable(self):
        obj, seam = self._split_form()
        self.doc.UndoMode = 1
        journal = EditJournal(obj)
        self.doc.openTransaction("Unweld")
        objects = Forms.unweld_segment(obj, seam)
        second_name = objects[1].Name
        journal.record_created(objects)
        self.doc.recompute()
        self.doc.commitTransaction()
        journal.restore()
        self.assertIsNone(self.doc.getObject(second_name))
        self.assertTrue(ControlCage.from_object(obj).is_closed)
        self.doc.undo()
        self.doc.recompute()
        self.assertIsNotNone(self.doc.getObject(second_name))
        self.doc.redo()
        self.assertIsNone(self.doc.getObject(second_name))

    def test_cancel_weld_recreates_other_form_and_reconnects_links(self):
        obj, seam = self._split_form()
        first, second = Forms.unweld_segment(obj, seam)
        self.doc.recompute()
        dependent = self.doc.addObject("App::FeaturePython", "Dependent")
        dependent.addProperty("App::PropertyLink", "Source")
        dependent.Source = second
        second_name = second.Name
        journal = EditJournal(first)
        journal.capture_removal(second)
        Forms.weld_boundaries(first, ControlCage.from_object(first).boundary_edges[0],
                              second, ControlCage.from_object(second).boundary_edges[0])
        self.doc.recompute()
        journal.restore()
        restored = self.doc.getObject(second_name)
        self.assertIsNotNone(restored)
        self.assertIs(dependent.Source, restored)
        self.assertFalse(ControlCage.from_object(first).is_closed)
        restored.touch()
        self.doc.recompute()
        self.assertTrue(restored.Shape.isValid())
        self.assertIsNotNone(restored.Proxy)

    def test_local_refinement_preserves_disconnected_components(self):
        first = Forms.create_box(self.doc)
        second = Forms.create_box(self.doc)
        second.Placement.Base = App.Vector(40, 0, 0)
        cage = ControlCage.from_object(first).disjoint_union(ControlCage.from_object(second))
        # Separate cages geometrically before joining their topology lists.
        count = len(first.ControlPoints)
        cage.vertices[count:] = [(x + 40, y, z) for x, y, z in cage.vertices[count:]]
        cage.write(first)
        first.CageMode = "Editable"
        Forms.subdivide_faces(first, [0])
        self.doc.recompute()
        self.assertFalse(first.Shape.isNull(), first.ConversionStatus)
        self.assertTrue(first.Shape.isValid())
        self.assertEqual(len(first.Shape.Solids), 2)

    @unittest.skipUnless(App.GuiUp, "GUI cancellation requires FreeCADGui")
    def test_gui_cancel_weld_and_unweld(self):
        import FreeCADGui as Gui
        from Forms.edit import active_form_session
        obj, seam = self._split_form()
        gui = Gui.getDocument(self.doc.Name)
        gui.setEdit(obj, 0)
        session = active_form_session(obj)
        session.start_unweld_tool()
        session.unweld_segment_edges = seam
        session.unweld_separate_forms.setChecked(True)
        self.assertTrue(session._commit_unweld_preview())
        self.assertEqual(len(self.doc.Objects), 2)
        session.stop_surface_tool()
        session.reject()
        self.assertEqual(len(self.doc.Objects), 1)
        self.assertTrue(ControlCage.from_object(obj).is_closed)
        first, second = Forms.unweld_segment(obj, seam)
        self.doc.recompute()
        other_name = second.Name
        gui.setEdit(first, 0)
        session = active_form_session(first)
        session.start_weld_tool()
        session.weld_other = second
        session.weld_first_edge = ControlCage.from_object(first).boundary_edges[0]
        session.weld_second_edge = ControlCage.from_object(second).boundary_edges[0]
        self.assertTrue(session.apply_weld_tool())
        self.assertIsNone(self.doc.getObject(other_name))
        session.reject()
        restored = self.doc.getObject(other_name)
        self.assertIsNotNone(restored)
        self.assertIsNotNone(restored.ViewObject.Proxy._cage_coordinates)
        self.assertTrue(restored.Shape.isValid())

    def test_motion_preview_does_not_fit_or_modify_the_brep(self):
        obj = Forms.create_box(self.doc)
        Forms.subdivide_faces(obj, [0])
        self.doc.recompute()
        before = obj.dumpContent(0)
        with patch.object(brep, "_make_surface", side_effect=AssertionError("BRep fitting in preview")):
            vertices, faces = mesh_preview(obj)
        self.assertTrue(vertices and faces)
        self.assertEqual(before, obj.dumpContent(0))


def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(ReviewTest)
