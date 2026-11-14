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
        self.path_b = os.path.join(self.tmpdir.name, "template-b.svg")
        write_template(
            self.path_a,
            [("Author", "Default Author"), ("Title", "Default Title"), ("Extra", "Default Extra")],
        )
        write_template(
            self.path_b,
            [("Author", "Other Author"), ("Title", "Other Title"), ("NewField", "Brand New")],
        )
        FreeCAD.newDocument("TDTemplateMerge")
        self.doc = FreeCAD.getDocument("TDTemplateMerge")
        self.template = self.doc.addObject("TechDraw::DrawSVGTemplate", "Template")

    def tearDown(self):
        try:
            if "TDTemplateMerge" in FreeCAD.listDocuments():
                FreeCAD.closeDocument("TDTemplateMerge")
        finally:
            self.tmpdir.cleanup()

    def texts(self):
        return dict(self.template.EditableTexts)

    def test_change_keeps_matching_fields(self):
        """Changing Template to a different SVG merges editable fields by name.

        Names present in both files keep the current EditableTexts value,
        including an empty string. Names only in the new file take that
        file's default. Names the new file does not have are dropped.
        """
        self.template.Template = self.path_a
        defaults = self.texts()
        self.assertEqual(defaults["Author"], "Default Author")
        self.assertEqual(defaults["Title"], "Default Title")
        self.assertEqual(defaults["Extra"], "Default Extra")

        self.template.EditableTexts = {
            "Author": "Kept Author",
            "Title": "",
            "Extra": "Dropped",
        }
        self.template.Template = self.path_b
        merged = self.texts()

        self.assertEqual(merged["Author"], "Kept Author")
        self.assertEqual(merged["Title"], "")
        self.assertEqual(merged["NewField"], "Brand New")
        self.assertNotIn("Extra", merged)

    def test_restore_keeps_saved_texts(self):
        """Opening a saved document must keep the stored EditableTexts.

        Restore must not re-read the SVG and merge or replace the saved
        field values.
        """
        self.template.Template = self.path_a
        self.template.EditableTexts = {"Author": "Saved", "Title": "", "Extra": "Also"}
        saved = os.path.join(self.tmpdir.name, "restore.FCStd")
        self.doc.saveAs(saved)
        FreeCAD.closeDocument("TDTemplateMerge")

        reopened = FreeCAD.openDocument(saved)
        self.doc = reopened
        self.template = reopened.getObject("Template")
        restored = self.texts()
        self.assertEqual(restored["Author"], "Saved")
        self.assertEqual(restored["Title"], "")
        self.assertEqual(restored["Extra"], "Also")


if __name__ == "__main__":
    unittest.main()
