# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import tempfile
import unittest

import FreeCAD


SVG_HEAD = """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     xmlns:freecad="https://www.freecad.org/wiki/index.php?title=Svg_Namespace"
     width="297mm" height="210mm">
"""
SVG_TAIL = "</svg>\n"


def write_template(path, fields):
    """fields is a list of (editable name, default text)."""
    parts = [SVG_HEAD]
    for name, default in fields:
        parts.append(
            '  <text freecad:editable="%s"><tspan>%s</tspan></text>\n' % (name, default)
        )
    parts.append(SVG_TAIL)
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("".join(parts))


class DrawSVGTemplateTest(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.path_a = os.path.join(self.tmpdir.name, "template-a.svg")
        write_template(
            self.path_a,
            [("Author", "Default Author"), ("Title", "Default Title"), ("Extra", "Default Extra")],
        )
        FreeCAD.newDocument("TDTemplateReload")
        self.doc = FreeCAD.getDocument("TDTemplateReload")
        self.template = self.doc.addObject("TechDraw::DrawSVGTemplate", "Template")

    def tearDown(self):
        try:
            if "TDTemplateReload" in FreeCAD.listDocuments():
                FreeCAD.closeDocument("TDTemplateReload")
        finally:
            self.tmpdir.cleanup()

    def texts(self):
        return dict(self.template.EditableTexts)

    def test_same_path_does_not_reload(self):
        """Re-assigning Template to its current path must not reload the file.

        PropertyString ignores setting the same path, so EditableTexts and the
        embedded SVG (PageResult) stay as they are even if the file on disk
        changed. That is why Reload Template exists: Recompute and a same-path
        assignment do not re-read the source SVG.
        """
        self.template.Template = self.path_a
        self.template.EditableTexts = {"Author": "Kept", "Title": "T", "Extra": "E"}
        write_template(
            self.path_a,
            [("Author", "Reloaded"), ("Title", "Reloaded"), ("Extra", "Reloaded")],
        )
        self.template.Template = self.path_a
        kept = self.texts()
        self.assertEqual(kept["Author"], "Kept")
        self.assertEqual(kept["Title"], "T")
        self.assertEqual(kept["Extra"], "E")
        embedded = self.template.PageResult
        self.assertTrue(os.path.isfile(embedded), "PageResult is not a file: %r" % (embedded,))
        with open(embedded, encoding="utf-8") as handle:
            body = handle.read()
        self.assertNotIn("Reloaded", body)


if __name__ == "__main__":
    unittest.main()
