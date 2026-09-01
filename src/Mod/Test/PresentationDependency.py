# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors

import unittest

import FreeCAD


class _RecomputeCounter:
    def __init__(self):
        self.count = 0

    def execute(self, obj):
        self.count += 1


class PresentationDependencyTestCase(unittest.TestCase):
    def setUp(self):
        self.document = FreeCAD.newDocument("PresentationDependencyTest")

    def tearDown(self):
        FreeCAD.closeDocument(self.document.Name)

    def _make_dependents(self, source):
        marked = self.document.addObject("App::FeaturePython", "Marked")
        marked.addProperty("App::PropertyLinkListGlobal", "Sources")
        marked.setPropertyStatus("Sources", "PresentationDependency")
        marked.Sources = [source]
        marked.Proxy = _RecomputeCounter()

        ordinary = self.document.addObject("App::FeaturePython", "Ordinary")
        ordinary.addProperty("App::PropertyLinkListGlobal", "Sources")
        ordinary.Sources = [source]
        ordinary.Proxy = _RecomputeCounter()

        return marked, ordinary

    def test_only_marked_links_receive_presentation_changes(self):
        source = self.document.addObject("App::FeaturePython", "Source")
        marked, ordinary = self._make_dependents(source)

        self.document.recompute()
        marked_count = marked.Proxy.count
        ordinary_count = ordinary.Proxy.count

        source.Label = "Renamed"
        self.document.recompute()

        self.assertGreater(marked.Proxy.count, marked_count)
        self.assertEqual(ordinary.Proxy.count, ordinary_count)

    @unittest.skipUnless(
        FreeCAD.GuiUp, "ViewObject dependency propagation requires the FreeCAD GUI"
    )
    def test_only_marked_links_receive_view_object_changes(self):
        source = self.document.addObject("Part::Feature", "Source")
        marked, ordinary = self._make_dependents(source)

        self.document.recompute()
        marked_count = marked.Proxy.count
        ordinary_count = ordinary.Proxy.count

        source.ViewObject.Visibility = not source.ViewObject.Visibility
        self.document.recompute()

        self.assertGreater(marked.Proxy.count, marked_count)
        self.assertEqual(ordinary.Proxy.count, ordinary_count)
