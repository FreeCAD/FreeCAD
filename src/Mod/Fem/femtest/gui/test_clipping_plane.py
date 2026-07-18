# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2026 CCNUdhj                                            *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""GUI tests for FEM clipping-plane commands."""

import unittest
import warnings

import FreeCAD as App
import FreeCADGui as Gui
from femcommands import commands


class TestClippingPlaneCommands(unittest.TestCase):
    """Test clipping-plane command behavior."""

    def setUp(self):
        """Create an empty document with a 3D view."""
        self.document = App.newDocument(self.__class__.__name__)
        Gui.activateView("Gui::View3DInventor", True)

    def tearDown(self):
        """Close the temporary document."""
        App.closeDocument(self.document.Name)

    def test_remove_all_without_planes_emits_no_swig_warning(self):
        """Removing from an empty scene should not emit the SWIG warning."""
        # Regression test for:
        # https://github.com/FreeCAD/FreeCAD/issues/28735
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            commands._ClippingPlaneRemoveAll().Activated()

        messages = [str(item.message) for item in caught]
        self.assertFalse(any("SwigPyObject" in message for message in messages), messages)

        from pivy import coin

        scene_graph = Gui.ActiveDocument.ActiveView.getSceneGraph()
        marker = coin.SoSeparator()
        scene_graph.addChild(marker)
        scene_graph.addChild(coin.SoClipPlane())
        scene_graph.addChild(coin.SoClipPlane())
        try:
            commands._ClippingPlaneRemoveAll().Activated()
            children = list(scene_graph.getChildren())
            self.assertIn(marker, children)
            self.assertFalse(any(isinstance(node, coin.SoClipPlane) for node in children))
        finally:
            scene_graph.removeChild(marker)
