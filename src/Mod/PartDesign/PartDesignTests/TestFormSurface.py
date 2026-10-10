# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import tempfile
import unittest

import FreeCAD as App
import Part

import Forms


class _RecordingToolHandlerView:
    """Delegate to a 3D view while recording custom cursor activations."""

    def __init__(self, view):
        self._view = view
        self.activated_cursors = []

    def activateToolHandler(self, cursor_icon):
        self.activated_cursors.append(cursor_icon)
        return self._view.activateToolHandler(cursor_icon)

    def __getattr__(self, name):
        return getattr(self._view, name)


class TestFormSurface(unittest.TestCase):
    def setUp(self):
        self.document = App.newDocument("PartDesignTestFormSurface")

    def tearDown(self):
        App.closeDocument(self.document.Name)

    def _make_surface(self):
        body = self.document.addObject("PartDesign::Body", "Body")
        source = body.newObject("PartDesign::Feature", "Source")
        source.Shape = Part.makeBox(10, 12, 14)
        surface = Forms.create_surface(body, source, "Face6")
        self.document.recompute()
        return body, source, surface

    def testCreatesBodyFeatureFromOneFace(self):
        body, source, surface = self._make_surface()

        self.assertEqual(surface.TypeId, "PartDesign::FeaturePython")
        self.assertEqual(surface.FormType, "Forms::Surface")
        self.assertEqual(surface.Continuity, "Connected")
        self.assertIs(surface.getParentGeoFeatureGroup(), body)
        self.assertIs(surface.SourceFace[0], source)
        self.assertEqual(tuple(surface.SourceFace[1]), ("Face6",))
        self.assertEqual(len(surface.ControlPoints), 9)
        self.assertEqual(len(surface.ControlFaces), 4)
        self.assertFalse(surface.Shape.isNull())
        self.assertEqual(len(surface.Shape.Solids), 1)
        self.assertEqual(
            len(surface.Shape.Faces),
            len(source.Shape.Faces) - 1 + len(surface.ControlFaces),
        )
        self.assertEqual(
            len(surface.FormSurfaceFaces),
            len(surface.ControlFaces),
            surface.ConversionStatus,
        )
        self.assertEqual(len(surface.FormSurfaceFaceMap), len(surface.ControlFaces))
        self.assertAlmostEqual(surface.Shape.Volume, source.Shape.Volume, places=5)

    def testParametricSegmentsAddInteriorControls(self):
        _body, _source, surface = self._make_surface()

        surface.USegments = 4
        surface.VSegments = 3
        self.document.recompute()

        self.assertEqual(len(surface.ControlPoints), 20)
        self.assertEqual(len(surface.ControlFaces), 12)
        self.assertEqual(len(surface.Shape.Solids), 1)
        self.assertEqual(len(surface.FormSurfaceFaces), 12)
        self.assertTrue(surface.Shape.isValid())

    def testInteriorPullPreservesAValidSolid(self):
        _body, source, surface = self._make_surface()
        source_volume = source.Shape.Volume

        Forms.make_editable(surface)
        points = list(surface.ControlPoints)
        points[4] = points[4] + App.Vector(0, 0, 3)
        surface.ControlPoints = points
        self.document.recompute()

        self.assertEqual(len(surface.Shape.Solids), 1)
        self.assertTrue(surface.Shape.isValid())
        self.assertEqual(
            len(surface.FormSurfaceFaces),
            len(surface.ControlFaces),
            surface.ConversionStatus,
        )
        self.assertEqual(len(surface.FormSurfaceFaceMap), len(surface.ControlFaces))
        self.assertEqual(
            len(surface.Shape.Faces),
            len(source.Shape.Faces) - 1 + len(surface.ControlFaces),
        )
        self.assertEqual(
            surface.ConversionStatus,
            "Valid solid with subdivided form surface",
        )
        self.assertNotAlmostEqual(surface.Shape.Volume, source_volume, places=5)
        self.assertGreater(surface.Shape.BoundBox.ZMax, source.Shape.BoundBox.ZMax)
        self.assertTrue(surface.FormSurfaceFaces)

    def testDetailedInteriorPullKeepsEveryPatch(self):
        _body, _source, surface = self._make_surface()
        surface.USegments = 4
        surface.VSegments = 3
        self.document.recompute()

        Forms.make_editable(surface)
        points = list(surface.ControlPoints)
        points[6] = points[6] + App.Vector(0, 0, 2)
        surface.ControlPoints = points
        self.document.recompute()

        self.assertEqual(
            surface.ConversionStatus,
            "Valid solid with subdivided form surface",
        )
        self.assertEqual(len(surface.FormSurfaceFaces), 12)
        self.assertEqual(len(surface.FormSurfaceFaceMap), 12)
        self.assertEqual(len(surface.Shape.Solids), 1)
        self.assertTrue(surface.Shape.isValid())

    def testSourceUpdatesUntilSurfaceIsEdited(self):
        _body, source, surface = self._make_surface()
        initial_points = list(surface.ControlPoints)

        source.Shape = Part.makeBox(20, 12, 14)
        self.document.recompute()
        linked_points = list(surface.ControlPoints)
        self.assertNotEqual(initial_points, linked_points)

        Forms.make_editable(surface)
        source.Shape = Part.makeBox(30, 12, 14)
        self.document.recompute()
        self.assertEqual(list(surface.ControlPoints), linked_points)

    def testBrokenSourceLinkClearsStaleSurface(self):
        _body, _source, surface = self._make_surface()
        self.assertFalse(surface.Shape.isNull())

        surface.SourceFace = None
        self.document.recompute()
        self.assertTrue(surface.Shape.isNull())
        self.assertIn("source face is required", surface.ConversionStatus.lower())


