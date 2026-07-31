# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#                                                                           *
#    Copyright (c) 2026 FreeCAD contributors.                               *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# ***************************************************************************/

from AssemblyTests.TestCore import AssemblyTestBase
import tempfile
import unittest

import FreeCAD as App

if App.GuiUp:
    import CommandCreateSimulation
    import FreeCADGui

VIDEO_EXPORT_AVAILABLE = True
try:
    import av
    from PIL import Image
except ImportError:
    VIDEO_EXPORT_AVAILABLE = False


def _msg(text, end="\n"):
    """Write messages to the console including the line ending."""
    App.Console.PrintMessage(text + end)


@unittest.skipIf(not App.GuiUp, "GUI tests require FreeCAD GUI mode")
@unittest.skipIf(
    not VIDEO_EXPORT_AVAILABLE,
    "Video export tests require the PyAV & PIL Python package",
)
class TestSimulationExport(AssemblyTestBase):
    def setUp(self):
        super().setUp()
        FreeCADGui.ActiveDocument.setEdit(self.assembly)

    def testVideoExport(self):
        task = CommandCreateSimulation.TaskAssemblyCreateSimulation()
        with tempfile.TemporaryDirectory() as temp_dir:
            size = (50, 50)
            img_names: list[str] = []
            for pos in range(10):
                img_names.append(temp_dir + f"/image{pos}.png")
                im = Image.new("RGB", (50, 50), color=(pos * 20, pos * 10, pos * 5))
                im.save(img_names[-1])

            vid_name = temp_dir + "/vid.mp4"
            task.create_video(vid_name, img_names, 2, size)

            with av.open(vid_name) as vid:
                frames = list(vid.decode(video=0))
                self.assertTrue(len(frames) == 10, "Video should have 10 frames")
                for frame in frames:
                    self.assertTrue(
                        frame.width == size[0],
                        "Frame width should be cohrent with image",
                    )
                    self.assertTrue(
                        frame.height == size[1],
                        "Frame height should be cohrent with image",
                    )
