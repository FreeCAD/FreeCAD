# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 FreeCAD contributors
# SPDX-FileNotice: Part of the FreeCAD project.

"""Tests for opening links from Draft Text and Label objects."""

from unittest.mock import patch

import Draft
import FreeCADGui as Gui
from draftguitools import gui_hyperlink
from drafttests.test_base import DraftTestCaseDoc


class DraftGuiHyperlink(DraftTestCaseDoc):
    def test_hyperlinks_preserve_dashes_in_local_paths(self):
        paths = [
            "/home/draft/notes/plain drawing.pdf",
            "/home/draft/notes/part-drawing.pdf",
            "/home/draft/project-notes/drawing.pdf",
            "/home/draft/project - notes/part - drawing.pdf",
            "/-draft/drawing.pdf",
            r"C:\Draft\drawing.pdf",
            r"C:\Draft\part-drawing.pdf",
            r"\\server\Draft\drawing.pdf",
            r"\\my-server\Draft\drawing.pdf",
            "https://example.org/docs",
        ]
        text = Draft.make_text(paths)
        label = Draft.make_label(custom_text=paths)
        self.doc.recompute()
        self.addCleanup(Gui.Selection.clearSelection)

        command = gui_hyperlink.Draft_Hyperlink()
        for annotation in (text, label):
            with self.subTest(object=annotation.Name):
                Gui.Selection.clearSelection()
                Gui.Selection.addSelection(annotation)
                self.assertTrue(command.has_hyperlinks())
                self.assertEqual(command.hyperlinks_list, paths)

    def test_hyperlink_after_spaced_dash_in_prose(self):
        text = Draft.make_text(["From /home/draft - see /tmp/notes.pdf"])
        self.addCleanup(Gui.Selection.clearSelection)
        Gui.Selection.clearSelection()
        Gui.Selection.addSelection(text)

        command = gui_hyperlink.Draft_Hyperlink()
        self.assertTrue(command.has_hyperlinks())
        self.assertEqual(command.hyperlinks_list, ["/tmp/notes.pdf"])

    def test_open_link_with_leading_dash_in_root_directory(self):
        path = "/-draft/drawing.pdf"
        command = gui_hyperlink.Draft_Hyperlink()
        with (
            patch.object(gui_hyperlink.os.path, "isfile", return_value=True) as is_file,
            patch.object(gui_hyperlink, "QDesktopServices") as desktop_services,
        ):
            command.open_hyperlink(path)

        is_file.assert_called_once_with(path)
        self.assertEqual(desktop_services.openUrl.call_args.args[0].toLocalFile(), path)