class TestAdditiveForm(unittest.TestCase):
    def setUp(self):
        self.document = App.newDocument("PartDesignTestAdditiveForm")
        self.body = self.document.addObject("PartDesign::Body", "Body")
        self.source = self.body.newObject("PartDesign::Feature", "Source")
        self.source.Shape = Part.makeBox(20, 20, 10)

    def tearDown(self):
        App.closeDocument(self.document.Name)

    @unittest.skipUnless(App.GuiUp, "GUI command registration requires FreeCADGui")
    def testPartDesignPrimitiveGroupAndMatchCommandsRegister(self):
        import FreeCADGui as Gui
        from PySide import QtCore

        import FormSurfaceFeature
        import CommandTopology  # noqa: F401

        commands = Gui.listCommands()
        self.assertIn("PartDesign_FormSurface", commands)
        self.assertIn("PartDesign_AdditiveForm", commands)
        self.assertIn("PartDesign_SubtractiveForm", commands)
        for primitive in FormSurfaceFeature.PRIMITIVES:
            for operation in ("Additive", "Subtractive"):
                self.assertIn(f"PartDesign_{operation}Form{primitive}", commands)
                icon = FormSurfaceFeature._primitive_icon(primitive, operation)
                self.assertTrue(QtCore.QFile.exists(icon), icon)
        self.assertIn("Forms_Match", commands)

    @unittest.skipUnless(App.GuiUp, "active Body lookup requires FreeCADGui")
    def testPartDesignPrimitiveCommandsDoNotRequireAnActiveBody(self):
        import FreeCADGui as Gui
        import PartDesignGui
        import FormSurfaceFeature

        empty_body = self.document.addObject("PartDesign::Body", "EmptyBody")
        Gui.Selection.clearSelection()
        Gui.activeView().setActiveObject("pdbody", None)
        try:
            for primitive in FormSurfaceFeature.PRIMITIVES:
                additive = FormSurfaceFeature.CommandAdditiveFormPrimitive(primitive)
                subtractive = FormSurfaceFeature.CommandSubtractiveFormPrimitive(primitive)
                self.assertTrue(additive.IsActive(), primitive)
                self.assertTrue(subtractive.IsActive(), primitive)
            self.assertTrue(FormSurfaceFeature.CommandAdditiveFormGroup().IsActive())
            self.assertTrue(FormSurfaceFeature.CommandSubtractiveFormGroup().IsActive())

            # The same native helper used by the commands automatically activates
            # the sole Body without presenting a dialog.
            self.document.removeObject(empty_body.Name)
            self.assertIs(PartDesignGui.getBody(False), self.body)
            self.assertIs(Gui.activeView().getActiveObject("pdbody"), self.body)
        finally:
            Gui.activeView().setActiveObject("pdbody", self.body)

    def testAdditiveFormCanBeFirstFeatureOfBody(self):
        empty_body = self.document.addObject("PartDesign::Body", "EmptyBody")
        form = Forms.create_additive_form(empty_body, None, "Box")
        self.document.recompute()

        self.assertIsNone(form.BaseFeature)
        self.assertEqual(form.TypeId, "PartDesign::FeatureAdditivePython")
        self.assertEqual(form.Operation, "Additive")
        self.assertIs(empty_body.Tip, form)
        self.assertFalse(form.Shape.isNull())
        self.assertEqual(form.Shape.ShapeType, "Solid")
        self.assertIn("no preceding feature", form.CombinationStatus)

    def testEveryPrimitiveCreatesSeparateEditableGeometry(self):
        for primitive in ("Box", "Cylinder", "Quadball", "Sphere", "Face", "Torus", "Tube"):
            with self.subTest(primitive=primitive):
                form = Forms.create_additive_form(
                    self.body,
                    self.source,
                    primitive,
                    name=f"Additive{primitive}",
                    placement=App.Placement(App.Vector(40, 0, 0), App.Rotation()),
                )
                form.EditingForm = True
                self.document.recompute()
                self.assertFalse(form.FormShape.isNull())
                self.assertFalse(form.Shape.isNull())
                self.assertIn("Editing form preview", form.CombinationStatus)
                self.assertEqual(len(form.Shape.Faces), len(form.FormShape.Faces))
                self.assertEqual(len(form.Shape.Edges), len(form.FormShape.Edges))
                self.body.removeObject(form)
                self.document.removeObject(form.Name)
                self.body.Tip = self.source

    def testPartDesignPipeLinksAndClaimsItsSketchPath(self):
        import Sketcher

        sketch = self.body.newObject("Sketcher::SketchObject", "PipePath")
        center = App.Vector(20, 10, 10)
        sketch.addGeometry(Part.LineSegment(center, App.Vector(30, 10, 10)), False)
        sketch.addGeometry(Part.LineSegment(center, App.Vector(20, 30, 10)), False)
        sketch.addGeometry(Part.LineSegment(center, App.Vector(10, 10, 10)), False)
        self.document.recompute()
        form = Forms.create_additive_form(self.body, self.source, "Pipe", path_object=sketch)
        form.Diameter = 4
        self.document.recompute()
        self.assertIs(form.PathObject, sketch)
        self.assertIs(form.BaseFeature, self.source)
        self.assertFalse(form.FormShape.isNull())
        from Forms.pipe import fused_pipe_shape

        fused = fused_pipe_shape(form.FormShape)
        self.assertIn(
            "Valid fused",
            form.CombinationStatus,
            (
                form.FormShape.ShapeType,
                len(form.FormShape.Solids),
                fused.ShapeType,
                len(fused.Solids),
                form.CombinationStatus,
            ),
        )
        if App.GuiUp:
            self.assertIn(sketch, form.ViewObject.Proxy.claimChildren())

    @unittest.skipUnless(App.GuiUp, "Body tree drop requires FreeCADGui")
    def testDroppingStandaloneFormCreatesAdditiveTipInsteadOfBaseFeature(self):
        from PySide import QtCore

        standalone = Forms.create_box(self.document, "StandaloneForm")
        standalone.Label = "Organic attachment"
        standalone.Length = 8
        standalone.Width = 6
        standalone.Height = 4
        standalone.Placement = App.Placement(
            App.Vector(32, 7, 5), App.Rotation(App.Vector(0, 0, 1), 20)
        )
        self.body.Placement = App.Placement(App.Vector(10, 0, 0), App.Rotation())
        self.document.recompute()
        expected_placement = (
            self.body.getGlobalPlacement().inverse() * standalone.getGlobalPlacement()
        )

        self.assertTrue(self.body.ViewObject.canDropObject(standalone))
        self.body.ViewObject.dropObject(standalone)
        # The dragged object must remain alive until the C++ tree drop callback
        # returns; conversion runs on the following event-loop turn.
        self.assertIs(self.document.getObject("StandaloneForm"), standalone)
        wait_loop = QtCore.QEventLoop()
        QtCore.QTimer.singleShot(20, wait_loop.quit)
        wait_loop.exec()
        self.document.recompute()

        result = self.body.Tip
        self.assertIsNone(self.document.getObject("StandaloneForm"))
        self.assertEqual(result.TypeId, "PartDesign::FeatureAdditivePython")
        self.assertEqual(result.FormType, "Forms::Box")
        self.assertEqual(result.PrimitiveKind, "Box")
        self.assertEqual(result.Label, "Organic attachment")
        self.assertIs(result.BaseFeature, self.source)
        self.assertIs(result.getParentGeoFeatureGroup(), self.body)
        self.assertEqual(result.FormPlacement, expected_placement)
        self.assertEqual(result.Length.Value, 8)
        self.assertEqual(result.Width.Value, 6)
        self.assertEqual(result.Height.Value, 4)
        self.assertFalse(
            any(child.TypeId == "PartDesign::FeatureBase" for child in self.body.Group)
        )

        self.document.undo()
        self.document.recompute()
        self.assertIsNotNone(self.document.getObject("StandaloneForm"))
        self.assertIs(self.body.Tip, self.source)
        self.document.redo()
        self.document.recompute()
        self.assertIsNone(self.document.getObject("StandaloneForm"))
        self.assertEqual(self.body.Tip.FormType, "Forms::Box")

    def testMovingEditableStandaloneFormPreservesBodyLocalCage(self):
        standalone = Forms.create_box(self.document, "EditableStandaloneForm")
        standalone.Placement = App.Placement(
            App.Vector(30, 4, 2), App.Rotation(App.Vector(0, 1, 0), 15)
        )
        self.body.Placement = App.Placement(App.Vector(10, 0, 0), App.Rotation())
        self.document.recompute()
        Forms.make_editable(standalone)
        points = list(standalone.ControlPoints)
        points[0] = points[0] + App.Vector(-3, 1, 2)
        standalone.ControlPoints = points
        self.document.recompute()
        relative = self.body.getGlobalPlacement().inverse() * standalone.getGlobalPlacement()
        expected = [relative.multVec(point) for point in standalone.ControlPoints]

        result = Forms.move_form_to_body(standalone, self.body)

        self.assertIs(result, self.body.Tip)
        self.assertIs(result.BaseFeature, self.source)
        self.assertEqual(result.CageMode, "Editable")
        self.assertEqual(result.FormPlacement, App.Placement())
        self.assertEqual(len(result.ControlPoints), len(expected))
        for actual, target in zip(result.ControlPoints, expected):
            self.assertLess(actual.sub(target).Length, 1.0e-9)
        self.assertIsNone(self.document.getObject("EditableStandaloneForm"))

    def testEditorMapsOnlyTheFormToolShape(self):
        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(40, 0, 0), App.Rotation()),
        )
        form.EditingForm = True
        self.document.recompute()

        from Forms.cage import ControlElementMapper
        from Forms.edit import FormEditSession

        mapper = ControlElementMapper(form)
        self.assertTrue(mapper.shape.isSame(form.FormShape))
        self.assertTrue(mapper.indices(form.FormShape.Faces[0]))
        self.assertFalse(mapper.indices(self.source.Shape.Faces[0]))
        session = FormEditSession.__new__(FormEditSession)
        session.obj = form
        selected_faces = session._form_face_subelements()
        self.assertEqual(len(selected_faces), len(form.FormShape.Faces))
        for face_name in selected_faces:
            face = form.Shape.getElement(face_name)
            self.assertTrue(any(face.isPartner(candidate) for candidate in form.FormShape.Faces))
            self.assertFalse(
                any(face.isPartner(candidate) for candidate in self.source.Shape.Faces)
            )

    @unittest.skipUnless(App.GuiUp, "interactive selection requires FreeCADGui")
    def testWholeFormEditSelectsOnlyFormFaces(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets

        from Forms.cage import canonical_subelement_name
        from Forms.edit import active_form_session

        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(40, 0, 0), App.Rotation()),
        )
        self.document.recompute()

        gui_document = Gui.getDocument(self.document.Name)
        try:
            Gui.Selection.clearSelection()
            Gui.Selection.addSelection(form)
            gui_document.setEdit(form, 0)
            QtWidgets.QApplication.processEvents()

            session = active_form_session(form)
            self.assertIsNotNone(session)
            self.assertTrue(session.whole_form_selected)
            self.assertTrue(form.ViewObject.Visibility)
            self.assertTrue(self.source.ViewObject.Visibility)
            self.assertEqual(form.ViewObject.Transparency, 0)
            self.assertEqual(
                tuple(form.ViewObject.ShapeColor)[:3], tuple(form.ViewObject.PreviewColor)[:3]
            )
            self.assertGreaterEqual(form.ViewObject.PointSize, 8.0)
            self.assertEqual(len(form.Shape.Faces), len(form.FormShape.Faces))
            self.assertEqual(len(form.Shape.Edges), len(form.FormShape.Edges))
            selected_faces = set(session._form_face_subelements())
            self.assertEqual(len(selected_faces), len(form.FormShape.Faces))
            self.assertTrue(selected_faces)

            selection = Gui.Selection.getSelectionEx("", Gui.Selection.ResolveMode.NoResolve)
            self.assertTrue(selection)
            # Part Design normally routes feature subelement selection through
            # the Body. The checks below resolve every routed name back to the
            # edited Form and verify that no base-feature face slipped in.
            self.assertTrue(
                all(item.Object == form or item.Object == self.body for item in selection),
                [(item.Object.Name, tuple(item.SubElementNames)) for item in selection],
            )
            self.assertTrue(all(item.SubElementNames for item in selection))
            actual_faces = set()
            for item in selection:
                for raw_name in item.SubElementNames:
                    form_name = session._form_selection_subelement(
                        self.document.Name,
                        item.Object.Name,
                        raw_name,
                    )
                    actual_faces.add(canonical_subelement_name(form_name))
            self.assertEqual(actual_faces, selected_faces)
            for face_name in selected_faces:
                face = form.Shape.getElement(face_name)
                self.assertTrue(
                    any(face.isPartner(form_face) for form_face in form.FormShape.Faces)
                )
        finally:
            Gui.Selection.clearSelection()
            gui_document.resetEdit()

    @unittest.skipUnless(App.GuiUp, "interactive selection requires FreeCADGui")
    def testCompoundWireSelectionEnablesMatch(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets

        import CommandTopology
        from Forms.cage import ControlCage
        from Forms.edit import active_form_session

        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(10, 10, 20), App.Rotation()),
        )
        form.Length = 8
        form.Width = 8
        form.Height = 20
        self.document.recompute()
        Forms.make_editable(form)
        points = list(form.ControlPoints)
        bottom = min(point.z for point in points)
        opening = next(
            index
            for index, encoded in enumerate(form.ControlFaces)
            if all(abs(points[int(vertex)].z - bottom) < 1.0e-7 for vertex in encoded.split())
        )
        Forms.delete_faces(form, [opening])
        self.document.recompute()

        gui_document = Gui.getDocument(self.document.Name)
        try:
            Gui.activateWorkbench("FormsWorkbench")
            gui_document.setEdit(form, 0)
            QtWidgets.QApplication.processEvents()
            session = active_form_session(form)
            self.assertIsNotNone(session)
            boundary_edge = ControlCage.from_object(form).boundary_edges[0]
            form_edge_names = [
                f"Edge{index}"
                for index, _edge in enumerate(form.Shape.Edges, 1)
                if session._control_edge_for_subelement(f"Edge{index}") == boundary_edge
            ]
            self.assertTrue(form_edge_names)
            session._select_edge_loop(form_edge_names[0])

            Gui.Selection.addSelection(self.source, "Face6")
            QtWidgets.QApplication.processEvents()

            self.assertTrue(CommandTopology.CommandMatch().IsActive())
            registered_match = Gui.Command.get("Forms_Match")
            self.assertTrue(registered_match.isActive())
            Gui.Command.update()
            QtWidgets.QApplication.processEvents()
            from Forms.toolbar import MODIFY_TOOLBAR, _find_toolbar

            modify_toolbar = _find_toolbar(MODIFY_TOOLBAR)
            self.assertIsNotNone(modify_toolbar)
            self.assertTrue(modify_toolbar.isEnabled())
            match_actions = registered_match.getAction()
            self.assertTrue(match_actions)
            self.assertTrue(any(action.isEnabled() for action in match_actions))
            inputs = CommandTopology._selected_match_inputs()
            self.assertIsNotNone(inputs)
            self.assertEqual(inputs[2][0], self.source)
            self.assertEqual(tuple(inputs[2][1]), ("Face6",))
            CommandTopology.CommandMatch().Activated()
            QtWidgets.QApplication.processEvents()
            self.assertTrue(session.match_tool_active)
            self.assertFalse(
                session.match_preview_shape.isNull(),
                session.match_preview_status.text(),
            )
            self.assertFalse(form.ViewObject.Visibility)
            self.assertTrue(self.source.ViewObject.Visibility)
            self.assertIsNotNone(session.match_preview_root)
            session.match_mode.setCurrentIndex(session.match_mode.findData("Connected"))
            QtWidgets.QApplication.processEvents()
            self.assertFalse(
                session.match_preview_shape.isNull(),
                session.match_preview_status.text(),
            )
            session.stop_match_tool()
            self.assertTrue(form.ViewObject.Visibility)
            self.assertTrue(self.source.ViewObject.Visibility)
            self.assertFalse(form.MatchBoundary)
        finally:
            gui_document.resetEdit()

    @unittest.skipUnless(App.GuiUp, "interactive preview requires FreeCADGui")
    def testBodyInsertEdgePreviewIsCachedAndWholeLoopClears(self):
        import FreeCADGui as Gui
        from PySide import QtCore, QtWidgets

        from Forms.edit import active_form_session

        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(10, 10, 20), App.Rotation()),
        )
        self.document.recompute()

        gui_document = Gui.getDocument(self.document.Name)
        try:
            gui_document.setEdit(form, 0)
            QtWidgets.QApplication.processEvents()
            session = active_form_session(form)
            self.assertIsNotNone(session)
            self.assertTrue(session.start_insert_edge_tool())
            session.view = _RecordingToolHandlerView(session.view)

            # Fix the hovered cage face so this test measures preview work rather
            # than depending on a screen coordinate or camera orientation.
            session._hovered_control_face = lambda _position: 0
            original_shape_face = session._shape_face_for_control_face
            shape_face_calls = []

            def counted_shape_face(face_index, mapper):
                shape_face_calls.append(face_index)
                return original_shape_face(face_index, mapper)

            session._shape_face_for_control_face = counted_shape_face
            session.insert_whole_loop.setChecked(True)
            session._update_insert_preview((0, 0))
            whole_loop_curve_count = session.surface_preview_lines.numVertices.getNum()
            first_call_count = len(shape_face_calls)
            self.assertGreater(whole_loop_curve_count, 1)
            self.assertGreater(first_call_count, 0)

            # Pixel motion over the same face must reuse the existing preview.
            session._update_insert_preview((1, 1))
            self.assertEqual(len(shape_face_calls), first_call_count)

            session.surface_cursor_position = (1, 1)
            session.insert_whole_loop.setChecked(False)
            self.assertEqual(session.surface_preview_lines.numVertices.getNum(), 1)
            self.assertEqual(
                session.surface_preview_coordinates.point.getNum(),
                session.surface_preview_lines.numVertices[0],
            )

            Gui.Selection.addSelection(form, "Face1")
            Gui.Selection.setPreselection(form, "Face1")
            self.assertTrue(Gui.Selection.getSelection())
            self.assertEqual(Gui.Selection.getPreselection().ObjectName, form.Name)
            self.assertTrue(session._commit_insert_preview())
            # Model the Body/viewer selection that may be delivered after the
            # topology callback has already completed its immediate clear.
            QtCore.QTimer.singleShot(0, lambda: Gui.Selection.addSelection(self.source, "Face1"))
            wait_loop = QtCore.QEventLoop()
            QtCore.QTimer.singleShot(80, wait_loop.quit)
            wait_loop.exec()
            self.assertFalse(Gui.Selection.getSelection())
            self.assertFalse(Gui.Selection.getPreselection().ObjectName)
            self.assertIn("Forms_Pointer_InsertEdge", session.view.activated_cursors)
        finally:
            gui_document.resetEdit()

    @unittest.skipUnless(App.GuiUp, "interactive preview requires FreeCADGui")
    def testBodySubdivideClearsDeferredUnderlyingSelection(self):
        import FreeCADGui as Gui
        from PySide import QtCore, QtWidgets

        from Forms.edit import active_form_session

        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(10, 10, 20), App.Rotation()),
        )
        self.document.recompute()

        gui_document = Gui.getDocument(self.document.Name)
        try:
            gui_document.setEdit(form, 0)
            QtWidgets.QApplication.processEvents()
            session = active_form_session(form)
            self.assertIsNotNone(session)
            self.assertTrue(session.start_subdivide_tool())
            session.view = _RecordingToolHandlerView(session.view)
            session.surface_hover_face = 0

            Gui.Selection.addSelection(form, "Face1")
            Gui.Selection.setPreselection(form, "Face1")
            self.assertTrue(session._commit_subdivide_preview())
            # The Body/viewer can deliver this selection after the topology
            # callback has completed its immediate and zero-delay clears.
            QtCore.QTimer.singleShot(0, lambda: Gui.Selection.addSelection(self.source, "Face1"))
            wait_loop = QtCore.QEventLoop()
            QtCore.QTimer.singleShot(80, wait_loop.quit)
            wait_loop.exec()
            self.assertFalse(Gui.Selection.getSelection())
            self.assertFalse(Gui.Selection.getPreselection().ObjectName)
            self.assertIn("Forms_Pointer_Subdivide", session.view.activated_cursors)
        finally:
            gui_document.resetEdit()

    @unittest.skipUnless(App.GuiUp, "interactive selection requires FreeCADGui")
    def testBodyRoutedDoubleClickSelectsFormWire(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets

        from Forms.cage import ControlCage
        from Forms.edit import active_form_session

        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(10, 10, 20), App.Rotation()),
        )
        self.document.recompute()
        Forms.make_editable(form)
        points = list(form.ControlPoints)
        bottom = min(point.z for point in points)
        opening = next(
            index
            for index, encoded in enumerate(form.ControlFaces)
            if all(abs(points[int(vertex)].z - bottom) < 1.0e-7 for vertex in encoded.split())
        )
        Forms.delete_faces(form, [opening])
        self.document.recompute()

        gui_document = Gui.getDocument(self.document.Name)
        try:
            gui_document.setEdit(form, 0)
            QtWidgets.QApplication.processEvents()
            session = active_form_session(form)
            self.assertIsNotNone(session)
            boundary_edge = ControlCage.from_object(form).boundary_edges[0]
            edge_name = next(
                f"Edge{index}"
                for index, _edge in enumerate(form.Shape.Edges, 1)
                if session._control_edge_for_subelement(f"Edge{index}") == boundary_edge
            )
            routed_name = f"{form.Name}.{edge_name}"
            self.assertEqual(
                session._form_selection_subelement(self.document.Name, self.body.Name, routed_name),
                edge_name,
            )
            source_routed_name = f"{self.source.Name}.;#f:3;:G;XTR;:H968:7,F.Face3"
            self.assertIsNone(
                session._form_selection_subelement(
                    self.document.Name,
                    self.body.Name,
                    source_routed_name,
                )
            )

            session.addSelection(self.document.Name, self.body.Name, routed_name, (0.0, 0.0, 0.0))
            session.removeSelection(self.document.Name, self.body.Name, routed_name)
            QtWidgets.QApplication.processEvents()

            selected_edges = session._selected_control_edges()
            selection_snapshot = [
                (selection.Object.Name, tuple(selection.SubElementNames))
                for selection in Gui.Selection.getSelectionEx(
                    "", Gui.Selection.ResolveMode.NoResolve
                )
            ]
            self.assertEqual(
                selected_edges,
                set(ControlCage.from_object(form).boundary_edges),
                selection_snapshot,
            )
        finally:
            gui_document.resetEdit()

    @unittest.skipUnless(App.GuiUp, "interactive transform requires FreeCADGui")
    def testCreationSelectionPositionsTheWholeParametricForm(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets
        from pivy import coin

        from Forms.edit import active_form_session

        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(30, 5, 4), App.Rotation()),
        )
        self.document.recompute()
        original_placement = form.FormPlacement.copy()
        original_points = [App.Vector(point) for point in form.ControlPoints]
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(form)

        gui_document = Gui.getDocument(self.document.Name)
        try:
            gui_document.setEdit(form, 0)
            QtWidgets.QApplication.processEvents()
            session = active_form_session(form)
            self.assertIsNotNone(session)
            self.assertTrue(session.whole_form_selected)
            self.assertEqual(len(session.selected), len(form.ControlPoints))
            self.assertNotEqual(session.dragger_switch.whichChild.getValue(), coin.SO_SWITCH_NONE)
            self.assertTrue(session.dimension_gizmo_switches)
            self.assertTrue(
                all(
                    switch.whichChild.getValue() == coin.SO_SWITCH_NONE
                    for switch in session.dimension_gizmo_switches.values()
                )
            )

            Gui.Selection.clearSelection()
            QtWidgets.QApplication.processEvents()
            self.assertFalse(session.whole_form_selected)
            self.assertTrue(
                all(
                    switch.whichChild.getValue() == coin.SO_SWITCH_ALL
                    for switch in session.dimension_gizmo_switches.values()
                )
            )
            Gui.Selection.addSelection(form)
            QtWidgets.QApplication.processEvents()
            self.assertTrue(session.whole_form_selected)
            self.assertTrue(
                all(
                    selection.SubElementNames
                    for selection in Gui.Selection.getSelectionEx(
                        "", Gui.Selection.ResolveMode.NoResolve
                    )
                )
            )

            offset = App.Vector(7, -3, 5)
            session.dragger_started(session.dragger)
            center = session.base_center.add(offset)
            session.dragger.translation.setValue(center.x, center.y, center.z)
            session.dragger_finished(session.dragger)

            self.assertEqual(form.CageMode, "Parametric")
            self.assertLess(
                form.FormPlacement.Base.sub(original_placement.Base).sub(offset).Length,
                1.0e-7,
            )
            for before, after in zip(original_points, form.ControlPoints):
                self.assertLess(after.sub(before).sub(offset).Length, 1.0e-7)
            form_center = App.Vector(form.FormShape.BoundBox.Center)
            for container, _dragger in session.dimension_gizmos.values():
                gizmo_center = App.Vector(*container.translation.getValue().getValue())
                self.assertLess(gizmo_center.sub(form_center).Length, 1.0e-7)
        finally:
            gui_document.resetEdit()

    @unittest.skipUnless(App.GuiUp, "creation task requires FreeCADGui")
    def testCancellingPartDesignFormCreationAbortsTheTransaction(self):
        import FreeCADGui as Gui
        from PySide import QtWidgets

        from Forms.edit import active_form_session

        self.document.openTransaction("Create additive Form")
        form = Forms.create_additive_form(self.body, self.source, "Box")
        form_name = form.Name
        form.ViewObject.Proxy._creation_transaction = True
        gui_document = Gui.getDocument(self.document.Name)
        gui_document.setEdit(form, 0)
        QtWidgets.QApplication.processEvents()

        session = active_form_session(form)
        self.assertIsNotNone(session)
        self.assertTrue(session.creation_transaction)
        session.reject()
        QtWidgets.QApplication.processEvents()

        self.assertEqual(self.document.getBookedTransactionID(), 0)
        self.assertIsNone(self.document.getObject(form_name))

    def testOverlappingClosedFormFusesWithBase(self):
        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(15, 10, 5), App.Rotation()),
        )
        form.Length = 12
        form.Width = 12
        form.Height = 12
        self.document.recompute()

        self.assertEqual(form.CombinationStatus, "Valid fused additive form")
        self.assertEqual(form.AddSubShape.ShapeType, "Solid")
        self.assertEqual(len(form.Shape.Solids), 1)
        self.assertTrue(form.Shape.isValid())
        self.assertGreater(form.Shape.Volume, self.source.Shape.Volume)

    def testOverlappingClosedSubtractiveFormCutsBase(self):
        form = Forms.create_subtractive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(10, 10, 5), App.Rotation()),
        )
        form.Length = 10
        form.Width = 10
        form.Height = 10
        self.document.recompute()

        self.assertEqual(form.TypeId, "PartDesign::FeatureSubtractivePython")
        self.assertEqual(form.Operation, "Subtractive")
        self.assertEqual(form.CombinationStatus, "Valid cut subtractive form")
        self.assertEqual(form.AddSubShape.ShapeType, "Solid")
        self.assertEqual(len(form.Shape.Solids), 1)
        self.assertTrue(form.Shape.isValid())
        self.assertLess(form.Shape.Volume, self.source.Shape.Volume)

    def testMatchStitchesBoxOpeningToBaseFace(self):
        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(10, 10, 20), App.Rotation()),
        )
        form.Length = 8
        form.Width = 8
        form.Height = 20
        form.XSegments = 1
        form.YSegments = 1
        form.ZSegments = 1
        self.document.recompute()
        Forms.make_editable(form)

        points = list(form.ControlPoints)
        bottom = min(point.z for point in points)
        opening = next(
            index
            for index, encoded in enumerate(form.ControlFaces)
            if all(abs(points[int(vertex)].z - bottom) < 1.0e-7 for vertex in encoded.split())
        )
        Forms.delete_faces(form, [opening])
        self.document.recompute()
        from Forms.cage import ControlCage

        cage = ControlCage.from_object(form)
        Forms.match_boundary(
            form,
            [cage.boundary_edges[0]],
            (self.source, ["Face6"]),
            "Connected",
        )
        form.EditingForm = False
        self.document.recompute()

        self.assertEqual(form.MatchSupport[0], self.source)
        self.assertEqual(form.MatchContinuity, "Connected")
        self.assertEqual(form.CombinationStatus, "Valid fused additive form")
        self.assertEqual(form.AddSubShape.ShapeType, "Solid")
        self.assertEqual(len(form.Shape.Solids), 1)
        self.assertTrue(form.Shape.isValid())

        self.source.Shape = Part.makeBox(20, 20, 15)
        self.document.recompute()
        matched_points = [form.ControlPoints[index] for index in form.MatchBoundary]
        self.assertTrue(matched_points)
        self.assertTrue(
            all(abs(point.z - 15.0) < 1.0e-7 for point in matched_points),
            matched_points,
        )
        self.assertEqual(form.CombinationStatus, "Valid fused additive form")
        self.assertTrue(form.Shape.isValid())

    def testRestoreClearsTemporaryEditCompound(self):
        handle, path = tempfile.mkstemp(suffix=".FCStd")
        os.close(handle)
        try:
            form = Forms.create_additive_form(
                self.body,
                self.source,
                "Box",
                placement=App.Placement(App.Vector(15, 10, 5), App.Rotation()),
            )
            form.Length = 12
            form.Width = 12
            form.Height = 12
            form.EditingForm = True
            self.document.recompute()
            self.assertIn("Editing form preview", form.CombinationStatus)
            self.document.saveAs(path)
            App.closeDocument(self.document.Name)

            self.document = App.openDocument(path)
            self.document.recompute()
            restored = self.document.getObject("AdditiveFormBox")
            self.assertEqual(restored.TypeId, "PartDesign::FeatureAdditivePython")
            self.assertEqual(restored.Operation, "Additive")
            self.assertFalse(restored.EditingForm)
            self.assertEqual(restored.PrimitiveKind, "Box")
            self.assertEqual(restored.CombinationStatus, "Valid fused additive form")
            self.assertEqual(len(restored.Shape.Solids), 1)
            self.assertTrue(restored.Shape.isValid())
        finally:
            if os.path.exists(path):
                os.remove(path)

    def testTangentMatchConstrainsTheAdjacentControlRing(self):
        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(10, 10, 20), App.Rotation()),
        )
        form.Length = 8
        form.Width = 8
        form.Height = 20
        form.XSegments = 1
        form.YSegments = 1
        form.ZSegments = 2
        self.document.recompute()
        Forms.make_editable(form)

        points = list(form.ControlPoints)
        bottom = min(point.z for point in points)
        opening = next(
            index
            for index, encoded in enumerate(form.ControlFaces)
            if all(abs(points[int(vertex)].z - bottom) < 1.0e-7 for vertex in encoded.split())
        )
        Forms.delete_faces(form, [opening])
        self.document.recompute()
        from Forms.cage import ControlCage

        cage = ControlCage.from_object(form)
        Forms.match_boundary(
            form,
            [cage.boundary_edges[0]],
            (self.source, ["Face6"]),
            "Tangent",
        )
        form.EditingForm = False
        self.document.recompute()

        cage = ControlCage.from_object(form)
        boundary = set(form.MatchBoundary)
        adjacent = {
            second if first in boundary else first
            for first, second in cage.edge_counts()
            if (first in boundary) != (second in boundary)
        }
        self.assertTrue(adjacent)
        self.assertTrue(
            all(cage.vertices[index][2] > 10.0 + 1.0e-7 for index in adjacent),
            [(index, cage.vertices[index]) for index in sorted(adjacent)],
        )
        self.assertEqual(form.MatchContinuity, "Tangent")

        from Forms.additive import _boundary_edges

        support = self.source.Shape.getElement("Face6")
        support_normal = support.normalAt(*support.Surface.parameter(support.CenterOfMass))
        seam_edges = _boundary_edges(form.FormShape)
        self.assertEqual(len(seam_edges), 4)
        for seam_edge in seam_edges:
            point = seam_edge.valueAt((seam_edge.FirstParameter + seam_edge.LastParameter) * 0.5)
            support_edge = next(
                edge
                for edge in support.OuterWire.OrderedEdges
                if all(vertex.distToShape(edge)[0] < 1.0e-6 for vertex in seam_edge.Vertexes)
            )
            neighbor = next(
                face
                for face in self.source.Shape.Faces
                if not face.isSame(support)
                and any(edge.isSame(support_edge) for edge in face.Edges)
            )
            form_face = next(
                face
                for face in form.FormShape.Faces
                if any(edge.isSame(seam_edge) for edge in face.Edges)
            )
            for fraction in (0.1, 0.5, 0.9):
                parameter = seam_edge.FirstParameter + fraction * (
                    seam_edge.LastParameter - seam_edge.FirstParameter
                )
                point = seam_edge.valueAt(parameter)
                form_normal = form_face.normalAt(*form_face.Surface.parameter(point))
                neighbor_normal = neighbor.normalAt(*neighbor.Surface.parameter(point))
                self.assertGreater(abs(form_normal.dot(neighbor_normal)), 0.99999)
                self.assertLess(abs(form_normal.dot(support_normal)), 0.005)

        form.MatchTangentMode = "SelectedFace"
        self.document.recompute()
        self.assertFalse(form.MatchCornerEdges)
        for seam_edge in _boundary_edges(form.FormShape):
            form_face = next(
                face
                for face in form.FormShape.Faces
                if any(edge.isSame(seam_edge) for edge in face.Edges)
            )
            for fraction in (0.1, 0.5, 0.9):
                parameter = seam_edge.FirstParameter + fraction * (
                    seam_edge.LastParameter - seam_edge.FirstParameter
                )
                point = seam_edge.valueAt(parameter)
                form_normal = form_face.normalAt(*form_face.Surface.parameter(point))
                self.assertGreater(abs(form_normal.dot(support_normal)), 0.99999)

    def testMatchPreservesRectangularSupportCorners(self):
        form = Forms.create_additive_form(
            self.body,
            self.source,
            "Box",
            placement=App.Placement(App.Vector(10, 10, 20), App.Rotation()),
        )
        form.Length = 8
        form.Width = 8
        form.Height = 20
        form.XSegments = 1
        form.YSegments = 1
        form.ZSegments = 2
        self.document.recompute()
        Forms.make_editable(form)

        points = list(form.ControlPoints)
        bottom = min(point.z for point in points)
        opening = next(
            index
            for index, encoded in enumerate(form.ControlFaces)
            if all(abs(points[int(vertex)].z - bottom) < 1.0e-7 for vertex in encoded.split())
        )
        Forms.delete_faces(form, [opening])
        self.document.recompute()
        from Forms.cage import ControlCage

        cage = ControlCage.from_object(form)
        Forms.match_boundary(
            form,
            [cage.boundary_edges[0]],
            (self.source, ["Face6"]),
            "Tangent",
        )
        form.EditingForm = False
        self.document.recompute()

        self.assertEqual(set(form.MatchCornerVertices), set(form.MatchBoundary))
        self.assertTrue(all(form.VertexSharpness[index] >= 10.0 for index in form.MatchBoundary))
        support = self.source.Shape.getElement("Face6")
        distances = [vertex.distToShape(form.FormShape)[0] for vertex in support.Vertexes]
        self.assertTrue(all(distance < 1.0e-6 for distance in distances), distances)
        self.assertEqual(form.CombinationStatus, "Valid fused additive form")
        self.assertTrue(form.Shape.isValid())


def suite():
    result = unittest.TestSuite()
    result.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(TestFormSurface))
    result.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(TestAdditiveForm))
    return result
