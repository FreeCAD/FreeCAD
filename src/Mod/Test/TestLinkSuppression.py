# SPDX-License-Identifier: LGPL-2.1-or-later

"""Regression tests for suppression of ordinary App::Link array elements.

Run with FreeCADCmd -t TestLinkSuppression, or FreeCAD -t TestLinkSuppression
to include the GUI visibility checks. No Part pattern features are required.
"""

import os
import tempfile
import unittest

import FreeCAD as App


class TestLinkSuppression(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestLinkSuppression")
        self.source = self.doc.addObject("App::Part", "Source")
        self.child = self.doc.addObject("App::FeaturePython", "Child")
        self.source.addObject(self.child)
        self.array = self.doc.addObject("App::Link", "Array")
        self.array.LinkedObject = self.source
        self.array.ShowElement = True
        self.array.ElementCount = 4
        self.doc.recompute()

    def tearDown(self):
        if self.doc is not None:
            App.closeDocument(self.doc.Name)

    def test_suppression_preserves_element_identity_and_indices(self):
        elements = self.array.ElementList
        self.assertEqual(self.array.getSubObjects(), ("0.", "1.", "2.", "3."))
        self.assertFalse(any(element.Suppressed for element in elements))

        elements[1].Suppressed = True
        self.doc.recompute()

        self.assertEqual(self.array.getSubObjects(), ("0.", "2.", "3."))
        self.assertEqual(self.array.ElementCount, 4)
        self.assertEqual(self.array.ElementList, elements)
        self.assertEqual(self.array.LinkedObject, self.source)
        self.assertEqual(self.source.Group, [self.child])

        elements[1].Suppressed = False
        self.doc.recompute()
        self.assertEqual(self.array.getSubObjects(), ("0.", "1.", "2.", "3."))
        self.assertEqual(self.array.ElementList, elements)

    def test_suppressed_element_remains_addressable_but_hides_subobjects(self):
        element = self.array.ElementList[1]
        self.assertEqual(element.getSubObjects(), ("Child.",))
        self.assertEqual(self.array.getSubObject("1.Child.", 1), self.child)

        element.Suppressed = True

        self.assertEqual(self.array.getSubObject("1.", 1), element)
        self.assertEqual(element.getSubObject("", 1), element)
        self.assertEqual(element.getSubObjects(), ())
        self.assertIsNone(element.getSubObject("Child.", 1))
        self.assertIsNone(self.array.getSubObject("1.Child.", 1))
        self.assertEqual(self.array.getSubObject("0.Child.", 1), self.child)

        element.Suppressed = False
        self.assertEqual(element.getSubObjects(), ("Child.",))
        self.assertEqual(self.array.getSubObject("1.Child.", 1), self.child)

    def test_all_elements_can_be_suppressed_and_restored(self):
        self.assertTrue(self.array.hasChildElement())
        for element in self.array.ElementList:
            element.Suppressed = True
        self.doc.recompute()

        self.assertEqual(self.array.getSubObjects(), ())
        self.assertFalse(self.array.hasChildElement())
        self.assertEqual(self.array.ElementCount, 4)

        self.array.ElementList[2].Suppressed = False
        self.doc.recompute()
        self.assertEqual(self.array.getSubObjects(), ("2.",))
        self.assertTrue(self.array.hasChildElement())

    def test_suppression_marks_the_owner_for_recompute(self):
        self.assertNotIn("Touched", self.array.State)
        self.array.ElementList[1].Suppressed = True
        self.assertIn("Touched", self.array.State)
        self.doc.recompute()
        self.assertNotIn("Touched", self.array.State)

        self.array.ElementList[1].Suppressed = False
        self.assertIn("Touched", self.array.State)

    def test_undo_redo_restores_suppression(self):
        self.doc.UndoMode = 1
        self.doc.openTransaction("Suppress link element")
        self.array.ElementList[1].Suppressed = True
        self.doc.commitTransaction()
        self.doc.recompute()

        self.doc.undo()
        self.doc.recompute()
        self.assertFalse(self.array.ElementList[1].Suppressed)
        self.assertEqual(self.array.getSubObjects(), ("0.", "1.", "2.", "3."))

        self.doc.redo()
        self.doc.recompute()
        self.assertTrue(self.array.ElementList[1].Suppressed)
        self.assertEqual(self.array.getSubObjects(), ("0.", "2.", "3."))

    def test_suppression_survives_save_and_reload(self):
        self.array.ElementList[1].Suppressed = True
        self.doc.recompute()
        with tempfile.TemporaryDirectory() as directory:
            filename = os.path.join(directory, "LinkSuppression.FCStd")
            self.doc.saveAs(filename)
            App.closeDocument(self.doc.Name)
            self.doc = None
            self.doc = App.openDocument(filename)
            try:
                array = self.doc.getObject("Array")
                self.assertEqual(
                    [e.Suppressed for e in array.ElementList], [False, True, False, False]
                )
                self.assertEqual(array.getSubObjects(), ("0.", "2.", "3."))
                array.ElementList[1].Suppressed = False
                self.doc.recompute()
                self.assertEqual(array.getSubObjects(), ("0.", "1.", "2.", "3."))
            finally:
                App.closeDocument(self.doc.Name)
                self.doc = None

    def test_other_links_to_the_same_source_are_unaffected(self):
        other = self.doc.addObject("App::Link", "Other")
        other.LinkedObject = self.source
        self.doc.recompute()
        self.array.ElementList[1].Suppressed = True
        self.doc.recompute()

        self.assertEqual(other.getSubObjects(), ("Child.",))
        self.assertEqual(other.getSubObject("Child.", 1), self.child)
        self.assertEqual(self.source.getSubObjects(), ("Child.",))

    @unittest.skipUnless(App.GuiUp, "Requires the FreeCAD GUI")
    def test_gui_updates_suppression_without_recompute(self):
        view = self.array.ViewObject.LinkView
        self.assertEqual(view.Visibilities, (True, True, True, True))
        self.array.ElementList[1].Suppressed = True
        self.assertEqual(view.Visibilities, (True, False, True, True))
        self.array.ElementList[1].Suppressed = False
        self.assertEqual(view.Visibilities, (True, True, True, True))

    @unittest.skipUnless(App.GuiUp, "Requires the FreeCAD GUI")
    def test_gui_preserves_independent_element_visibility(self):
        view = self.array.ViewObject.LinkView
        self.array.setElementVisible("0", False)
        self.array.ElementList[1].Suppressed = True
        self.assertEqual(view.Visibilities, (False, False, True, True))

        # Visibility changes must not reveal a suppressed instance.
        self.array.setElementVisible("1", False)
        self.array.setElementVisible("1", True)
        self.assertEqual(view.Visibilities, (False, False, True, True))
        self.array.ElementList[1].Suppressed = False
        self.assertEqual(view.Visibilities, (False, True, True, True))

        # Restoring suppression must not reveal an independently hidden instance.
        self.array.setElementVisible("1", False)
        self.array.ElementList[1].Suppressed = True
        self.array.ElementList[1].Suppressed = False
        self.assertEqual(view.Visibilities, (False, False, True, True))
