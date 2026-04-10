# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

try:
    import SketcherGui
except ImportError:
    SketcherGui = None


@unittest.skipIf(SketcherGui is None, "GUI not available")
class TestSketcherGuiModelTreeWidget(unittest.TestCase):

    def test_hasModelTreeWidget(self):
        result = SketcherGui.hasModelTreeWidget()
        self.assertIsInstance(result, bool)
        self.assertTrue(result, "Model TreeWidget expected to be available in GUI environment")
